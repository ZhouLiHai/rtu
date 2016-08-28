#include "lib104.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lib104Verify(unsigned char * TempBuf, unsigned short BufLen) {
    //登录报文
    char login[] = { 0x68,
                     0x04,
                     0x07,
                     0x00,
                     0x00,
                     0x00
                   };

    if ((BufLen > 255) || (BufLen < 6)) {
        return IEC101FRAMEERR;
    }
    //检查开头
    if (*TempBuf != 0x68 || TempBuf[1] != (BufLen - 2)) {
        return IEC101FRAMEERR;
    }
    //检查长度
    if (memcmp(login, TempBuf, 6) == 0 || BufLen == 6) {
        return IEC101FRAME_U;
    }
    return IEC101FRAME_I;
}

unsigned short GetShort(unsigned char *chan) {
    unsigned short res = (*chan) * 256 + *(chan + 1);
    return (res);
}

int lib104TypeU(unsigned char *TempBuf, unsigned short BufLen) {

    unsigned short ServSendNum = GetShort(TempBuf + 2);
    unsigned short ServRecvNum = GetShort(TempBuf + 4);

    return TempBuf[2];
}

int lib104TypeI(unsigned char *TempBuf, unsigned short BufLen, struct _IEC104Struct* str104) {

    unsigned short ServSendNum = GetShort(TempBuf + 2);
    unsigned short ServRecvNum = GetShort(TempBuf + 4);

    if (!(ServSendNum & 0x0001)) {
        ServRecvNum >>= 1;
        str104->usServRecvNum = ServRecvNum;

        if (ServSendNum == 0x7fff) {
            str104->usRecvNum = 0;
        } else
            str104->usRecvNum++;

        return TempBuf[6];
    }
}

int lib104BaseCheck(unsigned char *TempBuf, unsigned short BufLen, int MasterAddr) {
    if ((TempBuf[8] != IEC101_CAUSE_ACT)) {
        return IEC101_CAUSE_UNKNOWNCAUSE;
    }

    if ((TempBuf[10] != MasterAddr)) {
        return IEC101_CAUSE_UNKNOWNCOMMADDR;
    }

    if (0) {
        return IEC101_CAUSE_UNKNOWNINFOADDR;
    }
}

int IEC104_AP_RQALL_CON(unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104) {
    SendBuf[6] = IEC101_C_IC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTCON;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;
    SendBuf[12] = 0;
    SendBuf[13] = 0;
    SendBuf[14] = 0;
    SendBuf[15] = IEC101_CAUSE_INTROGEN;
    return lib104Send(SendBuf, 16, str104);
}

int IEC104_AP_RQALL_END(unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104) {
    SendBuf[6] = IEC101_C_IC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTTERM;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;
    SendBuf[12] = 0;
    SendBuf[13] = 0;
    SendBuf[14] = 0;
    SendBuf[15] = IEC101_CAUSE_INTROGEN;
    return lib104Send(SendBuf, 16, str104);
}

//不带时标的单点遥信，响应总召测，类型标识1
int IEC104_AP_SP_NA(unsigned char *SendBuf, struct _net * net, struct _remote * r, struct _IEC104Struct* str104) {

    SendBuf[6] = IEC101_M_SP_NA_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_INTROGEN;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    int len = 12;
    for (int i = 0; i < r->Slength; ++i) {
        SendBuf[len++] = r->SId[i] & 0xff;
        SendBuf[len++] = (r->SId[i] >> 8) & 0xff;
        SendBuf[len++] = 0x00;
        SendBuf[len++] = r->SBuf[i];
        if ((i + 1) % YXPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib104Send(SendBuf, len, str104);
            len = 12;
        }
    }
    return 1;
}

