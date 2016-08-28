#include "define.h"

#define SERICOM_BUFSIZE 255

#define FTU

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80
//数据帧格式
#define FRAME_LENGTH 20
#define FRAME_HEAD 0x28
#define FRAME_TAIL 0x29

#define HEAD_BYTE 0
#define SENDADDR_HIGH 1
#define SENDADDR_LOW 2
#define RCVADDR_HIGH 3
#define RCVADDR_LOW 4
#define FUNC_HIGH 5
#define FUNC_LOW 6
#define INFO_1 7
#define INFO_2 8
#define INFO_3 9
#define INFO_4 10
#define INFO_5 11
#define INFO_6 12
#define INFO_7 13
#define INFO_8 14
#define INFO_9 15
#define INFO_10 16
#define INFO_11 17
#define CHECK_BYTE 18
#define TAIL_BYTE 19

#define ADDR_HIGH 0x0c
#define ADDR_LOW 0x10

//功能码格式--高位
#define FUNC_REPORT 0x02        //上报
//功能码格式--低位
#define VALUE_I 0x10            //电流
#define VALUE_LINEINFO 0x20     //线路数据
#define VALUE_FAULT 0x90        //故障

static char rcvFrame[FRAME_LENGTH];
static int curIndex;
int hWLDown;


INT8S wldatasend(INT8U * pBuffer, INT16U iLen) {
    int nleft = iLen;
    int nwritten;
    INT8U *buffer;

    buffer = pBuffer;
    nwritten = 0;
    while (nleft > 0) {
        nwritten = write(hWLDown, buffer, nleft);
        if (nwritten > 0) {
            //prtsyslog("COM Send", (char*) buffer, nwritten);
        }
        if (nwritten <= 0) {
            DDBG("故障指示器，串口发送失败");
            return 0;
        }
        nleft -= nwritten;
        buffer += nwritten;
    }
    return 1;
}

int checkFrameFormat(char *rcvFrame) {
    if (rcvFrame[HEAD_BYTE] != FRAME_HEAD) {
        DDBG("无线接收：帧头出错。");
        return 0;
    }
    if (rcvFrame[TAIL_BYTE] != FRAME_TAIL) {
        DDBG("无线接收：帧尾出错。");
        return 0;
    }
    INT8U checkCode = 0x00;

    int index;

    for (index = SENDADDR_HIGH; index < CHECK_BYTE; index++) {
        checkCode += rcvFrame[index];
    }
    if (checkCode != rcvFrame[CHECK_BYTE]) {
        DDBG("无线接收：校验码出错。");
        return 0;
    }
    // if ((rcvFrame[RCVADDR_HIGH] != ADDR_HIGH) || (rcvFrame[RCVADDR_LOW] != ADDR_LOW)) {
    //     DDBG("无线接收：地址无法匹配。");
    //     return 2;
    // }
    return 1;
}

/*
 * 回复故障指示器主动上报的报文
 *
 * 1 信息部分数据到底是空的还是和上报报文一样,模拟软件和规约中不一样
 */
void reply_ReprotFrame(char *rcvFrame) {
    char sendFrame[FRAME_LENGTH];

    bzero(sendFrame, FRAME_LENGTH);
    memcpy(sendFrame, rcvFrame, FRAME_LENGTH);

    sendFrame[SENDADDR_HIGH] = rcvFrame[RCVADDR_HIGH];
    sendFrame[SENDADDR_LOW] = rcvFrame[RCVADDR_LOW];
    sendFrame[RCVADDR_HIGH] = rcvFrame[SENDADDR_HIGH];
    sendFrame[RCVADDR_LOW] = rcvFrame[SENDADDR_LOW];
    sendFrame[FUNC_LOW] = rcvFrame[FUNC_LOW] + 1;

    sendFrame[CHECK_BYTE] = rcvFrame[CHECK_BYTE] + 1;
    wldatasend((INT8U *) sendFrame, FRAME_LENGTH);
}

