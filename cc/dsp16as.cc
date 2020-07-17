#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>

using namespace std;

class Bin {
    int16_t *buf;
    int pt;    
public:
    const int max=8192;
    Bin();
    ~Bin();
    void push(int v);
    void dump( const char *name );
};

void assemble( ifstream& fin, Bin& bin );
int  make_rfield(const char *reg);
bool is_imm(const char *s, int& val);
bool is_ram( const char *s, int& val );

int main(int argc, char* argv[]) {
    ifstream asm_file("test.asm");
    Bin bin;
    assemble( asm_file, bin );
    bin.dump("test.bin");
    return 0;
}

Bin::Bin() {
    pt=0;
    buf = new int16_t[max];
}

Bin::~Bin() {
    delete[] buf;
    pt=0;
}

void Bin::push(int v) {
    if( pt<max ) {
        // swap bytes
        int a,b;
        a = v>>8;
        b = (v&0xff)<<8;
        v = b | a;
        buf[pt++] = v &0xffff;
    }
}

void Bin::dump( const char *name ) {
    ofstream bin_file(name, ios_base::binary);
    bin_file.write( (char*)buf, pt*2);
    for( int k=pt; k<max; k++ ) {
        char ff[2]= { ~0, ~0 };
        bin_file.write( ff, 2 );
    }
}

void assemble( ifstream& fin, Bin& bin ) {
    char    line[512];
    int     opcode=0;

    while( fin.getline(line, 512).good() ) {
        if( strchr(line,'=') ) {
            char *dest, *orig;
            int  aux;
            dest = strtok(line, " \t=");
            if( strcmp(dest,"move")==0 )
                dest = strtok( NULL," \t=");
            orig = strtok( NULL," \t=");
            int rfield=make_rfield(dest);
            if( is_imm(orig, aux) ) {
                if( aux < 512 && aux>=-257 && rfield<8 && rfield>=0 ) {
                    // Short immediate
                    opcode = 1<<12;
                    opcode |= (rfield&7)<<9;
                    opcode |= aux&0x1ff;
                    bin.push(opcode);
                } else {
                    // long immediate
                    opcode  = 0x14 << 10;
                    opcode |= (rfield&0x3f)<<4;
                    bin.push(opcode);
                    bin.push(aux);
                }
            } else
            if( is_ram(orig, aux) ) {
                opcode = 0x1E << 10;
                opcode |= (rfield)<<4;
                opcode |= aux;
                bin.push(opcode);
            }
        }
    }
}

#define MATCH(a,b) if(strcmp(reg,a)==0) return b;

int  make_rfield(const char *reg) {
    MATCH("r0",0);
    MATCH("r1",1);
    MATCH("r2",2);
    MATCH("r3",3);
    MATCH("j",4);
    MATCH("k",5);
    MATCH("rb",6);
    MATCH("re",7);
    MATCH("pt",8);
    MATCH("pr",9);
    MATCH("pi",10);
    MATCH("i",11);
    MATCH("x",16);
    MATCH("y",17);
    MATCH("yl",18);
    MATCH("auc",19);
    MATCH("psw",20);
    MATCH("c0",21);
    MATCH("c1",22);
    MATCH("c2",23);
    MATCH("sioc",24);
    MATCH("srta",25);
    MATCH("sdx",26);
    MATCH("tdms",27);
    MATCH("pioc",28);
    MATCH("pdx0",29);
    MATCH("pdx1",30);
    return -1;
}

bool is_imm(const char *s, int& val) {
    if( strchr(s, '*') ) return false;
    if( strchr(s, 'r') ) return false;
    val = strtol( s, NULL, 0);
    return true;
}

bool is_ram( const char *s, int& val ) {
    if( s[0] != '*' || s[1]!='r' ) return false;
    int r = (int)(s[2] - '0');
    if( r<0 || r>3 ) return false;
    const char *rest = s+3;
    int post;
    if( strcmp(rest,"")==0 ) post=0;
    if( strcmp(rest,"++")==0 ) post=1;
    if( strcmp(rest,"--")==0 ) post=2;
    if( strcmp(rest,"++j")==0 ) post=3;
    val = (r<<2) | post;
    return true;
}