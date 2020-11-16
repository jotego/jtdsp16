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
    //int64_t vcdtime = stim.forward("dsp_rst",0);

    // VCDsignal* sig_irq = stim.get("dsp_irq");
    VCDsignal* sig_cpu2dsp = stim.get("cpu2dsp");
    const VCDsignal::pointlist& cmdlist = sig_cpu2dsp->get_list();
    VCDsignal::pointlist::const_iterator n = cmdlist.cbegin();

    int reads;
    n++;
    int64_t vcdtime = n->time;
    int64_t next= n->time;
    rtl.clk( 20'000 ); // initialization
    for( int k=0; k<3'500'000; ) {
        int newcmd = n->val;
        int reads=0;
        int last_pids=1;
        rtl.set_irq(1);
        rtl.pbus_in( newcmd>>16 );
        n++;
        if( n==cmdlist.cend() ) break;
        vcdtime = next;
        next = n->time;
        int steps = (next-vcdtime)/18;
        if( steps>2'000'000 )
            steps=2'000'000;
        printf("%6d -> %ld\n", steps, vcdtime);
        while( steps>0 ) {
            rtl.clk(2);
            if( rtl.pids()==1 && last_pids==0 ) {
                reads++;
                if( reads==1 ) rtl.pbus_in( newcmd&&0xffff );
            }
            if( last_pids==0 ) rtl.set_irq(0);
            last_pids = rtl.pids();
            steps-=2;
            if( reads == 3 ) break;
        }
        rtl.clk( steps ); // Do the rest
        k+=steps;
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