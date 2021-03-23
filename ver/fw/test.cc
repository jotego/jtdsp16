// #include "verilated.h"

#include "common.h"
#include "DSP16emu.h"
#include "playfiles.h"

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <string>

using namespace std;


bool compare( RTL& rtl, DSP16emu& emu );
void dump( RTL& rtl, DSP16emu& emu );

const int GOTOJA   = 3;
const int SHORTIMM = 3<<2;
const int LONGIMM  = 1<<10;
const int AT_R     = 1<<8;
const int R_A0     = 1<<9;
const int R_A1     = 1<<10;
const int Y_R      = 1<<12;
const int DO_REDO  = 1<<14;
const int R_Y      = 1<<15;
// Format 1:
const int Ya1_F1     = 1<<4;
const int Y_F1       = 1<<6;
const int aTY_F1     = 1<<7;
const int Yy_F1      = 1<<20;
const int xY_F1      = 1<<22;
const int yY_F1      = 1<<23;
const int ya0_xX_F1  = 1<<25;
const int ya1_xX_F1  = 1<<27;
const int Ya0_F1     = 1<<28;
const int yY_xX_F1   = 1<<31;
// F2
const int IF_CON_F2  = 1<<19;
// Z operations
const int Zy_F1      = 1<<21;

int random_tests( ParseArgs& args );

int main( int argc, char *argv[] ) {
    ParseArgs args( argc, argv );
    if( args.error ) return 1;
    if( args.exit ) return 0;
    try {
        if( args.playback )
            return play_qs(args);
        else if( args.tracecmp )
            return cmptrace(args);
        else
            return random_tests(args);
    } catch( runtime_error e ) {
        printf("ERROR: %s\n",e.what());
        return 1;
    }
    return 0;
}

int random_tests( ParseArgs& args ) {
    RTL rtl( args.vcd_file.c_str());
    ROM rom;
    if( rom.random( // GOTOJA |
        SHORTIMM |
        LONGIMM |
        //DO_REDO |
        AT_R |
        R_A0 |
        R_A1 |
        Y_R  |
        R_Y  |
        // F1 operations
        Y_F1 |
        Ya1_F1 |
        Ya0_F1 |
        Yy_F1  |
        yY_F1  |
        xY_F1  |
        yY_xX_F1 |
        ya0_xX_F1 |
        ya1_xX_F1 |
        aTY_F1    |
        Zy_F1     |
        // F2
        IF_CON_F2 |
        0
     ) ) return 1;
    rtl.read_rom( rom.data() );
    DSP16emu emu( rom.data() );
    emu.randomize_ram();
    emu.verbose = args.verbose;
    rtl.program_ram( emu.get_ram() );

    bool good=true;

    // Simulate
    int k;
    for( k=0; k<3200 && !rtl.fault() && k<args.max; k++ ) {
        int ticks = emu.eval();
        rtl.clk(ticks<<1);
        good = compare(rtl,emu);
        if( !good ) {
            if(args.extra) rtl.clk(2);
            break;
        }
        if( args.step ) { dump(rtl, emu); putchar('\n'); }
    }

    // Close down
    if( rtl.fault() ) {
        cout << "ERROR: fault was asserted\n";
        return 1;
    }
    if( !good ) {
        printf("ERROR: emulator and RTL diverged after %d operations (seed=%d) \n", k, args.seed);
        dump(rtl, emu);
        return 1;
    }
    if( args.verbose ) dump(rtl, emu);
    printf("PASSED %d operations\n", k);
    return 0;
}

ROM::ROM() {
    ifstream fin("dl-1425.bin",ios_base::binary);
    if( fin.bad() ) {
        throw runtime_error("Cannot find dl-1425.bin");
    }
    rom=new int16_t[4*1024];
    fin.read( (char*)rom, 8*1024 );
}

ROM::~ROM() {
    delete rom;
    rom = nullptr;
}

