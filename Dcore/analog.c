#include "define.h"

void anaStart(void) {
    int res;
    pthread_t thread_key;
    pthread_attr_t attr_t;

    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    res = pthread_create(&thread_key, &attr_t, ANALOG.server, NULL);
    if (res != 0) {
        DDBG("\n thread_key created failed");
    }
}

//////////////////////////////////////////
/*
 * 高优先级线程，包括4部分
 * 1.更新共享内存数据
 * 2.根据投退检测越限
 * 3.三段保护检测函数
 * 4.检查故障之后的复归
 */
//////////////////////////////////////////
void *anaServer(void *job) {
    while (1) {

        ANALOG.refresh((void *) 0);
        //ANALOG.ultralimit();
        anaUltralimit();
        ANALOG.protection();
        checksetback();
        //更新全局时间
        ANALOG.devicetv = getDeviceTime();
        usleep(5 * 1000);
    }
    pthread_exit(NULL);
}

void anaUltralimit(void) {
    int upper = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (int i = 0; i < ANALOG.size; i++) {
        analog_elem elem;
        read_analog_elem(ANALOG.setting, i, &elem);

        if (!elem.open)
            continue;

        float now = ANALOG.point(elem.board, elem.shift);
        //printf("now = %f, elem.up = %f, elem.down = %f\n", now, elem.up, elem.down);

        //printf("%d %f %f\n", ANALOG.state[i], now, elem.up);

        switch (ANALOG.state[i]) {
        case 0:
            if (now < elem.down || now > elem.up) {
                ANALOG.state[i] = 3;
                ANALOG.tv[i] = tv;
            }
            break;
        case 3:
            if (now < elem.down || now > elem.up) {
                float tmp_t = elapse(&tv, &ANALOG.tv[i]);

                if (tmp_t > elem.otime) {
                    DDBG("越限检测 %f %d-%d", now, elem.board, elem.shift);
                    LED.on(3);
                    if (now < elem.down)
                        ANALOG.state[i] = 4;
                    else
                        ANALOG.state[i] = 5;
                }
            } else {
                ANALOG.state[i] = 0;
            }
            break;
        case 4:
            upper = REMOTE.search(8, elem.board, elem.shift);
            if (upper > 0)
                REMOTE.add(upper, 0, 1, ANALOG.devicetv);
            ANALOG.state[i] = 6;
            ANALOG.tv[i] = tv;
            break;
        case 5:
            upper = REMOTE.search(8, elem.board, elem.shift);
            if (upper > 0)
                REMOTE.add(upper, 1, 1, ANALOG.devicetv);
            ANALOG.state[i] = 7;
            ANALOG.tv[i] = tv;
            break;
        case 6:
            if (now >= elem.down && now < elem.up) {
                float tmp_t = elapse(&tv, &ANALOG.tv[i]);

                if (tmp_t > elem.otime) {
                    LED.off(3);
                    ANALOG.state[i] = 0;
                }
            } else if (now > elem.up) {
                LED.on(3);
                ANALOG.state[i] = 5;
            } else
                ANALOG.tv[i] = tv;
            break;
        case 7:
            if (now >= elem.down && now < elem.up) {
                float tmp_t = elapse(&tv, &ANALOG.tv[i]);

                if (tmp_t > elem.otime) {
                    LED.off(3);
                    ANALOG.state[i] = 0;
                }
            } else if (now < elem.down) {
                LED.on(3);
                ANALOG.state[i] = 4;
            } else
                ANALOG.tv[i] = tv;
            break;
        }
    }
}

int Protect(int i, float now, protect_elem elem) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    switch (ANALOG.prstate[i]) {
    case 0:
        if (now > ANALOG.threshold && getPress(REMOTE.press, elem.board)) {
            ANALOG.prstate[i] = 1;
            ANALOG.prtv[i] = tv;
        }
        break;
    case 1:
        if (now > ANALOG.threshold && getPress(REMOTE.press, elem.board)) {
            if (elapse(&tv, &ANALOG.prtv[i]) > ANALOG.otime) {
                LED.on(2);
                CONTRAL.justdo(elem.control, elem.how);
                REMOTE.add(elem.upper, 1, 1, ANALOG.devicetv);
                ANALOG.prtimes[i]++;
                ANALOG.prstate[i] = 0;
                return 1;
            }
        } else {
            ANALOG.prstate[i] = 0;
        }
    }
    return 0;
}

