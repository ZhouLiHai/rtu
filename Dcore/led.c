#include "define.h"

int ledstart(void) {
    pthread_attr_t attr_t;

    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&LED.thread_key, &attr_t, LED.server, NULL);
}

void *ledserver(void *args) {
    struct timespec timeout;

    while (1) {
        mktimeout(&timeout, 1);
        int r = pthread_cond_timedwait(&LED.sig, &LED.siglock, &timeout);

        if (r == 110)
            continue;
        int index = 0;

        for (index = 0; index < 7; index++) {
            switch (LED.state[index]) {
            case 0:
                set_led(index, 0);
                LED.state[index] = -1;
                break;
            case 1:
                set_led(index, 1);
                LED.state[index] = -1;
                break;
            default:
                break;
            }
        }
    }
    return NULL;
}

int ledcheckself(void) {
    int res = pthread_kill(LED.thread_key, 0);
    if (res == ESRCH)
        return 0;
    else
        return 1;
}

int ledon(int ind) {
    LED.state[ind] = 0;
    return pthread_cond_signal(&LED.sig);
}

int ledoff(int ind) {
    LED.state[ind] = 1;
    return pthread_cond_signal(&LED.sig);
}

int ledflash(int ind, int num) {
    LED.state[ind] = 5;
    LED.nm[ind] = num;
    return 1;
}