int random_rfield(bool pdx_en=false) {
    int r=32;
    const int PIOC=28; // do not allow random writes here
    const int TDMS=27; // do not allow random writes here
    const int SRTA=0x19; // do not allow random writes here
    const int SDX=0x1A; // do not allow random writes here
    const int PDX0=0x1D; // do not allow random writes here
    const int PDX1=0x1E; // do not allow random writes here
    const int PSW=0x14; // do not allow random writes here
    const int PI=0xa; // This register is hard to match in emulation
        // and it really only plays a role in interrupt handling which
        // is not tested in random sims
    // as it can enable interrupts
    while( !(r<=11 || (r>=16 && r<31) )
            || r==PIOC || r==TDMS || r==SDX || ( (r==PDX0 || r==PDX1 ) && !pdx_en )
            || r==PI
            || r==PSW ) r=rand()%31;
    return r;
}

int ROM::random( int valid ) {
    if(valid==0) valid=~0;
    // the cache mask avoids illegal instructions for the cache and also
    // the long immediate instruction because it complicates the random ROM filling
    // and it is never used inside the cache in the QSound firmware, so I don't test it
    const int cache_mask = (~( (1<<30) | (1<<10) | (1<<14) | (1<<1) | 1| (1<<16) | (1<<17) | (1<<24) | (1<<26) ))&0xFFFF'FFFF;
    int incache = 0;
    bool cache_ready = false;

    for( int k=0; k<4*1024; k++ ) {
        int r =0;
        do {
            r = rand()%32;
        } while( ((1<<r) & valid) == 0 || (incache && ((1<<r) & cache_mask)==0  ));
        //printf("%04X - %X\n",r, ((1<<r) & valid));
        int op;
        op  = r << 11;
        // prevents illegal OP codes
        int extra=0;
        switch( r ) {
            case 0:
            case 2:
            case 3: extra = rand()%4096; break; // GOTO, Short immediate
            case 8:  // aT=R
            case 9:  // R=a0
            case 11: // R=a1
                extra  = (rand()%2) << 10;
                extra |= random_rfield(r!=8) << 4; break; // aT=R
            case 10: extra = random_rfield(true) << 4; break; // R=imm
            case 14: // Do / Redo
                extra = rand()%0x800;
                while( ((extra>>7)&0xf) == 0 && !cache_ready )
                    extra |= (rand()%16)<<7; // The first cache use cannot be a Redo
                while( (extra&0x7f) < 2 )
                    extra |= rand()%128;
                incache = ((extra>>7)&0xf);
                if( incache > 0 ) incache++; // because 1 will be subtracted at the bottom of this for loop
                break;
            case 12: /* Y = R */ case 15: // R = Y
                extra  = random_rfield(false) << 4;
                extra |= rand()%16; // Y field
                break;
            // F1 operations:
            case 6:
                extra = rand()%0x800;
                extra &= ~0x10;
                break;
            case 4:  case 7:
            case 20: case 22: case 23: case 25: case 27:
            case 28: case 31:
                extra = rand()%0x800;
                break;
            case 21: // Z:y F1
                extra = rand()%0x800;
                extra &= ~3;
                break;
            case 19: // if CON F2
                do {
                    extra = rand()%0x800;
                } while( ((extra>>5) &0xf)==10 || (extra&0x1f)>17 ); // avoid reserved F2 value
                    // and avoid wrong CON values
                break;
            default:
                printf("Error: unsupported OP 0x%X (%d) for randomization\n", r, r);
                for( int j=0; j<k; j++ ) {
                    printf("%04X ", rom[j]&0xFFFF );
                    if( (j&7)==7 ) putchar('\n');
                }
                putchar('\n');
                return 1;
        }
        extra &= 0x7ff;
        op |= extra;
        //printf("%04X = %04X\n", k, op );
        rom[k] = op;
        if(incache>0) incache--;
    }
    return 0;
}


