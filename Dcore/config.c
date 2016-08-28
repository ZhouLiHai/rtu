#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "define.h"

int read_control_elem(config_setting_t * parent, int index, control_elem *e) {
    pthread_rwlock_rdlock(&CONFIG.rwlock);
    config_setting_t *r = config_setting_get_elem(parent, index);
    if (r != NULL) {
        config_setting_lookup_int(r, "l", &e->line);
        config_setting_lookup_int(r, "ot", &e->timeout);
        config_setting_lookup_int(r, "s", &e->switchs);
        pthread_rwlock_unlock(&CONFIG.rwlock);
        return 1;
    }
    pthread_rwlock_unlock(&CONFIG.rwlock);
    return 0;
}

int read_remote_elem(config_setting_t * parent, int index, remote_elem *e) {
    pthread_rwlock_rdlock(&CONFIG.rwlock);
    config_setting_t *r = config_setting_get_elem(parent, index);
    if (r != NULL) {
        config_setting_lookup_int(r, "n", &e->name);
        config_setting_lookup_int(r, "t", &e->type);
        config_setting_lookup_int(r, "l0", &e->line0);
        config_setting_lookup_int(r, "l1", &e->line1);
        pthread_rwlock_unlock(&CONFIG.rwlock);
        return 1;
    }
    pthread_rwlock_unlock(&CONFIG.rwlock);
    return 0;
}


int read_wireless_elem(config_setting_t * parent, int index, wireless_elem *e) {
    pthread_rwlock_rdlock(&CONFIG.rwlock);
    config_setting_t *r = config_setting_get_elem(parent, index);
    if (r != NULL) {
        config_setting_lookup_int(r, "n", &e->name);
        config_setting_lookup_int(r, "addr", &e->addr);
        pthread_rwlock_unlock(&CONFIG.rwlock);
        return 1;
    }
    pthread_rwlock_unlock(&CONFIG.rwlock);
    return 0;
}

int read_analog_elem(config_setting_t * parent, int index, analog_elem *e) {
    pthread_rwlock_rdlock(&CONFIG.rwlock);
    config_setting_t *r = config_setting_get_elem(parent, index);
    if (r != NULL) {
        config_setting_lookup_float(r, "down", &e->down);
        config_setting_lookup_float(r, "up", &e->up);
        config_setting_lookup_int(r, "b", &e->board);
        config_setting_lookup_int(r, "l", &e->shift);
        config_setting_lookup_int(r, "s", &e->open);
        config_setting_lookup_int(r, "time", &e->otime);
        pthread_rwlock_unlock(&CONFIG.rwlock);
        return 1;
    }
    pthread_rwlock_unlock(&CONFIG.rwlock);
    return 0;
}

int read_protect_elem(config_setting_t * parent, int index, protect_elem *e) {
    pthread_rwlock_rdlock(&CONFIG.rwlock);
    config_setting_t *r = config_setting_get_elem(parent, index);
    if (r != NULL) {
        config_setting_lookup_float(r, "th", &e->threshold);
        config_setting_lookup_int(r, "c", &e->control);
        config_setting_lookup_int(r, "w", &e->how);
        config_setting_lookup_int(r, "t", &e->t1);
        config_setting_lookup_int(r, "ot", &e->t2);
        config_setting_lookup_int(r, "r", &e->times);
        config_setting_lookup_int(r, "n", &e->upper);
        config_setting_lookup_int(r, "b", &e->board);
        config_setting_lookup_int(r, "l", &e->shift);
        config_setting_lookup_int(r, "s", &e->open);
        pthread_rwlock_unlock(&CONFIG.rwlock);
        return 1;
    }
    pthread_rwlock_unlock(&CONFIG.rwlock);
    return 0;
}