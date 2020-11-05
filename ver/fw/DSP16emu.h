class DSP16emu {
    int16_t *rom;
    int16_t read_rom(int a);
public:
    int pc;
    int ticks;
    DSP16emu( int16_t* _rom );
    int eval();
};

DSP16emu::DSP16emu( int16_t* _rom ) {
    pc=0;
    ticks=0;
    rom = _rom;
}

int DSP16emu::eval() {
    int op = read_rom(pc++);
    int delta=0;
    switch( op>>11 ) {
        case 0: // goto JA
        case 1:
            pc = op&0xfff;
            delta=2;
            break;
    }
    ticks += delta;
    return delta;
}

int16_t DSP16emu::read_rom(int a) {
    a &= 0xfff;
    return rom[a];
}