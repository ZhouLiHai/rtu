#include "define.h"
#include "anet.h"
#include "lib104.h"

#include <sys/socket.h>

// #pragma GCC visibility push(hidden)
// #include "../libiec104/libiec104.h"
// #pragma GCC visibility pop

int TCP_ConnectFlag = 0;

int netstart(void) {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    while (pthread_create(&NET.threadkey, &attr, NET.server, NULL) != 0) sleep(1);
    //while (pthread_create(&NET.autokey, &attr, NET.autosend, NULL) != 0) sleep(1);
    return 1;
}

int netexit(void) {
    close(NET.acceptSocket);
    close(NET.lsnSocket);
    DDBG("网络模块退出...\n");
}

void AutoSend(struct _IEC104Struct* str104) {
    static int chance = 0;
    static int headold = 0;
    static int bw = 0;

    while (bw != REMOTE.soe.info.head) {
        IEC104_SP_AUTO(&NET, REMOTE.soe.unit[bw].type, REMOTE.soe.unit[bw].number, REMOTE.soe.unit[bw].state, str104);
        bw = (++bw) % YX_SIZE;
    }

    if (++chance % 10 == 0) {
        IEC104_YC_AUTO(&NET, &ANALOG, str104);

        while (headold != REMOTE.soe.info.head) {
            IEC104_SO_AUTO(&NET, REMOTE.soe.unit[headold].number, REMOTE.soe.unit[headold].state, REMOTE.soe.unit[headold].time, REMOTE.soe.unit[headold].type, str104);
            headold = (++headold) % YX_SIZE;
        }
    }
}

int acceptTcpHandler(int fd) {
    int cport, cfd, max = 1;
    char cip[46];

    while (max--) {
        cfd = anetTcpAccept(NULL, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                DDBG("Accepting client connection: %s", NULL);
            return ANET_ERR;
        }
        DDBG("[server.c]建立连接 %s:%d", cip, cport);
        return cfd;
    }
}


int netControl(int line, int act, int how) {
    DDBG("网络遥控:线路 %d 动作 %d 分合 %d", line, act, how);
    if (line == 24) {
        struct timeval now;
        struct timezone tz;
        tz.tz_minuteswest = 0;
        tz.tz_dsttime = 0;
        gettimeofday(&now, &tz);

        setback();
        REMOTE.add(6603, 0, 1, ANALOG.devicetv);
    }
    if (act == 0) {
        return CONTRAL.select(line, how);
    } else if (act == 1) {
        CONTRAL.action(line, how);
    } else if (act == 99) {
        CONTRAL.cancel(line, how);
    } else {
        DDBG("[server.c]遥控类型未知-------");
    }
    return 1;
}

void* newClient(void* args) {
    int fd = *(int *)args;
    int autosend = 0;
    unsigned char RecBuf[255];
    unsigned char SenBuf[255];

    anetNonBlock(NULL, fd);
    anetSendTimeout(NULL, fd, 500);

    struct _IEC104Struct str104;
    IEC104_InitAllFlag(&str104);
    str104.sfd = fd;

    while (1) {
        usleep(5 * 1000);
        if (autosend) {
            AutoSend(&str104);
        }
        int len = recv(fd, RecBuf, sizeof(RecBuf), 0);
        if (len == 0) {
            DDBG("[server.c]网络连接断开-------");
            break;
        }

        if (len == -1)
            continue;

        int res = lib104Verify(RecBuf, len);
        if (res == IEC101FRAMEERR)
            continue;
        if (res == IEC101FRAME_U) {
            int type = lib104TypeU(RecBuf, len);
            printf("[typeU] = %d\n", type);

            switch (type) {
            case 0x07:
                libCmdSend(SenBuf, 0x0b, fd);
                autosend = 1;
                break;
            case 0x10:
                libCmdSend(SenBuf, 0x23, fd);
                break;
            case 0x43:
                libCmdSend(SenBuf, 0x83, fd);
                break;
            }
        }

        if (res == IEC101FRAME_I) {
            int type = lib104TypeI(RecBuf, len, &str104);
            printf("[typeI] = %d\n", type);
            struct timeval Ktime;
            int lin, act, how;

            switch (type) {
            case IEC101_C_IC_NA_1:
                IEC104_AP_RQALL_CON(SenBuf, &NET, &str104);
                IEC104_AP_SP_NA(SenBuf, &NET, &REMOTE, &str104);
                IEC104_AP_DP_NA(SenBuf, &NET, &REMOTE, &str104);
                IEC104_AP_ME_ND(SenBuf, &NET, &ANALOG, &str104);
                IEC104_AP_RQALL_END(SenBuf, &NET, &str104);
                break;
            case IEC101_C_CS_NA_1:
                IEC104_AP_CS(RecBuf, SenBuf, &NET, &Ktime, &str104);
                fix_time(Ktime);
                break;
            case IEC101_C_DC_NA_1:
                lin = IEC104_YK_PO(RecBuf) - 0x6001;
                act = IEC104_YK_AC(RecBuf);
                how = IEC104_YK_HW(RecBuf);

                if (IEC104_YK_CA(RecBuf) == 0) {
                    act == 99;
                }

                if (CONTRAL.getswitch(lin) != 1 && lin < 12) {
                    IEC104_YK_TR(RecBuf, SenBuf, &NET, &str104);
                } else {
                    netControl(lin, act, how);
                    IEC104_YK_RP(RecBuf, SenBuf, &NET, &str104);
                }
                break;
            }
        }
    }
}

