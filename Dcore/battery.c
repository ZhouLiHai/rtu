
#include "define.h"

int pwstart(void) {
    pthread_attr_t attr_t;

    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&CONTRAL.thread_key, &attr_t, BATTERY.server, NULL);
}

void *pwserver(void *args) {
    struct timespec timeout;

    while (1) {
        mktimeout(&timeout, 1);
        int r = pthread_cond_timedwait(&BATTERY.sig, &BATTERY.siglock, &timeout);

        if (r == 110)
            continue;
        pthread_mutex_lock(&BATTERY.colock);
        switch (BATTERY.state) {
        case 1:
            set_power_start(1);
            usleep(1500 * 1000);
            set_power_start(0);
            BATTERY.state = 0;
            break;
        case 2:
            set_power_end(1);
            usleep(1500 * 1000);
            set_power_end(0);
            BATTERY.state = 0;
            break;
        }
        pthread_mutex_unlock(&BATTERY.colock);
    }
    return NULL;
}

int pwbegin(void) {
    pthread_mutex_trylock(&BATTERY.colock);
    BATTERY.state = 1;
    pthread_mutex_unlock(&BATTERY.colock);
    return pthread_cond_signal(&BATTERY.sig);
}

int pwend(void) {
    pthread_mutex_trylock(&BATTERY.colock);
    BATTERY.state = 2;
    pthread_mutex_unlock(&BATTERY.colock);
    return pthread_cond_signal(&BATTERY.sig);
}
