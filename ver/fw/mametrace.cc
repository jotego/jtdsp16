#include "mametrace.h"

#include <string>
#include <cstring>
#include <cstdlib>

using namespace std;

MAMEtrace::MAMEtrace( const char *file_name ) {
    fin.open(file_name);
    line_cnt=0;
    next();
}

bool MAMEtrace::next() {
    if( !fin.good() ) return false;
    string line;
    getline( fin, line );
    if( !fin.good() || fin.eof() ) return false;
    char *tok = new char[ line.size()+1 ];
    strcpy( tok, line.c_str() );
    sscanf( line.c_str(),
        "pc=%X pt=%X pr=%X pi=%X "
        "i=%X r0=%X r1=%X r2=%X r3=%X rb=%X re=%X "
        "j=%X k=%X x=%X y=%X "
        "p=%X a0=%lX a1=%lX "
        "c0=%X c1=%X c2=%X auc=%X psw=%X",
        &pc, &pt, &pr, &pi,
        &i, &r0, &r1, &r2, &r3, &rb, &re,
        &j, &k, &x, &y,
        &p, &a0, &a1,
        &c0, &c1, &c2, &auc, &psw );
    line_cnt++;
    return true;
}

void MAMEtrace::dump() {
    printf(
        "pc=%X pt=%X pr=%X pi=%X "
        "i=%X r0=%X r1=%X r2=%X r3=%X rb=%X re=%X "
        "j=%X k=%X x=%X y=%X "
        "p=%X a0=%lX a1=%lX "
        "c0=%X c1=%X c2=%X auc=%X psw=%X\n",
        pc, pt, pr, pi,
        i, r0, r1, r2, r3, rb, re,
        j, k, x, y,
        p, a0, a1,
        c0, c1, c2, auc, psw );
}