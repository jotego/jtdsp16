#ifndef __VCDPARSE_H
#define __VCDPARSE_H

#include <list>
#include <map>
#include <string>

struct VCDpoint {
    int64_t time, val;
};


class VCDsignal {
public:
    using pointlist=std::list<VCDpoint>;
private:
    pointlist points;
    pointlist::iterator k, n;
    int64_t mask;
    std::string name;
public:
    VCDsignal( const std::string& _name, int64_t _mask );
    void push( int64_t time, int64_t val );
    void rewind();
    int64_t forward(int64_t time);
    int64_t next(int64_t val);
    int64_t cur();
    int64_t get_tend();
    //int64_t time() { return k->time; }
    int data_points() { return points.size(); }
    const pointlist& get_list() const { return points; }
};

class VCDfile {
    typedef std::map<std::string,VCDsignal*> sigmap;
    sigmap signals, handlers;
    void parse_var  ( const std::string& str );
    void parse_value( int64_t t, const std::string& str );
    void parse_t0   ( std::ifstream& fin );
    void parse_rest ( std::ifstream& fin );
    int64_t parse_bin( const std::string& );
    VCDsignal* get_or_throw( const std::string& name );
    int line, scale;
public:
    VCDfile( const char *fname, int downscale=1000 ); // scale from ps to ns
    VCDsignal* get_signal( const char *name );
    ~VCDfile();
    // move all signals
    void rewind();
    int64_t forward(int64_t time);
    int64_t forward(std::string name, int64_t val);
    VCDsignal* get( const std::string& name );
    void report();
};

#endif