int wlgetIndex(unsigned short addr) {
    int n, a;
    for (int i = 0; i < WIREL.size; i++) {
        wireless_elem elem;
        read_wireless_elem(WIREL.setting, i, &elem);
        if (elem.addr == addr) {
            return elem.name;
        }
    }
    return -1;
}

void setYCValue(INT16U addr, INT16S value) {
    INT8S index = WIREL.index(addr);

    if (index == -1)
        return;
    WIREL.value[index] = value;
}

void setYXValue(INT16U addr, INT16U value) {
    INT8S index = WIREL.index(addr);
    int r = 0;
    if (index == -1)
        return;
    switch (value) {
    case WLSIG_ON:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 1, 1, ANALOG.devicetv);
        break;
    case WLSIG_OFF:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 0, 1, ANALOG.devicetv);
        break;
    case WLSIG_SHORT:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 1, 1, ANALOG.devicetv);
        break;
    case WLSIG_SHORTRECOVER:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 0, 1, ANALOG.devicetv);
        break;
    case WLSIG_GROUND:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 1, 1, ANALOG.devicetv);
        break;
    case WLSIG_GROUNDRECOVER:
        r = REMOTE.search(value, index, 0);
        REMOTE.add(r, 0, 1, ANALOG.devicetv);
        break;
    default:
        DDBG("无线接收：未知故障类型。");
    }
}

/*
 * 处理故障指示器主动上报的报文 和召唤回复的报文
 * frameKind 0 --指示器主动上报的报文
 * frameKind 1 --召唤回复的报文
 */
void handleInfoFrame(char *rcvFrame) {
    prtsyslog("[frame] ", (char *) rcvFrame, FRAME_LENGTH);
// #ifdef FTU
    INT16S valueI = 0;
    INT16U addr = 0x0000;

    addr = rcvFrame[SENDADDR_HIGH] & 0x00ff;
    addr = addr << 8;
    addr += rcvFrame[SENDADDR_LOW];

    if (rcvFrame[FUNC_LOW] == VALUE_I) { //电流值
        valueI = 0x0000;
        valueI = rcvFrame[INFO_1] & 0x000f;
        valueI = valueI << 8;
        valueI += rcvFrame[INFO_2];

        setYCValue(addr, valueI);
    } else if (rcvFrame[FUNC_LOW] == VALUE_FAULT) { //故障
        setYXValue(addr, rcvFrame[INFO_2]);
    } else if (rcvFrame[FUNC_LOW] == VALUE_LINEINFO) { //线路数据
        valueI = 0x0000;
        valueI = rcvFrame[INFO_1] & 0x000f;
        valueI = valueI << 8;
        valueI += rcvFrame[INFO_2];

        setYCValue(addr, valueI);
    } else {
        DDBG("无线接收，未知功能码。");
        return;
    }
// #else
//     if ((rcvFrame[FUNC_HIGH] != FUNC_REPORT) || (rcvFrame[FUNC_LOW] != VALUE_DTU)) {
//         DDBG("无线接收，未知功能码。");
//         return;
//     }
//     int lunum = rcvFrame[SENDADDR_LOW] >> 4;

//     lunum = lunum & 0x0f;
//     if (rcvFrame[INFO_1] & BIT7)
//         DDBG("故障电流: ");
//     else
//         DDBG("负荷电流: ");

//     if ((rcvFrame[INFO_2] == 0xff) && (rcvFrame[INFO_3] == 0xff))
//         DDBG("无效值\n");
//     else {
//         INT16S valuei = 0x0000;

//         valuei = rcvFrame[INFO_2] & 0x000f;
//         valuei = valuei << 8;
//         valuei = valuei + (rcvFrame[INFO_3] & 0x000f);

//         DDBG("%dA\n", valuei);
//     }

//     if (rcvFrame[INFO_1] & BIT6)
//         DDBG("故障温度:");
//     else
//         DDBG("负荷温度:");

//     if ((rcvFrame[INFO_4] == 0xee) && (rcvFrame[INFO_5] == 0xee))
//         DDBG("无效值\n");
//     else {
//         int valuei = (rcvFrame[INFO_4] << 16) + rcvFrame[INFO_5];

//         DDBG("%d度\n", valuei);
//     }

//     if (rcvFrame[INFO_1] & BIT5) {
//         DDBG("温度告警数据无效\n");
//     } else {
//         if (rcvFrame[INFO_1] & BIT4) {
//             if (rcvFrame[INFO_1] & BIT3)
//                 DDBG("线路温度告警\n");
//             else
//                 DDBG("线路温度正常\n");
//         } else {
//             if (rcvFrame[INFO_1] & BIT3)
//                 DDBG("线路温度正常变告警\n");
//             else
//                 DDBG("线路温度告警变正常\n");
//         }
//     }

//     if (rcvFrame[INFO_1] & BIT2) {
//         DDBG("电流告警数据无效\n");
//     } else {
//         if (rcvFrame[INFO_1] & BIT1) {
//             if (rcvFrame[INFO_1] & BIT0)
//                 DDBG("线路电流告警\n");
//             else
//                 DDBG("线路电流正常\n");
//         } else {
//             if (rcvFrame[INFO_1] & BIT0) {
//                 DDBG("线路电流正常变告警:");
//                 if ((rcvFrame[SENDADDR_LOW] & 0x0f) == 4)
//                     DDBG("接地\n");
//                 else
//                     DDBG("短路\n");

//             } else {
//                 DDBG("线路电流告警变正常:");
//                 if ((rcvFrame[SENDADDR_LOW] & 0x0f) == 4)
//                     DDBG("接地\n");
//                 else
//                     DDBG("短路\n");
//             }

//         }
//     }
// #endif
    reply_ReprotFrame(rcvFrame);
}

