class DSP16emu {
    int16_t *rom;
    int16_t read_rom(int a);
    int next_j, next_k, next_rb, next_re, next_r0, next_r1, next_r2, next_r3;
    int next_pt, next_pr, next_pi, next_i;
    int next_x,  next_y,  next_yl;
    int next_auc, next_psw, next_c0, next_c1, next_c2, next_sioc, next_srta, next_sdx;
    int next_tdms, next_pioc, next_pdx0, next_pdx1;
    void update_regs();
public:
    int pc, j, k, rb, re, r0, r1, r2, r3;
    int pt, pr, pi, i;
    int x, y, yl;
    int auc, psw, c0, c1, c2, sioc, srta, sdx;
    int tdms, pioc, pdx0, pdx1;
    int ticks;
    DSP16emu( int16_t* _rom );
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
    ticks=0;
    rom = _rom;
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

    auc  = next_auc;
    psw  = next_psw;
    c0   = next_c0;
    c1   = next_c1;
    c2   = next_c2;
    sioc = next_sioc;
    srta = next_srta;
    sdx  = next_sdx;

    tdms = next_tdms;
    pioc = next_pioc;
    pdx0 = next_pdx0;
    pdx1 = next_pdx1;
}

int DSP16emu::eval() {
    int op = read_rom(pc++);
    int delta=0;
    int aux, aux2;

    next_pi = pc;
    update_regs();
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
        case 0xa: // long imm
            aux  = (op>>4)&0x3f;
            aux2 = read_rom(pc++);
            next_pi = pc;
            pi = pc;
            delta=2;
            //printf("DEBUG = %X\n", aux);
            switch(aux) {
                case  0: next_r0 = r0 = aux2; break;
                case  1: next_r1 = r1 = aux2; break;
                case  2: next_r2 = r2 = aux2; break;
                case  3: next_r3 = r3 = aux2; break;
                case  4: next_j  = j  = aux2; break;
                case  5: next_k  = k  = aux2; break;
                case  6: next_rb = rb = aux2; break;
                case  7: next_re = re = aux2; break;
                case  8: next_pt = pt = aux2; break;
                case  9: next_pr = pr = aux2; break;
                case 10: next_pi = pi = aux2; break;
                case 11: next_i  = i  = aux2 & 0xfff; break;
                case 16: next_x  = x  = aux2; break;
                case 17: next_y  = y  = aux2; break;
                case 18: next_yl = yl = aux2; break;
                case 19: next_auc   = aux2; break;
                case 20: next_psw   = aux2; break;
                case 21: next_c0 = c0 = aux2 & 0xff; break;
                case 22: next_c1 = c1 = aux2 & 0xff; break;
                case 23: next_c2 = c2 = aux2 & 0xff; break;
                case 24: next_sioc  = aux2; break;
                case 25: next_srta  = aux2; break;
                case 26: next_sdx   = aux2; break;
                case 27: next_tdms  = aux2; break;
                case 28: next_pioc  = aux2; break;
                case 29: next_pdx0  = aux2; break;
                case 30: next_pdx1  = aux2; break;
            }
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