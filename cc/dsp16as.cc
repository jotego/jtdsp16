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

class strcopy {
    char *buf;
public:
    strcopy( const char *s ) {
        buf=new char[strlen(s)];
        strcpy(buf,s);
    }
    ~strcopy() { delete[] buf;}
    char *get_buf() { return buf; }
};

int  assemble( ifstream& fin, Bin& bin );
int  make_rfield(const char *reg);
bool is_imm(const char *s, int& val);
bool is_ram( const char *s, int& val );
bool is_aTR( const char *s, int& val );
bool is_alu( char *str, int& op );
bool parse_if( char **str, int& op, char* err_msg);
void strip_blanks( char *s );
void simplify( char *s );

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

#define BAD_LINE(str) { cout << "ERROR: " << str << " at line " << linecnt << '\n'; return linecnt; }
#define NOTINCACHE    if(cache.incache()) { cout << "ERROR: instruction not cacheable at line " << linecnt << '\n'; return linecnt; }

class CacheLoop {
    bool inloop;
    int  ni, k;
    int cache_mem[127];
    char *msg;
    Bin& bin;
public:
    CacheLoop( Bin& _bin) : bin(_bin) {
        inloop=false; ni=0; k=0;
        msg = new char[512];
        *msg=0;
    }
    ~CacheLoop() {
        delete[] msg;
        msg=0;
    }
    void push(int op);
    void dump();
    const char *get_msg() const { return msg; }
    bool parse( char *line );
    bool error() { return *msg!=0; }
    bool incache() { return inloop; }
};

void CacheLoop::push(int op) {
    if( inloop ) {
        if(ni>126 && *msg==0) {
            strcpy(msg,"Cache only has room for 127 instructions");
        }
        else
            cache_mem[ni++] = op;
    } else {
        bin.push(op);
    }
}

void CacheLoop::dump() {
    int op = 7<<12;
    op |= ni<<7;
    op |= k;
    bin.push(op);
    for( int k=0; k<ni; k++ ) {
        bin.push( cache_mem[k] );
    }
}

bool CacheLoop::parse( char *line ) {
    strcopy cpy( line );

    //cout << "Parsing " << cpy.get_buf() << endl;
    char *tok = strtok( cpy.get_buf()," ");

    if(strcmp(tok,"do")==0) { // do start
        if( inloop ) {
            strcpy(msg,"cannot nest do loops");
            return true;
        }
        tok=strtok(NULL," ");
        if(strchr(tok,'{')!=NULL) {
            strcpy(msg,"A space must exist between do K and {");
            return true;
        }
        k = strtol(tok,NULL,0);
        if( k<2 || k>127 ) {
            strcpy(msg,"K in 'do K' must be between 2 and 127");
            return true;
        }
        tok=strtok(NULL,"");
        if(strcmp(tok,"{")!=0) {
            strcpy(msg,"A new line must come after { in 'do K {' statements");
            return true;
        }
        ni=0;
        inloop = true;
        return true;
    }
    if( strchr(tok,'}') ) {
        if(!inloop ) {
            strcpy(msg,"Unexpecte loop end }");
            return true;
        }
        if(strcmp(tok,"}")!=0) {
            strcpy(msg,"The loop end } must be in its own line");
            return true;
        }
        dump();
        inloop = false;
        return true;
    }
    return *msg==0 ? false : true; // force a true to parse error messages
}