void anaProtection(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (int i = 0; i < ANALOG.prsize; i++) {
        protect_elem elem;
        read_protect_elem(ANALOG.prsetting, i, &elem);

        if (!elem.open)
            continue;
        float now = ANALOG.point(elem.board, elem.shift);

        switch (ANALOG.rebackstate[i]) {
        case 0:
            if (Protect(i, now, elem)) {
                ANALOG.rebackstate[i] = 1;
                ANALOG.rebacktv[i] = tv;
            }
            break;
        case 1:
            if (elapse(&tv, &ANALOG.rebacktv[i]) > elem.t2) {
                ANALOG.rebackstate[i] = 2;
                ANALOG.rebacktv[i] = tv;
            }
            break;
        case 2:
            if (elapse(&tv, &ANALOG.rebacktv[i]) < ANALOG.rebacktime) {
                if (Protect(i, now, elem)) {

                    if (ANALOG.prtimes[i] > elem.times) {
                        ANALOG.rebackstate[i] = 10;
                    } else {
                        ANALOG.rebacktv[i] = tv;
                        ANALOG.rebackstate[i] = 1;
                    }
                }
            } else {
                if (ANALOG.reback == 1) {
                    LED.off(2);
                    if (ANALOG.warn == 1) {
                        LED.on(3);
                    }
                    REMOTE.add(6601, 0, 1, ANALOG.devicetv);
                    // LCD.closelight();
                }
                ANALOG.rebackstate[i] = 0;
                ANALOG.prtimes[i] = 0;
            }
            break;
        case 10:
            REMOTE.add(elem.upper, 0, 1, ANALOG.devicetv);
            ANALOG.rebackstate[i] = 11;
            break;
        case 11:
            break;
        }
    }
}

void setback(void) {
    DDBG("复归函数-开始复归");

    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (int i = 0; i < ANALOG.prsize; i++) {
        ANALOG.prtv[i] = tv;
        ANALOG.prstate[i] = 0;
        ANALOG.prtimes[i] = 0;
        ANALOG.rebackstate[i] = 0;
        ANALOG.rebacktv[i] = tv;

    }
    LED.off(2);
    LED.off(3);
    //LCD.closelight();
}

void checksetback() {
    static int state = 0;
    static struct timeval tv;

    struct timeval now;
    gettimeofday(&now, NULL);

    int res = gpioRead(104);

    //printf("%d\n", res);

    switch (state) {
    case 0:
        if (res == 48) {
            state = 1;
            gettimeofday(&tv, NULL);
        }
        break;
    case 1:
        if (res == 48) {
            if (elapse(&now, &tv) > 1000) {
                state = 2;
                REMOTE.add(6602, 0, 1, ANALOG.devicetv);
                setback();
            }
        } else {
            state = 0;
        }
        break;
    case 2:
        if (elapse(&now, &tv) > 1000) {
            state = 0;
        }
        break;
    }
}

//扫描系数是否存在
int ScanCoeffect(unsigned short * base) {
    //共24个系数
    for (int i = 2; i < 26; ++i) {
        if (base[i] == 0) {
            return 0;
        }
    }
    return 1;
}

//获取遥测参数
int anaInitCoeffect(void) {
    //遥测最多三块板
    //获取数据基地址
    unsigned short *curbase = ycbase[0];

    //判断板子在线状态
    if (curbase[ENABLE] != 0xeb90)
        return -1;
    //判断系数状态,做出警告
    if (!ScanCoeffect(curbase)) {
        //只给出警告,程序继续
        DDBG("警告:遥测板系数可能不存在!");
    }

    ANALOG.Coe[0].CUa = 220.0 / curbase[COEF04];
    ANALOG.Coe[0].CUb = 220.0 / curbase[COEF05];
    ANALOG.Coe[0].CUc = 220.0 / curbase[COEF06];

    ANALOG.Coe[0].CIa1 = 5.0 / HEXtoDEC(curbase[COEF01]);
    ANALOG.Coe[0].CIb1 = 5.0 / HEXtoDEC(curbase[COEF02]);
    ANALOG.Coe[0].CIc1 = 5.0 / HEXtoDEC(curbase[COEF03]);

    ANALOG.Coe[0].CP1 = curbase[COEF17];
    ANALOG.Coe[0].CQ1 = curbase[COEF18];

    return 1;
}


