class DSP16emu {
    int16_t *rom;
    int16_t read_rom(int a);
public:
    int pc, j, k, rb, re, r0, r1, r2, r3;
    int ticks;
    DSP16emu( int16_t* _rom );
    int eval();
};

DSP16emu::DSP16emu( int16_t* _rom ) {
    pc=0;
    j = k = rb = re = r0 = r1 = r2 = r3 = 0;
    ticks=0;
    rom = _rom;
}

int DSP16emu::eval() {
    int op = read_rom(pc++);
    int delta=0;
    int aux;
    switch( op>>11 ) {
        case 0: // goto JA
        case 1:
            pc = op&0xfff;
            delta=2;
            break;
        case 2: // short immediate
        case 3:
            aux = op & 0x1ff;
            //printf("DEBUG = %X\n", (pc>>9)&7);
            switch( (op>>9)&7 ) {
                case 0: j  = aux; if(j&0x100) j |= 0xff00; break;
                case 1: k  = aux; if(k&0x100) k |= 0xff00; break;
                case 2: rb = aux; break;
                case 3: re = aux; break;
                case 4: r0 = aux; break;
                case 5: r1 = aux; break;
                case 6: r2 = aux; break;
                case 7: r3 = aux; break;
            }
            delta=1;
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