int assemble( ifstream& fin, Bin& bin ) {
    char      line_in[512];
    char      line_cpy[512];
    char      err_msg[512];
    int       opcode=0, linecnt=0;
    CacheLoop cache(bin);

    while( fin.getline(line_in, 512).good() ) {
        char *paux, *line;
        int  aux;
        bool gotoif=false;
        strcpy( line_cpy, line_in );
        linecnt++;
        line = line_in;
        simplify(line);

        if( line[0]==0 || line[0]=='\n' ) // blank line
            continue;

        // parse line
        if( cache.parse(line) ) {
            if( cache.error() )
                BAD_LINE( cache.get_msg() )
            continue;
        }
        if( strncmp(line,"redo ",5)==0 ) {
            aux=strtol(line+5,NULL,0);
            if(aux<2 || aux>127) BAD_LINE("1<K<128 for redo K")
            aux|=7<<12;
            bin.push(aux);
            continue;
        }
        strip_blanks(line);
        if( is_alu(line, aux) ) {
            //cout << "ALU\n";
            cache.push(aux);
        } else
        if( strchr(line,'=') ) {
            char *dest, *orig;
            bool move=false;

            if( strncmp(line,"move",4)==0 ) {
                line+=4;
                move=true;
            }

            dest = strtok(line, " \t=");
            if( dest==NULL ) continue;


            orig = strtok( NULL," \t=");
            // cout << setw(3) << linecnt << "> " << line_cpy << " - ";
            // cout << "Orig = " << orig << " Dest = " << dest << '\n';
            int rfield=make_rfield(dest);
            //cout << "DEST=" << dest << '\n';
            if( is_imm(orig, aux)  && !move ) {
                if( rfield==-1 ) BAD_LINE(("(imm) Bad register name "+string(dest)))
                if( aux < 512 && aux>=-257 && rfield<8 && rfield>=0 ) {
                    // Short immediate
                    opcode = 1<<12;
                    opcode |= ((rfield&7)^4)<<9;
                    opcode |= aux&0x1ff;
                    cache.push(opcode);
                } else {
                    // long immediate
                    opcode  = 0x14 << 10;
                    opcode |= (rfield&0x3f)<<4;
                    cache.push(opcode);
                    cache.push(aux);
                }
            } else
            if( is_ram(orig, aux) ) { // Read from RAM
                if( rfield==-1 ) BAD_LINE(("(ram read) Bad register name "+string(dest)))
                opcode = 0x1E << 10;
                opcode |= (rfield)<<4;
                opcode |= aux;
                cache.push(opcode);
            } else
            if( is_ram(dest, aux) ) { // Write to RAM
                rfield=make_rfield(orig);
                if( rfield==-1 ) {
                    int as;
                    if( !is_aTR(orig, as) ) BAD_LINE(("(ram write) Bad register name "+string(orig)))
                    opcode = ( as ? 4 : 28) << 11;
                    opcode |= aux;
                    opcode |= (6<<5); // F1 NOP
                    opcode |= 0x10; // select high part of a0/a1

                } else {
                    opcode = 0xC << 11;
                    opcode |= (rfield)<<4;
                    opcode |= aux;
                }
                cache.push(opcode);
            } else
            if( is_aTR(dest, aux)) {
                rfield=make_rfield(orig);
                if( rfield==-1 ) BAD_LINE(("(aT=R) Bad register name "+string(orig)))
                opcode = 8<<11;
                opcode |= (1-aux) << 10;
                opcode |= rfield <<4;
                cache.push(opcode);
            } else {
                BAD_LINE("bad syntax")
            }
        } else {
            // Restores the blanks in the line
            strcpy( line, line_cpy);
            simplify( line );
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
                if( parse_if( &cmd, opcode, err_msg ) ) {
                    if(strlen(err_msg)>0) BAD_LINE(err_msg)
                    gotoif=true;
                    bin.push(opcode);
                    rest=strtok(NULL,"");
                } // note there is no else here
                if( strcmp(cmd,"goto")==0 ) {
                    NOTINCACHE
                    aux = bin.get_label( rest );
                    opcode = aux&0xFFF;
                    bin.push(opcode);
                } else
                if( strcmp(cmd,"return")==0 ) {
                    NOTINCACHE
                    opcode  = 0x18<<11;
                    bin.push(opcode);
                } else
                if( strcmp(cmd,"call")==0 ) {
                    NOTINCACHE
                    if( strcmp(cmd,"pt")==0 ) {
                        opcode  = 0x18<<11;
                        opcode |= 3 << 8;
                    } else {
                        aux = bin.get_label( rest );
                        opcode  = 8<<12;
                        opcode |= (aux&0xfff);
                    }
                    bin.push(opcode);
                } else
                if( strcmp(cmd,"ireturn")==0 ) {
                    NOTINCACHE
                    if(gotoif) {
                        BAD_LINE("ireturn cannot be part of an if expression")
                    }
                    opcode  = 0x18<<11;
                    opcode |= 1 << 8;
                    bin.push(opcode);
                } else
                if(gotoif) {
                    BAD_LINE("no statement after if expression")
                } else
                {
                    BAD_LINE("Syntax error")
                }
            }
        }
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
    if( make_rfield(s)!=-1 ) return false;
    if( strchr(s, '*') ) return false;
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

void strip_blanks( char *s ) {
    const int len = strlen(s);
    char *scan=s;
    if( len<= 0 ) return;

    char *buf = new char[ len ];
    char *aux = buf;
    while( *scan && *scan != '#') { // ignores comments too
        if( *scan != ' ' && *scan != '\t' ) *aux++=*scan;
        scan++;
    }
    *aux=0;
    //cout << "From " << s << " to " << buf << endl;
    strcpy( s, buf );
    delete []buf;
}

// Simplify is similar to strip blanks but it only removes:
// initial blanks
// duplicated blanks
// comments
void simplify( char *s ) {
    const int len = strlen(s);
    char *scan=s;
    if( len<= 0 ) return;
    bool first=true;

    char *buf = new char[ len ];
    char *aux = buf;
    while( *scan && *scan != '#') { // ignores comments too
        if( *scan != ' ' && *scan != '\t' ) {
            *aux++=*scan;
            first = true;
        }
        else {
            if( first ) *aux++=' ';
            first = false;

        }
        scan++;
    }
    *aux=0;
    //cout << "From " << s << " to " << buf << endl;
    strcpy( s, buf );
    delete []buf;
}

#define AUXCMP(a) (strcmp(aux,a)==0)

bool is_alu( char *str, int& op ) {
    strcopy copy(str);
    char *aux=strtok(copy.get_buf(),",");
    int d=-1, s=-1,at=-1;
    int pre_f1=-1;
    int y_field = -1;
    //cout << "is_alu: aux=>  " << aux << endl;
    if( AUXCMP("p=x*y") ){
        pre_f1 = 2;
        aux = strtok(NULL,","); // get the *r0++ part
    } else {
        if( aux[1]=='0' || aux[1]=='1')
            { d=aux[1]-'0'; aux[1]='x'; }
        else
            if( aux[1]!='o' && d==-1 ) return false;
        if( strlen(aux)>=5 ) {
            if(aux[4]=='0' || aux[4]=='1') {
                s=aux[4]-'0';
                aux[4]='x';
            }
        }
        //cout << "AUX=" << aux << endl;
        if( AUXCMP("ax=p")    ) pre_f1 = 4;
        if( AUXCMP("ax=ax+p") ) pre_f1 = 5;
        if( AUXCMP("nop")     ) pre_f1 = 6;
        if( AUXCMP("ax=ax-p") ) pre_f1 = 7;
        if( AUXCMP("ax=ax|y") ) pre_f1 = 8;
        if( AUXCMP("ax=ax^y") ) pre_f1 = 9;
        if( AUXCMP("ax&y")    ) { s=d; d=0; pre_f1 = 10; }
        if( AUXCMP("ax-y")    ) { s=d; d=0; pre_f1 = 11; }
        if( AUXCMP("ax=y")    ) pre_f1 = 12;
        if( AUXCMP("ax=ax+y") ) pre_f1 = 13;
        if( AUXCMP("ax=ax&y") ) pre_f1 = 14;
        if( AUXCMP("ax=ax-y") ) pre_f1 = 15;
        aux = strtok(NULL,",");
        if( aux ) {
            if( strcmp(aux,"p=x*y")==0 ) {
                if( pre_f1>=4 && pre_f1<=7)
                    pre_f1-=4;
                else
                    return false;
                aux = strtok(NULL,","); // get the *r0++ part
            }
        }
        if(s==-1) s=0;
        if(d==-1) d=0;
        if(pre_f1==-1) return false;
    }
    if( aux ) {
        char* equpos= strchr(aux,'=');
        if( equpos ) { // picks the a0= optional part
            if( aux[0]!='a') return false;
            at = aux[1]-'0';
            aux=equpos+1;
        }
        if( aux[2]>='0' || aux[2] <='3' ) y_field = aux[2]-'0';
        else return false;
        aux[2]='x';
        if( AUXCMP("*rx")    ) y_field |= 0;
        if( AUXCMP("*rx++")  ) y_field |= 1;
        if( AUXCMP("*rx--")  ) y_field |= 2;
        if( AUXCMP("*rx++j") ) y_field |= 3;
        if( y_field==-1 ) return false;
    } else {
        y_field = 0;
    }
    // makes OP
    op=0;
    if( at!=-1 ) {
        if( at==d ) return false;
        op |= 1<<11;
    }
    if( d==-1 ) d=0;
    if( s==-1 ) s=0;
    op = (d<<10) | (s<<9) | (pre_f1<<5) | (y_field);
    op |= 3<<12;
    return true;
}

bool parse_if( char **str, int& op, char* err_msg) {
    *err_msg = 0;
    if(strcmp(*str,"if")!=0) return false;
    *str=strtok(NULL," ");
    if( *str==NULL ) {
        strcpy(err_msg,"line incomplete after if keyword");
        return true;
    }
    int con=-1;
    if( strcmp(*str,"mi")==0    ) con=0;
    if( strcmp(*str,"pl")==0    ) con=1;
    if( strcmp(*str,"eq")==0    ) con=2;
    if( strcmp(*str,"ne")==0    ) con=3;
    if( strcmp(*str,"lvs")==0   ) con=4;
    if( strcmp(*str,"lvc")==0   ) con=5;
    if( strcmp(*str,"mvs")==0   ) con=6;
    if( strcmp(*str,"mvc")==0   ) con=7;
    if( strcmp(*str,"heads")==0 ) con=8;
    if( strcmp(*str,"tails")==0 ) con=9;
    if( strcmp(*str,"c0ge")==0  ) con=10;
    if( strcmp(*str,"c0lt")==0  ) con=11;
    if( strcmp(*str,"c1ge")==0  ) con=12;
    if( strcmp(*str,"c1lt")==0  ) con=13;
    if( strcmp(*str,"true")==0  ) con=14;
    if( strcmp(*str,"false")==0 ) con=15;
    if( strcmp(*str,"gt")==0    ) con=16;
    if( strcmp(*str,"le")==0    ) con=17;
    if( con==-1 ) {
        strcpy(err_msg,"Wrong condition expression");
        return true;
    }
    *str=strtok(NULL, " ");
    if( *str==NULL ) {
        strcpy(err_msg,"line incomplete after if condition");
        return true;
    }
    op=13<<12;
    op|=con;
    return true;
}