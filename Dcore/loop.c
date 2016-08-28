#include "define.h"
#include "NtpTime.h"

int lostart(void) {
    pthread_attr_t attr_t;

    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&CONTRAL.thread_key, &attr_t, LOOP.server, NULL);
}

void *loserver(void *args) {
    int StepPower = 0;
    char ykstate = gpioRead(76);

    while (1) {
        sleep(1);
        //判断硬件状态，点亮故障灯
        //checkstate() == 1 ? LED.on(4) : LED.off(4);
        //呼吸灯

        // printf("setback %d\n", gpioRead(74));
        if (48 == gpioRead(74)) {
            gpioWrite(70, 1);
            sleep(1);
            gpioWrite(70, 0);
            sleep(1);
            system("reboot");
        }

        if (gpioRead(76) != ykstate) {
            ykstate = gpioRead(76);
            int upper = REMOTE.search(9, 0, 0);
            if (upper > 0)
                REMOTE.add(upper, ykstate, 1, ANALOG.devicetv);
        }
        LOOP.step++ % 2 == 0 ? LED.on(5) : LED.off(5);



        if (LOOP.step % 15 == 0) {
            sntp_client(LOOP.ip);
            //REMOTE.add(6001, 0x00, 2, ANALOG.devicetv);
            //REMOTE.add(6001, 0x01, 2, ANALOG.devicetv);
            //REMOTE.add(6001, 0x02, 2, ANALOG.devicetv);
            //REMOTE.add(6001, 0x03, 2, ANALOG.devicetv);
        }

        //保存记录日志

        if (ANALOG.loop == 1) {
            switch (StepPower) {
            case 0:
                if (LOOP.step > BATTERY.activate) {
                    BATTERY.begin();
                    StepPower = 1;
                }
                break;
            case 1:
                if (LOOP.step > BATTERY.activate + BATTERY.onbattery) {
                    BATTERY.end();
                    LOOP.step = 0;
                    StepPower = 0;
                }
                break;
            }
        }
    }
    return NULL;
}

int checkstate(void) {
    /*
     *      0-5 :遥信遥控板
     *      6   :电源板
     *      7   :按键板
     *      8-10:遥测板
     */

    for (int i = 0; i < 6; i++) {
        if (LOOP.board_state[i] != get_board_state(i)) {
            LOOP.board_state[i] = get_board_state(i);
            int upper = REMOTE.search(6, i, 0);
            if (upper > 0)
                REMOTE.add(upper, LOOP.board_state[i], 1, ANALOG.devicetv);
        }
    }

    if (LOOP.board_state[6] != get_board_state(6)) {
        LOOP.board_state[6] = get_board_state(6);
        int upper = REMOTE.search(6, 6, 0);
        if (upper > 0)
            REMOTE.add(upper, LOOP.board_state[6], 1, ANALOG.devicetv);
    }

    if (LOOP.board_state[7] != get_board_state(7)) {
        LOOP.board_state[7] = get_board_state(7);
        int upper = REMOTE.search(6, 7, 0);
        if (upper > 0)
            REMOTE.add(upper, LOOP.board_state[7], 1, ANALOG.devicetv);
    }

    for (int i = 8; i < 11; i++) {
        if (LOOP.board_state[i] != get_board_state(i)) {
            LOOP.board_state[i] = get_board_state(i);
            ANALOG.initCoeffect();
            int upper = REMOTE.search(6, i, 0);
            if (upper > 0)
                REMOTE.add(upper, LOOP.board_state[i], 1, ANALOG.devicetv);
        }
    }

    //判断各板的状态是否与初始时状态相同
    int error_state = 0;

    for (int i = 0; i < 11; i++) {
        if (LOOP.board_state_org[i] != LOOP.board_state[i]) {
            error_state = 1;
            break;
        }
    }
    return error_state;
}
