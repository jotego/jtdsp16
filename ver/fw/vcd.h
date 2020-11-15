#ifndef __VCDPARSE_H
#define __VCDPARSE_H

#include <list>
#include <map>
#include <string>

class VCDpoint {
    int64_t time, val;
};

class VCDsignal {
    std::list<VCDpoint> points;
    std::list<VCDpoint>::iterator k;
    int64_t mask;
public:
    VCDsignal( const std::string& name, int64_t mask );
    void push( int64_t time, int64_t val );
    void rewind();
    int64_t move_to(int64_t time);
    int64_t cur();
};

class VCDfile {
    std::map<std::string,VCDsignal*> signals, handlers;
    void parse_var  ( const std::string& str );
    void parse_value( int64_t t, const std::string& str );
    void parse_t0   ( std::ifstream& fin );
    void parse_rest ( std::ifstream& fin );
    int64_t parse_bin( const std::string& );
    int line;
public:
    VCDfile( const char *fname );
    VCDsignal* get_signal( const char *name );
    ~VCDfile();
};

#endif