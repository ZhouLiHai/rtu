#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include "define.h"

void adapter(float u, float mi, float p, float q) {

    float cash[24];
    for (int j = 0; j < 1000; ++j) {
        /* code */
        for (int i = 26; i < 26 + 16; ++i) {
            cash[i - 26] = HEXtoDEC(gFpgaic[0x500 + i]) * 0.01 + cash[i - 26] * 0.99;
        }

        cash[16] = cash[16] * 0.99 + Doubletonum(gFpgaic[0x600 + OP1], gFpgaic[0x600 + OP1 + 1]) * 0.01;
        cash[17] = cash[17] * 0.99 + Doubletonum(gFpgaic[0x600 + OQ1], gFpgaic[0x600 + OQ1 + 1]) * 0.01;

        usleep(5 * 1000);
    }

    for (int i = 0; i < 24; ++i) {
        // if (cash[i] < 200) {cash[i] = 0.0;}
        printf("%10.1f\t", cash[i]);
        if ((i + 1) % 4 == 0) {printf("\n");}
    }
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //获取参数值，并存放在参数位置
    float base[24];
    for (int i = 0; i < 24; i++) {
        base[i] = 0.0;

        if (i < 3) {
            base[i] = cash[i] * 5.0 / mi;
        } else if (i < 6) {
            base[i] = cash[i] * 220.0 / u;
            //printf("%f- %f- %f\n", base[i], cash[i], mi);
        } else {
            if (i % 2 == 0) {
                base[i] = cash[i] / p;
            } else {
                base[i] = cash[i] / q;
            }
        }
    }
    for (int i = 0; i < 24; ++i) {
        printf("%10.1f\t", base[i]);
        if ((i + 1) % 4 == 0) {printf("\n");}
    }
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // 从参数位置将参数一一写入FPGA
    // 启动
    for (int i = 0; i < 1; ++i) {
        usleep(50 * 1000);
        for (unsigned short index = 1; index < 24; index++) {
            gFpgaic[0x471] = index;
            usleep(50 * 1000);
            gFpgaic[0x472] = base[index - 1];
            usleep(50 * 1000);
        }
        gFpgaic[0x473] = 0x01;
        printf("writing...\n");
    }
}

int ipcstart(void) {
    pthread_attr_t attr_t;

    pthread_attr_init(&attr_t);
    pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
    return pthread_create(&IPC.thread_key, &attr_t, IPC.server, NULL);
}

int ipccheckself(void) {
    int res = pthread_kill(IPC.thread_key, 0);
    if (res == ESRCH)
        return 0;
    else
        return 1;
}

void *ipcserver(void *args) {
    int upper = 0;

    while (1) {
        usleep(200 * 1000);

        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        //接受一个到server_socket代表的socket的一个连接
        //如果没有连接请求,就等待到有连接请求--这是accept函数的特性
        //accept函数返回一个新的socket,这个socket(new_server_socket)用于同连接到的客户的通信
        //new_server_socket代表了服务器和客户端之间的一个通信通道
        //accept函数把连接到的客户端信息填写到客户端的socket地址结构client_addr中
        int new_server_socket = accept(IPC.sp, (struct sockaddr*)&client_addr, &length);
        if (new_server_socket < 0) {
            printf("Server Accept Failed!\n");
            break;
        }

        _cmd cmd;

        length = recv(new_server_socket, (void *)&cmd, sizeof(cmd), 0);
        printf("command = %d\n", cmd.type);

        int reCmd = 0;

        switch (cmd.type) {
        case 1:
            adapter(cmd.u, cmd.i, cmd.p, cmd.q);
            reCmd = 1;
            send(new_server_socket, &reCmd, sizeof(reCmd), 0);
            break;
        case 2:
            pthread_rwlock_wrlock(&CONFIG.rwlock);
            config_write_file(IPC.cfg, "a.cfg");
            pthread_rwlock_unlock(&CONFIG.rwlock);
            break;
        case 3:
            send(new_server_socket, &ANALOG.Val[cmd.board], sizeof(_YcValue), 0);
            while ((length = recv(new_server_socket, (void *)&cmd, sizeof(cmd), 0)) > 0) {
                send(new_server_socket, &ANALOG.Val[cmd.board], sizeof(_YcValue), 0);
            }
            break;
        case 4:
            send(new_server_socket, &REMOTE.soe, sizeof(_Soe), 0);
            while ((length = recv(new_server_socket, (void *)&cmd, sizeof(cmd), 0)) > 0) {
                send(new_server_socket, &REMOTE.soe, sizeof(_Soe), 0);
            }
            break;
        }

    }
    return NULL;
}
