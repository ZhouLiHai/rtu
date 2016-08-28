#include "define.h"
#include "libhd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <error.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>

void keysetbit(int *val, char bit, char bit_offset) {
    if (bit_offset < 0 || bit_offset > 31) {
        fprintf(stderr, "\n bit_offset > 31");
        return;
    }
    if (bit == 1) {
        //置1
        *val |= (1 << bit_offset);
    } else if (bit == 0) {
        //置0
        *val &= (~(1 << bit_offset));
    } else
        fprintf(stderr, "\n setbit bit != 1 && bit != 0");
    return;
}

void keyenable() {
    int fd;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        return;
    KEY.keyAddr = (short *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x14000000);
    if (KEY.keyAddr == NULL)
        DDBG("按键模块内存映射失败。");
}

int keyget() {
    int keypress = 0;

    if (KEY.keyAddr[LCD_KEY_ADDR] == ADD_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_A);
        //fprintf(stderr,"press add\n");
    }                           //+

    if (KEY.keyAddr[LCD_KEY_ADDR] == JIAN_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_J);
        //fprintf(stderr,"press jian\n");
    }                           //-

    if (KEY.keyAddr[LCD_KEY_ADDR] == LEFT_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_L);
        //fprintf(stderr,"press left\n");
    }                           //left

    if (KEY.keyAddr[LCD_KEY_ADDR] == RIGHT_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_R);
        //fprintf(stderr,"press right\n");
    }                           //right

    if (KEY.keyAddr[LCD_KEY_ADDR] == UP_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_U);
        //fprintf(stderr,"press up\n");
    }                           //UP

    if (KEY.keyAddr[LCD_KEY_ADDR] == DOWN_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_D);
        //fprintf(stderr,"press down\n");
    }                           //down

    if (KEY.keyAddr[LCD_KEY_ADDR] == OK_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_O);
        //fprintf(stderr,"press ok\n");
    }                           //ok

    if (KEY.keyAddr[LCD_KEY_ADDR] == RST_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_S);
        //fprintf(stderr,"press rst\n");
    }                           //RST

    if (KEY.keyAddr[LCD_KEY_ADDR] == ESC_VALUE) {
        KEY.setbit(&keypress, 1, OFFSET_E);
        //fprintf(stderr,"press esc\n");
    }                           //ESC

    return keypress;
}

void keystart(void) {
    int res;

    pthread_attr_init(&KEY.attr_t);
    pthread_attr_setdetachstate(&KEY.attr_t, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&KEY.attr_t, 256000);
    res = pthread_create(&KEY.key, &KEY.attr_t, KEY.server, NULL);
    if (res != 0) {
        DDBG("线程创建失败。");
    }
}

void* keyserver(void* args) {
    int turn = 0;

    KEY.KeyBeenPressed = 0;
    INT8U Key_State = 0;

    int keypress = 0, presskey_first = 0, presskey_qudou = 0, presskey_qudou_old = 0;

    int count = 0;

    while (1) {
        if (ERINFO.isShowing())
            turn = 0;
        turn++;
        if (turn > 200)
            LCD.closelight();
        keypress = 0;
        presskey_qudou_old = presskey_qudou;
        presskey_qudou = KEY.get();
        switch (Key_State) {
        case 0:
            if (presskey_qudou != 0 && presskey_qudou_old == 0) {
                presskey_first = presskey_qudou;
                Key_State = 1;
            }
            break;
        case 1:
            if (presskey_qudou != 0) {
                count++;
                if (count >= 1) {
                    Key_State = 2;
                    count = 0;
                }
            }
            break;
        case 2:
            keypress = presskey_first;
            Key_State = 0;
            break;
        }

        if (keypress != 0) {
            KEY.PressKey = keypress;
            fprintf(stderr, "PressKey = %d\n", KEY.PressKey);
            KEY.KeyBeenPressed = ERINFO.isShowing() ? 2 : 1;
            fprintf(stderr, "KeyBeenPressed = %d\n", KEY.KeyBeenPressed);
            turn = 0;
            LCD.openlight();
        }
        usleep(50 * 1000);
    }
    return NULL;
}

int keysignal(void) {
    if (KEY.KeyBeenPressed == 1) {
        KEY.KeyBeenPressed = 0;
        return KEY.PressKey;
    }
    return -1;
}
int keyerrsignal(void) {
    if (KEY.KeyBeenPressed == 2) {
        KEY.KeyBeenPressed = 0;
        return KEY.PressKey;
    }
    return -1;
}


void KEYINIT(void) {
    memset(&KEY, 0, sizeof(KEY));

    KEY.get = keyget;
    KEY.enable = keyenable;
    KEY.setbit = keysetbit;

    KEY.start = keystart;
    KEY.server = keyserver;

    KEY.signal = keysignal;
    KEY.errsignal = keyerrsignal;

    KEY.enable();
}
