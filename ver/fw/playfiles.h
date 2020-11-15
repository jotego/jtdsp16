#include "common.h"
#include "vcd.h"
#include <fstream>
#include <cstdio>
#include <vector>

using namespace std;

int playfiles();

class QSndData {
    int get_offset( char *header, int p );
    char *data;
public:
    QSndData( const char *rompath );
    ~QSndData();
};

class QSndLog {
    struct LogPair{ long int ticks; int value; };
public:
    QSndLog( const char *path);
};

int playfiles() {
    RTL rtl;
    ROM rom;
    //QSndData samples("punisher.rom");
    rtl.read_rom(rom.data());

    rtl.clk( 100'000 );
    rtl.set_irq();
    for( int k=0; k<5'000; k++ ) {
        rtl.clk(2);
        if( rtl.pids()==0 ) rtl.set_irq(0);
    }

    return 0;
}

QSndData::QSndData( const char *rompath ) {
    ifstream fin(rompath, ios_base::binary);
    // Get the header
    char header[64];
    fin.read( header, 64 );
    int start = get_offset( header, 2 );
    int end   = get_offset( header, 4 );
    end = end-start;
    if( end>8*1024*1024 )
        end = 8*1024*1024;
    data = new char[end];
    fin.seekg( start );
    fin.read( data, end );
    // printf("Read %d MB as PCM data\n", c>>10 );
}

QSndData::~QSndData() {
    delete data;
    data = nullptr;
}

int QSndData::get_offset( char *header, int p ) {
    int o = (((int)header[p+1])&0xff)<<16 | (((int)header[p])&0xff);
    o <<= 10;
    return o;
}