// 0xff 0x56 0xae 0x35 0xa9 0x55 0xf0

void *wlserver(void *args) {
    int sTemp_RecvLength = 0;
    char pTemp_RecvBuf[SERICOM_BUFSIZE];
    int sericom_flag = 0;
    int curIndex = 0;
    char rcvFrame[FRAME_LENGTH];

    bzero(rcvFrame, FRAME_LENGTH);

    while (1) {
        if (sericom_flag == 0) {
            usleep(5000000);    //延时5s
            WIREL.SeriCom_fd = SerialOpen("/dev/ttyO2", B9600);
            if (WIREL.SeriCom_fd < 1) {
                continue;
            } else {
                hWLDown = WIREL.SeriCom_fd;
                sericom_flag = 1;
                curIndex = 0;
            }

        }

        usleep(800000);         //延时100ms
        memset(pTemp_RecvBuf, 0, SERICOM_BUFSIZE);

        sTemp_RecvLength = read(WIREL.SeriCom_fd, pTemp_RecvBuf, SERICOM_BUFSIZE);

        if (sTemp_RecvLength == -1) {
            sericom_flag = 0;
            close(WIREL.SeriCom_fd);
            continue;
        }

        int pindex = 0;
        for (pindex = 0; pindex < sTemp_RecvLength; pindex++) {

            rcvFrame[curIndex] = pTemp_RecvBuf[pindex];
            printf("rcvFrame %d, %02x\n", curIndex, rcvFrame[curIndex]);
            curIndex++;
            if (curIndex == FRAME_LENGTH) {
                if (checkFrameFormat(rcvFrame) == 1) {
                    handleInfoFrame(rcvFrame);
                    curIndex = 0;
                    bzero(rcvFrame, FRAME_LENGTH);
                } else if (checkFrameFormat(rcvFrame) == 2) {
                    //不处理的正常报文
                    curIndex = 0;
                    bzero(rcvFrame, FRAME_LENGTH);
                    continue;
                } else {
                    //错误报文,丢弃本次接受的所有内容
                    curIndex = 0;
                    bzero(rcvFrame, FRAME_LENGTH);
                    break;
                }
            }
        }
    }
    close(WIREL.SeriCom_fd);
    return (void *) 1;
}

int wlstart(void) {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while (pthread_create(&WIREL.ptWlcom, NULL, WIREL.server, NULL) != 0)
        usleep(1000000);
    return 1;
}