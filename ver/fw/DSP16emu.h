#ifndef __DSP16EMU_H
#define __DSP16EMU_H

#include <cstdio>
#include <cstdint>

struct EmuStats {
    int ram_reads, ram_writes;
};

class DSP16emu {
    int16_t *rom, *ram;
    int16_t read_rom(int a);
    int next_j, next_k, next_rb, next_re, next_r0, next_r1, next_r2, next_r3;
    int next_pt, next_pr, next_pi, next_i;
    int next_x,  next_y,  next_yl, next_p;
    int next_auc, next_psw, next_c0, next_c1, next_c2, next_sioc, next_srta, next_sdx;
    int next_tdms, next_pioc, next_pdx0, next_pdx1, next_pbus;
    int lfsr;
    int64_t next_a0, next_a1;
    // Cache
    bool in_cache,cache_first;
    int  cache_start, cache_end, cache_left;

    void    update_regs();
    int     get_register( int rfield );
    void    set_register( int rfield, int v );
    int64_t assign_high( int clr_mask, int64_t& dest, int val );
    int     sign_extend( int v, int msb=7 );
    void    disasm(int op);
    const char *disasm_r( int op );

    int     Yparse( int Y, bool up_now );
    void    Yparse_write( int Y, int v );
    int     Yparse_read( int Y, bool up_now=true );

    void    F1parse( int op, bool up_now=false ) { F12parse( op, false, up_now); }
    void    F2parse( int op, bool up_now=false ) { F12parse( op, true, up_now); }
    void    F12parse( int op, bool special, bool up_now=false );
    int     parse_pt( int op );
    void    parseZ( int op );
    void    set_psw( int lmi, int leq, int llv, int lmv, int ov0, int ov1, bool up_now );
    bool    CONparse( int op );
    bool    processDo();

    int64_t extend_p();
    int64_t extend_y();
    int     extend_i();
    void    product();

    void    assign_acc( int aD, int selhigh, int v, bool up_now );
    int     get_acc( int w, bool high=true, bool sat=true );
    void    step_aau_r( int* pr, int s );

    void    ram_write( int a, int v );
    int     ram_read( int a );

    bool    next_lfsr();
public:
    int pc, j, k, rb, re, r0, r1, r2, r3;
    int pt, pr, pi, i;
    int x, y, yl, p;
    int auc, psw, c0, c1, c2, sioc, srta, sdx;
    int tdms, pioc, pdx0, pdx1, pbus_out;
    int64_t a0, a1;
    bool verbose;

    EmuStats stats;

    int ticks;
    DSP16emu( int16_t* _rom );
    ~DSP16emu();
    void randomize_ram();
    int16_t *get_ram() { return ram; }
    int eval();
};

#endif