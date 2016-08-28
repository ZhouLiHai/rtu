#include "define.h"

int HEXtoDEC(int a) {
    return a;
}

int Doubletonum(unsigned short a, unsigned short b) {
    int trans = 0x00;

    trans = b;
    trans = trans << 16;
    trans = (trans + a);
    return trans;
}

float elapse(struct timeval *ptv, struct timeval *tv) {
    if (ptv->tv_sec < tv->tv_sec) {
        tv->tv_sec = ptv->tv_sec;
        tv->tv_usec = ptv->tv_usec;
    }

    float sta = (float)tv->tv_usec / 1000.0f;
    float end = (float)ptv->tv_usec / 1000.0f + (float)(ptv->tv_sec - tv->tv_sec) * 1000.0f;
    float res = end - sta;

    return res;
}

int subTo10(unsigned short v) {
    int h = v / 16;
    int l = v % 16;

    return l * 10 + h;
}

int revTo10(unsigned short v) {
    int h = v / 256;
    int l = v % 256;

    h = subTo10(h);
    l = subTo10(l);

    return l * 100 + h;
}