//////////////////////////////////////////
/*
 * 遥测数据采集函数，分为三部分。
 * 第一部分，纯粹数据采集，存共享内存，方便所有其他模块调用。
 * 第二部分，软件滤波，弃掉抖动，存自动上送区域。
 * 第三部分，杂项、直流量、时间、频率等。
 */
//////////////////////////////////////////
void *anaRefresh(void *args) {
    //基础数据获取

    unsigned short *curbase = ycbase[0];

    ANALOG.Val[0].Enable = (curbase[ENABLE] == 0xeb90 ? 1 : 0);
    if (ANALOG.Val[0].Enable != 1)
        return NULL;

    ANALOG.Val[0].Hz1 = curbase[HZ] / 1000.0;

    ANALOG.Val[0].Ua = curbase[UA] * ANALOG.Coe[0].CUa;
    ANALOG.Val[0].Ub = curbase[UB] * ANALOG.Coe[0].CUb;
    ANALOG.Val[0].Uc = curbase[UC] * ANALOG.Coe[0].CUc;

    ANALOG.Val[0].Ia1 = curbase[IA] * ANALOG.Coe[0].CIa1;
    ANALOG.Val[0].Ib1 = curbase[IB] * ANALOG.Coe[0].CIb1;
    ANALOG.Val[0].Ic1 = curbase[IC] * ANALOG.Coe[0].CIc1;

    curbase = ycbase[1];

    ANALOG.Val[0].P1 = Doubletonum(curbase[OP1], curbase[OP1 + 1]) / ANALOG.Coe[0].CP1;
    ANALOG.Val[0].Q1 = Doubletonum(curbase[OQ1], curbase[OQ1 + 1]) / ANALOG.Coe[0].CQ1;

    // float *t = &ANALOG.Val[0].Ua;
    // for (int i = 0; i < 24; ++i) {
    //     printf("%10.1f\t", t[i]);
    //     if ((i + 1) % 4 == 0) {printf("\n");}
    // }
    // printf("\n\n\n\n\n\n\n\nn\n\n\n");

    ////////////////
    //保存过滤过程状态
    static int filter[256];

    //获取当前时间
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (int i = 0; i < ANALOG.size; i++) {
        analog_elem elem;
        read_analog_elem(ANALOG.setting, i, &elem);

        //读取当前点的值
        float now = ANALOG.point(elem.board, elem.shift);

        //波动在值的百分之一之内缓慢逼近,其它情况快速逼近
        float cmpval = now * 0.01;

        if (fabsf(now - ANALOG.Form[i]) > cmpval) {
            if (filter[i] > 20) {
                ANALOG.Form[i] = now * 0.01 + ANALOG.Form[i] * 0.99;
            }
            filter[i] ++;
        } else {
            ANALOG.Form[i] = now * 0.002 + ANALOG.Form[i] * 0.998;
            filter[i] = 0;
        }
    }

    ANALOG.buflength = ANALOG.size - 1;
    return NULL;
}

float anaPoint(int board, int index) {
    if (board > 0 && board <= 3) {
        float *base = (float *) &ANALOG.Val[board - 1];
        if (index >= 0 && index < (sizeof(_YcValue) / sizeof(float))) {
            return base[index - 1];
        }
    }

    //功率-直流量
    if (board == 4) {
        if (index == 1)
            return getDc(0) / ANALOG.dcCoe1;
        if (index == 2)
            return getDc(1) / ANALOG.dcCoe2;
        if (index == 3)
            return ANALOG.Val[0].Hz1 * 10;
        if (index == 4)
            return ANALOG.Val[1].Hz1 * 10;
        if (index == 5)
            return ANALOG.Val[2].Hz1 * 10;
    }

    //故障指示器量
    if (board == 5) {
        return WIREL.value[index - 1];
    }
    return 0.0f;
}