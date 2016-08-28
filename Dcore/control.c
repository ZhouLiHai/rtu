/*
 * control.c
 *
 *  Created on: 2014-6-30
 *      Author: root
 */

#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "define.h"

int ctstart(void) {
    pthread_attr_t attr_t;
    pthread_attr_init(&attr_t);
    pthread_attr_setschedpolicy(&attr_t, SCHED_RR);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&CONTRAL.thread_key, &attr_t, CONTRAL.server, NULL);
}

void mktimeout(struct timespec *tsp, long sec) {
    struct timeval now;
    gettimeofday(&now, NULL);
    tsp->tv_sec = now.tv_sec;
    tsp->tv_nsec = now.tv_usec * 1000;
    tsp->tv_sec += sec;
}

void *ctserver(void *args) {
    struct timespec timeout;
    int selectout = 0;
    DDBG("遥控服务模块开始运行");
    while (1) {
        mktimeout(&timeout, 1);
        if (CONTRAL.ac == 1)
            selectout++;
        if (selectout > 30) {
            CONTRAL.ac = 0;
            selectout = 0;
            set_cancel(CONTRAL.id, CONTRAL.hw);
        }
        int r = pthread_cond_timedwait(&CONTRAL.sig, &CONTRAL.siglock, &timeout);
        if (r == 110)
            continue;
        pthread_mutex_lock(&CONTRAL.colock);
        DDBG("遥控 %d-%d（%d）", CONTRAL.id, CONTRAL.hw, CONTRAL.ac);
        switch (CONTRAL.ac) {
        case 1:
            if (CONTRAL.id < 12)
                set_select(CONTRAL.id, CONTRAL.hw);
            else {
                if (CONTRAL.id == 0x15)
                    BATTERY.begin();
                if (CONTRAL.id == 0x16)
                    BATTERY.end();
            }
            break;
        case 2:
            set_action(CONTRAL.id, CONTRAL.hw);
            usleep(CONTRAL.timeout * 1000);
            set_cancel(CONTRAL.id, CONTRAL.hw);
            break;
        case 3:
            set_cancel(CONTRAL.id, CONTRAL.hw);
            usleep(CONTRAL.timeout * 1000);
            break;
        case 4:
            set_select(CONTRAL.id, CONTRAL.hw);
            usleep(5 * 1000);
            set_action(CONTRAL.id, CONTRAL.hw);
            usleep(CONTRAL.timeout * 1000);
            set_cancel(CONTRAL.id, CONTRAL.hw);
            break;
        }
        pthread_mutex_unlock(&CONTRAL.colock);
    }
    return NULL;
}

int cttimeout(int num) {
    for (int i = 0; i < CONTRAL.size; i++) {
        control_elem element;
        read_control_elem(CONTRAL.setting, i, &element);
        if (element.line == num) return element.timeout;
    }
    return 0;
}

int ctswitch(int num) {
    for (int i = 0; i < CONTRAL.size; i++) {
        control_elem element;
        read_control_elem(CONTRAL.setting, i, &element);
        if (element.line == num) return element.switchs;
    }
    return 0;
}

int ctselect(int line, int how) {
    if (0 != pthread_mutex_trylock(&CONTRAL.colock))
        return -1;
    if (line < 12) {
        int switchs = CONTRAL.getswitch(line);
        if (switchs != 1) {
            pthread_mutex_unlock(&CONTRAL.colock);
            return -2;
        }
    }
//    if(CONTRAL.ac == 1)
//    {
//      CONTRAL.ac = 0;
//      return -1;
//    }
    CONTRAL.id = line;
    CONTRAL.hw = how;
    CONTRAL.ac = 1;
    CONTRAL.timeout = CONTRAL.getot(line);
    pthread_mutex_unlock(&CONTRAL.colock);
    return pthread_cond_signal(&CONTRAL.sig);
}

int ctaction(int line, int how) {
    if (0 != pthread_mutex_trylock(&CONTRAL.colock))
        return -1;
//  printf("%d-%d\t%d-%d\n", CONTRAL.id, line, CONTRAL.hw, how);
//  if(CONTRAL.id != line)
//      return -1;
    CONTRAL.id = line;
    CONTRAL.hw = how;
    CONTRAL.ac = 2;
    CONTRAL.timeout = CONTRAL.getot(line);
    pthread_mutex_unlock(&CONTRAL.colock);
    return pthread_cond_signal(&CONTRAL.sig);
}

int ctcancel(int line, int how) {
    if (0 != pthread_mutex_trylock(&CONTRAL.colock))
        return -1;
    CONTRAL.id = line;
    CONTRAL.hw = how;
    CONTRAL.ac = 3;
    CONTRAL.timeout = CONTRAL.getot(line);
    pthread_mutex_unlock(&CONTRAL.colock);
    return pthread_cond_signal(&CONTRAL.sig);
}

int ctjustdo(int line, int how) {
    pthread_mutex_trylock(&CONTRAL.colock);
    CONTRAL.id = line;
    CONTRAL.hw = how;
    CONTRAL.ac = 4;
    CONTRAL.timeout = CONTRAL.getot(line);
    pthread_mutex_unlock(&CONTRAL.colock);
    return pthread_cond_signal(&CONTRAL.sig);
}