bool compare( RTL& rtl, DSP16emu& emu ) {
    bool g = true;
    // ROM AAU
    g = g && rtl.pc() == emu.pc;
    g = g && rtl.pr() == emu.pr;
    //g = g && rtl.pi() == emu.pi;
    g = g && rtl.pt() == emu.pt;
    g = g && rtl.i()  == emu.i;
    // RAM AAU
    g = g && rtl.r0() == emu.r0;
    g = g && rtl.r1() == emu.r1;
    g = g && rtl.r2() == emu.r2;
    g = g && rtl.r3() == emu.r3;
    g = g && rtl.re() == emu.re;
    g = g && rtl.rb() == emu.rb;
    g = g && rtl.j()  == emu.j;
    g = g && rtl.k()  == emu.k;
    // DAU
    g = g && ( (rtl.psw()&0xfe10) == (emu.psw&0xfe10)); // guard bits are not compared
    g = g && rtl.x()   == emu.x;
    g = g && rtl.y()   == emu.y;
    g = g && rtl.yl()  == emu.yl;
    g = g && rtl.c0()  == emu.c0;
    g = g && rtl.c1()  == emu.c1;
    g = g && rtl.c2()  == emu.c2;
    g = g && rtl.a0()  == emu.a0;
    g = g && rtl.p()  == emu.p;
    // if( rtl.a0() != emu.a0 ) {
    //     printf("RTL A0 = %010lX\n",rtl.a0());
    //     printf("Emu A0 = %010lX\n",emu.a0);
    // }
    g = g && rtl.a1()  == emu.a1;
    // SIO
    g = g && rtl.srta()  == emu.srta;
    g = g && rtl.sioc()  == emu.sioc;
    g = g && rtl.pbus_out()  == emu.pbus_out;
    return g;
}

#define REG_DUMP( r, emu, rtl ) printf("%4s - %04X - %04X %c\n", #r, emu , rtl(), emu!=rtl() ? '*' : ' ' );
#define REG_DUMPL( r, emu, rtl ) printf("%4s - %09lX - %09lX %c\n", #r, emu/*&0xF'FFFF'FFFF*/ , rtl()/*&0xF'FFFF'FFFF*/, emu!=rtl() ? '*' : ' ' );

void dump( RTL& rtl, DSP16emu& emu ) {
    printf("      EMU -  RTL \n");
    // ROM AAU
    cout << "-- ROM AAU --\n";
    REG_DUMP(PC, emu.pc, rtl.pc )
    REG_DUMP(PR, emu.pr, rtl.pr )
    // REG_DUMP(PI, emu.pi, rtl.pi )
    REG_DUMP(PT, emu.pt, rtl.pt )
    REG_DUMP( I, emu.i , rtl.i  )
    // RAM AAU
    cout << "-- RAM AAU --\n";
    REG_DUMP(R0, emu.r0, rtl.r0 )
    REG_DUMP(R1, emu.r1, rtl.r1 )
    REG_DUMP(R2, emu.r2, rtl.r2 )
    REG_DUMP(R3, emu.r3, rtl.r3 )
    REG_DUMP(RB, emu.rb, rtl.rb )
    REG_DUMP(RE, emu.re, rtl.re )
    REG_DUMP( J, emu.j,  rtl.j  )
    REG_DUMP( K, emu.k,  rtl.k  )
    // DAU
    cout << "-- DAU --\n";
    REG_DUMP(PSW, emu.psw , rtl.psw )
    REG_DUMP( X, emu.x , rtl.x  )
    REG_DUMP( Y, emu.y , rtl.y  )
    REG_DUMP(YL, emu.yl, rtl.yl )
    REG_DUMP(C0, emu.c0, rtl.c0 )
    REG_DUMP(C1, emu.c1, rtl.c1 )
    REG_DUMP(C2, emu.c2, rtl.c2 )
    REG_DUMP(AUC, emu.auc, rtl.auc )
    REG_DUMP(P,  emu.p,  rtl.p  )
    REG_DUMPL(A0, emu.a0, rtl.a0 )
    REG_DUMPL(A1, emu.a1, rtl.a1 )

    cout << "-- SIO --\n";
    REG_DUMP(SRTA, emu.srta, rtl.srta )
    REG_DUMP(SIOC, emu.sioc, rtl.sioc )
    cout << "-- PIO --\n";
    REG_DUMP(PBUS, emu.pbus_out, rtl.pbus_out )

    cout << "-- STATS --\n";
    printf("%d RAM reads and %d RAM writes\n", emu.stats.ram_reads, emu.stats.ram_writes );
}