void *netserver(void *args) {

    NET.lsnSocket = anetTcpServer(NULL, NET.servport, NULL, 1);
    if (NET.lsnSocket != ANET_ERR) {
        //anetNonBlock(NULL, NET.lsnSocket);
        DDBG("[server.c]网络监听成功.");
    } else {
        DDBG("[server.c]网络监听失败.-------");
    }

    while (1) {
        NET.acceptSocket = acceptTcpHandler(NET.lsnSocket);
        if (NET.acceptSocket != ANET_ERR) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            pthread_t tempkey;
            while (pthread_create(&tempkey, &attr, newClient, &NET.acceptSocket) != 0) sleep(1);
        }
    }
    /*
    NET.Local_addr.sin_family = AF_INET;
    NET.Local_addr.sin_port = htons(NET.servport);
    NET.Local_addr.sin_addr.s_addr = INADDR_ANY;

        NET.lsnSocket = socket(AF_INET, SOCK_STREAM, 0);
        bind(NET.lsnSocket, (struct sockaddr *)&NET.Local_addr, sizeof(struct sockaddr));

        if (listen(NET.lsnSocket, 1) < 0) {
            DDBG("Socket监听失败");
            exit(1);
        }

        while (1) {

            struct timeval tv;
            tv.tv_sec = 1, tv.tv_usec = 0;

            FD_ZERO(&NET.readfd); FD_SET(NET.lsnSocket, &NET.readfd);

            int conn = select(NET.lsnSocket + 1, &NET.readfd, NULL, NULL, &tv);
            if (conn == 0) continue;

            socklen_t sin_size = sizeof(struct sockaddr_in);
            printf("before accept\n");
            NET.acceptSocket = accept(NET.lsnSocket, (struct sockaddr *)&NET.Svr_Addr, &sin_size);
            if (NET.acceptSocket < 0) {
                DDBG("接受远方连接请求时发生错误，原因： %d", NET.acceptSocket);
                exit(1);
            }

            IEC10X_InitAllFlag();
            netdealmsg(NET.acceptSocket);
            close(NET.acceptSocket);
        }
        return NULL;*/
}



/*
int netdealmsg(int accSocket) {
    INT8U pTemp_RecvBuf[MAXRECLEN];

    fd_set tempset;

    while (1) {
        FD_ZERO(&tempset);
        FD_SET(accSocket, &tempset);
        struct timeval tv;

        tv.tv_sec = 1, tv.tv_usec = 0;

        int conn = select(accSocket + 1, &tempset, NULL, NULL, &tv);

        if (conn == 0) continue;
        if (conn < 0) break;
        int sTemp_RecvLength = recv(accSocket, (char *)pTemp_RecvBuf, MAXRECLEN, 0);

        if (sTemp_RecvLength <= 0) break;

        iec10x_init_timer();

        INT8U dealbuf[MAXRECLEN];

        int pindex = 0, len_once = 0;

        while (pindex < sTemp_RecvLength) {
            len_once = 0;
            while (pTemp_RecvBuf[pindex] != 0x68) {
                pindex++;
                if (pindex >= sTemp_RecvLength) break;
            }
            pindex++;
            if (pindex >= sTemp_RecvLength) break;

            len_once = pTemp_RecvBuf[pindex++];

            memset(dealbuf, 0, sizeof(dealbuf));
            memcpy(dealbuf, &pTemp_RecvBuf[pindex - 2], len_once + 2);

            pindex += len_once;

            ///////////////////////////////
            YKDATAIEC YK;

            int res = iec10x_verify_handle(NET.Addr, NET.MasterAddr,
                                           dealbuf,
                                           len_once + 2,
                                           YK,
                                           REMOTE.SBuf,
                                           REMOTE.Slength,
                                           REMOTE.DBuf,
                                           REMOTE.Dlength,
                                           ANALOG.Form,
                                           ANALOG.buflength,
                                           REMOTE.SId,
                                           REMOTE.DId,
                                           ANALOG.Id,
                                           ANALOG.Unit,
                                           ANALOG.Larg,
                                           &settime,
                                           1,
                                           NET.iTranCauseL,
                                           NET.iCommonAddrL,
                                           NET.iInfoAddrL,
                                           NULL);
            if (res == 10) {
                TCP_ConnectFlag = 1;
            }
        }
    }
    return 1;
}

int netControl(int line, int act, int how) {
    DDBG("网络遥控:线路 %d 动作 %d 分合 %d", line, act, how);
    if (line == 24) {
        struct timeval now;
        struct timezone tz;
        tz.tz_minuteswest = 0;
        tz.tz_dsttime = 0;
        gettimeofday(&now, &tz);

        setback();
        REMOTE.add(6603, 0, 1, ANALOG.devicetv);
    }
    if (act == 1) {
        return CONTRAL.select(line, how);
    } else if (act == 0) {
        CONTRAL.action(line, how);
    } else if (act == 99) {
        DDBG("来自网络:设定时间 %d %d", settime.tv_sec, settime.tv_usec);
        fix_time(settime);
    } else {
        DDBG("规约遥控函数，线路输入");
    }
    return 1;
}

unsigned int netSend(INT8U * pBuffer, INT16U iLen) {
    char ucTemp_nSendCount;
    for (int i = 0; i < iLen; ++i) {
        printf("%02x ", pBuffer[i]);
    }
    printf("\n");

    ucTemp_nSendCount = send(NET.acceptSocket, (char *)(pBuffer), iLen, 0);

    if (ucTemp_nSendCount == -1)
        DDBG("网络进程发送报文时失败，失败原因： %s", strerror(errno));
    return 1;
}
*/