#include "vcd.h"
#include <fstream>
#include <cstring>

using namespace std;

VCDfile::VCDfile( const char *fname ) {
    ifstream fin( fname );
    string str;
    line=1;
    while( getline( fin, str ) ) {
        size_t blank=str.find_first_of(' ');
        if( blank==string::npos ) continue;
        string cmd=str.substr(blank-1);
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
}

void VCDfile::parse_var( const string& str ) {
    char *s = new char[str.size()+1];
    string handle, name;
    strcpy( s, str.c_str() );
    char *token = strtok( s, " \t" );
    try{
        // move across the line
        if( strcmp(token,"$var")!=0 ) throw "VCD: expecting $var";
        token = strtok(NULL," \t");
        if( token==NULL || strcmp(token,"wire")!=0 ) throw "VCD: expecting wire";
        token = strtok(NULL," \t");
        // Get width
        if( token==NULL ) throw "VCD: expecting signal width";
        int w = atoi( token );
        if( w<1 || w>64 ) throw "VCD: signal width not supported";
        token = strtok(NULL," \t");
        // Get handle
        if( token==NULL ) throw "VCD: expecting signal token";
        handle = token;
        token = strtok(NULL," \t");
        // Get full name
        if( token==NULL ) throw "VCD: expecting signal name";
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
    handlers[handler]->push( 0, v );
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
        if( str.at(0)='#' ) {
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