#ifndef __FWMODEL_H
#define __FWMODEL_H

#include "dsp16_model.h"

#include <cstdlib>

class Model {
    DSP16 st;
    DSP16_in inputs;
    i16 *rom, *ram;
public:
    Model( ROM& rom_data ) {
        rom = rom_data.data();
        ram = new i16[0x1000];
        std::memset( &inputs, 0, sizeof(DSP16_in));
        std::memset( &st, 0, sizeof(DSP16) );
        inputs.clk_en=1;
    }
    ~Model() { delete []ram; ram=nullptr; }
    void set_irq( int irq ) { inputs.irq = irq; }
    void pbus_in( int v ) { inputs.pbus_in = v; }
    void rb_din( int v ) { inputs.rb_din=v; }
    void clk(int p) {
        inputs.clk=0;
        p<<=1;
        while( p-->0 ) {
            inputs.clk = 1-inputs.clk;
            eval_DSP16( &st, &inputs, rom, ram );
        }
    }
};

class Dual {
    Model &ref;
    RTL   &dut;
public:
    Dual( Model& _ref, RTL& _dut ) : ref(_ref), dut(_dut) { }
    void set_irq(int irq) {
        dut.set_irq(irq);
        ref.set_irq(irq);
    }
    void pbus_in(int v) {
        dut.pbus_in(v);
        ref.pbus_in(v);
    }
    void clk(int p) {
        // each one could go in a different thread
        dut.clk(p);
        ref.clk(p);
    }
    void rb_din( int v ) {
        dut.rb_din(v);
        ref.rb_din(v);
    }
};

#endif