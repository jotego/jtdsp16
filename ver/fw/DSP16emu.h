class DSP16emu {
    int16_t *rom, *ram;
    int16_t read_rom(int a);
    int next_j, next_k, next_rb, next_re, next_r0, next_r1, next_r2, next_r3;
    int next_pt, next_pr, next_pi, next_i;
    int next_x,  next_y,  next_yl;
    int next_auc, next_psw, next_c0, next_c1, next_c2, next_sioc, next_srta, next_sdx;
    int next_tdms, next_pioc, next_pdx0, next_pdx1;
    int64_t next_a0, next_a1;

    void    update_regs();
    int     get_register( int rfield );
    void    set_register( int rfield, int v );
    int64_t assign_high( int clr_mask, int64_t& dest, int val );
    int     sign_extend( int v, int msb=7 );
    void    Yparse( int Y, int v );
public:
    int pc, j, k, rb, re, r0, r1, r2, r3;
    int pt, pr, pi, i;
    int x, y, yl;
    int auc, psw, c0, c1, c2, sioc, srta, sdx;
    int tdms, pioc, pdx0, pdx1;
    int64_t a0, a1;

    int ticks;
    DSP16emu( int16_t* _rom );
    ~DSP16emu();
    int eval();
};

DSP16emu::DSP16emu( int16_t* _rom ) {
    pc=0;
    j = k = rb = re = r0 = r1 = r2 = r3 = 0;
    next_j = next_k = next_rb = next_re = next_r0 = next_r1 = next_r2 = next_r3 = 0;
    next_pt  = next_pr  = next_pi = next_i = 0;
    next_x   = next_y   = next_yl = 0;
    next_auc = next_psw = next_c0 = next_c1 = next_c2 = next_sioc = next_srta = next_sdx = 0;
    next_tdms = next_pioc = next_pdx0 = next_pdx1 = 0;
    next_a0 = a0 = next_a1 = a1 = 0;
    ticks=0;
    rom = _rom;
    ram = new int16_t[2048];
    for(int k=0; k<2048; k++) ram[k]=0;
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
        case 24: next_sioc  = v; break;
        case 25: srta = next_srta  = v&0xff; break;
        case 26: next_sdx   = v; break;
        case 27: next_tdms  = v; break;
        case 28: next_pioc  = v; break;
        case 29: next_pdx0  = v; break;
        case 30: next_pdx1  = v; break;
    }
}

int64_t DSP16emu::assign_high( int clr_mask, int64_t& dest, int val ) {
    const int64_t mask36 = 0xF'FFFF'FFFF;
    dest &= ~(0xffff<<16);
    val  &= 0xffff;
    dest |= (val<<16);
    if( ((auc>>4) & clr_mask)==0 )
        dest &= ~0xffff; // clear low bits
    dest &= mask36;
    return dest;
}

void DSP16emu::Yparse( int Y, int v ) {
    int* rpt;
    int* rpt_next;
    switch( (Y>>2)&3 ) {
        case 0: rpt = &r0; rpt_next = &next_r0; break;
        case 1: rpt = &r1; rpt_next = &next_r1; break;
        case 2: rpt = &r2; rpt_next = &next_r2; break;
        case 3: rpt = &r3; rpt_next = &next_r3; break;
    }
    ram[ (*rpt)&0x7ff ] = v;
    switch( Y&3 ) {
        case 1:
            if( re==0 || re != *rpt ) // virtual shift register feature
                *rpt = *rpt_next = (*rpt+1)&0xffff;
            else
                *rpt = *rpt_next = rb;
            break;
        case 2: *rpt = *rpt_next = (*rpt-1)&0xffff; break;
        case 3: *rpt = *rpt_next = (*rpt+j)&0xffff; break;
    }
}

int DSP16emu::eval() {
    int op = read_rom(pc++);
    int delta=0;
    int aux, aux2;

    next_pi = pc;
    update_regs();
    // printf("OP=%04X\n",op);
    switch( op>>11 ) {
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
            Yparse( aux, aux2 );
            delta = 2;
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