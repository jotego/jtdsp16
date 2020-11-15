#include "common.h"
#include "vcd.h"
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>

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

int playfiles( const char* vcd_file ) {
    RTL rtl(vcd_file);
    ROM rom;
    //QSndData samples("punisher.rom");
    rtl.read_rom(rom.data());
    VCDfile stim("wof_coin.vcd"); // VCD sample input from CPS 1.5 ver/game folder
    stim.report();
    // Move until rst is low
    //printf("VCD forwarded to %ld\n",  );
    int64_t vcdtime = stim.forward("dsp_rst",0);

    VCDsignal* sig_irq = stim.get("dsp_irq");
    VCDsignal* sig_cpu2dsp = stim.get("cpu2dsp");
    int last_irq;
    int64_t next= vcdtime + 9*2*20;
    for( int k=0; k<60'500'000; k++ ) {
        int steps = (next-vcdtime)/18;
        printf("%6d -> %ld\n", steps, vcdtime);
        rtl.clk(steps);
        int cpu2dsp = sig_cpu2dsp->cur();
        rtl.pbus_in( cpu2dsp&0xFFFF );
        vcdtime = next;
        next = stim.forward(vcdtime);
        if( next == vcdtime ) break;

        // if( rtl.pids()==0 ) rtl.set_irq(0);
        // else if( sig_irq->cur() ) rtl.set_irq(1);
        //if( sig_irq->cur()!= last_irq ) printf("%ld, %ld\n", vcdtime, sig_irq->cur() );
        //last_irq = sig_irq->cur();
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