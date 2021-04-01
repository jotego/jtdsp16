#ifndef __FWMODEL_H
#define __FWMODEL_H

#include "dsp16_model.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <iomanip>

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
        st.div = 1;
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
    // register access
    int pc() { return st.pc; }
    int r0() { return st.yauu_regs[R0]; }
    int r1() { return st.yauu_regs[R1]; }
    int r2() { return st.yauu_regs[R2]; }
    int r3() { return st.yauu_regs[R3]; }
    int fault() { return st.fault; }
    // status access
    bool in_cache() { return st.cache.k>0; }
};

#define CHECK( a ) if( ref.a() != dut.a() ) good=false;
#define PRINTM( a, M ) std::cout << std::setfill(' ') << #a << " = " \
                                 << std::setfill('0') << std::setw(4) \
                                 << std::hex << (ref.a()&M) << " - " \
                                 << std::setfill('0') << std::setw(4) \
                                 << std::hex << (dut.a()&M) << '\n';

class Dual {
    Model &ref;
    RTL   &dut;
    i64 ticks;

    void side_dump() {
        std::cout << "      Ref - DUT         (" << std::dec << ticks << ")\n";
        PRINTM( pc, 0xFFFF )
        PRINTM( r0, 0xFFFF )
        PRINTM( r1, 0xFFFF )
        PRINTM( r2, 0xFFFF )
        PRINTM( r3, 0xFFFF )
    }

    void cmp() {
        bool good = true;
        static int bad=0;
        if( !ref.in_cache() ) {
            //CHECK( pc );
            CHECK( r0 );
            CHECK( r1 );
            CHECK( r2 );
            CHECK( r3 );
        }
        if( !good ) {
            side_dump();
            if( ++bad > 40 )
                throw std::runtime_error("Error: Ref and DUT diverged\n");
        }
        if( ref.fault() )
            throw std::runtime_error("Error: Ref is in fault state\n");
    }

public:
    Dual( Model& _ref, RTL& _dut ) : ref(_ref), dut(_dut), ticks(0) { }
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
        while ( p-->0 ) {
            dut.clk(1);
            ref.clk(1);
            ticks++;
            cmp();
        }
    }
    void rb_din( int v ) {
        dut.rb_din(v);
        ref.rb_din(v);
    }
};

#undef CHECK
#undef PRINTM

#endif