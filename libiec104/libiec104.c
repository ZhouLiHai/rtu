/*
 * libiec101.c
 *
 *  Created on: 2014-01-08
 *  Author: Monday, May 04 2015 18:21:59
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/timeb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include "libiec104.h"
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include "libhd.h"

IEC101Struct *pIEC10X_Struct;

//主动上送时间
INT8U iec10x_time_autosend(INT16U Addr,
                           INT16U MasterAddr,
                           void * pSOE,
                           INT32U head,
                           INT8U iSOENum,
                           INT8U iFlag,
                           INT8U iTranCauseL,
                           INT8U iCommonAddrL,
                           INT8U iInfoAddrL
                          ) {
    INT16U len = 0;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pAutoSendYXBuf + 6);

    pTemp_ASDUFrame->ucType = 201;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    struct tm p;
    struct timeval t;

    time((time_t *)&t);
    localtime_r(&(t.tv_sec), &p);

    INT16U MS = t.tv_usec + p.tm_sec * 1000;

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(MS & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((MS >> 8) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_min;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_hour;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mday;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mon + 1;
    pTemp_ASDUFrame->ucInfoUnit[len++] = (p.tm_year + 1900) % 100;

    pTemp_ASDUFrame->ucRep_Num = 1;
    iec10x_data_send(FORMAT_I, len + 6, AUTOYX);

    return 1;
}

//不带时标的单点遥信，响应总召测，类型标识1
INT8U IEC10X_AP_SP_NA(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                      INT16U Addr,
                      INT16U MasterAddr,
                      INT8U YXBuf[],
                      INT8U YXBufLength,
                      INT16U YXAddr[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL) {

    INT16U iPacketNum = YXBufLength / YXPacketContain;
    INT16U iLastPacketContain = YXBufLength % YXPacketContain;
    INT16U iCycleNum, iCycleNumber;
    INT16U j = 0, len = 0;

    if (YXBufLength <= 0)
        return 0;

    if ((iPacketNum > 0) && (iLastPacketContain == 0))
        iCycleNumber = iPacketNum;
    else
        iCycleNumber = iPacketNum + 1;

    /*
     * 一下开始组织报文，根据具体情况会发送不定数量个报文。
     */
    for (iCycleNum = 0; iCycleNum < iCycleNumber; iCycleNum++) {
        /*
         * 根据报文需要组织的信息点个数填写控制信息位。
         */
        INT16U iEveryPacketContain = 0;

        if (iCycleNum == iPacketNum)
            iEveryPacketContain = iLastPacketContain;
        else
            iEveryPacketContain = YXPacketContain;

        IEC10X_ASDU_Frame *pTemp_ASDUFrame;

        pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

        pTemp_ASDUFrame->ucType = IEC101_M_SP_NA_1;
        pTemp_ASDUFrame->ucRep_Num = iEveryPacketContain;

        /*
         * 继续完成控制信息位。
         */
        pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_INTROGEN;
        pTemp_ASDUFrame->ucCauseH = MasterAddr;

        pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
        pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

        /*
         * 以下为报文的具体内容。
         */
        if ((iPacketNum == 0) || ((iPacketNum > 0) && (iLastPacketContain == 0))) {
            for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
            }
        } else {
            if (iCycleNum == iPacketNum) {
                for (j = iPacketNum * YXPacketContain; j < iPacketNum * YXPacketContain + iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
                }
            } else {
                for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
                }
            }
        }
        iec10x_data_send(FORMAT_I, len + 6, OTHERS);
        len = 0;
    }
    return 1;
}

//不带时标的双点遥信，响应总召测，类型标识2
INT8U IEC10X_AP_DP_NA(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                      INT16U Addr,
                      INT16U MasterAddr,
                      INT8U YXDBuf[],
                      INT8U YXDBufLength,
                      INT16U YXDAddr[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL) {
    INT16U iPacketNum = YXDBufLength / YXDPacketContain;
    INT16U iLastPacketContain = YXDBufLength % YXDPacketContain;
    INT16U iCycleNum, iCycleNumber;
    INT16U j = 0, len = 0;

    if (YXDBufLength <= 0)
        return 0;

    if ((iPacketNum > 0) && (iLastPacketContain == 0))
        iCycleNumber = iPacketNum;
    else
        iCycleNumber = iPacketNum + 1;


    for (iCycleNum = 0; iCycleNum < iCycleNumber; iCycleNum++) {
        /*
         * 根据报文需要组织的信息点个数填写控制信息位。
         */
        INT16U iEveryPacketContain = 0;

        if (iCycleNum == iPacketNum)
            iEveryPacketContain = iLastPacketContain;
        else
            iEveryPacketContain = YXDPacketContain;

        IEC10X_ASDU_Frame *pTemp_ASDUFrame;

        pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

        pTemp_ASDUFrame->ucType = IEC101_M_DP_NA_1;
        pTemp_ASDUFrame->ucRep_Num = iEveryPacketContain;
        pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_INTROGEN;
        pTemp_ASDUFrame->ucCauseH = MasterAddr;

        pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
        pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);
        /*
         * 以下为报文的具体内容。
         */
        if ((iPacketNum == 0) || ((iPacketNum > 0) && (iLastPacketContain == 0))) {
            for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXDAddr[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXDAddr[j] >> 8) & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                pTemp_ASDUFrame->ucInfoUnit[len++] = YXDBuf[j];
            }
        } else {
            if (iCycleNum == iPacketNum) {
                for (j = iPacketNum * YXDPacketContain; j < iPacketNum * YXDPacketContain + iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXDAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXDAddr[j] >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXDBuf[j];
                }
            } else {
                for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXDAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXDAddr[j] >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXDBuf[j];
                }
            }
        }
        iec10x_data_send(FORMAT_I, len + 6, OTHERS);
        len = 0;
    }
    return 1;
}

