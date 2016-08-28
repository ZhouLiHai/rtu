#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include "define.h"

int remstart(void) {
    pthread_attr_t attr_t;
    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&CONTRAL.thread_key, &attr_t, REMOTE.server, NULL);
}

int remget(void) {
    unsigned short yxhead = gFpgaic[0x400];

    unsigned short frame = 0;


    if (REMOTE.yxoriginal == yxhead)
        return 0;
    printf("%d\n", yxhead);

    frame = gFpgaic[REMOTE.yxoriginal];
    REMOTE.records.state[0] = frame >> 15;
    frame = frame << 1;
    frame = frame >> 1;
    REMOTE.records.number = (int) frame;

    REMOTE.records.time = gFpgaic[REMOTE.yxoriginal + 1];
    REMOTE.records.time = REMOTE.records.time << 16;
    REMOTE.records.time += gFpgaic[REMOTE.yxoriginal + 2];
    REMOTE.records.time = REMOTE.records.time << 16;
    REMOTE.records.time += gFpgaic[REMOTE.yxoriginal + 3];

    REMOTE.yxoriginal = (REMOTE.yxoriginal + 0x04) % 0x400;

    DDBG("硬件遥信产生 %d-%d\n", REMOTE.records.number, REMOTE.records.state[0]);

    memcpy((void *)REMOTE.state, (void *) & (gFpgaic[0x402]),
           sizeof(unsigned short) * 5);
    return 1;
}

int remadd(int id, int sign, int type, long long tv) {

    DDBG("添加遥信 %d-%d（%d）", id, sign, type);

    pthread_mutex_lock(&REMOTE.soelock);
    if (type == 2) {
        int p = REMOTE.soe.info.head;

        if (p < 0 || p >= YX_SIZE) {
            DDBG("遥信头位置出错");
        }

        REMOTE.soe.unit[p].number = id;
        REMOTE.soe.unit[p].state[0] = (sign >> 1);
        REMOTE.soe.unit[p].state[1] = (sign & 0x01);
        REMOTE.soe.unit[p].time = tv;
        REMOTE.soe.unit[p].type = type;

        REMOTE.soe.info.head = (p + 1) % YX_SIZE;
        REMOTE.soe.info.number = (REMOTE.soe.info.number + 1) % YX_SIZE;

    } else {
        int p = REMOTE.soe.info.head;

        if (p < 0 || p >= YX_SIZE) {
            DDBG("遥信头位置出错");
        }

        REMOTE.soe.unit[p].number = id;
        REMOTE.soe.unit[p].state[0] = sign;
        REMOTE.soe.unit[p].state[1] = 0x00;
        REMOTE.soe.unit[p].time = tv;
        REMOTE.soe.unit[p].type = 1;

        REMOTE.soe.info.head = (p + 1) % YX_SIZE;
        REMOTE.soe.info.number = (REMOTE.soe.info.number + 1) % YX_SIZE;

    }
    pthread_mutex_unlock(&REMOTE.soelock);
    return 1;
}

void *remserver(void *args) {
    int i, j, k;

    struct timeval tv;

    while (1) {
        // gettimeofday (&old_tv, NULL);

        // printf(" OUT %d %d %f \n", REMOTE.press, REMOTE.size, elapse(&old_tv, &tv));

        usleep(50 * 1000);
        gettimeofday(&tv, NULL);

        while (REMOTE.get()) {
            for (int i = 0; i < REMOTE.size; i++) {
                remote_elem elem;
                read_remote_elem(REMOTE.setting, i, &elem);

                if (elem.line0 == REMOTE.records.number && elem.type == 1) {
                    REMOTE.add(elem.name, REMOTE.records.state[0], elem.type, REMOTE.records.time);

                }

                if ((elem.line0 == REMOTE.records.number || elem.line1 == REMOTE.records.number) && elem.type == 2) {
                    int state = (getSignal(elem.line0) << 1) + (getSignal(elem.line1));
                    REMOTE.add(elem.name, state, elem.type, ANALOG.devicetv);
                }
            }
        }


        for (i = 0, j = 0, k = 0; i < REMOTE.size; i++) {
            remote_elem elem;
            read_remote_elem(REMOTE.setting, i, &elem);

            switch (elem.type) {
            case 1:
                REMOTE.SId[j] = elem.name;
                REMOTE.SBuf[j] = getSignal(elem.line0);
                j++;
                break;
            case 2:
                REMOTE.DId[k] = elem.name;
                REMOTE.DBuf[k] = (getSignal(elem.line0) << 1) + (getSignal(elem.line1));
                k++;
                break;
            case 5:
                REMOTE.SId[j] = elem.name;
                if (REMOTE.SBuf[j] != getPress(REMOTE.press, elem.line0)) {
                    DDBG("elem.name %d, elem.line0 %d, getPress %d REMOTE.SBuf[j] %d\n", elem.name, elem.line0, getPress(REMOTE.press, elem.line0), REMOTE.SBuf[j]);
                    REMOTE.SBuf[j] = getPress(REMOTE.press, elem.line0);
                    REMOTE.add(elem.name, REMOTE.SBuf[j], 5, ANALOG.devicetv);
                }
                j++;
                break;
            }
        }
        REMOTE.Slength = j;
        REMOTE.Dlength = k;
    }
    return NULL;
}

int getPress(int th, int index) {
    if (ANALOG.Val[index - 1].Enable == 1) {
        if (ANALOG.Val[index - 1].Ua > th || ANALOG.Val[index - 1].Ub > th ||
                ANALOG.Val[index - 1].Uc > th || ANALOG.Val[index - 1].Ud > th) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

int getSignal(int index) {
    if (index >= 0 && index <= 79) {
        int i = index / 16;
        int j = index % 16;
        return ((REMOTE.state[i]) & (1 << j) ? 1 : 0);
    }
    return 0;
}

int remSearch(int type, int board, int shift) {
    printf("remSearch.type=%d l0=%d l1=%d\n", type, board, shift);
    for (int i = 0; i < REMOTE.size; i++) {
        remote_elem elem;
        read_remote_elem(REMOTE.setting, i, &elem);
        if (elem.type == type && elem.line0 == board && elem.line1 == shift) {
            printf("remSearch.name=%d\n", elem.name);
            return elem.name;
        }
    }
    return -1;
}
