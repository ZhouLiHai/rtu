#include "define.h"

// #pragma GCC visibility push(hidden)
// #include "../libiec101/libiec101.h"
// #pragma GCC visibility pop

#include "lib101.h"
#include "adlist.h"

int SerialOpen(const char *DeviceName, int baund) {
    DDBG("[com.c]打开串口设备[%s]\n", DeviceName);
    int fd = open(DeviceName, O_RDWR);
    if (fd < 0) {
        DDBG("打开串口失败，失败原因： %d", errno);
        return -1;
    }

    struct termios DevTermi;
    bzero(&DevTermi, sizeof(DevTermi));
    //忽略调制解调器状态行，接收使能
    DevTermi.c_cflag |= (CLOCAL | CREAD);
    //选择为原始输入模式
    DevTermi.c_lflag &= ~(ICANON | ECHO | ECHOE);
    //选择为原始输出模式
    DevTermi.c_oflag &= ~OPOST;
    DevTermi.c_cc[VTIME] = 2;
    DevTermi.c_cc[VMIN] = 0;
    DevTermi.c_cflag &= ~CSIZE;
    DevTermi.c_iflag &= ~ISTRIP;
    DevTermi.c_cflag |= CS8;
    DevTermi.c_cflag &= ~PARENB;
    DevTermi.c_cflag &= ~CSTOPB;
    //设置输入拨特率
    cfsetispeed(&DevTermi, B9600);

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &DevTermi) != 0) {
        DDBG("串口参数设定失败");
        return 0;
    }
    return fd;
}

void prtsyslog(char *prefix, char *buf, int len) {
    char tmpbuf_prt[SERICOM_BUFSIZE * 3];
    memset(tmpbuf_prt, 0, SERICOM_BUFSIZE * 3);

    if (len < 0 || len >= SERICOM_BUFSIZE) {
        DDBG("prtsyslog: 输入长度为 %d, 缓冲区长度为 %d [无法有效打印].", len, SERICOM_BUFSIZE);
    }

    for (int i = 0; i < len && i < SERICOM_BUFSIZE; i++) {
        sprintf((char *) &tmpbuf_prt[i * 3], "%02x ", buf[i]);
    }

    fprintf(stderr, "%s[%d]:%s\n", prefix, len, tmpbuf_prt);
}

unsigned int comSend(int fd, unsigned char  * pBuffer, int iLen) {
    int nleft;
    int nwritten;
    INT8U *buffer;

    buffer = pBuffer;
    nwritten = 0;
    nleft = iLen;
    while (nleft > 0) {
        nwritten = write(fd, buffer, nleft);
        if (nwritten > 0) {
            prtsyslog("COM Send", (char *) buffer, nwritten);
        }
        if (nwritten <= 0) {
            DDBG("串口数据发送失败");
            return 0;
        }
        nleft -= nwritten;
        buffer += nwritten;
    }
    return 1;
}

int comControl(int line, int act, int how) {
    DDBG("串口遥控:线路 %d 动作 %d 分合 %d", line, act, how);
    if (act == 1) {
        CONTRAL.select(line, how);
    } else if (act == 0) {
        CONTRAL.action(line, how);
    } else if (act == 99) {
        judge_time(1);
    } else {
        DDBG("comControl:收到未定义行为.");
    }
}

int getMessage(char * buf, int fd) {
    static char recvbuf[1024];
    static int head = 0;

    int res = 0;

    int recvLength = read(fd, &recvbuf[head], 1024 - head);
    if (recvLength <= 0) {
        return res;
    }

    head += recvLength;
    if (head > 1024) {
        memset(recvbuf, 0, sizeof(recvbuf));
        head = 0;
        return res;
    }

    int i = 0;
    while (i < 1024) {
        if (recvbuf[i] == 0x68 || recvbuf [i] == 0x10) {
            break;
        }
        i++;
    }

    if (recvbuf[i] == 0x10 && recvbuf[i + 4] == 0x16) {
        res = 5;
    } else if (recvbuf[i] == 0x68 && recvbuf[recvbuf[i + 1] + 5] == 0x16) {
        res = recvbuf[i + 1] + 6;
    }

    for (int j = 0; j < res; ++j) {
        buf[j] = recvbuf[i + j];
    }
    for (int k = 0; k < 1024 - i; ++k) {
        recvbuf[k] = recvbuf[i + k];
    }
    head -= (i + res);

    return res;
}