//测量值，不带品质描述词的归一化值，响应总召测，类型标识21
INT8U IEC10X_AP_ME_ND(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                      INT16U Addr,
                      INT16U MasterAddr,
                      FP32 YcValue[],
                      INT16U YcNum,
                      INT16U YCAddr[],
                      FP32 YcUnitaryTime[],
                      FP32 YcLargerTime[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL) {
    INT16U i = 0, len = 0, num = 0;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pAutoSendYCBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_M_ME_ND_1;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    while (i < YcNum) {
        INT16U Value = (short)(YcValue[i] / YcLargerTime[i] / YcUnitaryTime[i] * 100);

        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[i] & 0xff);
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[i] >> 8) & 0xff);
        pTemp_ASDUFrame->ucInfoUnit[len++] = 0;
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
        num++;
        if (num >= YCPacketContain) {
            pTemp_ASDUFrame->ucRep_Num = YCPacketContain;
            iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
            len = 0;
            num = 0;
        }
        i++;
    }

    if ((num > 0) && (num < YCPacketContain)) {
        pTemp_ASDUFrame->ucRep_Num = num;
        iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
    }
    return 1;
}

//测量值，不带品质描述词的归一化值，主动上报，类型标识21
//参数：Addr  公共地址
INT8U IEC10X_YC_AUTO(INT16U Addr,
                     INT16U MasterAddr,
                     FP32 YcValue[],
                     INT16U YcNum,
                     INT16U YcObjectAddr[],
                     FP32 YcUnitaryTime[],
                     FP32 YcLargerTime[],
                     FP32 YcCriticalValue[],
                     INT8U iTranCauseL,
                     INT8U iCommonAddrL,
                     INT8U iInfoAddrL
                    ) {

    static float YcValueD[128];
    INT16U i = 0, len = 0, num = 0;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pAutoSendYCBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_M_ME_ND_1;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    while (i < YcNum) {
        if (fabs((double) YcValue[i] - (double) YcValueD[i]) > YcCriticalValue[i]) {
            INT16U Value = (short)(YcValue[i] / YcLargerTime[i] / YcUnitaryTime[i] * 100);

            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YcObjectAddr[i] & 0xff);
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YcObjectAddr[i] >> 8) & 0xff);
            pTemp_ASDUFrame->ucInfoUnit[len++] = 0;
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
            YcValueD[i] = YcValue[i];
            num++;
        }
        if (num >= YCPacketContain) {
            pTemp_ASDUFrame->ucRep_Num = YCPacketContain;
            iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
            len = 0;
            num = 0;
        }
        i++;
    }

    if ((num > 0) && (num < YCPacketContain)) {
        pTemp_ASDUFrame->ucRep_Num = num;
        iec10x_data_send(FORMAT_I, len + 6, AUTOYC);
    }
    return 1;
}

//不带时标的单点遥信，主动上报，类型标识1
INT8U iec10x_ap_sp_na_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[],
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              ) {
    INT16U len = 0;
    printf("iFlag == %d\n", iFlag);

    IEC10X_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pAutoSendYXBuf + 6);
    pTemp_ASDUFrame->ucType = (iFlag == 1) ? IEC101_M_SP_NA_1 : IEC101_M_DP_NA_1;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;

    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0x00ff)) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0xff00) >> 8) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = 0;

    if (iFlag == 1) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U) state[0];
    }
    if (iFlag == 2) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((state[0] << 1) + state[1]);
    }

    pTemp_ASDUFrame->ucRep_Num = 1;
    iec10x_data_send(FORMAT_I, len + 6, AUTOYX);

    return 1;
}