//不带时标的双点遥信，响应总召测，类型标识2
int IEC104_AP_DP_NA(unsigned char *SendBuf, struct _net * net, struct _remote * r, struct _IEC104Struct* str104) {
    SendBuf[6] = IEC101_M_DP_NA_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_INTROGEN;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    int len = 12;
    for (int i = 0; i < r->Dlength; ++i) {
        SendBuf[len++] = r->DId[i] & 0xff;
        SendBuf[len++] = (r->DId[i] >> 8) & 0xff;
        SendBuf[len++] = 0x00;
        SendBuf[len++] = r->DBuf[i];
        if ((i + 1) % YXPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib104Send(SendBuf, len, str104);
            len = 12;
        }
    }
    return 1;
}

//测量值，不带品质描述词的归一化值，响应总召测，类型标识21
int IEC104_AP_ME_ND(unsigned char *SendBuf, struct _net * net, struct _analog * a, struct _IEC104Struct* str104) {
    SendBuf[6] = IEC101_M_ME_ND_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    int len = 12;
    for (int i = 0; i < a->buflength; ++i) {
        unsigned short Value = (short)(a->Form[i] / a->Larg[i] / a->Unit[i] * 100);

        SendBuf[len++] = a->Id[i] & 0xff;
        SendBuf[len++] = (a->Id[i] >> 8) & 0xff;
        SendBuf[len++] = 0;
        SendBuf[len++] = Value & 0xff;
        SendBuf[len++] = (Value >> 8) & 0xff;
        if ((i + 1) % YCPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib104Send(SendBuf, len, str104);
            len = 12;
        }
    }
    return 1;
}

void IEC104_InitAllFlag(struct _IEC104Struct * str104) {

    str104->ucDataSend_Flag = 0;
    str104->ucYK_TimeCount_Flag = 0;
    str104->ucYK_TimeCount = 0x00;

    str104->ucSendCountOverturn_Flag = 0;

    str104->usRecvNum = 0;
    str104->usSendNum = 0;

    str104->usServRecvNum = 0;
    str104->usServSendNum = 0;

    str104->ucIdleCount_Flag = 0;
    str104->ucIdleCount = 0;

    str104->ucNoRecvCount = 0;
    str104->ucNoRecvT3 = 0;

    str104->ucWaitServConCount_Flag = 0;
    str104->ucWaitServConCount = 0;

    str104->w = 0;
    str104->k = 0;
}

//对时，类型标识103
int IEC104_AP_CS(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct timeval *settime, struct _IEC104Struct* str104) {

    INT16U usMSec;
    unsigned char ucSec, ucMin, ucHour, ucDay, ucMonth, ucYear;

    usMSec = RecvBuf[16] * 256 + RecvBuf[15];
    ucSec = usMSec / 1000;
    ucMin = RecvBuf[17] & 0x3f;
    ucHour = RecvBuf[18] & 0x1f;
    ucDay = RecvBuf[19] & 0x1f;
    ucMonth = RecvBuf[20] & 0x0f;
    ucYear = RecvBuf[21] & 0x7f;

    if (ucSec > 59 || ucMin > 59 || ucHour > 23 || ucYear > 99 || ucMonth > 12)
        return 0;

    if ((ucMonth == 0) || (ucDay == 0))
        return 0;
    if (ucMonth == 2) {
        if (((ucYear + 2000) % 4 == 0 && (ucYear + 2000) % 100 != 0) || (ucYear + 2000) % 400 == 0) {
            if (ucDay > 29)
                return 0;
        } else {
            if (ucDay > 28)
                return 0;
        }
    }
    if (ucMonth <= 7 && ucMonth % 2 == 0 && ucDay > 30)
        return 0;
    if (ucMonth >= 8 && ucMonth % 2 == 1 && ucDay > 30)
        return 0;

    struct tm settimetm;
    settimetm.tm_sec = ucSec;
    settimetm.tm_min = ucMin;
    settimetm.tm_hour = ucHour;
    settimetm.tm_mday = ucDay;
    settimetm.tm_mon = ucMonth > 0 ? ucMonth - 1 : 0;
    settimetm.tm_year = ucYear + 100;

    struct timeval settimeval;
    settimeval.tv_sec = mktime(&settimetm);
    settimeval.tv_usec = usMSec * 1000;

    memcpy(settime, &settimeval, sizeof(settimeval));
    SendBuf[6] = RecvBuf[6];
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTCON;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = net->Addr & 0xff;
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    for (int i = 12; i < 12 + 10; i++) {
        SendBuf[i] = RecvBuf[i];
    }
    int res = lib104Send(SendBuf, 12 + 10, str104);
    return res;
}

