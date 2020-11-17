#include "vcd.h"
#include <fstream>
#include <cstring>
#include <iostream>

using namespace std;

VCDfile::VCDfile( const char *fname, int downscale ) {
    ifstream fin( fname );
    string str;
    line=1;
    scale = downscale;
    while( getline( fin, str ) ) {
        string cmd;
        int blank = str.find_first_of(" \t");
        cmd = blank ? str.substr(0,blank) : str;
        try {
            if( cmd=="$var" ) parse_var(str);
        } catch( const char *msg ) {
            char err_msg[256];
            int len = strlen(msg);
            strncpy( err_msg, msg, 200 );
            char *n = err_msg+len;
            sprintf(n," at line %d.", line );
            throw err_msg;
        }
        if( cmd=="$dumpvars" ) break;
        line ++;
    }
    parse_t0( fin );
    parse_rest( fin );
    rewind();
}

void VCDfile::parse_var( const string& str ) {
    char *s = new char[str.size()+1];
    string handle, name;
    strcpy( s, str.c_str() );
    char *token = strtok( s, " \t" );
    try{
        // move across the line
        if( strcmp(token,"$var")!=0 ) throw runtime_error("VCD: expecting $var");
        token = strtok(NULL," \t");
        if( token==NULL || (strcmp(token,"wire")!=0 && strcmp(token,"integer")!=0)
            ) throw runtime_error("VCD: expecting wire or integer");
        token = strtok(NULL," \t");
        // Get width
        if( token==NULL ) throw runtime_error("VCD: expecting signal width");
        int w = atoi( token );
        if( w<1 || w>64 ) throw runtime_error("VCD: signal width not supported");
        token = strtok(NULL," \t");
        // Get handle
        if( token==NULL ) throw runtime_error("VCD: expecting signal token");
        handle = token;
        token = strtok(NULL," \t");
        // Get full name
        if( token==NULL ) throw runtime_error("VCD: expecting signal name");
        name = token;
        // Create signal
        const int64_t mask = w==64 ? ~0L : (1L<<w)-1;
        VCDsignal *sig = new VCDsignal( name, mask );
        signals[name] = sig;
        handlers[handle] = sig;
        delete[] s;
        s = nullptr;
    } catch( ... ) {
        delete[] s;
        throw;
    }
}

void VCDfile::parse_value( int64_t t, const string& str ) {
    string handler;
    int64_t v;
    if( str.at(0)=='b' ) {
        v=parse_bin(str.substr(1));
        handler = str.substr( str.find_first_of(' ')+1 );
    } else {
        v = str.at(0)=='1';
        handler = str.substr(1);
    }
    handlers[handler]->push( t/scale, v );
}

void VCDfile::parse_t0( ifstream& fin ) {
    string str;
    while( getline( fin, str ) ) {
        if( str.find_first_of("$end")==0 ) { line++; return; }
        parse_value( 0, str );
        line++;
    };
}

void VCDfile::parse_rest( ifstream& fin ) {
    string str;
    int64_t t=0L;
    while( getline( fin, str ) ) {
        if( str.at(0)=='#' ) {
            sscanf( str.c_str()+1, "%ld", &t );
        } else parse_value( t, str );
        line++;
    };
}

int64_t VCDfile::parse_bin( const string& str) {
    int64_t v=0;
    const char *c = str.c_str();
    while(*c!=' ') {
        v<<=1;
        if( *c=='1' ) v|=1;
        c++;
    }
    return v;
}

VCDfile::~VCDfile() {
    for( auto k : signals ) {
        delete k.second;
    }
}

void VCDfile::rewind() {
    for( auto k : signals ) {
        k.second->rewind();
    }
}

int64_t VCDfile::forward(int64_t time) {
    int64_t next=0;
    for( auto k : signals ) {
        int64_t nt = k.second->forward(time);
        if( nt > next) next=nt;
    }
    return next;
}

VCDsignal* VCDfile::get( const std::string& name ) {
    sigmap::iterator k = signals.find(name);
    return k==signals.end() ? nullptr : k->second;
}

VCDsignal* VCDfile::get_or_throw( const std::string& name ) {
    VCDsignal *s = get(name);
    if( s==nullptr ) {
        char s[256];
        sprintf(s, "Cannot find signal name %s in VCD file", name.substr(0,50).c_str() );
        throw runtime_error(s);
    }
    return s;
}

int64_t VCDfile::forward(std::string name, int64_t val) {
    VCDsignal *p = get_or_throw(name);
    int64_t time = p->next(val);
    forward( time );
    return time;
}

void VCDfile::report() {
    printf("VCD signals:");
    for( auto s : signals ) {
        printf("\n\t%s (%4d points - ends at %14ld)", s.first.c_str(), s.second->data_points(),
            s.second->get_tend() );
    }
    putchar('\n');
}


////////////////////////////////////////////////////////////////////////////////

VCDsignal::VCDsignal( const std::string& _name, int64_t _mask ) {
    name = _name;
    mask = _mask;
    k = points.end();
    n = points.end();
}

void VCDsignal::push( int64_t time, int64_t val ) {
    VCDpoint pt;
    pt.time = time;
    pt.val = val;
    points.push_back( pt );
}

void VCDsignal::rewind() {
    k = points.begin();
    n = k; n++;
}

int64_t VCDsignal::forward( int64_t time ) {
    static bool ended=false;
    while( time > k->time && n!=points.end() ) {
        k++;
        n++;
    }
    if(n==points.end() && !ended) {
        printf("%s passed end\n", name.c_str());
        ended=true;
    }
    return n==points.end() ? k->time : n->time;
}

int64_t VCDsignal::get_tend() {
    return points.back().time;
}

int64_t VCDsignal::cur() {
    return k->val;
}

int64_t VCDsignal::next(int64_t val) {
    val &= mask;
    while( k->val != val && n!=points.end() ) {
        k++;
        n++;
    }
    return k->time;
}