//单点遥信SOE，主动上报，类型标识30
INT8U iec10x_ap_sp_tb_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[0],
                               long long stime,
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              ) {
    INT16U len = 0;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pAutoSendYXBuf + 6);

    pTemp_ASDUFrame->ucType = (iFlag == 1) ? IEC101_M_SP_TB_1 : IEC101_M_DP_TB_1;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);


    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0x00ff)) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number) >> 8) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = 0;
    if (iFlag == 1) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U) state[0];
    }
    if (iFlag == 2) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((state[0] << 1) + state[1]);
    }

    struct tm p;
    struct timeval t;

    t.tv_sec = stime / 1000; t.tv_usec = stime % 1000;
    localtime_r(&(t.tv_sec), &p);
    INT16U MS = t.tv_usec + p.tm_sec * 1000;

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(MS & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((MS >> 8) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_min;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_hour;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mday;
    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mon + 1;
    pTemp_ASDUFrame->ucInfoUnit[len++] = (p.tm_year + 1900) % 100;

    pTemp_ASDUFrame->ucRep_Num = 1;
    iec10x_data_send(FORMAT_I, len + 6, AUTOYX);
    return 1;
}

/*
 * 用作，且仅用作遥控执行.
 */
int AddrHandle_YK2E_10x(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                        YKDATAIEC YK,
                        INT16U PointNo,
                        INT8U iCauseIndex,
                        INT8U iUnitIndex, INT8U iCheckLen, INT8U ucTemp_Length, SharedMemory * RtuDataAddr) {
    if (PointNo >= YKBase && PointNo < YKBase + YKMax) {
        int line = (PointNo - YKBase);
        int act = ((pRecv_ASDUFrame->ucInfoUnit[3] & 0x80) == 0x80);
        int how = ((pRecv_ASDUFrame->ucInfoUnit[3] & 0x01) == 0x01);
        return pIEC10X_Struct->yk(line, act, how);
    }
    return -1;
}

//遥控，双命令，类型标识46
void IEC10X_AP_YK2E(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                    INT8U ucTemp_Length,
                    INT16U Addr,
                    INT16U MasterAddr,
                    YKDATAIEC YK,
                    INT8U iTranCauseL,
                    INT8U iCommonAddrL,
                    INT8U iInfoAddrL,
                    SharedMemory * RtuDataAddr) {

    INT16U CommonAddr = (pRecv_ASDUFrame->ucCommonAddrH << 8) + pRecv_ASDUFrame->ucCommonAddrL;

    if ((CommonAddr != Addr)) {
        IEC10X_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    if (pRecv_ASDUFrame->ucCauseL != IEC101_CAUSE_ACT && pRecv_ASDUFrame->ucCauseL != IEC101_CAUSE_DEACT) {
        IEC10X_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_DC_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL =
        (pRecv_ASDUFrame->ucCauseL == IEC101_CAUSE_ACT) ? IEC101_CAUSE_ACTCON : IEC101_CAUSE_DEACTCON;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[0] = pRecv_ASDUFrame->ucInfoUnit[0];
    pTemp_ASDUFrame->ucInfoUnit[1] = pRecv_ASDUFrame->ucInfoUnit[1];
    pTemp_ASDUFrame->ucInfoUnit[2] = pRecv_ASDUFrame->ucInfoUnit[2];
    pTemp_ASDUFrame->ucInfoUnit[3] = pRecv_ASDUFrame->ucInfoUnit[3];

    INT16U PointNo = (pRecv_ASDUFrame->ucInfoUnit[1] << 8) + pRecv_ASDUFrame->ucInfoUnit[0];

    int res = AddrHandle_YK2E_10x(pRecv_ASDUFrame, YK, PointNo, 0, 3, 6, ucTemp_Length, RtuDataAddr);
    int outlen = 10;
    if (res == -2) { outlen += 9;}
    iec10x_ykpacketdata_send(IEC_FourI, outlen, IEC101_CAUSE_ACTCON, 0, 0, 0);
}

//总召唤确认
void IEC10X_AP_RQALL_CON(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL) {

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_IC_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_ACTCON;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[0] = 0;
    pTemp_ASDUFrame->ucInfoUnit[1] = 0;
    pTemp_ASDUFrame->ucInfoUnit[2] = 0;
    pTemp_ASDUFrame->ucInfoUnit[3] = IEC101_CAUSE_INTROGEN;
    iec10x_data_send(FORMAT_I, 10, OTHERS);
}

//总召唤结束，在遥信、遥测数据上报完之后发送
void IEC10X_AP_RQALL_END(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL) {

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_IC_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_ACTTERM;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[0] = 0x00;
    pTemp_ASDUFrame->ucInfoUnit[1] = 0x00;
    pTemp_ASDUFrame->ucInfoUnit[2] = 0x00;
    pTemp_ASDUFrame->ucInfoUnit[3] = IEC101_CAUSE_INTROGEN;

    iec10x_data_send(FORMAT_I, 10, OTHERS);
}

//总召唤，类型标识100
void IEC10X_AP_IC(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  INT8U YXBuf[],
                  INT8U YXBufLength,
                  INT8U YXDBuf[],
                  INT8U YXDBufLength,
                  FP32 YcValue[],
                  INT16U YcNum,
                  INT16U YXAddr[],
                  INT16U YXDAddr[],
                  INT16U YCAddr[],
                  FP32 YcUnitaryTime[],
                  FP32 YcLargerTime[],
                  INT8U iTranCauseL, INT8U iCommonAddrL, INT8U iInfoAddrL, SharedMemory * RtuDataAddr) {
    INT16U InfoAddr = (pRecv_ASDUFrame->ucInfoUnit[1] << 8) + pRecv_ASDUFrame->ucInfoUnit[0];

    INT16U CommonAddr = (pRecv_ASDUFrame->ucCommonAddrH << 8) + pRecv_ASDUFrame->ucCommonAddrL;

    if ((pRecv_ASDUFrame->ucCauseL != IEC101_CAUSE_ACT)) {
        printf("uncau\n");
        IEC10X_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    printf("Addr = %d, CommonAddr = %d\n", Addr, CommonAddr);
    if ((CommonAddr != Addr)) {
        printf("unaddr\n");
        IEC10X_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if ((InfoAddr != 0)) {
        printf("unmaddr\n");
        IEC10X_AP_UnknownInfoAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    //召唤可以分组，其中20召唤限定词为总召唤
    if (pRecv_ASDUFrame->ucInfoUnit[3] == 20) {
        IEC10X_AP_RQALL_CON(pRecv_ASDUFrame, Addr, MasterAddr, iTranCauseL, iCommonAddrL, iInfoAddrL);
        usleep(10000);
        IEC10X_AP_SP_NA(pRecv_ASDUFrame, Addr, MasterAddr, YXBuf, YXBufLength, YXAddr, iTranCauseL, iCommonAddrL,
                        iInfoAddrL);
        usleep(10000);
        IEC10X_AP_DP_NA(pRecv_ASDUFrame, Addr, MasterAddr, YXDBuf, YXDBufLength, YXDAddr, iTranCauseL, iCommonAddrL,
                        iInfoAddrL);
        usleep(10000);
        IEC10X_AP_ME_ND(pRecv_ASDUFrame, Addr, MasterAddr, YcValue, YcNum, YCAddr, YcUnitaryTime, YcLargerTime,
                        iTranCauseL, iCommonAddrL, iInfoAddrL);
        usleep(10000);
        IEC10X_AP_RQALL_END(pRecv_ASDUFrame, Addr, MasterAddr, iTranCauseL, iCommonAddrL, iInfoAddrL);
        return;
    } else
        return;
}

//对时，类型标识103
void IEC10X_AP_CS(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  struct timeval *settime,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr) {
    INT8U i;

    struct tm settimetm;
    struct timeval settimeval;
    INT16U usMSec;
    INT8U ucSec, ucMin, ucHour, ucDay, ucMonth, ucYear;
    INT8U *buf;
    INT16U CommonAddr = (pRecv_ASDUFrame->ucCommonAddrH << 8) + pRecv_ASDUFrame->ucCommonAddrL;
    INT16U InfoAddr = (pRecv_ASDUFrame->ucInfoUnit[1] << 8) + pRecv_ASDUFrame->ucInfoUnit[0];

    if ((pRecv_ASDUFrame->ucCauseL != IEC101_CAUSE_ACT)) {
        IEC10X_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if ((CommonAddr != Addr)) {
        IEC10X_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if ((InfoAddr != 0)) {
        IEC10X_AP_UnknownInfoAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    IEC10X_ASDU_Frame *pTemp_ASDUFrame;

    pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    buf = (INT8U *)(pRecv_ASDUFrame);
    usMSec = buf[10] * 256 + buf[9];
    ucSec = (INT8U)(usMSec / 1000);
    ucMin = buf[11] & 0x3f;
    ucHour = buf[12] & 0x1f;
    ucDay = buf[13] & 0x1f;
    ucMonth = buf[14] & 0x0f;
    ucYear = buf[15] & 0x7f;

    if (ucSec > 59 || ucMin > 59 || ucHour > 23 || ucYear > 99 || ucMonth > 12)
        return;

    if ((ucMonth == 0) || (ucDay == 0))
        return;

    if (ucMonth == 2) {
        if (((ucYear + 2000) % 4 == 0 && (ucYear + 2000) % 100 != 0) || (ucYear + 2000) % 400 == 0) {
            if (ucDay > 29)
                return;
        } else {
            if (ucDay > 28)
                return;
        }
    }

    if (ucMonth <= 7 && ucMonth % 2 == 0 && ucDay > 30)
        return;
    if (ucMonth >= 8 && ucMonth % 2 == 1 && ucDay > 30)
        return;

    settimetm.tm_sec = ucSec;
    settimetm.tm_min = ucMin;
    settimetm.tm_hour = ucHour;
    settimetm.tm_mday = ucDay;
    settimetm.tm_mon = ucMonth > 0 ? ucMonth - 1 : 0;
    settimetm.tm_year = ucYear + 100;

    settimeval.tv_sec = mktime(&settimetm);
    DDBG("%d\n%d\n", settimetm.tm_mday, settimetm.tm_mon);
    settimeval.tv_usec = usMSec * 1000;

    memcpy(settime, &settimeval, sizeof(settimeval));

    pIEC10X_Struct->yk(0, 99, 0);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_ACTCON;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);
    for (i = 0; i < 10; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }

    iec10x_data_send(FORMAT_I, 16, OTHERS);
    return;
}

//复位远方链路，类型标识105
void IEC10X_UP_DA(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr, INT32S ClearFlag, INT8U iTranCauseL, INT8U iCommonAddrL, INT8U iInfoAddrL) {
    //用于保存下一包序号和本次升级包的总数
    static unsigned short curIdx = 0, tolNum = 0;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);
    pTemp_ASDUFrame->ucType = IEC101_C_RP_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_ACTCON;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    //当前包的序号
    unsigned int index = pRecv_ASDUFrame->ucInfoUnit[0] + (pRecv_ASDUFrame->ucInfoUnit[1] * 256);
    unsigned int num = pRecv_ASDUFrame->ucInfoUnit[2] + (pRecv_ASDUFrame->ucInfoUnit[3] * 256);
    unsigned char state = 0x00;

    for (int i = 0; i < 4; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }

    printf("update index = %d, num = %d, ucTemp_Length = %d\n", index, num, ucTemp_Length);

    //升级启动
    if (index == 0xffff && num == 0xffff) {
        //printf("%s\n", (char *)&pRecv_ASDUFrame->ucInfoUnit[4]);
        unsigned char buf[128];
        memset(buf, 0, 128);
        int fd = open("fingerPrint.log", O_RDONLY);
        int size = read(fd, buf, 32);
        printf("%s\n", buf);
        if (!strcmp(buf, (char *)&pRecv_ASDUFrame->ucInfoUnit[4])) {
            //在这里填写想要执行的脚本
            system("ls");
        }
        return;
    }

    //收到启动帧
    if (index == 0) {
        //如果需要续传
        if (curIdx != 0) {
            pTemp_ASDUFrame->ucInfoUnit[0] = curIdx & 0xff;
            pTemp_ASDUFrame->ucInfoUnit[1] = (curIdx >> 8) & 0xff;

            //填入状态码
            pTemp_ASDUFrame->ucInfoUnit[4] = state & 0xff;

            iec10x_data_send(FORMAT_I, 5 + 6, OTHERS);
            printf(">>>>>continue\n");
            return;
        }

        //全新升级 1.记录本次升级总包数 2.计数器清零
        tolNum = num;
        curIdx = 0;

        //删除升级文件
        int res = unlink("/opt/update.tar");
        if (res == -1) {
            //文件删除失败
            state = 0x01;
        }
    }

    //不需要续传
    int fd = open("/opt/update.tar", O_WRONLY | O_CREAT | O_APPEND);
    if (fd == -1) {
        //文件创建失败
        state = 0x02;
    }
    int nw = write(fd, &pRecv_ASDUFrame->ucInfoUnit[4], ucTemp_Length - 10);
    if (nw != ucTemp_Length - 10) {
        //文件写入失败
        state = 0x03;
    }
    close(fd);

    curIdx ++;

    pTemp_ASDUFrame->ucInfoUnit[0] = curIdx & 0xff;
    pTemp_ASDUFrame->ucInfoUnit[1] = (curIdx >> 8) & 0xff;

    //最后一帧
    if (index == num) {
        curIdx = 0;
        tolNum = 0;
        system("rm fingerPrint.log; md5sum update.tar >> fingerPrint.log");
    }

    //填入状态码
    pTemp_ASDUFrame->ucInfoUnit[4] = state & 0xff;

    iec10x_data_send(FORMAT_I, 5 + 6, OTHERS);
}

//复位远方链路，类型标识105
void IEC10X_AP_RP(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr, INT32S ClearFlag, INT8U iTranCauseL, INT8U iCommonAddrL, INT8U iInfoAddrL) {
    INT8U i;

    INT16U CommonAddr = (pRecv_ASDUFrame->ucCommonAddrH << 8) + pRecv_ASDUFrame->ucCommonAddrL;
    INT16U InfoAddr = (pRecv_ASDUFrame->ucInfoUnit[1] << 8) + pRecv_ASDUFrame->ucInfoUnit[0];
    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    if ((CommonAddr != Addr)) {
        IEC10X_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if ((InfoAddr != 0)) {
        IEC10X_AP_UnknownInfoAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if (pRecv_ASDUFrame->ucCauseL != IEC101_CAUSE_ACT) {
        IEC10X_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    pTemp_ASDUFrame->ucType = IEC101_C_RP_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_ACTCON;
    pTemp_ASDUFrame->ucCauseH = MasterAddr;
    pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    for (i = 0; i < 4; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }

    if (pTemp_ASDUFrame->ucInfoUnit[3] == 1) {
        iec10x_data_send(FORMAT_I, 10, OTHERS);
        sleep(1);
        kill(getpid(), SIGKILL);
    }
}

//未知的类型标识
void IEC10X_AP_UnknownType(IEC10X_ASDU_Frame * pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNTYPE;
    pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
    pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
    pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

    for (i = 0; i < ucTemp_Length - 6; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }

    iec10x_data_send(FORMAT_I, ucTemp_Length, OTHERS);
}

//未知的传送原因
void IEC10X_AP_UnknownCause(IEC10X_ASDU_Frame * pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNCAUSE;
    pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
    pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
    pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

    for (i = 0; i < ucTemp_Length - 6; i++)
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    iec10x_data_send(FORMAT_I, ucTemp_Length, OTHERS);
}

//未知的公共地址
void IEC10X_AP_UnknownCommAddr(IEC10X_ASDU_Frame * pRecv_ASDUFrame, INT8U ucTemp_Length) {      //ASDU长度
    INT8U i;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNCOMMADDR;
    pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
    pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
    pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

    for (i = 0; i < ucTemp_Length - 6; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iec10x_data_send(FORMAT_I, ucTemp_Length, OTHERS);
}

//未知的信息对象地址
void IEC10X_AP_UnknownInfoAddr(IEC10X_ASDU_Frame * pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;

    IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *)(pIEC10X_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
    pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNINFOADDR;
    pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
    pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
    pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

    for (i = 0; i < ucTemp_Length - 6; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iec10x_data_send(FORMAT_I, ucTemp_Length, OTHERS);
}

INT8U Init_IEC10X_Buf() {
    INT16U usTemp_Step1;
    BufStruct *pTemp_BufList, *pTemp_BufList1;

    void * phead_IEC10XStruct = malloc(sizeof(IEC101Struct));
    if (phead_IEC10XStruct == NULL) {
        return 0;
    }

    pIEC10X_Struct = (IEC101Struct *) phead_IEC10XStruct;

    pIEC10X_Struct->pSendBuf = (INT8U *) malloc(IEC101BUFUPLENGTH);
    pIEC10X_Struct->pAutoSendYXBuf = (INT8U *) malloc(IEC101BUFUPLENGTH);
    pIEC10X_Struct->pAutoSendYCBuf = (INT8U *) malloc(IEC101BUFUPLENGTH);
    return 1;
}

void iec10x_destroy() {
    free(pIEC10X_Struct->pSendBuf);
    free(pIEC10X_Struct->pAutoSendYXBuf);
    free(pIEC10X_Struct->pAutoSendYCBuf);
    free(pIEC10X_Struct);
}

//提供的初始化函数接口
INT8U iec10x_create(INITIEC101VAULE * initiec101Value,
                    unsigned int (*send_callback)(INT8U * pBuffer, INT16U iLen),
                    int (*yk_callback)(int line, int act, int how)) {
    INT8U ucRet;

    ucRet = Init_IEC10X_Buf();
    if (0 == ucRet) {
        return 0;
    }
    pIEC10X_Struct->ucYK_Limit_Time = initiec101Value->ucYK_Limit_Time;
    pIEC10X_Struct->ucTimeOut_t0 = initiec101Value->ucTimeOut_t0;
    pIEC10X_Struct->ucTimeOut_t1 = initiec101Value->ucTimeOut_t1;
    pIEC10X_Struct->ucTimeOut_t2 = initiec101Value->ucTimeOut_t2;
    pIEC10X_Struct->ucTimeOut_t3 = initiec101Value->ucTimeOut_t3;
    pIEC10X_Struct->ucMax_k = initiec101Value->ucMax_k;
    pIEC10X_Struct->ucMax_w = initiec101Value->ucMax_w;
    IEC10X_InitAllFlag();

    pIEC10X_Struct->send = send_callback;
    pIEC10X_Struct->yk = yk_callback;

    return 1;
}

//清空计时器
void iec10x_init_timer() {
    pIEC10X_Struct->ucWaitServConCount_Flag = FALSE;    //关闭t1计时器
    pIEC10X_Struct->ucWaitServConCount = 0;     //t1计时器清零
    pIEC10X_Struct->ucNoRecvT3 = 0;     //t3计时器清零
}

//101规约处理入口函数
int iec10x_verify_handle(INT16U Addr,
                         INT16U MasterAddr,
                         INT8U * DealBuf, INT16U BufLength, YKDATAIEC YK,
                         INT8U YXBuf[],
                         INT8U YXBufLength,
                         INT8U YXDBuf[],
                         INT8U YXDBufLength,
                         FP32 YcValue[],
                         INT16U YcNum,
                         INT16U YXAddr[],
                         INT16U YXDAddr[],
                         INT16U YCAddr[],
                         FP32 YcUnitaryTime[],
                         FP32 YcLargerTime[],
                         struct timeval *settime,
                         INT32S ClearFlag,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL,
                         SharedMemory * RtuDataAddr) {
    INT8U ucTemp_ReturnFlag = iec10x_verify(DealBuf, BufLength);
    //printf("ucTemp_ReturnFlag = %d\n", ucTemp_ReturnFlag);

    if ((ucTemp_ReturnFlag == IEC101FRAME_I) || (ucTemp_ReturnFlag == IEC101FRAME_U)) {
        return iec10x_handle(DealBuf, BufLength, Addr, MasterAddr, YK, YXBuf, YXBufLength, YXDBuf, YXDBufLength,
                             YcValue, YcNum, YXAddr, YXDAddr, YCAddr, YcUnitaryTime, YcLargerTime, settime, ClearFlag,
                             ucTemp_ReturnFlag, iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
    }
    if (ucTemp_ReturnFlag == IECFRAMENEEDACK) {
        iec10x_data_send(FORMAT_S, 0, OTHERS);
        return 0;
    }
    if (ucTemp_ReturnFlag == IEC101FRAMEERR) {
        DDBG("iec10x_verify：报文格式错误");
        return -1;
    }
}

//判断是否为有效报文
INT8U iec10x_verify(INT8U * pTemp_Buf, INT16U usTemp_Length) {
    INT16U usTemp_ServSendNum;  //主站端的发送序列号

    INT16U usTemp_ServRecvNum;  //主站端的接收序列号

    INT8U ucTemp_Length;

    char logpkg[] = { 0x68, 0x04, 0x07, 0x00, 0x00, 0x00 };

    if (*pTemp_Buf != 0x68) {
        return IEC101FRAMEERR;
    }
    if (memcmp(logpkg, pTemp_Buf, 6) == 0) {
        return IEC101FRAME_U;
    }
    if ((usTemp_Length > 255) || (usTemp_Length < 6)) {
        return IEC101FRAMEERR;
    }

    pTemp_Buf++;
    ucTemp_Length = usTemp_Length & 0xff;
    if (*pTemp_Buf != (ucTemp_Length - 2))
        return IEC101FRAMEERR;

    pTemp_Buf++;
    usTemp_ServSendNum = ChangeShort(pTemp_Buf);

    pTemp_Buf += 2;
    usTemp_ServRecvNum = ChangeShort(pTemp_Buf);
    //FORMAT_I
    //printf("usTemp_ServRecvNum = %d\n", usTemp_ServRecvNum);
    if (!(usTemp_ServSendNum & 0x0001)) {
        if (ucTemp_Length < 12) {
            return IEC101FRAMEERR;
        }

        usTemp_ServRecvNum >>= 1;
        pIEC10X_Struct->usServRecvNum = usTemp_ServRecvNum;

        //FTU发送计数发生翻转
        if (pIEC10X_Struct->ucSendCountOverturn_Flag) {
            //主站接收计数发生翻转，因为ucMax_w=8，所以主站必须在8帧以内回应从站，主站的接收序号不会超过8
            if (usTemp_ServRecvNum >= 0x0000 && usTemp_ServRecvNum < 0x08) {
                pIEC10X_Struct->ucSendCountOverturn_Flag = FALSE;
                pIEC10X_Struct->k = pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;
            } else
                //主站接收计数未发生翻转
                pIEC10X_Struct->k = 0x8000 + pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;
        } else
            //FTU发送计数未发生翻转
        {
            if (usTemp_ServRecvNum <= pIEC10X_Struct->usSendNum) {
                pIEC10X_Struct->k = pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;
            }
        }

        usTemp_ServSendNum >>= 1;
        pIEC10X_Struct->usServSendNum = usTemp_ServSendNum;
        //本端接收序号 != 对端发送序号
        if (pIEC10X_Struct->usRecvNum != usTemp_ServSendNum) {
            //printf("IEC101FRAMESEQERR\n");
        } else {
            if (usTemp_ServSendNum == 0x7fff) {
                pIEC10X_Struct->usRecvNum = 0;
            } else
                pIEC10X_Struct->usRecvNum++;

            pIEC10X_Struct->w++;
            if (pIEC10X_Struct->w >= pIEC10X_Struct->ucMax_w) {
                return IECFRAMENEEDACK;
            }
        }
        return IEC101FRAME_I;
    }
    //FORMAT_S
    else if (!(usTemp_ServSendNum & 0x0002)) {
        if (ucTemp_Length != 6)
            return IEC101FRAMEERR;
        usTemp_ServRecvNum >>= 1;

        if (pIEC10X_Struct->ucSendCountOverturn_Flag) {
            if (usTemp_ServRecvNum >= 0x0000 && usTemp_ServRecvNum < 0x08) {
                pIEC10X_Struct->ucSendCountOverturn_Flag = FALSE;
                pIEC10X_Struct->k = pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;
            } else
                pIEC10X_Struct->k = 0x8000 + pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;

        } else {
            if (usTemp_ServRecvNum <= pIEC10X_Struct->usSendNum) {
                pIEC10X_Struct->k = pIEC10X_Struct->usSendNum - usTemp_ServRecvNum;
            }
        }
        return IECFRAMENEEDACK;
    } else {
        return IEC101FRAME_U;
    }
}

//根据主站命令，执行不同操作
int iec10x_handle(INT8U * pTemp_Buf,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  YKDATAIEC YK,
                  INT8U YXBuf[],
                  INT8U YXBufLength,
                  INT8U YXDBuf[],
                  INT8U YXDBufLength,
                  FP32 YcValue[],
                  INT16U YcNum,
                  INT16U YXAddr[],
                  INT16U YXDAddr[],
                  INT16U YCAddr[],
                  FP32 YcUnitaryTime[],
                  FP32 YcLargerTime[],
                  struct timeval *settime,
                  INT32S ClearFlag,
                  INT8U iPacketKind,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr) {

    INT8U *pTemp_Pointer = pTemp_Buf + 2;
    INT16U usTemp_ServSendNum = ChangeShort(pTemp_Pointer);
    pTemp_Pointer += 2;
    INT16U usTemp_ServRecvNum = ChangeShort(pTemp_Pointer);
    pTemp_Pointer += 2;
    //printf("usTemp_ServSendNum = %d\n", usTemp_ServSendNum);

    if (!(usTemp_ServSendNum & 0x0001)) {
        IEC10X_ASDU_Frame *pTemp_ASDUFrame = (IEC10X_ASDU_Frame *) pTemp_Pointer;

        INT8U ucTemp_Type = pTemp_ASDUFrame->ucType;

        printf("[DBG]ucTemp_Type = %d\n", ucTemp_Type);
        switch (ucTemp_Type) {
        case IEC101_C_DC_NA_1: //遥控，双命令
            IEC10X_AP_YK2E(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, YK, iTranCauseL, iCommonAddrL,
                           iInfoAddrL, RtuDataAddr);
            break;
        case IEC101_C_IC_NA_1: //总召唤命令
            IEC10X_AP_IC(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, YXBuf, YXBufLength, YXDBuf,
                         YXDBufLength, YcValue, YcNum, YXAddr, YXDAddr, YCAddr, YcUnitaryTime, YcLargerTime,
                         iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
            break;
        case IEC101_C_CS_NA_1: //时钟同步命令
            IEC10X_AP_CS(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, settime, iTranCauseL, iCommonAddrL,
                         iInfoAddrL, RtuDataAddr);
            break;
        case IEC101_C_RP_NA_1: //复位进程命令
            IEC10X_AP_RP(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, ClearFlag, iTranCauseL, iCommonAddrL,
                         iInfoAddrL);
            break;
        case 0xff: //系统升级
            IEC10X_UP_DA(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, ClearFlag, iTranCauseL, iCommonAddrL,
                         iInfoAddrL);
            break;
        default:
            IEC10X_AP_UnknownType(pTemp_ASDUFrame, ucTemp_Length - 6);
            break;
        }
        return 1;
    } else if (!(usTemp_ServSendNum & 0x0002)) {
        return 1;
    } else {
        //登陆报文
        if (usTemp_ServSendNum & 0x04) {
            iec10x_data_send(FORMAT_U, 0x0b, OTHERS);
            pIEC10X_Struct->ucDataSend_Flag = TRUE;
            return 10;
        }
        //退出报文
        if (usTemp_ServSendNum & 0x10) {
            iec10x_data_send(FORMAT_U, 0x23, OTHERS);
            pIEC10X_Struct->ucDataSend_Flag = FALSE;
            return 11;
        }
        //心跳报文
        if (usTemp_ServSendNum & 0x40) {
            if (pIEC10X_Struct->ucDataSend_Flag == TRUE)
                iec10x_data_send(FORMAT_U, 0x83, OTHERS);
            else
                IEC10X_InitAllFlag();
            return 12;
        }
    }
}

//初始化101规约里面的状态标志
void IEC10X_InitAllFlag() {

    pIEC10X_Struct->ucDataSend_Flag = FALSE;
    pIEC10X_Struct->ucYK_TimeCount_Flag = FALSE;
    pIEC10X_Struct->ucYK_TimeCount = 0x00;

    pIEC10X_Struct->ucSendCountOverturn_Flag = FALSE;

    pIEC10X_Struct->usRecvNum = 0;
    pIEC10X_Struct->usSendNum = 0;

    pIEC10X_Struct->usServRecvNum = 0;
    pIEC10X_Struct->usServSendNum = 0;

    pIEC10X_Struct->ucIdleCount_Flag = FALSE;
    pIEC10X_Struct->ucIdleCount = 0;

    pIEC10X_Struct->ucNoRecvCount = 0;
    pIEC10X_Struct->ucNoRecvT3 = 0;

    pIEC10X_Struct->ucWaitServConCount_Flag = FALSE;
    pIEC10X_Struct->ucWaitServConCount = 0;

    pIEC10X_Struct->w = 0;
    pIEC10X_Struct->k = 0;
}

//发送报文
void iec10x_data_send(INT8U ucTemp_FrameFormat, INT8U ucTemp_Value, ENUM sk) {
    INT8U *pTemp_SendBuf;
    INT16U usTemp_Num;
    int outlen = 0;

    //决定使用哪个发送buf
    switch (sk) {
    case AUTOYX:
        pTemp_SendBuf = pIEC10X_Struct->pAutoSendYXBuf;
        break;
    case AUTOYC:
        pTemp_SendBuf = pIEC10X_Struct->pAutoSendYCBuf;
        break;
    case OTHERS:
    default:
        pTemp_SendBuf = pIEC10X_Struct->pSendBuf;
        break;
    }

    switch (ucTemp_FrameFormat) {
    case FORMAT_I:
        if (pIEC10X_Struct->ucDataSend_Flag) {
            //报文头发送、接受序列
            *pTemp_SendBuf++ = 0x68;
            *pTemp_SendBuf++ = ucTemp_Value + 0x04;
            usTemp_Num = pIEC10X_Struct->usSendNum << 1;
            *pTemp_SendBuf++ = usTemp_Num & 0xff;
            *pTemp_SendBuf++ = usTemp_Num >> 8;

            usTemp_Num = pIEC10X_Struct->usRecvNum << 1;
            *pTemp_SendBuf++ = usTemp_Num & 0xff;
            *pTemp_SendBuf++ = usTemp_Num >> 8;

            //内容+头6帧=总的发送长度
            outlen = (ucTemp_Value + 6);

            pTemp_SendBuf -= 6;
            pIEC10X_Struct->send((INT8U *) pTemp_SendBuf, outlen);

            //判断发送序列是否需要反转
            if (pIEC10X_Struct->usSendNum == 0x7fff) {
                pIEC10X_Struct->usSendNum = 0;
                pIEC10X_Struct->ucSendCountOverturn_Flag = TRUE;
            } else
                pIEC10X_Struct->usSendNum++;

            pIEC10X_Struct->k++;
            pIEC10X_Struct->w = 0;

        }
        break;

    case FORMAT_S:
        //S帧不需要应用层处理
        break;

    case FORMAT_U:
        printf("3\n");
        *pTemp_SendBuf++ = 0x68;
        *pTemp_SendBuf++ = 0x04;
        *pTemp_SendBuf++ = ucTemp_Value;
        *pTemp_SendBuf++ = 0x00;
        *pTemp_SendBuf++ = 0x00;
        *pTemp_SendBuf++ = 0x00;

        outlen = 6;
        pTemp_SendBuf -= 6;

        pIEC10X_Struct->send((INT8U *) pTemp_SendBuf, 6);
        break;
    }
    Print(pTemp_SendBuf, outlen, 1);
}

//发送报文
void iec10x_ykpacketdata_send(INT8U ucIEC_FrameKind,
                              INT8U ucTemp_Value,
                              INT8U ucActValue, INT8U iTranCauseL, INT8U iCommonAddrL, INT8U iInfoAddrL) {
    INT16U usTemp_Num;

    INT8U *pTemp_SendBuf = pIEC10X_Struct->pSendBuf;
    printf("iec10x_ykpacketdata_send\n");
    switch (ucIEC_FrameKind) {
    case IEC_FourI:
        if (pIEC10X_Struct->ucDataSend_Flag) {
            *pTemp_SendBuf++ = 0x68;
            *pTemp_SendBuf++ = ucTemp_Value + 0x04;
            usTemp_Num = pIEC10X_Struct->usSendNum << 1;
            *pTemp_SendBuf++ = usTemp_Num & 0xff;
            *pTemp_SendBuf++ = usTemp_Num >> 8;

            usTemp_Num = pIEC10X_Struct->usRecvNum << 1;
            *pTemp_SendBuf++ = usTemp_Num & 0xff;
            *pTemp_SendBuf = usTemp_Num >> 8;

            int outlen = (ucTemp_Value + 6);

            pTemp_SendBuf -= 5;

            if (0 != ucActValue)
                *(pTemp_SendBuf + 8) = ucActValue;

            pIEC10X_Struct->send((INT8U *) pTemp_SendBuf, outlen);
            Print(pTemp_SendBuf, outlen, 1);

            if (pIEC10X_Struct->usSendNum == 0x7fff) {
                pIEC10X_Struct->usSendNum = 0;
                pIEC10X_Struct->ucSendCountOverturn_Flag = TRUE;
            } else
                pIEC10X_Struct->usSendNum++;

            pIEC10X_Struct->k++;
            pIEC10X_Struct->w = 0;
        }
        break;
    }
}
