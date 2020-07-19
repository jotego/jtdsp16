#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <map>

using namespace std;

typedef map<string,int> Labels;

class Bin {
    int16_t *buf;
    int pt;
    Labels labels;
public:
    const int max=8192;
    Bin();
    ~Bin();
    void restart() {pt=0;}
    int  len() { return pt; }
    void push(int v);
    void dump( const char *name );
    void add_label( const char *name );
    int get_label( const char *name, bool fallback=true );
};

int  assemble( ifstream& fin, Bin& bin );
int  make_rfield(const char *reg);
bool is_imm(const char *s, int& val);
bool is_ram( const char *s, int& val );
bool is_aTR( const char *s, int& val );

int main(int argc, char* argv[]) {
    // Get input file
    if( argc!=2 ) {
        cout << "ERROR: missing assembler input file\n";
        return 1;
    }
    const char *asmname = argv[1];
    ifstream asm_file(asmname);
    if( !asm_file.good() ) {
        cout << "ERROR: cannot open file " << asmname << '\n';
        return 2;
    }

    // Assemble
    Bin bin;
    for( int k=0; k<2; k++ ) {
        asm_file.clear();
        asm_file.seekg(0, ios_base::beg);
        if( !asm_file.good() ) {
            cout << "Error: cannot rewind on pass " << (k+1) << "\n";
            return 1;
        }
        bin.restart();
        int bad_line = assemble( asm_file, bin );
        if( bad_line ) {
            cout << "dsp16as error at line " << bad_line << '\n';
            return bad_line;
        }
        // cout << "Len = " << bin.len() <<'\n';
    }
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

void Bin::add_label( const char *name ) {
    // cout << "Label '" << name << "' = " << pt << '\n';
    labels[string(name)] = pt;
}

int Bin::get_label( const char *name, bool fallback ) {
    Labels::iterator k = labels.find(name);
    if( k!=labels.end() )
        return k->second;
    else {
        if( fallback )
            return strtol( name, NULL, 0);
        else
            return -1;
    }
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

#define BAD_LINE(str) { cout << "ERROR: " << str << " at line " << linecnt << "\n\t> " << line_cpy << '\n'; return linecnt; }

int assemble( ifstream& fin, Bin& bin ) {
    char    line_in[512];
    char    line_cpy[512];
    int     opcode=0, linecnt=1;

    while( fin.getline(line_in, 512).good() ) {
        char *paux, *line;
        int  aux;
        strcpy( line_cpy, line_in );
        line = line_in;
        // remove comments
        paux = strchr(line, '#');
        if( paux!= NULL ) *paux = 0;
        // strip all initial blanks
        while( *line==' ' || *line=='\t' ) line++;
        // strip all final blanks
        paux=line;
        while( *paux ) paux++;
        do {
            paux--;
            if( *paux==' ' || *paux=='\t' )
                *paux=0;
            else
                break;
        }while( paux>line );
        if( line[0]==0 || line[0]=='\n' ) // blank line
            continue;

        // parse line
        if( strchr(line,'=') ) {
            char *dest, *orig;
            dest = strtok(line, " \t=");
            if( dest==NULL ) continue;

            if( strcmp(dest,"move")==0 )
                dest = strtok( NULL," \t=");

            orig = strtok( NULL," \t=");
            // cout << setw(3) << linecnt << "> " << line_cpy << " - ";
            // cout << "Orig = " << orig << " Dest = " << dest << '\n';
            int rfield=make_rfield(dest);
            //cout << "DEST=" << dest << '\n';
            if( is_imm(orig, aux) ) {
                if( rfield==-1 ) BAD_LINE("Bad register name");
                if( aux < 512 && aux>=-257 && rfield<8 && rfield>=0 ) {
                    // Short immediate
                    opcode = 1<<12;
                    opcode |= ((rfield&7)^4)<<9;
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
            if( is_ram(orig, aux) ) { // Read from RAM
                if( rfield==-1 ) BAD_LINE("Bad register name")
                opcode = 0x1E << 10;
                opcode |= (rfield)<<4;
                opcode |= aux;
                bin.push(opcode);
            } else
            if( is_ram(dest, aux) ) { // Write to RAM
                rfield=make_rfield(orig);
                if( rfield==-1 ) BAD_LINE("Bad register name")
                opcode = 0xC << 11;
                opcode |= (rfield)<<4;
                opcode |= aux;
                bin.push(opcode);
            } else
            if( is_aTR(dest, aux)) {
                rfield=make_rfield(orig);
                if( rfield==-1 ) BAD_LINE("Bad register name")
                opcode = 4<<11;
                opcode |= aux << 10;
                opcode |= rfield <<4;
                bin.push(opcode);
            } else {
                BAD_LINE("bad syntax")
            }
        } else
        // Labels
        if( paux = strchr(line,':') ) {
            *paux = 0;
            bin.add_label( line );
            continue;
        } else {
            char *cmd = strtok( line, " \t" );
            if( cmd == NULL ) {
                BAD_LINE("Cannot break up in words")
            }
            char *rest = cmd + strlen(cmd) + 1;
            if( strcmp(cmd,"goto")==0 ) {
                aux = bin.get_label( rest );
                opcode = aux&0xFFF;
                bin.push(opcode);
            } else
            if( strcmp(cmd,"return")==0 ) {
                opcode  = 0x18<<11;
                bin.push(opcode);
            } else
            if( strcmp(cmd,"ireturn")==0 ) {
                opcode  = 0x18<<11;
                opcode |= 1 << 8;
                bin.push(opcode);
            } else
            if( strcmp(cmd,"call")==0 ) {
                if( strcmp(cmd,"pt")==0 ) {
                    opcode  = 0x18<<11;
                    opcode |= 3 << 8;
                } else {
                    aux = bin.get_label( rest );
                    opcode  = 8<<12;
                    opcode |= (aux&0xfff);
                }
                bin.push(opcode);
            }
        }
        linecnt++;
    }
    return 0;
}

#define MATCH(a,b) if(strcmp(reg,a)==0) return b;

int make_rfield(const char *reg) {
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

bool is_aTR( const char *s, int& val ) {
    if( strcmp(s,"a0")==0 ) { val=0; return true; }
    if( strcmp(s,"a1")==0 ) { val=1; return true; }
    return false;
}