ParseArgs::ParseArgs( int argc, char *argv[]) {
    extra = step = verbose = playback = tracecmp = allcmd = false;
    exit = false;
    error = false;
    vcd_file="test.vcd";
    trace_file="wof.tr";
    write_vcd=false;
    seed=0;
    max = 100'000;
    if( argc==1 ) return;
    for( int k=1; k<argc; k++ ) {
        if( argv[k][0]=='-' ) {
            if( strcmp(argv[k],"-extra")==0 )  { extra=true;  continue; }
            if( strcmp(argv[k],"-v")==0 ) {
                verbose=true;
                if(verbose) step=true;
                continue;
            }
            if( strcmp(argv[k],"-w")==0 ) {
                write_vcd=true;
                continue;
            }
            if( strcmp(argv[k],"-play")==0 ) {
                qsnd_rom = "spf2t.rom";
                playfile = "spf2t_b1.qs";
                if( k+1 < argc )
                    qsnd_rom = argv[++k];
                if( k+1 < argc ) {
                    playfile = argv[++k];
                }
                playback=true;
                continue;
            }
            if( strcmp(argv[k],"-allcmd")==0 ) { allcmd=true; continue; }
            if( strcmp(argv[k],"-tracecmp")==0 ) { tracecmp=true; continue; }
            if( strcmp(argv[k],"-max")==0 ) {
                if( ++k<argc ) {
                    max = strtol(argv[k], NULL, 0);
                }
                continue;
            }
            if( strcmp(argv[k],"-mintime")==0 ) {
                if( ++k < argc )
                    min_sim_time=atoi(argv[k]);
                else {
                    throw runtime_error("Expecting minimum simulation time after -mintime");
                }
                continue;
            }
            if( strcmp(argv[k],"-vcd")==0 ) {
                if( ++k < argc )
                    vcd_file=argv[k];
                else {
                    throw runtime_error("Expecting name of VCD file after -vcd");
                }
                continue;
            }
            if( strcmp(argv[k],"-h")==0 ) {
                cout <<
"-play [CPS rom file] [playfile] \n"
"                      enables playback. The rom file is an MRA output.\n"
"                      The play file should follow spf2t_b1.qs example\n"
"-allcmd               parses all command inputs in the file\n"
"-tracecmp             enables comparative traces\n"
"-v                    verbose\n"
"-vcd                  name of output VCD file\n";
                exit=true;
                break;
            }
            error = true;
            cout << "Error: cannot recognize argument " << argv[k] << '\n';
            return;
        } else {
            // parse as seed
            seed = strtol(argv[k], NULL, 0);
        }
    }
    srand(seed);
    if(!playback && !tracecmp) printf("Random seed = %d\n", seed);
}

QSCmd::QSCmd( const std::string& fname ) {
    ifstream fin(fname);
    if(!fin.good()) {
        stringstream ss("Cannot open file ");
        ss << fname;
        throw runtime_error(ss.str());
    }
    string line;
    bool first=true;
    int ref_sample=10;
    int skip=-1;
    while( !fin.eof() ) {
        getline(fin, line, '\n');
        //cout << "0** " << line << "**\n";
        if( fin.bad() ) {
            cout << "ERROR: fin is bad\n";
            break;
        }

        if( line.find('#')!= string::npos ) {
            line = line.substr( 0, line.find_first_not_of(" \t#")-2 );
        }
        if( !line.size() ) {
            skip++;
            continue;
        }
        stringstream ss(line);
        int sample, addr, cmd;
        ss >> sample >> addr >> cmd;
        if( first ) {
            ref_sample = sample;
            first = false;
        }
        decltype(VCDpoint::time) time=(sample-ref_sample)*20'833LL;
        int val = (addr<<16) | (cmd&0xffff);
        points.push_back( {time, val} );
    }
    // for( auto p : points ) {
    //     cout << dec << p.time/1000'000 << "ms   " << hex << p.val << "\n";
    // }
    cout << "Read " << dec << points.size() << " data points ("<<skip<<" skipped)\n";
    cout << "Final time=" << points.back().time/1000'000 << "ms\n";
}