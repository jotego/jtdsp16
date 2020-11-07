// #include "verilated.h"
#include "Vjtdsp16.h"
#include "DSP16emu.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>

using namespace std;

class RTL {
    vluint64_t ticks, sim_time, half_period;
    VerilatedVcdC vcd;
public:
    Vjtdsp16 top;
    RTL();
    void reset();
    void clk( int n=1 );
    void read_rom( int16_t* data );
    bool fault();
    // access to registers
    int  pc() { return top.debug_pc; }
    int  pt() { return top.debug_pt; }
    int  pr() { return top.debug_pr; }
    int  pi() { return top.debug_pi; }
    int  i()  { return top.debug_i;  }

    int  re() { return top.debug_re; }
    int  rb() { return top.debug_rb; }
    int  j()  { return top.debug_j; }
    int  k()  { return top.debug_k; }
    int  r0() { return top.debug_r0; }
    int  r1() { return top.debug_r1; }
    int  r2() { return top.debug_r2; }
    int  r3() { return top.debug_r3; }

    // DAU
    int  psw(){ return top.debug_psw;}
    int  x()  { return top.debug_x;  }
    int  y()  { return top.debug_y;  }
    int  yl() { return top.debug_yl; }

    int  c0() { return top.debug_c0; }
    int  c1() { return top.debug_c1; }
    int  c2() { return top.debug_c2; }

    int64_t a0() { return top.debug_a0; }
    int64_t a1() { return top.debug_a1; }

    // SIO
    int  srta() { return top.debug_srta; }
    int  sioc() { return top.debug_sioc; }

    int get_ticks() { return ticks; }
};

class ROM {
    int16_t *rom;
public:
    ROM();
    ~ROM();
    void random( int valid );
    int16_t *data() { return rom; }
};

bool compare( RTL& rtl, DSP16emu& emu );
void dump( RTL& rtl, DSP16emu& emu );

const int GOTOJA   = 3;
const int SHORTIMM = 3<<2;
const int LONGIMM  = 1<<10;
const int AT_R     = 1<<8;
const int R_A0     = 1<<9;
const int R_A1     = 1<<10;
const int Y_R      = 1<<12;
const int R_Y      = 1<<15;

class ParseArgs {
public:
    ParseArgs( int argc, char *argv[]);
};

int main( int argc, char *argv[] ) {
    ParseArgs args( argc, argv );

    RTL rtl;
    ROM rom;
    rom.random( /*GOTOJA | */
        SHORTIMM |
        LONGIMM |
        AT_R |
        R_A0 |
        R_A1 |
        Y_R  |
        R_Y  |
        0
     );
    rtl.read_rom( rom.data() );
    DSP16emu emu( rom.data() );
    bool good=true;

    // Simulate
    int k;
    for( k=0; k<3200 && !rtl.fault(); k++ ) {
        int ticks = emu.eval();
        rtl.clk(ticks<<1);
        good = compare(rtl,emu);
        if( !good ) {
            //rtl.clk(2);
            break;
        }
    }

    // Close down
    if( rtl.fault() ) {
        cout << "ERROR: fault was asserted\n";
        return 1;
    }
    if( !good ) {
        printf("ERROR: emulator and RTL diverged after %d operations \n", k);
        dump(rtl, emu);
        return 1;
    }
    dump(rtl, emu);
    printf("PASSED %d operations\n", k);
    return 0;
}

ROM::ROM() {
    ifstream fin("dl-1425.bin",ios_base::binary);
    rom=new int16_t[4*1024];
    fin.read( (char*)rom, 8*1024 );
}

ROM::~ROM() {
    delete rom;
    rom = nullptr;
}

int random_rfield() {
    int r=32;
    const int PIOC=28; // do not allow random writes here
    const int TDMS=27; // do not allow random writes here
    const int SRTA=0x19; // do not allow random writes here
    const int SDX=0x1A; // do not allow random writes here
    const int PDX0=0x1D; // do not allow random writes here
    const int PDX1=0x1E; // do not allow random writes here
    const int PSW=0x14; // do not allow random writes here
    // as it can enable interrupts
    while( !(r<=11 || (r>=16 && r<31) )
            || r==PIOC || r==TDMS || r==SDX || r==PDX0 || r==PDX1 || r==PSW ) r=rand()%31;
    return r;
}

void ROM::random( int valid ) {
    if(valid==0) valid=~0;

    for( int k=0; k<4*1024; k++ ) {
        int r =0;
        do {
            r = rand()%32;
        } while( ((1<<r) & valid) == 0 );
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
                extra |= random_rfield() << 4; break; // aT=R
            case 10: extra = random_rfield() << 4; break; // R=imm
            case 12: // Y = R
            case 15: // R = Y
                extra  = random_rfield() << 4;
                extra |= rand()%16; // Y field
                break;
            default: cout << "Error: unsupported OP for randomization\n";
        }
        extra &= 0x7ff;
        op |= extra;
        rom[k] = op;
    }
}

