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
    int mask;
    char *data;
public:
    QSndData( const char *rompath );
    int get( int addr );
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
    QSndData samples("wof.rom");
    rtl.read_rom(rom.data());
    VCDfile stim("wof_coin.vcd"); // VCD sample input from CPS 1.5 ver/game folder
    stim.report();
    // Move until rst is low
    //printf("VCD forwarded to %ld\n",  );
    //int64_t vcdtime = stim.forward("dsp_rst",0);

    // VCDsignal* sig_irq = stim.get("dsp_irq");
    VCDsignal* sig_cpu2dsp = stim.get("cpu2dsp0");
    const VCDsignal::pointlist& cmdlist = sig_cpu2dsp->get_list();
    VCDsignal::pointlist::const_iterator n = cmdlist.cbegin();

    int reads;
    n++;
    int64_t vcdtime = n->time;
    int64_t next= n->time;
    rtl.clk( 200'000 ); // initialization

    int sim_time=0;
    do {
        int newcmd = n->val;
        int reads=0;
        int last_pids=1;
        int rom_addr=0;
        rtl.set_irq(1);
        rtl.pbus_in( newcmd>>16 );
        n++;
        if( n==cmdlist.cend() ) break;
        vcdtime = next;
        next = n->time;
        int steps = (next-vcdtime)/18;
        if( steps>2'000'000 )
            steps=2'000'000;
        sim_time = (int)(rtl.time()/1000'000L);
        printf("%d ms -> %02X_%04X\n", sim_time, newcmd>>16, newcmd&0xffff);
        while( steps>0 ) {
            rtl.clk(2);
            if( rtl.pids()==1 && last_pids==0 ) {
                reads++;
                if( reads==1 ) rtl.pbus_in( newcmd&0xffff );
            }
            if( last_pids==0 ) rtl.set_irq(0);
            last_pids = rtl.pids();
            if( !rtl.pods() ) {
                rom_addr = 0;
                rom_addr |= rtl.pbus_out()&0xFFFF;
            }
            rom_addr &= 0xFFFF;
            rom_addr |= (rtl.ab()&0xff)<<16;
            if( rtl.ab()&0x8000 ) { // update the value only when external reads occur
                int din = samples.get( rom_addr );
                //printf("Read %X from %X\n", din, rom_addr );
                rtl.rb_din( din<<8 );
            }
            steps-=2;
        }
    }while( sim_time < 200 );

    return 0;
}

QSndData::QSndData( const char *rompath ) {
    ifstream fin(rompath, ios_base::binary);
    if( !fin.good() ) throw runtime_error("Cannot open ROM file for game");
    // Get the header
    char header[64];
    fin.read( header, 64 );
    if( !fin.good() ) throw runtime_error("Cannot read ROM header");
    int start = get_offset( header, 2 );
    int end   = get_offset( header, 4 );
    int expected = end-start;
    if( expected>8*1024*1024 )
        expected = 8*1024*1024;
    data = new char[expected];
    mask = expected-1;
    printf("PCM data start %10X\nPCM data end   %10X. Mask=%0x\n", start, end, mask );
    fin.seekg( start+64 );
    if( !fin.good() ) {
        char s[256];
        sprintf(s,"Cannot seek to PCM start of game ROM (%X)", start);
        throw runtime_error(s);
    }
    fin.read( data, expected );
    int data_cnt = fin.gcount();
    if( data_cnt!=expected ) {
        char s[256];
        sprintf(s,"Expected 0x%X bytes of data but only 0x%X were read", expected, data_cnt );
        throw runtime_error(s);
    }
    printf("Read %d (%d MB) as PCM data\n", data_cnt, data_cnt>>20 );
}

QSndData::~QSndData() {
    delete[] data;
    data = nullptr;
}

int QSndData::get( int addr ) {
    addr &= mask;
    return data[addr]&0xff;
}

int QSndData::get_offset( char *header, int p ) {
    int o = ((((int)header[p+1])&0xff)<<8) | (((int)header[p])&0xff);
    printf("%X%X->%X\n", (unsigned)header[p+1]&0xff, (unsigned)header[p]&0xff, o);
    o <<= 10;
    return o;
}