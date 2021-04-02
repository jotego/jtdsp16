#ifndef __DSP16_COMMON
#define __DSP16_COMMON
#include "Vjtdsp16.h"
#include "verilated_vcd_c.h"
#include "vcd.h"
#include <string>
#include <fstream>

class RTL {
    vluint64_t ticks, sim_time, half_period;
    VerilatedVcdC vcd;
    void dump(const char *, int d );
    void dump(const char *, int64_t d );
public:
    Vjtdsp16 top;
    bool vcd_dump;
    RTL(const char *vcd_name);
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
    int  fl() { return top.debug_psw>>12;} // flags
    int  auc(){ return top.debug_auc;}
    int  x()  { return top.debug_x;  }
    int  y()  { return top.debug_y;  }
    int  yh() { return top.debug_y;  }
    int  yl() { return top.debug_yl; }

    int  c0() { return top.debug_c0; }
    int  c1() { return top.debug_c1; }
    int  c2() { return top.debug_c2; }

    int  p() { return top.debug_p; }

    int64_t a0() { return top.debug_a0; }
    int64_t a1() { return top.debug_a1; }

    // SIO
    int  sadd() { return top.sadd; }
    int  srta() { return top.debug_srta; }
    int  sioc() { return top.debug_sioc; }
    int ser_out() { return top.ser_out; }

    // PIO
    int psel() { return top.psel; }
    int pids() { return top.pids_n; }
    int pods() { return top.pods_n; }
    int pbus_out() { return top.pbus_out; }
    void pbus_in(int v) { top.pbus_in = v; }

    // external ROM
    int ab() { return top.ab; }
    void rb_din(int d) { top.rb_din = d; }

    // IRQ
    void set_irq(int v=1) { top.irq = v; }
    int iack() { return top.iack; }

    int get_ticks() { return ticks; }
    vluint64_t time() { return sim_time; }

    void dump_ram();
    void screen_dump();
};

class ROM {
    int16_t *rom;
public:
    ROM();
    ~ROM();
    int random( int valid );
    int16_t *data() { return rom; }
};

class ParseArgs {
public:
    bool step, extra, verbose, playback, tracecmp, allcmd, error, exit,
         write_vcd=false;
    int max, seed;
    int min_sim_time=0;
    std::string vcd_file, trace_file, qsnd_rom="punisher.rom", playfile;
    ParseArgs( int argc, char *argv[]);
};

class QSCmd {
    VCDsignal::pointlist points;
public:
    QSCmd( const std::string& fname );
    const VCDsignal::pointlist cmdlist() const { return points; }
};

#endif