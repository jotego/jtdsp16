#ifndef __VCDPARSE_H
#define __VCDPARSE_H

#include <list>
#include <map>
#include <string>

struct VCDpoint {
    int64_t time, val;
};

class VCDsignal {
    std::list<VCDpoint> points;
    std::list<VCDpoint>::iterator k, n;
    int64_t mask;
    std::string name;
public:
    VCDsignal( const std::string& _name, int64_t _mask );
    void push( int64_t time, int64_t val );
    void rewind();
    void forward(int64_t time);
    int64_t cur();
};

class VCDfile {
    typedef std::map<std::string,VCDsignal*> sigmap;
    sigmap signals, handlers;
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
    // move all signals
    void rewind();
    void forward(int64_t time);
    VCDsignal* get( std::string name );
};

#endif