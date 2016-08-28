/*
 * fpga.c
 *
 *  Created on: 2014-6-30
 *      Author: root
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "define.h"

void fix_time(struct timeval settime) {
    union {
        unsigned long long value;
        struct {
            unsigned int l;
            unsigned int h;
        } st;

        struct {
            unsigned short l;
            unsigned short m;
            unsigned short h;
            unsigned short v;
        } sts;

    } LUINT;

    union {
        unsigned char value;
        struct {
            unsigned l: 4;
            unsigned h: 4;
        } st;
    } u;

    struct tm tn;

    struct timeval tv;

    u.value = 0;

    LUINT.st.l = 0;
    LUINT.st.h = 0;

    tv = settime;
    DDBG("设置:设定时间 %d %d", tv.tv_sec, tv.tv_usec);
    settimeofday(&tv, NULL);

    LUINT.value = (tv.tv_sec);
    LUINT.value *= 1000;
    LUINT.value += (int)(tv.tv_usec / 1000.0f);

    localtime_r(&tv.tv_sec, &tn);
    tn.tm_year = tn.tm_year + 1900 - 2000;

    tn.tm_sec %= 60;
    u.st.h = tn.tm_sec / 10;
    u.st.l = tn.tm_sec % 10;
    gFpgaic[0x44E] = 0x0000 + u.value;

    usleep(10000);

    tn.tm_min %= 60;
    u.st.h = tn.tm_min / 10;
    u.st.l = tn.tm_min % 10;
    gFpgaic[0x44E] = 0x0100 + u.value;

    usleep(10000);

    tn.tm_year %= 100;
    u.st.h = (tn.tm_year / 10);
    u.st.l = (tn.tm_year % 10);
    gFpgaic[0x44E] = 0x0600 + u.value;

    usleep(10000);

    tn.tm_mon += 1;
    u.st.h = tn.tm_mon / 10;
    u.st.l = tn.tm_mon % 10;
    gFpgaic[0x44E] = 0x0500 + u.value;

    usleep(10000);

    u.st.h = tn.tm_mday / 10;
    u.st.l = tn.tm_mday % 10;
    gFpgaic[0x44E] = 0x0400 + u.value;

    usleep(10000);

    tn.tm_hour %= 60;
    u.st.h = tn.tm_hour / 10;
    u.st.l = tn.tm_hour % 10;
    gFpgaic[0x44E] = 0x0200 + u.value;
}

void judge_time(int dir) {

    DDBG("时间设定 %s", dir ? "FPGA" : "CPU");

    union {
        unsigned long long value;
        struct {
            unsigned int l;
            unsigned int h;
        } st;

        struct {
            unsigned short l;
            unsigned short m;
            unsigned short h;
            unsigned short v;
        } sts;

    } LUINT;

    union {
        unsigned char value;
        struct {
            unsigned l: 4;
            unsigned h: 4;
        } st;
    } u;

    struct tm tn;

    struct timeval tv;

    u.value = 0;

    LUINT.st.l = 0;
    LUINT.st.h = 0;

    if (dir) {
        // tv = settime;
        // DDBG("设置:设定时间 %d %d", settime.tv_sec, settime.tv_usec);
        // settimeofday(&tv, NULL);

        LUINT.value = (tv.tv_sec);
        LUINT.value *= 1000;
        LUINT.value += (int)(tv.tv_usec / 1000.0f);

        localtime_r(&tv.tv_sec, &tn);
        tn.tm_year = tn.tm_year + 1900 - 2000;

        tn.tm_sec %= 60;
        u.st.h = tn.tm_sec / 10;
        u.st.l = tn.tm_sec % 10;
        gFpgaic[0x44E] = 0x0000 + u.value;

        usleep(10000);

        tn.tm_min %= 60;
        u.st.h = tn.tm_min / 10;
        u.st.l = tn.tm_min % 10;
        gFpgaic[0x44E] = 0x0100 + u.value;

        usleep(10000);

        tn.tm_year %= 100;
        u.st.h = (tn.tm_year / 10);
        u.st.l = (tn.tm_year % 10);
        gFpgaic[0x44E] = 0x0600 + u.value;

        usleep(10000);

        tn.tm_mon += 1;
        u.st.h = tn.tm_mon / 10;
        u.st.l = tn.tm_mon % 10;
        gFpgaic[0x44E] = 0x0500 + u.value;

        usleep(10000);

        u.st.h = tn.tm_mday / 10;
        u.st.l = tn.tm_mday % 10;
        gFpgaic[0x44E] = 0x0400 + u.value;

        usleep(10000);

        tn.tm_hour %= 60;
        u.st.h = tn.tm_hour / 10;
        u.st.l = tn.tm_hour % 10;
        gFpgaic[0x44E] = 0x0200 + u.value;

    } else {
        memset(&tn, 0, sizeof(struct tm));

        u.value = (gFpgaic[0x44A] & 0xFF00) >> 8;
        tn.tm_year = (2000 + (u.st.h * 10) + u.st.l) - 1900;

        u.value = gFpgaic[0x44A] & 0x00FF;
        tn.tm_mon = u.st.h * 10 + u.st.l;
        tn.tm_mon -= 1;

        u.value = (gFpgaic[0x44B] & 0xFF00) >> 8;
        tn.tm_mday = u.st.h * 10 + u.st.l;

        u.value = gFpgaic[0x44B] & 0x00FF;
        tn.tm_hour = u.st.h * 10 + u.st.l;

        u.value = (gFpgaic[0x44C] & 0xFF00) >> 8;
        tn.tm_min = u.st.h * 10 + u.st.l;

        u.value = gFpgaic[0x44C] & 0x00FF;
        tn.tm_sec = u.st.h * 10 + u.st.l;

        tv.tv_sec = mktime(&tn);
        tv.tv_usec = 0;

        settimeofday(&tv, NULL);

        LUINT.value = tv.tv_sec;
        LUINT.value *= 1000;
    }
}

int get_board_state(int id) {
    if (id < 0 || id > 10)
        return 0;
    /*
     *      0-5 :遥信遥控板
     *      6   :电源板
     *      7   :按键板
     *  8-10:遥测板
     */
    if (id < 8) {
        unsigned int state;

        state = gFpgaic[0x434];
        return (state & 1 << id) != 0 ? 1 : 0;
    } else {
        gFpgaic[0x470] = 0;
        switch (id) {
        case 8:
            return gFpgaic[0x500] == 0xEB90 ? 1 : 0;
            break;
        case 9:
            return gFpgaic[0x600] == 0xEB90 ? 1 : 0;
            break;
        case 10:
            return gFpgaic[0x700] == 0xEB90 ? 1 : 0;
            break;
        }
    }
    return 0;
}