/////////////////////////
RTL::RTL() {
    Verilated::traceEverOn(true);
    top.trace(&vcd, 99);
    vcd.open("test.vcd");
    ticks=0;
    sim_time=0;
    half_period=10;
    reset();
}

bool RTL::fault() {
    int f = top.fault;
    return f!=0;
}

void RTL::reset() {
    top.rst       = 1;
    top.clk       = 0;
    top.cen       = 1;
    top.pbus_in   = 0;
    top.di        = 0;
    top.ick       = 0;
    top.ild       = 0;
    top.irq       = 0;
    top.prog_addr = 0;
    top.prog_data = 0;
    top.prog_we   = 0;
    for(int k=0; k<32; k++ )
        clk();
    top.rst = 0;
}

void RTL::clk(int n) {
    while( n-- ) {
        sim_time += half_period;
        top.clk = 0;
        top.eval();
        vcd.dump(sim_time);

        sim_time += half_period;
        top.clk = 1;
        top.eval();
        vcd.dump(sim_time);
        ticks++;
    }
};

void RTL::read_rom( int16_t* data ) {
    int addr = 0;
    top.prog_we = 1;
    top.rst = 1;
    for( int j=0; j<4*1024; j++ ) {
        int16_t d = *data++;
        // LSB
        top.prog_addr = addr++;
        top.prog_data = d&0xff;
        clk();
        // MSB
        top.prog_addr = addr++;
        top.prog_data = d>>8;
        clk();
    }
    top.prog_we = 0;
    reset();
}

bool compare( RTL& rtl, DSP16emu& emu ) {
    bool g = true;
    // ROM AAU
    g = g && rtl.pc() == emu.pc;
    g = g && rtl.pr() == emu.pr;
    g = g && rtl.pi() == emu.pi;
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
    //g = g && rtl.psw() == emu.psw;
    g = g && rtl.x()   == emu.x;
    g = g && rtl.y()   == emu.y;
    g = g && rtl.yl()  == emu.yl;
    g = g && rtl.c0()  == emu.c0;
    g = g && rtl.c1()  == emu.c1;
    g = g && rtl.c2()  == emu.c2;
    g = g && rtl.a0()  == emu.a0;
    g = g && rtl.a1()  == emu.a1;
    // SIO
    g = g && rtl.srta()  == emu.srta;
    g = g && rtl.sioc()  == emu.sioc;
    return g;
}

#define REG_DUMP( r, emu, rtl ) printf("%4s - %04X - %04X %c\n", #r, emu , rtl(), emu!=rtl() ? '*' : ' ' );
#define REG_DUMPL( r, emu, rtl ) printf("%4s - %09lX - %09lX %c\n", #r, emu&0xF'FFFF'FFFF , rtl()&0xF'FFFF'FFFF, emu!=rtl() ? '*' : ' ' );

void dump( RTL& rtl, DSP16emu& emu ) {
    printf("      EMU -  RTL \n");
    // ROM AAU
    cout << "-- ROM AAU --\n";
    REG_DUMP(PC, emu.pc, rtl.pc )
    REG_DUMP(PR, emu.pr, rtl.pr )
    REG_DUMP(PI, emu.pi, rtl.pi )
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
    //REG_DUMP(PSW, emu.psw , rtl.psw )
    REG_DUMP( X, emu.x , rtl.x  )
    REG_DUMP( Y, emu.y , rtl.y  )
    REG_DUMP(YL, emu.yl, rtl.yl )
    REG_DUMP(C0, emu.c0, rtl.c0 )
    REG_DUMP(C1, emu.c1, rtl.c1 )
    REG_DUMP(C2, emu.c2, rtl.c2 )
    REG_DUMPL(A0, emu.a0, rtl.a0 )
    REG_DUMPL(A1, emu.a1, rtl.a1 )

    cout << "-- SIO --\n";
    REG_DUMP(SRTA, emu.srta, rtl.srta )
    REG_DUMP(SIOC, emu.sioc, rtl.sioc )

    cout << "-- STATS --\n";
    printf("%d RAM reads and %d RAM writes\n", emu.stats.ram_reads, emu.stats.ram_writes );
}

ParseArgs::ParseArgs( int argc, char *argv[]) {
    if( argc==1 ) return;
    if( argc==2 ) {
        int n = strtol(argv[1], NULL, 0);
        srand(n);
        printf("Random seed = %d\n", n);
    }
}