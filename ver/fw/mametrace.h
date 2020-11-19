#ifndef __MAMETRACE_H
#define __MAMETRACE_H

#include <fstream>

struct CPUstate {
    int pc, pt, pr, pi, i, r0, r1, r2, r3, rb, re, j, k, x, y, p;
    int c0, c1, c2, auc, psw;
    int ticks;
    int64_t a0, a1;
};

class MAMEtrace {
    std::ifstream fin;
    int line_cnt;
public:
    int pc, pt, pr, pi, i, r0, r1, r2, r3, rb, re, j, k, x, y, p;
    int c0, c1, c2, auc, psw;
    int64_t a0, a1;
    MAMEtrace( const char *file_name );
    void dump();
    bool next();
    int get_line() const { return line_cnt; }
};

#endif