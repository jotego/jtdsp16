#include "common.h"
#include "vcd.h"
#include "mametrace.h"
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>

using namespace std;

int playfiles();

class QSndData {
    int get_offset( char *header, int p );
    int mask;
    char *data;
public:
    QSndData( const char *rompath );
    int get( int addr );
    ~QSndData();
};

class QSndLog {
    struct LogPair{ long int ticks; int value; };
public:
    QSndLog( const char *path);
};

int playfiles( const char* vcd_file ) {
    RTL rtl(vcd_file);
    ROM rom;
    QSndData samples("wof.rom");
    rtl.read_rom(rom.data());
    VCDfile stim("wof_coin.vcd"); // VCD sample input from CPS 1.5 ver/game folder
    stim.report();
    // Move until rst is low
    //printf("VCD forwarded to %ld\n",  );
    //int64_t vcdtime = stim.forward("dsp_rst",0);

    // VCDsignal* sig_irq = stim.get("dsp_irq");
    VCDsignal* sig_cpu2dsp = stim.get("cpu2dsp0");
    const VCDsignal::pointlist& cmdlist = sig_cpu2dsp->get_list();
    VCDsignal::pointlist::const_iterator n = cmdlist.cbegin();

    int reads;
    n++;
    int64_t vcdtime = n->time;
    int64_t next= n->time;

    rtl.clk( 0x800*4 );
    rtl.step();
    rtl.clk( 200'000 ); // initialization

    int sim_time=0;
    do {
        int newcmd = n->val;
        int reads=0;
        int last_pids=1;
        int rom_addr=0;
        rtl.set_irq(1);
        rtl.pbus_in( newcmd>>16 );
        n++;
        if( n==cmdlist.cend() ) break;
        vcdtime = next;
        next = n->time;
        int steps = (next-vcdtime)/18;
        if( steps>2'000'000 )
            steps=2'000'000;
        sim_time = (int)(rtl.time()/1000'000L);
        printf("%d ms -> %02X_%04X\n", sim_time, newcmd>>16, newcmd&0xffff);
        while( steps>0 ) {
            rtl.clk(2);
            if( rtl.pids()==1 && last_pids==0 ) {
                reads++;
                if( reads==1 ) rtl.pbus_in( newcmd&0xffff );
            }
            if( last_pids==0 ) rtl.set_irq(0);
            last_pids = rtl.pids();
            if( !rtl.pods() ) {
                rom_addr = 0;
                rom_addr |= rtl.pbus_out()&0xFFFF;
            }
            rom_addr &= 0xFFFF;
            rom_addr |= (rtl.ab()&0xff)<<16;
            if( rtl.ab()&0x8000 ) { // update the value only when external reads occur
                int din = samples.get( rom_addr );
                //printf("Read %X from %X\n", din, rom_addr );
                rtl.rb_din( din<<8 );
            }
            steps-=2;
        }
    }while( sim_time < 100 );
    rtl.dump_ram();

    return 0;
}

#define CHECK( a ) if( rtl.a() != tr.a ) { /*printf("Register " #a " is wrong\n");*/ return false; }

bool compare( RTL& rtl, MAMEtrace& tr ) {
    CHECK( r0 )
    CHECK( r0 );
    CHECK( r1 );
    CHECK( r2 );
    CHECK( r3 );
    CHECK( rb );
    CHECK( re );
    CHECK( i );
    CHECK( j );
    CHECK( k );
    CHECK( x );
    int y = (rtl.y()<<16)|(rtl.yl()&0xffff);
    if( y != tr.y ) return false;
    CHECK( p );
    CHECK( a0 );
    CHECK( a1 );
    return true;
}

#undef CHECK

void sidebyside( RTL& rtl, MAMEtrace& tr ) {
    int y = (rtl.y()<<16)|(rtl.yl()&0xffff);

    printf("     RTL     EMU \n");
    printf("PC   %04X - %04X %c\n", rtl.pc(), tr.pc, (rtl.pc() != tr.pc)?'*':' ' );
    printf("PR   %04X - %04X %c\n", rtl.pr(), tr.pr, (rtl.pr() != tr.pr)?'*':' ' );
    printf("PT   %04X - %04X %c\n", rtl.pt(), tr.pt, (rtl.pt() != tr.pt)?'*':' ' );
    printf("r0   %04X - %04X %c\n", rtl.r0(), tr.r0, (rtl.r0() != tr.r0)?'*':' ' );
    printf("r1   %04X - %04X %c\n", rtl.r1(), tr.r1, (rtl.r1() != tr.r1)?'*':' ' );
    printf("r2   %04X - %04X %c\n", rtl.r2(), tr.r2, (rtl.r2() != tr.r2)?'*':' ' );
    printf("r3   %04X - %04X %c\n", rtl.r3(), tr.r3, (rtl.r3() != tr.r3)?'*':' ' );
    printf("rb   %04X - %04X %c\n", rtl.rb(), tr.rb, (rtl.rb() != tr.rb)?'*':' ' );
    printf("re   %04X - %04X %c\n", rtl.re(), tr.re, (rtl.re() != tr.re)?'*':' ' );
    printf("i    %04X - %04X %c\n", rtl.i(),  tr.i , (rtl.i()  != tr.i )?'*':' ' );
    printf("j    %04X - %04X %c\n", rtl.j(),  tr.j , (rtl.j()  != tr.j )?'*':' ' );
    printf("k    %04X - %04X %c\n", rtl.k(),  tr.k , (rtl.k()  != tr.k )?'*':' ' );
    printf("p    %04X - %04X %c\n", rtl.p(),  tr.p , (rtl.p()  != tr.p )?'*':' ' );
    printf("c1   %02X - %02X %c\n", rtl.c1(), tr.c1, (rtl.c1() != tr.c1)?'*':' ' );
    printf("c2   %02X - %02X %c\n", rtl.c2(), tr.c2, (rtl.c2() != tr.c2)?'*':' ' );
    //printf("c3   %02X - %02X\n", rtl.c3(), tr.c3 );
    printf("x     %04X -  %04X %c\n", rtl.x(),  tr.x, (rtl.x() != tr.x)?'*':' '  );
    printf("y     %08X -  %08X %c\n", y,  tr.y, (y != tr.y)?'*':' '  );
    printf("a0   %09lX - %09lX %c\n", rtl.a0(),  tr.a0, (rtl.a0() !=  tr.a0)?'*':' ' );
    printf("a1   %09lX - %09lX %c\n", rtl.a1(),  tr.a1, (rtl.a1() !=  tr.a1)?'*':' ' );
    printf("auc   %02X -  %02X %c\n", rtl.auc(), tr.auc, (rtl.auc() != tr.auc)?'*':' ' );
    printf("psw   %02X -  %02X %c\n", rtl.psw(), tr.psw, (rtl.psw() != tr.psw)?'*':' ' );
}

void handle_din( RTL& rtl, int& rom_addr, QSndData& samples ) {
    if( !rtl.pods() ) {
        rom_addr = 0;
        rom_addr |= rtl.pbus_out()&0xFFFF;
    }
    rom_addr &= 0xFFFF;
    rom_addr |= (rtl.ab()&0xff)<<16;
    if( rtl.ab()&0x8000 ) { // update the value only when external reads occur
        int din = samples.get( rom_addr );
        //printf("Read %X from %X\n", din, rom_addr );
        rtl.rb_din( din<<8 );
    }
}

int cmptrace( ParseArgs& args ) {
    RTL rtl(args.vcd_file.c_str());
    ROM rom;
    QSndData samples("wof.rom");
    rtl.read_rom(rom.data());
    MAMEtrace tr( args.trace_file.c_str() );
    // Move until rst is low
    //printf("VCD forwarded to %ld\n",  );
    //int64_t vcdtime = stim.forward("dsp_rst",0);

    // VCDsignal* sig_irq = stim.get("dsp_irq");
    int bad=0;
    int line_bad;
    int rom_addr=0;
    for(int k=0; /*tr.get_line()<309480*/; k++ ) {
        //tr.show();
        rtl.clk(2);
        handle_din( rtl, rom_addr, samples );
        if( !compare(rtl, tr) ) {
            if( bad<6 /*&& rtl.pc() <= tr.pc*/ ) {
                if( bad==0 ) {
                    line_bad = tr.get_line();
                    //printf("************** %8d ********************\n", tr.get_line());
                    //sidebyside( rtl, tr );
                }
                bad++;
                //printf("bad\n");
                continue; // give an extra cycle
            } else {
                printf("Diverged at line %d\n", line_bad);
                //sidebyside( rtl, tr );
                rtl.clk(4); // Add some extra cycles
                return 1;
            }
        } else {
            // printf("************** %8d ********************\n", tr.get_line());
            // sidebyside( rtl, tr );
            bad = 0;
            if( !tr.next() ) {
                break;
            }
        }
    }
    printf("comparison found no differences\n");
    return 0;
}

QSndData::QSndData( const char *rompath ) {
    ifstream fin(rompath, ios_base::binary);
    if( !fin.good() ) throw runtime_error("Cannot open ROM file for game");
    // Get the header
    char header[64];
    fin.read( header, 64 );
    if( !fin.good() ) throw runtime_error("Cannot read ROM header");
    int start = get_offset( header, 2 );
    int end   = get_offset( header, 4 );
    int expected = end-start;
    if( expected>8*1024*1024 )
        expected = 8*1024*1024;
    data = new char[expected];
    mask = expected-1;
    printf("PCM data start %10X\nPCM data end   %10X. Mask=%0x\n", start, end, mask );
    fin.seekg( start+64 );
    if( !fin.good() ) {
        char s[256];
        sprintf(s,"Cannot seek to PCM start of game ROM (%X)", start);
        throw runtime_error(s);
    }
    fin.read( data, expected );
    int data_cnt = fin.gcount();
    if( data_cnt!=expected ) {
        char s[256];
        sprintf(s,"Expected 0x%X bytes of data but only 0x%X were read", expected, data_cnt );
        throw runtime_error(s);
    }
    printf("Read %d (%d MB) as PCM data\n", data_cnt, data_cnt>>20 );
}

QSndData::~QSndData() {
    delete[] data;
    data = nullptr;
}

int QSndData::get( int addr ) {
    addr &= mask;
    return data[addr]&0xff;
}

int QSndData::get_offset( char *header, int p ) {
    int o = ((((int)header[p+1])&0xff)<<8) | (((int)header[p])&0xff);
    // printf("%X%X->%X\n", (unsigned)header[p+1]&0xff, (unsigned)header[p]&0xff, o);
    o <<= 10;
    return o;
}