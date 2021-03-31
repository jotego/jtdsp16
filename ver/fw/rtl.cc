#include "common.h"
#include <fstream>
#include <cstdio>
#include <iostream>

using namespace std;

RTL::RTL( const char *vcd_name) {
    Verilated::traceEverOn(true);
    vcd_dump = true;
    top.trace(&vcd, 99);
    vcd.open(vcd_name);
    ticks=0;
    sim_time=0;
    half_period=9;

    reset();
}

bool RTL::fault() {
    int f = top.fault;
    return f!=0;
}

void RTL::reset() {
    top.rst       = 1;
    top.clk       = 0;
    top.clk_en    = 1;
    top.pbus_in   = 0;
    top.di        = 0;
    top.ick       = 0;
    top.ild       = 0;
    top.irq       = 0;
    top.prog_addr = 0;
    top.prog_data = 0;
    top.prog_we   = 0;
    top.ext_ok    = 1;
    for(int k=0; k<32; k++ )
        clk();
    top.rst = 0;
}

void RTL::clk(int n) {
    while( n-- > 0 ) {
        sim_time += half_period;
        top.clk = 0;
        top.eval();
        if(vcd_dump) vcd.dump(sim_time);

        sim_time += half_period;
        top.clk = 1;
        top.eval();
        if(vcd_dump) vcd.dump(sim_time);
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

void RTL::program_ram( int16_t* data ) {
    int addr = 0;
    top.debug_ram_we = 1;
    top.rst = 1;
    for( int j=0; j<2*1024; j++ ) {
        top.debug_ram_addr = addr++;
        top.debug_ram_din  = data[j];
        clk();
    }
    top.debug_ram_we = 0;
    reset();
}

void RTL::dump_ram() {
    ofstream fout("ram.bin",ios_base::binary);
    fout.write( (char*)top.jtdsp16__DOT__u_ram__DOT__ram, 2048*2 );
}

void RTL::screen_dump() {
    dump("PC", pc() );
    dump("PT", pt() );
    dump("PR", pr() );
    dump("i",   i() );
    dump("r0", r0() );
    dump("r1", r1() );
    dump("r2", r2() );
    dump("r3", r3() );
    dump("rb", rb() );
    dump("re", re() );
    dump("j",   j() );
    dump("k",   k() );
    dump("x",   x() );
    dump("y",   (y()<<16)|(yl()&0xffff) );
    dump("p",   p() );
    dump("a0", a0() );
    dump("a1", a1() );
    dump("c0", c0() );
    dump("c1", c1() );
    dump("c2", c2() );
    dump("auc",auc());
    dump("psw",psw());
}

void RTL::dump(const char *s, int d ) {
    printf("%4s = %04X\n",s, d);
}

void RTL::dump(const char *s, int64_t d ) {
    printf("%4s = %010lX\n",s, d);
}