void set_select(int id, int how) {
    gpioWrite(105, 0);
    gpioWrite(106, 0);
    if (how == 0) {
        gpioWrite(12, 1);
    } else {
        gpioWrite(40, 1);
    }
}

void set_action(int id, int how) {
    gpioWrite(17, 1);
}

void set_cancel(int id, int how) {
    gpioWrite(17, 0);
    gpioWrite(12, 0);
    gpioWrite(40, 0);
    gpioWrite(105, 1);
    gpioWrite(106, 1);
}

int get_sta(int id, int how) {
    if (id < 0 || id >= 12)
        return 0;

    if (get_board_state(id / 2)) {
        unsigned int state;

        state = gFpgaic[0x427] << 16 | gFpgaic[0x426];
        how = (how ? 1 : 0);
        return ((state & 1 << (id * 2 + how)) != 0 ? 1 : 0);
    } else
        return 0;
}

void set_led(int id, int o) {
    static unsigned short gled = 0xff;

    if (id >= 0 && id < 7) {
        id += 2;
        if (0 == o)
            gled &= ~(1 << id);

        if (1 == o)
            gled |= (1 << id);
    }
    gFpgaic[0x41C] = gled;
}

void set_power_start(int hw) {
    gpioWrite(83, hw);
}

void set_power_end(int hw) {
    gpioWrite(82, hw);
}

long long getDeviceTime() {
    long long tv = 0;
    tv = tv + gFpgaic[0x446];
    tv = tv << 16;
    tv = tv + gFpgaic[0x447];
    tv = tv << 16;
    tv = tv + gFpgaic[0x448];

    return tv;
}

unsigned short getDc(int type) {
    if (type == 0) {
        return gFpgaic[0x43b];
    } else {
        return gFpgaic[0x43c];
    }
}

int gpioWrite(int number, int v) {
    char devName[64];
    memset(devName, 0, sizeof(devName));
    sprintf(devName, "/sys/class/gpio/gpio%d/value", number);
    int fd = open(devName, O_WRONLY);
    char ch = v == 1 ? '1' : '0';
    write(fd, &ch, sizeof(ch));
    close(fd);
}

int gpioRead(int number) {
    char devName[64];
    memset(devName, 0, sizeof(devName));
    sprintf(devName, "/sys/class/gpio/gpio%d/value", number);
    int fd = open(devName, O_RDONLY);
    char ch;
    read(fd, &ch, sizeof(ch));
    close(fd);
    return ch;
}