struct EmuStats {
    int ram_reads, ram_writes;
};

class DSP16emu {
    int16_t *rom, *ram;
    int16_t read_rom(int a);
    int next_j, next_k, next_rb, next_re, next_r0, next_r1, next_r2, next_r3;
    int next_pt, next_pr, next_pi, next_i;
    int next_x,  next_y,  next_yl;
    int next_auc, next_psw, next_c0, next_c1, next_c2, next_sioc, next_srta, next_sdx;
    int next_tdms, next_pioc, next_pdx0, next_pdx1;
    int64_t next_a0, next_a1;
    bool in_cache;

    void    update_regs();
    int     get_register( int rfield );
    void    set_register( int rfield, int v );
    int64_t assign_high( int clr_mask, int64_t& dest, int val );
    int     sign_extend( int v, int msb=7 );

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

    int64_t extend_p();
    int64_t extend_y();
    int     extend_i();

    void    assign_acc( int aD, int v, bool up_now );
    void    step_aau_r( int* pr, int s );

    void    ram_write( int a, int v );
    int     ram_read( int a );

public:
    int pc, j, k, rb, re, r0, r1, r2, r3;
    int pt, pr, pi, i;
    int x, y, yl, p;
    int auc, psw, c0, c1, c2, sioc, srta, sdx;
    int tdms, pioc, pdx0, pdx1;
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

void DSP16emu::randomize_ram() {
    for (int k=0; k<2048; k++ ) {
        ram[k] = rand();
    }
}

DSP16emu::DSP16emu( int16_t* _rom ) {
    verbose = false;
    pc=0;
    j = k = rb = re = r0 = r1 = r2 = r3 = 0;
    next_j = next_k = next_rb = next_re = next_r0 = next_r1 = next_r2 = next_r3 = 0;
    next_pt  = next_pr  = next_pi = next_i = 0;
    next_x   = next_y   = next_yl = 0;
    next_auc = next_psw = next_c0 = next_c1 = next_c2 = next_sioc = next_srta = next_sdx = 0;
    next_tdms = next_pioc = next_pdx0 = next_pdx1 = 0;
    next_a0 = a0 = next_a1 = a1 = 0;
    p = 0;
    ticks=0;
    in_cache = false;
    rom = _rom;
    ram = new int16_t[2048];
    for(int k=0; k<2048; k++) ram[k]=0;
    stats.ram_reads = stats.ram_writes = 0;
}

DSP16emu::~DSP16emu() {
    delete[] ram;
    ram = nullptr;
}

int DSP16emu::sign_extend( int v, int msb ) {
    int sign_bit = 1<<msb;
    int mask = sign_bit-1;
    if( sign_bit & v ) {
        return v | ~mask;
    } else
        return v;
}

void DSP16emu::update_regs() {
    j = next_j;
    k = next_k;

    rb = next_rb;
    re = next_re;
    r0 = next_r0;
    r1 = next_r1;
    r2 = next_r2;
    r3 = next_r3;

    pt = next_pt;
    pr = next_pr;
    pi = next_pi;
    i  = next_i;

    x  = next_x;
    y  = next_y;
    yl = next_yl;

    auc  = next_auc & 0x7f;
    psw  = next_psw;
    c0   = next_c0 & 0xff;
    c1   = next_c1 & 0xff;
    c2   = next_c2 & 0xff;
    sioc = next_sioc & 0x3ff;
    srta = next_srta & 0xff; // only transmit address is tested
    sdx  = next_sdx;

    tdms = next_tdms;
    pioc = next_pioc;
    pdx0 = next_pdx0;
    pdx1 = next_pdx1;

    a0   = next_a0;
    a1   = next_a1;
}

int DSP16emu::get_register( int rfield ) {
    switch( rfield ) {
        case  0: return r0;
        case  1: return r1;
        case  2: return r2;
        case  3: return r3;
        case  4: return j;
        case  5: return k;
        case  6: return rb;
        case  7: return re;
        case  8: return pt;
        case  9: return pr;
        case 10: return pi;
        case 11: return sign_extend(i,11);
        case 16: return x;
        case 17: return y;
        case 18: return yl;
        case 19: return auc;
        case 20: return psw;
        case 21: return sign_extend(c0);
        case 22: return sign_extend(c1);
        case 23: return sign_extend(c2);
        case 24: return sioc;
        case 25: return srta;
        case 26: return sdx;
        case 27: return tdms;
        case 28: return pioc;
        case 29: return pdx0;
        case 30: return pdx1;
    }
    return 0;
}

void DSP16emu::set_register( int rfield, int v ) {
    v &= 0xffff;
    switch(rfield) {
        case  0: next_r0 = r0 = v; break;
        case  1: next_r1 = r1 = v; break;
        case  2: next_r2 = r2 = v; break;
        case  3: next_r3 = r3 = v; break;
        case  4: next_j  = j  = v; break;
        case  5: next_k  = k  = v; break;
        case  6: next_rb = rb = v; break;
        case  7: next_re = re = v; break;
        case  8: next_pt = pt = v; break;
        case  9: next_pr = pr = v; break;
        case 10: next_pi = pi = v; break;
        case 11: next_i  = i  = v & 0xfff; break;
        case 16: next_x  = x  = v; break;
        case 17: next_y  = y  = v;
            if( (auc>>6)==0 ) next_yl = yl = 0;
            break;
        case 18: next_yl = yl = v; break;
        case 19: next_auc   = v; break;
        case 20: next_psw   = v; break;
        case 21: next_c0 = c0 = v & 0xff; break;
        case 22: next_c1 = c1 = v & 0xff; break;
        case 23: next_c2 = c2 = v & 0xff; break;
        case 24: next_sioc = sioc = v & 0x3ff; break;
        case 25: srta = next_srta = v & 0xff; break;
        case 26: next_sdx   = v; break;
        case 27: next_tdms  = v; break;
        case 28: next_pioc  = v; break;
        case 29: next_pdx0  = v; break;
        case 30: next_pdx1  = v; break;
    }
}

int64_t DSP16emu::assign_high( int clr_mask, int64_t& dest, int val ) {
    const int64_t mask36 = 0xF'FFFF'FFFFL;
    dest &= ~(0xffffL<<16);
    val  &= 0xffff;
    dest |= (val<<16);
    // sign extension
    if( (dest>>31)&1 )
        dest |= 0xF'0000'0000;
    else
        dest &= 0x0'FFFF'FFFF;
    if( ((auc>>4) & clr_mask)==0 )
        dest &= ~0xffff; // clear low bits
    dest &= mask36;
    return dest;
}

int64_t DSP16emu::extend_p() {
    int64_t psh = p; // sign extension here is automatic
    switch( auc&3 ) {
        case 1: psh >>= 2; break;
        case 2: psh <<= 2; break;
    }
    return psh &0x1F'FFFF'FFFF;
}

int64_t DSP16emu::extend_y() {
    int64_t yext = (int16_t)y; // sign extension here is automatic
    yext<<=16;
    yext|=yl;
    yext &= 0x1F'FFFF'FFFFL;
    return yext;
}

int DSP16emu::extend_i() {
    int iext = i;
    if( iext>>11 ) iext |= 0xf000;
    return iext;
}

bool DSP16emu::CONparse( int op ) {
    bool lmi = psw&0x8000,
         leq = psw&0x4000,
         llv = psw&0x2000,
         lmv = psw&0x1000;
    bool v = false;
    switch( (op>>1)&0xf ) {
        case 0: v = lmi;
        case 1: v = leq;
        case 2: v = llv;
        case 3: v = lmv;
        case 4: v = false; // change to LFSR!
        case 5: v = c0&0x80==0; // positive
        case 6: v = c1&0x80==0; // positive
        case 7: v = true;
        case 8: v = !lmi && !leq;
    }
    if( op&1 ) v = !v;
    return v;
}

void DSP16emu::F12parse( int op, bool special, bool up_now ) {
    int64_t *ad, *next_ad, as;
    int ov0 = (psw&0x100)!=0;
    int ov1 = (psw&0x200)!=0;
    int* pov;
    int ovsat;
    int f = (op>>5)&0x3f;
    bool no_r = false;
    if( f&0x10 ) {
        as = a1;
    } else {
        as = a0;
    }
    if( f&0x20 ) {
        ad = &a1;
        next_ad = &next_a1;
        pov = &ov1;
        ovsat = (~auc>>3)&1;
    } else {
        ad = &a0;
        next_ad = &next_a0;
        pov = &ov0;
        ovsat = (~auc>>2)&1;
    }
    int64_t old_ad = *ad, r=*ad;
    bool flag_up = true;
    if ( as >> 35 )
        as |= 0x10'0000'0000; // sign extend to 36 bits
    switch( f&0xf ) {
        case 0:
            if( special ) {
                r =  as >> 1;
                if( as>>35 ) r |= 1L << 36; // keep sign
            } else {
                r = extend_p();
                p = x*y;
            }
            break;
        case 1:
            if( special ) {
                r = as << 1;
                r &= 0x1F'FFFF'FFFF;
            } else {
                r = as + extend_p();
                //printf("F1=1  ->  %lX + %lX = %lX\n", as, extend_p(), r);
                p = x*y;
            }
            break;
        case 2:
            if( special ) {
                r =  as >> 4;
                if( as>>35 ) r |= 0xFL << 33; // keep sign
            } else {
                p = x*y;
                flag_up = false;
                no_r = true;
            }
            break;
        case 3:
            if( special ) {
                r = as << 4;
                r &= 0x1F'FFFF'FFFF;
            } else {
                r = as - extend_p();
                p = x*y;
            }
            break;
        case 4:
            if( special ) {
                r = as >> 8;
                if( as>>35 ) r |= 0xFFL << 29; // keep sign
            } else {
                r = extend_p();
            }
            break;
        case 5:
            if( special ) {
                r = as << 8;
                r &= 0x1F'FFFF'FFFF;
            } else {
                r = as + extend_p();
            }
            break;
        case 6:
            if( special ) {
                r = as >> 16;
                if( as>>35 ) r |= 0xFFFFL << 21; // keep sign
            } else {
                flag_up = false;
                no_r = true;
            }
            break;
        case 7:
            if( special ) {
                r = as << 16;
                r &= 0x1F'FFFF'FFFF;
            } else {
                r = as - extend_p();
            }
            break;
        case 8:
            if( special ) {
                r = extend_p();
            } else {
                r = as | extend_y();
            }
            break;
        case 9:
            if( special ) {
                r += (as & 0x1F'FFFF'0000) + 0x1'0000; // aDh = aSh+1
            }
            r = as ^ extend_y();
            //printf("as ^ y => %lX = %lX ^ %lX\n", r, as, extend_y());
            break;
        case 10:
            r = as & extend_y();
            no_r = true;
            break;
        case 11:
            if( special ) { // round function
                r=as & ~0xFFFFL;
                if( (as>>15)&1 ) r += 0x1'0000; // round up
            }
            else {
                r = as - extend_y();
            }
            //printf("as - y => %lX = %lX - %lX\n", r, as, extend_y());
            no_r = true;
            break;
        case 12:
            r = extend_y(); // same function for special and non special
            //printf("           *********           r = extend_y  = %lX\n", r );
            break;
        case 13:
            //printf("as + y = %lX + %lX\n", as, extend_y());
            r = special ? 1L : as + extend_y();
            break;
        case 14:
            r = special ? as : as & extend_y();
            break;
        case 15:
            if( special ) {
                r = ~as + 1;
                r &= 0x1F'FFFF'FFFF;
            } else {
                r = as - extend_y();
            }
            break;
    }
    // update flags
    int leq = (r&0xF'FFFF'FFFFL) == 0;
    int sign_bits = (r>>31)&0x1F;
    int llv = ((r>>35)&1) != ((r>>36)&1); // number doesn't fit in 36-bit integer
    int lmv = sign_bits!=0 && sign_bits!=0x1F; // number doesn't fit in 32-bit integer
    const int sign = (r>>35)&1;
    int lmi = sign;
    *pov = llv;
    // store the final value
    if( verbose ) printf("Flags = %d%d%d%d - OVSAT %d - a%d<-a%d (F1=%X) - (%lX)\n", lmi, leq, llv, lmv,
             ovsat, (f>>5)&1, (f>>4)&1, f&0xf, r);
    if( lmv && ovsat ) {// saturate to a 32 bit value
    //     printf("Saturation\n");
        r = sign ? 0xF'8000'0000 : 0x0'7FFF'FFFF;
    }
    r &= 0x0F'FFFF'FFFFL;
    if( !no_r ) {
        *next_ad = r;
        if( up_now ) *ad = r;
    }
    if(flag_up) set_psw( lmi, leq, llv, lmv, ov0, ov1, up_now );
}

void DSP16emu::set_psw( int lmi, int leq, int llv, int lmv, int ov0, int ov1, bool up_now ) {
    next_psw =
          ((lmi&1)<<15) |
          ((leq&1)<<14) |
          ((llv&1)<<13) |
          ((lmv&1)<<12) |
          ((ov1&1)<< 9) |
          ((ov0&1)<< 4) |
          (((next_a1>>32)&0xf)<<5) |
           ((next_a0>>32)&0xf);
    if( up_now ) psw=next_psw;
}

int DSP16emu::Yparse( int Y, bool up_now ) {
    int* rpt;
    int* rpt_next;
    if( verbose ) {
        printf("Access to *r%d", (Y>>2)&&3);
        switch( Y&3 ) {
            case 0: puts(""); break;
            case 1: puts("++"); break;
            case 2: puts("--"); break;
            case 3: puts("++j"); break;
        }
    }
    switch( (Y>>2)&3 ) {
        case 0: rpt = &r0; rpt_next = &next_r0; break;
        case 1: rpt = &r1; rpt_next = &next_r1; break;
        case 2: rpt = &r2; rpt_next = &next_r2; break;
        case 3: rpt = &r3; rpt_next = &next_r3; break;
    }
    int delta;
    switch( Y&3 ) {
        case 0: delta=0;  break;
        case 1: delta=1;  break;
        case 2: delta=-1; break;
        case 3: delta=j;  break;
    }
    if( re==0 || re != *rpt || delta!=1 ) // virtual shift register feature
        *rpt_next = (*rpt+delta)&0xffff;
    else
        *rpt_next = rb;

    int retval = *rpt;
    if( up_now )
        *rpt = *rpt_next;
    return retval;
}

void DSP16emu::Yparse_write( int Y, int v ) {
    int addr = Yparse( Y, true );
    ram_write( addr, v );
}

int DSP16emu::Yparse_read( int Y, bool up_now ) {
    int addr = Yparse( Y, up_now );
    int v = ram_read( addr );
    return v;
}

int DSP16emu::ram_read( int a ) {
    a &= 0x7ff;
    int v = ram[ a ];
    v &= 0xffff;
    if( verbose ) printf("RAM read [%04X]=%04X\n", a, v );
    stats.ram_writes++;
    return v;
}

void DSP16emu::ram_write( int a, int v ) {
    a &= 0x7ff;
    v &= 0xffff;
    ram[ a ] = v;
    if( verbose ) printf("RAM write [%04X]=%04X\n", a, v );
    stats.ram_reads++;
}

int DSP16emu::parse_pt( int op ) {
    const int X = (op>>4)&1;
    int v = read_rom( pt ) & 0xffff;
    if( X )
        pt+=extend_i();
    else
        pt++;
    pt &= 0xffff;
    next_pt = pt;
    return v;
}

void DSP16emu::assign_acc( int aD, int v, bool up_now ) {
    int64_t *pr, *pnext;
    int64_t newv;
    if( aD ) {
        pr = &a1;
        pnext = &next_a1;
    } else {
        pr = &a0;
        pnext = &next_a0;
    }
    int64_t v64 = v;
    v64 &= 0xffff;
    v64 <<= 16;
    newv = v64 | (*pr & 0xffff );
    if( v &0x8000 )
        newv |= 0xf'0000'0000; // sign extend
    newv &= 0xF'FFFF'FFFFL;
    int clr = auc>>4;
    clr >>= aD;
    clr &=1;
    if( !clr ) newv &= ~0xffff; // clear low
    *pnext = newv;
    if( up_now ) *pr = newv;
}

void DSP16emu::step_aau_r( int* pr, int s ) {
    // this account for the virtual shift register feature
    if( s==1 && re!=0 && (*pr==re) ) {
        *pr = rb;
    } else {
        *pr += s;
        *pr &= 0xffff;
    }
}

void DSP16emu::parseZ( int op ) {
    int *py, *pnext;
    int old;
    if( op&0x10 ) {
        py    = &y;
        pnext = &next_y;
    } else {
        py    = &yl;
        pnext = &next_yl;
    }
    old = *py;
    // memory pointer
    int *pr, *prn;
    switch( (op>>2)&3 ) {
        case 0:
            pr  = &r0;
            prn = &next_r0;
            break;
        case 1:
            pr  = &r1;
            prn = &next_r1;
            break;
        case 2:
            pr  = &r2;
            prn = &next_r2;
            break;
        case 3:
            pr  = &r3;
            prn = &next_r3;
            break;
    }
    *pnext = ram_read( *pr );
    int step=0;
    switch( op&3 ) {
        case 1: step = 1; break;
        case 2: step =-1; break;
        case 3: step = j; break;
    }
    step_aau_r( pr, step );
    ram_write( *pr, old );
    switch( op&3 ) {
        case 0: step = 1; break;
        case 2: step = 2; break;
        case 3: step = k; break;
    }
    step_aau_r( pr, step );
    *prn = *pr;
    *py = *pnext;
    // Delete yl if necessary
    if( ((auc>>6)&1)==0 && (op&0x10)!=0 ) {
        //printf("YL cleared\n");
        next_yl = yl = 0;
    }
}

int DSP16emu::eval() {
    int op = read_rom(pc++) &0xffff;
    int delta=0;
    int aux, aux2;
    const int opcode = (op>>11) & 0x1f;

    next_pi = pc;
    update_regs();
    if(verbose) printf("OP=%04X (0x%X=%d)\n",op, opcode, opcode );
    switch( opcode ) {
        case 0: // goto JA
        case 1:
            pc = op&0xfff;
            next_pi = pc;
            pi = pc;
            delta=2;
            break;
        case 2: // short immediate
        case 3:
            aux = op & 0x1ff;
            //printf("DEBUG = %X\n", (pc>>9)&7);
            switch( (op>>9)&7 ) {
                case 0: next_j  = aux; if(next_j&0x100) next_j |= 0xff00; break;
                case 1: next_k  = aux; if(next_k&0x100) next_k |= 0xff00; break;
                case 2: next_rb = aux; break;
                case 3: next_re = aux; break;
                case 4: next_r0 = aux; break;
                case 5: next_r1 = aux; break;
                case 6: next_r2 = aux; break;
                case 7: next_r3 = aux; break;
            }
            delta=1;
            break;
        case 0x8: // aT = R
            aux = (op>>4)&0x3f;
            aux2 = get_register(aux);
            //printf("aT=R (%d)  -- op == %X\n",aux,op);
            if( (op>>10)&1 )
                a0 = assign_high( 1, next_a0, aux2 ); // 1 selects a0
            else
                a1 = assign_high( 2, next_a1, aux2 ); // 0 selects a1
            delta = 2;
            break;
        case 0x9: // R = a0
            aux  = (op>>4)&0x3f;
            delta = 2;
            set_register( aux, (a0>>16) & 0xffff );
            break;
        case 0xa: // long imm
            aux  = (op>>4)&0x3f;
            aux2 = read_rom(pc++);
            next_pi = pc;
            pi = pc;
            delta=2;
            //printf("R = imm    [%X] = %X\n", aux, aux2);
            set_register( aux, aux2 );
            break;
        case 0xc: // Y=R
            aux  = (op>>4)&0x3f;
            aux2 = get_register(aux);
            aux  = op&0xf;
            Yparse_write( aux, aux2 );
            // printf("Y = R [%X] = %X\n",aux,aux2);
            delta = 2;
            break;
        case 0xf: // R=Y
            aux  = (op>>4)&0x3f;
            aux2 = op&0xf;
            //printf("R=Y [%02X] = %X\n", aux, aux2);
            //printf("next a0 = %lX\n", next_a0 );
            set_register( aux, Yparse_read( aux2 ) );
            delta = 2;
            break;
        // F1 operations:
        case 0x14: // 20 Y=y[l] F1
            aux2 = (op&0x10) ? y : yl;
            aux2 &= 0xffff;
            F1parse( op );
            Yparse_write( op&0xf, aux2 );
            update_regs();
            delta = 2;
            break;
        case 7: // aT[l] = Y
            F1parse( op );
            aux = Yparse_read( op&0xf, false );
            assign_acc( ((~op)>>10)&1, aux, false );
            delta = 1;
            break;
        case 21: // Z:y F1
            F1parse( op, true );
            parseZ(op);
            delta = 2;
            break;
        case 22: // x=Y F1
            F1parse( op );
            aux = Yparse_read( op&0xf, false );
            next_x = aux;
            delta = 1;
            break;
        case 23: // y=Y F1
            F1parse( op );
            aux = Yparse_read( op&0xf, false );
            if( op&0x10 ) {
                next_y = aux;
                if( ((auc>>6)&1)==0 ) next_yl=0;
            }
            else
                next_yl = aux;
            delta = 1;
            break;
        case 25: // 0x19
        case 27:
            if( opcode==25 ) {
                aux  = a0 >> 16;
                aux2 = a0;
            } else {
                aux  = a1 >> 16;
                aux2 = a1;
            }
            aux  &=0xffff;
            aux2 &=0xffff;
            F1parse( op, true );
            next_y  = y  = aux;
            next_yl = yl = aux2;
            x = next_x = parse_pt(op);
            delta = in_cache ? 1 : 2;
            break;
        // case 28:
        case 31: // F1 y=Y x=*pt++[i]
            F1parse( op, true );
            aux = Yparse_read( op&0xf, !in_cache );
            //printf("next_a1 = %lX\n", next_a1);
            y = next_y = aux;
            if( ((auc>>6)&1)==0 ) yl=next_yl=0;
            x = next_x = parse_pt(op);
            delta = in_cache ? 1 : 2;
            break;
        case 4: // F1 Y=a1
        case 28: // 0x1C
            // get old value of accumulator
            if( opcode==4 )
                aux2 = (op&0x10) ? (a1>>16) : a1;
            else
                aux2 = (op&0x10) ? (a0>>16) : a0;
            aux2 &= 0xffff;
            F1parse( op );
            Yparse_write( op&0xf, aux2 );
            update_regs();
            delta = 2;
            break;
        case 6: // F1 Y
            F1parse( op );
            Yparse_read( op&0xf, false );
            delta = 1;
            break;
        // default:
    }
    ticks += delta;
    return delta;
}

int16_t DSP16emu::read_rom(int a) {
    a &= 0xfff;
    return rom[a];
}