listNode * findFirst(list * l) {
    listIter* lr = listGetIterator(l, AL_START_HEAD);
    for (int i = 0; i < listLength(l); ++i) {
        listNode * n = listNext(lr);
        if (((struct Sunit *)n->value)->type == 1) {
            return n;
        }
    }
    return NULL;
}

listNode * findVip(list * l) {
    listIter* lr = listGetIterator(l, AL_START_HEAD);
    for (int i = 0; i < listLength(l); ++i) {
        listNode * n = listNext(lr);
        if (((struct Sunit *)n->value)->type == 0) {
            return n;
        }
    }
    return NULL;
}

void cAutoSend(struct _IEC101Struct* str101) {
    static int headold = 0;
    static int bw = 0;

    while (bw < REMOTE.soe.info.head) {
        IEC101_SP_AUTO(&COM, REMOTE.soe.unit[bw].type, REMOTE.soe.unit[bw].number, REMOTE.soe.unit[bw].state, str101);
        bw = (++bw) % YX_SIZE;
    }
    while (headold < REMOTE.soe.info.head) {
        IEC101_SO_AUTO(&COM, REMOTE.soe.unit[headold].number, REMOTE.soe.unit[headold].state, REMOTE.soe.unit[headold].time, REMOTE.soe.unit[headold].type, str101);
        headold = (++headold) % YX_SIZE;
    }
}

void *Server(void *args) {
    int fd = *(int*)args;
    int autosend = 0;
    struct _IEC101Struct str101;
    str101.rlist = listCreate();
    listSetFreeMethod(str101.rlist, free);

    while (1) {
        unsigned char dbuf[255];
        unsigned char sendbuf[255];

        if (autosend) {
            cAutoSend(&str101);
        }

        int len = getMessage(dbuf, fd);
        if (len <= 0) {
            continue;
        }
        printf("[%d]len = %d\n", fd, len);
        memset(sendbuf, 0, sizeof(sendbuf));

        if (len == 5) {
            int type = lib101TypeU(dbuf, len);
            switch (type) {
            case 0x49:
                autosend = 1;
                libCommandSend(sendbuf, 0x0b, fd);
                break;
            case 0x40:
                libCommandSend(sendbuf, 0x00, fd);
                break;
            case 0x5b:
            case 0x7b:
                do {
                    listNode *n = findFirst(str101.rlist);
                    if (n != NULL) {
                        libCommandSend(sendbuf, 0x29, fd);
                    } else {
                        libCommandSend(sendbuf, 0x09, fd);
                    }
                } while (0);
                break;
            case 0x5a:
            case 0x7a:
                do {
                    listNode *nVip = findVip(str101.rlist);
                    listNode *nFir = findFirst(str101.rlist);
                    if (nVip != NULL) {
                        comSend(fd, ((struct Sunit *)nVip->value)->buf, ((struct Sunit *)nVip->value)->len);
                        listDelNode(str101.rlist, nVip);
                    } else if (nFir != NULL) {
                        comSend(fd, ((struct Sunit *)nFir->value)->buf, ((struct Sunit *)nFir->value)->len);
                        listDelNode(str101.rlist, nFir);
                    } else {
                        libCommandSend(sendbuf, 0x29, fd);
                    }
                } while (0);
                break;
            }
        } else {
            int type = lib101TypeI(dbuf, len);
            switch (type) {
            case IEC101_C_IC_NA_1:
                IEC101_AP_RQALL_CON(sendbuf, &COM, &str101);
                IEC101_AP_SP_NA(sendbuf, &COM, &REMOTE, &str101);
                IEC101_AP_DP_NA(sendbuf, &COM, &REMOTE, &str101);
                IEC101_AP_ME_ND(sendbuf, &COM, &ANALOG, &str101);
                IEC101_AP_RQALL_END(sendbuf, &COM, &str101);
                libCommandSend(sendbuf, 0x29, fd);
                break;
            }
        }

    }



}

int comStart(void) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < COM.devNum; i++) {
        COM.SeriCom_fd[i] = SerialOpen(COM.devName[i], B9600);
        if (COM.SeriCom_fd[i] < 1) {
            printf("[com.c]: 串口设备打开失败.[%s]", COM.devName[i]);
            return -1;
        }

        if (pthread_create(&COM.pCom[i], &attr, Server, &COM.SeriCom_fd[i]) != 0) {
            printf("[com.c]: 串口服务线程打开失败.[%s]", COM.devName[i]);
            return -1;
        }
    }
    DDBG("[com.c]: 串口模块初始化完毕...");
    return 1;
}
