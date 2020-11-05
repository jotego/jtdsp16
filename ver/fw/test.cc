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

int main( int argc, char *argv[] ) {
    RTL rtl;
    ROM rom;
    rom.random( 0x3 );
    rtl.read_rom( rom.data() );
    DSP16emu emu( rom.data() );
    bool good=true;

    // Simulate
    for(int k=0; k<100 && !rtl.fault() && good; k++ ) {
        int ticks = emu.eval();
        rtl.clk(ticks<<1);
        good = compare(rtl,emu);
    }

    // Close down
    if( rtl.fault() ) {
        cout << "ERROR: fault was asserted\n";
        return 1;
    }
    if( !good ) {
        cout << "ERROR: emulator and RTL diverged\n";
        return 1;
    }
    cout << "PASSED\n";
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

void ROM::random( int valid ) {
    if(valid==0) valid=~0;

    for( int k=0; k<4*1024; k++ ) {
        int r =0;
        while( ((1<<r) & valid) == 0 )
            r = rand()%32;
        r <<= 11;
        r |= rand()%0x7ff;
        rom[k] = r;
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
    g = g && rtl.pc() == emu.pc;
    return g;
}