//遥控，双命令，类型标识46
int IEC104_YK_PO(char * RecvBuf) {
    int line = (RecvBuf[13] << 8) + RecvBuf[12];
    return line;
}
//0x00 预选 0x01 执行
int IEC104_YK_AC(char * RecvBuf) {
    int act = RecvBuf[15];
    return (act & 0x80) == 0x00;
}
//0x01 合 0x00 分
int IEC104_YK_HW(char * RecvBuf) {
    int how = RecvBuf[15];
    return (how & 0x01) == 0x01;
}
//0x00 停止  0x01 开始
int IEC104_YK_CA(char * RecvBuf) {
    int how = RecvBuf[8];
    return (how & 0x06) == 0x06;
}

int IEC104_YK_RP(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104) {

    SendBuf[6] = IEC101_C_DC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = (RecvBuf[8] == IEC101_CAUSE_ACT) ? IEC101_CAUSE_ACTCON : IEC101_CAUSE_DEACTCON;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    SendBuf[12] = RecvBuf[12];
    SendBuf[13] = RecvBuf[13];
    SendBuf[14] = RecvBuf[14];
    SendBuf[15] = RecvBuf[15];

    int res = lib104Send(SendBuf, 16, str104);
    return res;
}

int IEC104_YK_TR(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104) {

    SendBuf[6] = IEC101_C_DC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTTERM;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    SendBuf[12] = RecvBuf[12];
    SendBuf[13] = RecvBuf[13];
    SendBuf[14] = RecvBuf[14];
    SendBuf[15] = RecvBuf[15];

    int res = lib104Send(SendBuf, 16, str104);
    return res;
}

