// #include "verilated.h"
#include "Vjtdsp16.h"
// #include "verilated_vcd_c.h"

#include <iostream>
#include <fstream>

using namespace std;

class RTL {
    vluint64_t ticks;
public:
    Vjtdsp16 top;
    RTL();
    void reset();
    void clk();
    void read_rom( int16_t* data );
};

class ROM {
    int16_t *rom;
public:
    ROM();
    ~ROM();
    int16_t *data() { return rom; }
};

int main( int argc, char *argv[] ) {
    RTL rtl;
    ROM rom;
    rtl.read_rom( rom.data() );

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

/////////////////////////
RTL::RTL() {
    reset();
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

void RTL::clk() {
    top.clk = 0;
    top.eval();
    top.clk = 1;
    top.eval();
    ticks++;
};

void RTL::read_rom( int16_t* data ) {
    int addr = 0;
    top.prog_we = 1;
    for( int j=0; j<8*1024; j++ ) {
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