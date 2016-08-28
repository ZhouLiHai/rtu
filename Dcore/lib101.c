#include "lib101.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int libCommandSend(unsigned char * buf, int type, int fd) {
    buf[0] = 0x10;
    buf[1] = type;
    buf[2] = 0x01;
    buf[3] = (buf[1] + buf[2]) & 0xff;
    buf[4] = 0x16;

    int res = write(fd, buf, 5);
    return res;
}

int lib101TypeU(unsigned char *TempBuf, unsigned short BufLen) {
    return TempBuf[1];
}

int lib101TypeI(unsigned char *TempBuf, unsigned short BufLen) {
    return TempBuf[6];
}

int IEC101_AP_RQALL_CON(unsigned char *SendBuf, struct _com * com, struct _IEC101Struct* str101) {
    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = IEC101_C_IC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTCON;
    SendBuf[9] = com->Addr;
    SendBuf[10] = 0x00;
    SendBuf[11] = 0x00;
    SendBuf[12] = 0x14;

    lib101Send(SendBuf, 12, str101, 1);
}

int IEC101_AP_RQALL_END(unsigned char *SendBuf, struct _com * com, struct _IEC101Struct* str101) {
    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = IEC101_C_IC_NA_1;
    SendBuf[7] = 0x01;
    SendBuf[8] = IEC101_CAUSE_ACTTERM;
    SendBuf[9] = com->Addr;
    SendBuf[10] = 0x07;
    SendBuf[11] = 0x01;
    SendBuf[12] = 0x14;

    lib101Send(SendBuf, 12, str101, 1);
}

int IEC101_AP_SP_NA(unsigned char *SendBuf, struct _com * com, struct _remote * r, struct _IEC101Struct* str101) {
    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = IEC101_M_SP_NA_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_INTROGEN;
    SendBuf[9] = com->Addr;

    int len = 10;
    for (int i = 0; i < r->Slength; ++i) {
        SendBuf[len++] = r->SId[i] & 0xff;
        SendBuf[len++] = (r->SId[i] >> 8) & 0xff;
        //SendBuf[len++] = 0x00;
        SendBuf[len++] = r->SBuf[i];
        if ((i + 1) % YXPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib101Send(SendBuf, len, str101, 1);
            len = 10;
        }
    }
    //lib101Send(SendBuf, len, str101, 1);
    return 1;
}
int IEC101_AP_DP_NA(unsigned char *SendBuf, struct _com * com, struct _remote * r, struct _IEC101Struct* str101) {
    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = IEC101_M_DP_NA_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_INTROGEN;
    SendBuf[9] = com->Addr;

    int len = 10;
    for (int i = 0; i < r->Dlength; ++i) {
        SendBuf[len++] = r->DId[i] & 0xff;
        SendBuf[len++] = (r->DId[i] >> 8) & 0xff;
        //SendBuf[len++] = 0x00;
        SendBuf[len++] = r->DBuf[i];
        if ((i + 1) % YXPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib101Send(SendBuf, len, str101, 1);
            len = 10;
        }
    }
    //lib101Send(SendBuf, len, str101, 1);
    return 1;
}
int IEC101_AP_ME_ND(unsigned char *SendBuf, struct _com * com, struct _analog * a, struct _IEC101Struct* str101) {
    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = IEC101_M_ME_ND_1;
    SendBuf[7] = 0x00;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = com->Addr;

    int len = 10;
    for (int i = 0; i < a->buflength; ++i) {
        unsigned short Value = (short)(a->Form[i] / a->Larg[i] / a->Unit[i] * 100);

        SendBuf[len++] = a->Id[i] & 0xff;
        SendBuf[len++] = (a->Id[i] >> 8) & 0xff;
        //SendBuf[len++] = 0;
        SendBuf[len++] = Value & 0xff;
        SendBuf[len++] = (Value >> 8) & 0xff;
        if ((i + 1) % YCPacketContain == 0) {
            SendBuf[7] = i + 1;
            lib101Send(SendBuf, len, str101, 1);
            len = 10;
        }
    }
    //ib101Send(SendBuf, len, str101, 1);
    return 1;
}


unsigned short checkcode(unsigned char * SendBuf, int len) {
    unsigned short cs = 0x00;
    for (int i = 4; i <= len; ++i) {
        cs += SendBuf[i];
        cs &= 0xff;
    }
    return cs;
}

int lib101Send(unsigned char * SendBuf, int len, struct _IEC101Struct* str101, int type) {

    SendBuf[0] = 0x68;
    SendBuf[1] = len - 3;
    SendBuf[2] = len - 3;
    SendBuf[3] = 0x68;

    SendBuf[len + 1] = checkcode(SendBuf, len);
    SendBuf[len + 2] = 0x16;

    struct Sunit * su = malloc(sizeof(struct Sunit));
    su->type = type;
    su->len = len + 3;
    memcpy(su->buf, SendBuf, len + 3);
    str101->rlist = listAddNodeTail(str101->rlist, su);
}

int IEC101_SP_AUTO(struct _com * com, int type, int num, int state[], struct _IEC101Struct* str101) {
    unsigned char SendBuf[255];

    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = (type == 1) ? IEC101_M_SP_NA_1 : IEC101_M_DP_NA_1;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = com->Addr;

    SendBuf[10] = (INT8U)(((num & 0x00ff)) & 0xff);
    SendBuf[11] = (INT8U)(((num & 0xff00) >> 8) & 0xff);
    SendBuf[12] = 0;

    if (type == 1) {
        SendBuf[13] = (INT8U) state[0];
    }
    if (type == 2) {
        SendBuf[13] = (INT8U)((state[0] << 1) + state[1]);
    }

    SendBuf[7] = 1;
    int res = lib101Send(SendBuf, 13, str101, 0);
    return res;
}
int IEC101_SO_AUTO(struct _com * com, int number, int state[0], long long stime, int type,
                   struct _IEC101Struct * str101) {
    unsigned char SendBuf[255];

    SendBuf[4] = 0x28;
    SendBuf[5] = com->MasterAddr;
    SendBuf[6] = (type == 1) ? IEC101_M_SP_TB_1 : IEC101_M_DP_TB_1;
    SendBuf[8] = IEC101_CAUSE_SPONT;
    SendBuf[9] = com->MasterAddr;

    SendBuf[10] = (number & 0xff);
    SendBuf[11] = (number >> 8) & 0xff;
    SendBuf[12] = 0;

    if (type == 1) {
        SendBuf[13] = state[0];
    }
    if (type == 2) {
        SendBuf[13] = (state[0] << 1) + state[1];
    }

    struct tm p;
    struct timeval t;

    t.tv_sec = stime / 1000; t.tv_usec = stime % 1000;
    localtime_r(&(t.tv_sec), &p);
    unsigned short MS = t.tv_usec + p.tm_sec * 1000;

    SendBuf[14] = (unsigned short)(MS & 0xff);
    SendBuf[15] = (unsigned short)((MS >> 8) & 0xff);
    SendBuf[16] = p.tm_min;
    SendBuf[17] = p.tm_hour;
    SendBuf[28] = p.tm_mday;
    SendBuf[19] = p.tm_mon + 1;
    SendBuf[20] = (p.tm_year + 1900) % 100;

    SendBuf[7] = 1;
    int res = lib101Send(SendBuf, 20, str101, 1);
    return res;
}