int IEC104_SP_AUTO(struct _net * net, int type, int num, int state[], struct _IEC104Struct* str104) {
    unsigned char SendBuf[255];

    SendBuf[6] = (type == 1) ? IEC101_M_SP_NA_1 : IEC101_M_DP_NA_1;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = net->MasterAddr;

    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    SendBuf[12] = (INT8U)(((num & 0x00ff)) & 0xff);
    SendBuf[13] = (INT8U)(((num & 0xff00) >> 8) & 0xff);
    SendBuf[14] = 0;

    if (type == 1) {
        SendBuf[15] = (INT8U) state[0];
    }
    if (type == 2) {
        SendBuf[15] = (INT8U)((state[0] << 1) + state[1]);
    }

    SendBuf[7] = 1;
    int res = lib104Send(SendBuf, 16, str104);
    return res;
}
int IEC104_YC_AUTO(struct _net * net, struct _analog * a, struct _IEC104Struct* str104) {
    static float YcValueD[128];
    unsigned char SendBuf[255];

    SendBuf[6] = IEC101_M_ME_ND_1;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;

    // while (i < YcNum) {
    //     if (fabs((double) YcValue[i] - (double) YcValueD[i]) > YcCriticalValue[i]) {
    //         INT16U Value = (short)(YcValue[i] / YcLargerTime[i] / YcUnitaryTime[i] * 100);

    //         SendBuf->ucInfoUnit[len++] = (INT8U)(YcObjectAddr[i] & 0xff);
    //         SendBuf->ucInfoUnit[len++] = (INT8U)((YcObjectAddr[i] >> 8) & 0xff);
    //         SendBuf->ucInfoUnit[len++] = 0;
    //         SendBuf->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
    //         SendBuf->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
    //         YcValueD[i] = YcValue[i];
    //         num++;
    //     }
    //     if (num >= YCPacketContain) {
    //         SendBuf->ucRep_Num = YCPacketContain;
    //         iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
    //         len = 0;
    //         num = 0;
    //     }
    //     i++;
    // }

    // if ((num > 0) && (num < YCPacketContain)) {
    //     SendBuf->ucRep_Num = num;
    //     iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
    // }
    // return 1;
/////
    int len = 12, num = 0;
    for (int i = 0; i < a->buflength; ++i) {
        //printf("%f, %f\n", a->Form[i], YcValueD[i]);
        if (fabs((double) a->Form[i] - (double) YcValueD[i]) > a->Dead[i]) {
            num ++;
            YcValueD[i] = a->Form[i];
            unsigned short Value = (short)(a->Form[i] / a->Larg[i] / a->Unit[i] * 100);

            SendBuf[len++] = a->Id[i] & 0xff;
            SendBuf[len++] = (a->Id[i] >> 8) & 0xff;
            SendBuf[len++] = 0;
            SendBuf[len++] = Value & 0xff;
            SendBuf[len++] = (Value >> 8) & 0xff;
            if ((num + 1) % YCPacketContain == 0) {
                SendBuf[7] = num;
                //printf("num = %d, len = %d\n", num, len);
                lib104Send(SendBuf, len, str104);
                len = 12;
                num = 0;
            }
        }
    }

    if ((num > 0) && (num < YCPacketContain)) {
        SendBuf[7] = num;
        //printf("num = %d, len = %d\n", num, len);
        int res = lib104Send(SendBuf, len, str104);
        return res;
    }
    return 1;
}
int IEC104_SO_AUTO(struct _net * net, int number, int state[0], long long stime, int type,
                   struct _IEC104Struct * str104) {
    unsigned char SendBuf[255];

    SendBuf[6] = (type == 1) ? IEC101_M_SP_TB_1 : IEC101_M_DP_TB_1;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = net->MasterAddr;
    SendBuf[10] = (net->Addr & 0xff);
    SendBuf[11] = (net->Addr >> 8) & 0xff;


    SendBuf[12] = (number & 0xff);
    SendBuf[13] = (number >> 8) & 0xff;
    SendBuf[14] = 0;

    if (type == 1) {
        SendBuf[15] = state[0];
    }
    if (type == 2) {
        SendBuf[15] = (state[0] << 1) + state[1];
    }

    struct tm p;
    struct timeval t;

    t.tv_sec = stime / 1000; t.tv_usec = stime % 1000;
    localtime_r(&(t.tv_sec), &p);
    unsigned short MS = t.tv_usec + p.tm_sec * 1000;

    SendBuf[16] = (unsigned short)(MS & 0xff);
    SendBuf[17] = (unsigned short)((MS >> 8) & 0xff);
    SendBuf[18] = p.tm_min;
    SendBuf[19] = p.tm_hour;
    SendBuf[20] = p.tm_mday;
    SendBuf[21] = p.tm_mon + 1;
    SendBuf[22] = (p.tm_year + 1900) % 100;

    SendBuf[7] = 1;
    int res = lib104Send(SendBuf, 23, str104);
    return res;
}



int lib104Send(unsigned char * buf, int length, struct _IEC104Struct * str104) {
    buf[0] = 0x68;
    buf[1] = length - 2;
    int sendnum = str104->usSendNum << 1;
    buf[2] = sendnum & 0xff;
    buf[3] = (sendnum >> 8) & 0xff;

    int recvnum = str104->usRecvNum << 1;
    buf[4] = recvnum & 0xff;
    buf[5] = (recvnum >> 8) & 0xff;

    int res = send(str104->sfd, buf, length, 0);
    memset(buf, 0, length);

    //判断发送序列是否需要反转
    if (str104->usSendNum == 0x7fff) {
        str104->usSendNum = 0;
        str104->ucSendCountOverturn_Flag = TRUE;
    } else
        str104->usSendNum++;

    str104->k++;
    str104->w = 0;

    return res;
}

int libCmdSend(unsigned char * buf, int type, int fd) {
    buf[0] = 0x68;
    buf[1] = 0x04;
    buf[2] = type;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;

    int res = send(fd, buf, 6, 0);
    return res;
}