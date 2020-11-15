#ifndef __DSP16_COMMON
#define __DSP16_COMMON

#include "verilated_vcd_c.h"


class RTL {
    vluint64_t ticks, sim_time, half_period;
    VerilatedVcdC vcd;
public:
    Vjtdsp16 top;
    RTL();
    void reset();
    void clk( int n=1 );
    void read_rom( int16_t* data );
    void program_ram( int16_t* data );
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
    int  auc(){ return top.debug_auc;}
    int  x()  { return top.debug_x;  }
    int  y()  { return top.debug_y;  }
    int  yl() { return top.debug_yl; }

    int  c0() { return top.debug_c0; }
    int  c1() { return top.debug_c1; }
    int  c2() { return top.debug_c2; }

    int  p() { return top.debug_p; }

    int64_t a0() { return top.debug_a0; }
    int64_t a1() { return top.debug_a1; }

    // SIO
    int  srta() { return top.debug_srta; }
    int  sioc() { return top.debug_sioc; }

    // IRQ
    void set_irq() { top.irq = 1; }
    int iack() { return top.iack; }

    int get_ticks() { return ticks; }
};

class ROM {
    int16_t *rom;
public:
    ROM();
    ~ROM();
    int random( int valid );
    int16_t *data() { return rom; }
};

#endif