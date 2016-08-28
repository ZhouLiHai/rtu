/*
 * libiec101.c
 *
 *  Created on: 2014-01-08
 *  Author:
 */

#include "libiec101.h"

INT8U iLinkControl;
INT8U iLinkAddress;

INT8U iecframecheckcode(INT8U *pTemp_Buf, INT16U usTemp_Length, char cPacketKind) {
    INT8U iSum = 0;
    INT8U iucCheckCode = 0;
    INT8U *pBuf;

    switch (cPacketKind) {
    case 68:
        pBuf = pTemp_Buf + 4;
        int i;
        for (i = 0; i < usTemp_Length - 6; i++) {
            printf("%02x ", *pBuf);
            iSum += *pBuf;
            pBuf++;
        }
        *pBuf = 0x16;
        iucCheckCode = iSum;
        printf(">>%02x\n", iSum);
        break;

    case 10:
        pBuf = pTemp_Buf;
        iSum = *(pTemp_Buf + 1) + *(pTemp_Buf + 2);
        iucCheckCode = (iSum % 256) & 0xff;
        break;

    case 18://特殊计算校验码
        pBuf = pTemp_Buf + 4;
        iSum = iSum + iLinkControl;
        iSum = iSum + iLinkAddress;
        int j ;
        for (j = 0; j < usTemp_Length - 6; j++) {
            iSum += *pBuf;
            pBuf++;
        }
        iucCheckCode = (iSum % 256) & 0xff;
        break;

    default:
        printf("PacketKind Error!\n");
        break;
    }
    return iucCheckCode;
}

//新版规约函数重新定义标志位位@@@@@@
//不带时标的单点遥信，响应总召测，类型标识1
INT8U IEC101_AP_SP_NA(IEC101_ASDU_Frame *pRecv_ASDUFrame,   //指向ASDU信息单位
                      INT16U Addr,                                      //公共地址
                      INT16U MasterAddr,                                    //主站地址
                      INT8U YXBuf[],                                        //1点遥信YX数据缓冲区,共160路YX
                      INT8U YXBufLength,                                    //1点遥信路长度
                      INT16U YXAddr[], INT8U iTranCauseL, INT8U iCommonAddrL, INT8U iInfoAddrL) {
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
     * 一下开始组织报文，柑橘具体情况会发送不定数量个报文。
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

        IEC101_ASDU_Frame *pTemp_ASDUFrame;
        pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

        pTemp_ASDUFrame->ucType = IEC101_M_SP_NA_1;
        pTemp_ASDUFrame->ucRep_Num = iEveryPacketContain;

        /*
         * 继续完成控制信息位。
         */

        pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_INTROGEN;
        if (iTranCauseL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
        if (iCommonAddrL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);

        /*
         * 以下为报文的具体内容。
         */
        if ((iPacketNum == 0) || ((iPacketNum > 0) && (iLastPacketContain == 0))) {
            for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);

                if (iInfoAddrL == 3)
                {pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}

                pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
            }
        } else {
            if (iCycleNum == iPacketNum) {
                for (j = iPacketNum * YXPacketContain; j < iPacketNum * YXPacketContain + iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);
                    if (iInfoAddrL == 3)
                    {pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}

                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
                }
            } else {
                for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YXAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YXAddr[j] >> 8) & 0xff);

                    if (iInfoAddrL == 3)
                    {pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}

                    pTemp_ASDUFrame->ucInfoUnit[len++] = YXBuf[j];
                }
            }
        }

        iLinkControl = 0x28;

        iec101_data_send(FORMAT_68, len + 4, OTHERS);

        len = 0;
    }
    return 1;
}

//测量值，不带品质描述词 不带时标 的归一化值，响应总召测，类型标识21
//参数：Addr  公共地址
INT8U IEC101_AP_ME_ND(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT16U Addr,
                      INT16U MasterAddr,
                      FP32 YcValue[],
                      INT16U YcNum,
                      INT16U YCAddr[],
                      FP32 YcUnitaryTime[],
                      FP32 YcLargerTime[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL) {
    INT16U iPacketNum = YcNum / YCPacketContain;
    INT16U iLastPacketContain = YcNum % YCPacketContain;
    INT16U iCycleNum, iCycleNumber;
    INT16U j = 0, len = 0;
    INT16U Value[YcNum];

    if (YcNum <= 0)
        return 0;

    if ((iPacketNum > 0) && (iLastPacketContain == 0))
        iCycleNumber = iPacketNum;
    else
        iCycleNumber = iPacketNum + 1;

    for (iCycleNum = 0; iCycleNum < iCycleNumber; iCycleNum++) {
        INT16U iEveryPacketContain = 0;
        if (iCycleNum == iPacketNum)
            iEveryPacketContain = iLastPacketContain;
        else
            iEveryPacketContain = YCPacketContain;

        IEC101_ASDU_Frame *pTemp_ASDUFrame;
        pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);
        pTemp_ASDUFrame->ucType = IEC101_M_ME_ND_1;
        pTemp_ASDUFrame->ucRep_Num = iEveryPacketContain;

        pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_INTROGEN;
        if (iTranCauseL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
        if (iCommonAddrL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);


        if ((iPacketNum == 0) || ((iPacketNum > 0) && (iLastPacketContain == 0))) {
            for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                float temp = YcValue[j] / YcLargerTime[j] / YcUnitaryTime[j];
                Value[j] = (unsigned short)(temp * 100);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);
                if (iInfoAddrL == 3)
                {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] >> 8 & 0xff);

            }
        } else {
            if (iCycleNum == iPacketNum) {
                for (j = iPacketNum * YCPacketContain; j < iPacketNum * YCPacketContain + iEveryPacketContain; j++) {
                    float temp = YcValue[j] / YcLargerTime[j] / YcUnitaryTime[j];
                    Value[j] = (unsigned short)(temp * 100);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);

                    if (iInfoAddrL == 3)
                    {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] >> 8 & 0xff);

                }
            } else {
                for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                    float temp = YcValue[j] / YcLargerTime[j] / YcUnitaryTime[j];
                    Value[j] = (unsigned short)(temp * 100);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);

                    if (iInfoAddrL == 3)
                    {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value[j] >> 8 & 0xff);
                }
            }
        }

        iLinkControl = 0x28;
        iec101_data_send(FORMAT_68, len + 4, OTHERS);

        len = 0;
    }
    return 1;
}

//测量值，带时标 带品质描述词的标度化值
INT8U IEC101_AP_ME_TE(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT16U Addr,
                      INT16U MasterAddr,
                      FP32 YcValue[],
                      INT16U YcNum,
                      INT16U YCAddr[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL,
                      SharedMemory * RtuDataAddr) {
    INT16U iPacketNum = YcNum / YCTimePacketContain;
    INT16U iLastPacketContain = YcNum % YCTimePacketContain;
    INT16U iCycleNum, iCycleNumber;
    INT16U j = 0, len = 0;
    INT16U Value = 0;
    struct tm p;
    INT16U MS;

    if (YcNum <= 0)
        return 0;

    if ((iPacketNum > 0) && (iLastPacketContain == 0))
        iCycleNumber = iPacketNum;
    else
        iCycleNumber = iPacketNum + 1;

    for (iCycleNum = 0; iCycleNum < iCycleNumber; iCycleNum++) {
        INT16U iEveryPacketContain = 0;
        if (iCycleNum == iPacketNum)
            iEveryPacketContain = iLastPacketContain;
        else
            iEveryPacketContain = YCTimePacketContain;

        IEC101_ASDU_Frame *pTemp_ASDUFrame;
        pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);
        pTemp_ASDUFrame->ucType = IEC101_M_ME_TE_1;
        pTemp_ASDUFrame->ucRep_Num = iEveryPacketContain;

        pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_INTROGEN;
        if (iTranCauseL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
        if (iCommonAddrL == 2)
            pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);


        if ((iPacketNum == 0) || ((iPacketNum > 0) && (iLastPacketContain == 0))) {
            for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                Value = (unsigned short)(YcValue[j] * 100);

                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);
                if (iInfoAddrL == 3)
                {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = 0; //品质描述词暂定为0

                struct timeval t;
                t.tv_sec = 1 / 1000;
                t.tv_usec = 1 % 1000;
                localtime_r(&(t.tv_sec), &p);
                MS = t.tv_usec + p.tm_sec * 1000;
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(MS & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((MS >> 8) & 0xff);
                pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_min;
                pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_hour;
                pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mday;
                pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mon + 1;
                pTemp_ASDUFrame->ucInfoUnit[len++] = (p.tm_year + 1900) % 100;

            }
        } else {
            if (iCycleNum == iPacketNum) {
                for (j = iPacketNum * YCTimePacketContain; j < iPacketNum * YCTimePacketContain + iEveryPacketContain; j++) {
                    Value = (unsigned short)(YcValue[j] * 100);

                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);

                    if (iInfoAddrL == 3)
                    {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0; //品质描述词暂定为0

                    struct timeval t;
                    t.tv_sec = 1 / 1000;
                    t.tv_usec = 1 % 1000;
                    localtime_r(&(t.tv_sec), &p);
                    MS = t.tv_usec + p.tm_sec * 1000;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(MS & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((MS >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_min;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_hour;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mday;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mon + 1;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (p.tm_year + 1900) % 100;

                }
            } else {
                for (j = iCycleNum * iEveryPacketContain; j < (iCycleNum + 1) * iEveryPacketContain; j++) {
                    Value = (unsigned short)(YcValue[j] * 100);

                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(YCAddr[j] & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((YCAddr[j] >> 8) & 0xff);

                    if (iInfoAddrL == 3)
                    {   pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;}
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Value >> 8 & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = 0; //品质描述词暂定为0

                    struct timeval t;
                    t.tv_sec = 1 / 1000;
                    t.tv_usec = 1 % 1000;
                    localtime_r(&(t.tv_sec), &p);
                    MS = t.tv_usec + p.tm_sec * 1000;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(MS & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((MS >> 8) & 0xff);
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_min;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_hour;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mday;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = p.tm_mon + 1;
                    pTemp_ASDUFrame->ucInfoUnit[len++] = (p.tm_year + 1900) % 100;

                }
            }
        }

        iLinkControl = 0x28;
        iec101_data_send(FORMAT_68, len + 4, OTHERS);

        len = 0;
    }
    return 1;
}

//单点遥信SOE，主动上报，类型标识30
INT8U iec101_ap_sp_tb_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[],
                               long long stime,
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              ) {
    printf("[SOE]iFlag == %d state = %d\n", iFlag, state[0]);
    INT16U len = 0;

    IEC101_ASDU_Frame *pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pAutoSendYXBuf + 6);
    pTemp_ASDUFrame->ucType = (iFlag == 1) ? IEC101_M_SP_TB_1 : 0x04;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_SPONT;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);
    // pTemp_ASDUFrame->ucCauseL = IEC101_CAUSE_SPONT;
    // pTemp_ASDUFrame->ucCauseH = MasterAddr;
    // pTemp_ASDUFrame->ucCommonAddrL = (INT8U)(Addr & 0xff);
    // pTemp_ASDUFrame->ucCommonAddrH = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0x00ff)) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number) >> 8) & 0xff);
    if (iInfoAddrL == 3)
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
    iLinkControl = 0x08;
    iec101_data_send(FORMAT_68, len + 2 , AUTOYX);

    return 1;
}

//不带时标的单点遥信，主动上报，类型标识1
INT8U iec101_ap_sp_na_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[],
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL                           //信息地址长度
                              ) {
    INT16U len = 0;
    printf("iFlag == %d state = %d\n", iFlag, state[0]);

    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pAutoSendYXBuf + 6);
    pTemp_ASDUFrame->ucType = (iFlag == 1) ? IEC101_M_SP_NA_1 : IEC101_M_DP_NA_1;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_SPONT;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0x00ff)) & 0xff);
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(((number & 0xff00) >> 8) & 0xff);
    if (iInfoAddrL == 3)
        pTemp_ASDUFrame->ucInfoUnit[len++] = 0;

    if (iFlag == 1) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U) state[0];
    }
    if (iFlag == 2) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((state[0] << 1) + state[1]);
    }

    pTemp_ASDUFrame->ucRep_Num = 1;

    iLinkControl = 0x28;
    for (int i = 0; i < len + 8; i++) {
        printf("[%02x]\n", pIEC101_Struct->pAutoSendYXBuf[i]);
    }
    iec101_data_send(FORMAT_68, len + 2 , YXBW);
}

/*
 * 用作，且仅用作遥控执行.
 */
void AddrHandle_YK2E(IEC101_ASDU_Frame *pRecv_ASDUFrame,
                     INT16U PointNo,
                     INT8U InfoAddrIndex,
                     INT8U InfoValueIndex) {
    printf("PointNo %d pRecv_ASDUFrame->ucInfoUnit[3] %d\n", PointNo, pRecv_ASDUFrame->ucInfoUnit[3]);
    if (PointNo >= YKBase && PointNo < YKBase + YKMax) {
        int line = (PointNo - YKBase);
        int act = ((pRecv_ASDUFrame->ucInfoUnit[4] & 0x80) == 0x80);
        int how = ((pRecv_ASDUFrame->ucInfoUnit[4] & 0x01) == 0x01);
        printf("line = %d,act = %d,how = %d\n", line, act, how);
        pIEC101_Struct->yk(line, act, how);
    }
    //iec_ykpacketdata_add(1, InfoValueIndex);
}

//遥控，双命令，类型标识46
void IEC101_AP_YK2E(IEC101_ASDU_Frame * pRecv_ASDUFrame,
                    INT8U ucTemp_Length,
                    INT16U Addr,
                    INT16U MasterAddr,
                    INT8U iTranCauseL,
                    INT8U iCommonAddrL,
                    INT8U iInfoAddrL,
                    SharedMemory * RtuDataAddr) {
    INT8U infoAddrIndex = 0;
    INT8U infoValueIndex = 0;
    INT8U len = 0;
    INT16U PointNo = 0;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_DC_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_ACTCON;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr >> 8 & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    infoAddrIndex = len;
    len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    infoValueIndex = len;
    len++;
    if (iInfoAddrL == 3) {
        pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
        infoValueIndex = len;
    }

    PointNo = (pRecv_ASDUFrame->ucInfoUnit[infoAddrIndex + 1] << 8)
              + pRecv_ASDUFrame->ucInfoUnit[infoAddrIndex];
    //发送遥控命令
    AddrHandle_YK2E(pRecv_ASDUFrame, PointNo, infoAddrIndex, infoValueIndex);
    iec101_data_send(FORMAT_68, len + 2, OTHERS);
}


//总召唤确认
void IEC101_AP_RQALL_CON(IEC101_ASDU_Frame *pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL) {
    printf("IEC101_AP_RQALL_CON %d %d %d \n", iTranCauseL, iCommonAddrL, iInfoAddrL);
    int len = 0;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_IC_NA_1; len++;
    pTemp_ASDUFrame->ucRep_Num = 0x01; len++;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_ACTCON;
    if (iTranCauseL == 2) {
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;
    }

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2) {
        pTemp_ASDUFrame->ucInfoUnit[len++]  = (INT8U)((Addr >> 8) & 0xff);
    }

    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len]; len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len]; len++;
    if (iInfoAddrL == 3) {
        pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len]; len++;
    }
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len]; len++;

    iLinkControl = 0x28;
    iec101_data_send(FORMAT_68, len, OTHERS);

}

//总召唤结束，在遥信、遥测数据上报完之后发送
void IEC101_AP_RQALL_END(IEC101_ASDU_Frame *pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL) {

    INT8U len = 0;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_IC_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_ACTTERM;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
    pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
    if (iInfoAddrL == 3)
        pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_INTROGEN;

    iLinkControl = 0x08;

    iec101_data_send(FORMAT_68, len + 4, OTHERS);

}

//总召唤，类型标识100
void IEC101_AP_IC(IEC101_ASDU_Frame *pRecv_ASDUFrame,
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
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr) {
    printf("IEC101_AP_IC\n");

    IEC101_AP_RQALL_CON(pRecv_ASDUFrame, Addr, MasterAddr, iTranCauseL, iCommonAddrL, iInfoAddrL);
    IEC101_AP_SP_NA(pRecv_ASDUFrame, Addr, MasterAddr, YXBuf, YXBufLength, YXAddr, iTranCauseL, iCommonAddrL, iInfoAddrL);
    IEC101_AP_ME_ND(pRecv_ASDUFrame, Addr, MasterAddr, YcValue, YcNum, YCAddr, YcUnitaryTime, YcLargerTime, iTranCauseL, iCommonAddrL, iInfoAddrL);
    // IEC101_AP_ME_TE(pRecv_ASDUFrame, Addr, MasterAddr, YcValue, YcNum, YCAddr, iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
    IEC101_AP_RQALL_END(pRecv_ASDUFrame, Addr, MasterAddr, iTranCauseL, iCommonAddrL, iInfoAddrL);
    iec101_data_send(FORMAT_10, 0x20, OTHERS);
}

//对时，类型标识103
void IEC101_AP_CS(IEC101_ASDU_Frame *pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  struct timeval settime,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr) {

    struct tm settimetm;

    INT16U usMSec;
    INT8U ucSec, ucMin, ucHour, ucDay, ucMonth, ucYear;

    INT8U len = 0;
    INT16U CommonAddr = 0;
    INT16U InfoAddr = 0;

    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = 0x01;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_ACTCON;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

    CommonAddr = pRecv_ASDUFrame->ucInfoUnit[len];

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
    len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
    len++;

    if (iInfoAddrL == 3) {
        pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
        InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
        len++;
    }

    if ((InfoAddr != 0)) {
        IEC101_AP_UnknownInfoAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    if ((pRecv_ASDUFrame->ucInfoUnit[0] != IEC101_CAUSE_ACT)) {
        IEC101_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }
    if ((CommonAddr != Addr)) {
        IEC101_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        return;
    }

    //Sec
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    usMSec = pTemp_ASDUFrame->ucInfoUnit[len];
    usMSec = usMSec & 0x00ff;
    len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    INT16U hMS = pTemp_ASDUFrame->ucInfoUnit[len];
    hMS = hMS << 8;
    usMSec += hMS;
    ucSec = (INT8U)(usMSec / 1000);
    len++;
    //Min
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    ucMin = pTemp_ASDUFrame->ucInfoUnit[len] & 0x3f;
    len++;
    //hour
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    ucHour = pTemp_ASDUFrame->ucInfoUnit[len] & 0x1f;
    len++;
    //day
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    ucDay = pTemp_ASDUFrame->ucInfoUnit[len] & 0x1f;
    len++;
    //month
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    ucMonth = pTemp_ASDUFrame->ucInfoUnit[len] & 0x0f;
    len++;
    //year
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    ucYear = pTemp_ASDUFrame->ucInfoUnit[len] & 0x7f;
    len++;

    if (ucSec > 59 || ucMin > 59 || ucHour > 23 || ucYear > 99 || ucMonth > 12)
        return;

    if ((ucMonth == 0) || (ucDay == 0))
        return;

    if (ucMonth == 2) {
        if ((ucYear + 2000) / 4 == 0 && ucDay > 28)
            return;
        if ((ucYear + 2000) / 4 != 0 && ucDay > 29)
            return;
    }

    if (ucMonth <= 7 && ucMonth % 2 == 0 && ucDay > 30)
        return;
    if (ucMonth >= 8 && ucMonth % 2 == 1 && ucDay > 30)
        return;

    settimetm.tm_sec = ucSec;
    settimetm.tm_min = ucMin;
    settimetm.tm_hour = ucHour;
    settimetm.tm_mday = ucDay;
    settimetm.tm_mon = ucMonth - 1;
    settimetm.tm_year = ucYear + 100;
    printf("\n%d %d %d :%d %d %d\n", settimetm.tm_year + 1900, settimetm.tm_mon + 1, settimetm.tm_mday, settimetm.tm_hour, settimetm.tm_min, settimetm.tm_sec);
    //settimeval.tv_sec = mktime(&settimetm);
    //settimeval.tv_usec = usMSec*1000;
    //printf("%ld  %ld\n",settimeval.tv_sec,settimeval.tv_usec);

    settime.tv_sec = mktime(&settimetm);
    //memcpy(&(RtuDataAddr->settime), &settimeval, sizeof(settimeval));

    YKDATAIEC YK;
    YK.YKAct = 11; //YK动作：选择
    //psendbuffyk_cb_hp(YK, sizeof(YKDATAIEC));

    iLinkControl = 0x08;

    iec101_data_send(FORMAT_68, len + 4, OTHERS);

    printf("对时命令\n");
    return;
}

//复位远方链路，类型标识105
void IEC101_AP_RP(IEC101_ASDU_Frame     *pRecv_ASDUFrame,
                  INT8U                 ucTemp_Length,
                  INT16U                Addr,
                  INT16U                MasterAddr,
                  INT32S                ClearFlag,
                  INT8U                 iTranCauseL,
                  INT8U                 iCommonAddrL,
                  INT8U                 iInfoAddrL,
                  SharedMemory          *RtuDataAddr
                 ) {
    INT16U InfoAddr = 0;
    INT8U len = 0;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_C_RP_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;
    if (pRecv_ASDUFrame->ucInfoUnit[len] != IEC101_CAUSE_ACT) {
        //未知的传送原因
        IEC101_AP_UnknownCause(pRecv_ASDUFrame, ucTemp_Length);
        printf("//未知的传送原因\n");
    }
    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_ACTCON;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

    if (pRecv_ASDUFrame->ucInfoUnit[len] != (INT8U)(Addr & 0xff)) {
        printf("//未知的公共地址\n");
        IEC101_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
    }
    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);

    if (iCommonAddrL == 2) {
        if (pRecv_ASDUFrame->ucInfoUnit[len] != (INT8U)((Addr >> 8) & 0xff)) {
            printf("//未知的公共地址\n");
            IEC101_AP_UnknownCommAddr(pRecv_ASDUFrame, ucTemp_Length);
        }
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);
    }
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
    len++;
    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
    len++;
    if (iInfoAddrL == 3) {
        pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
        InfoAddr += pTemp_ASDUFrame->ucInfoUnit[len];
        len++;
    }

    pTemp_ASDUFrame->ucInfoUnit[len] = pRecv_ASDUFrame->ucInfoUnit[len];
    INT8U ops = pTemp_ASDUFrame->ucInfoUnit[len] ;
    if (ops == 1) //通知主进程进行101规约进程的复位
        iLinkControl = 0x28;
    else if (ops == 2)      //复位事件缓冲区等待处理的带时标的信息
        iLinkControl = 0x08;
    else {
        iec101_data_send(FORMAT_10, 0x09, OTHERS);
        return;
    }
    iec101_data_send(FORMAT_68, len + 4, OTHERS);
    iec101_data_send(FORMAT_10, 0x20, OTHERS);
    printf("---------------------%d-------\n", ops);
    if (ops == 2) { //复位事件缓冲区等待处理的带时标的信息
        YKDATAIEC YK;
        YK.YKAct = 5;
    }

}
//复位远方链路 10 40 01 41 16
void IEC101_M_EI_NA(IEC101_ASDU_Frame  *pRecv_ASDUFrame,
                    INT8U               ucTemp_Length,
                    INT16U              Addr,
                    INT16U              MasterAddr,
                    INT8U               iTranCauseL,
                    INT8U               iCommonAddrL,
                    INT8U               iInfoAddrL,
                    INT8U               initkind
                   ) { //此处长度为ASDU长度
    INT8U len = 0;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = IEC101_M_EI_NA_1;
    pTemp_ASDUFrame->ucRep_Num = 0x01;

    pTemp_ASDUFrame->ucInfoUnit[len++] = IEC101_CAUSE_INIT;
    if (iTranCauseL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = MasterAddr;

    pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)(Addr & 0xff);
    if (iCommonAddrL == 2)
        pTemp_ASDUFrame->ucInfoUnit[len++] = (INT8U)((Addr >> 8) & 0xff);

    pTemp_ASDUFrame->ucInfoUnit[len++] = 0;
    pTemp_ASDUFrame->ucInfoUnit[len++] = 0;
    if (iInfoAddrL == 3)
        pTemp_ASDUFrame->ucInfoUnit[len++] = 0;

    if (initkind == 1)
        pTemp_ASDUFrame->ucInfoUnit[len++] = 2;
    else
        pTemp_ASDUFrame->ucInfoUnit[len++] = 0;

    iLinkControl = 0x08;
    //iec101_data_send(FORMAT_68, len + 4, OTHERS);
    iec101_data_send(FORMAT_10, 0x00, OTHERS);
}

//未知的类型标识
void IEC101_AP_UnknownType(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;

    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;

    pTemp_ASDUFrame->ucInfoUnit[0] = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNTYPE;

    for (i = 1; i < ucTemp_Length - 2 - 2; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iLinkControl = 0x80;
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 2 - 2] = iecframecheckcode((INT8U*)pTemp_ASDUFrame, ucTemp_Length - 2, 18);
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 3] = 0x16;
    iec101_data_send(FORMAT_68, ucTemp_Length, OTHERS);

}

//未知的传送原因
void IEC101_AP_UnknownCause(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    //保持原报文所有字节原封不动
    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;

    pTemp_ASDUFrame->ucInfoUnit[0] = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNCAUSE;

    for (i = 1; i < ucTemp_Length - 2 - 2; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iLinkControl = 0x80;
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 2 - 2] = iecframecheckcode((INT8U*)pTemp_ASDUFrame, ucTemp_Length - 2, 18);
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 3] = 0x16;
    iec101_data_send(FORMAT_68, ucTemp_Length, OTHERS);

}

//未知的公共地址
void IEC101_AP_UnknownCommAddr(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT8U ucTemp_Length) { //ASDU长度
    INT8U i;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    //保持原报文所有字节原封不动
    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;

    pTemp_ASDUFrame->ucInfoUnit[0] = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNCOMMADDR;

    for (i = 1; i < ucTemp_Length - 2 - 2; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iLinkControl = 0x80;
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 2 - 2] = iecframecheckcode((INT8U*)pTemp_ASDUFrame, ucTemp_Length - 2, 18);
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 3] = 0x16;
    iec101_data_send(FORMAT_68, ucTemp_Length, OTHERS);

}

//未知的信息对象地址
void IEC101_AP_UnknownInfoAddr(IEC101_ASDU_Frame *pRecv_ASDUFrame, INT8U ucTemp_Length) {
    INT8U i;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    //保持原报文所有字节原封不动
    pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
    pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;

    pTemp_ASDUFrame->ucInfoUnit[0] = IEC101_CAUSE_N_BIT | IEC101_CAUSE_UNKNOWNINFOADDR;

    for (i = 1; i < ucTemp_Length - 2 - 2; i++) {
        pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
    }
    iLinkControl = 0x80;
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 2 - 2] = iecframecheckcode((INT8U*)pTemp_ASDUFrame, ucTemp_Length - 2, 18);
    pTemp_ASDUFrame->ucInfoUnit[ucTemp_Length - 3] = 0x16;
    iec101_data_send(FORMAT_68, ucTemp_Length, OTHERS);
}

//初始化101规约的缓冲区
void * phead_IEC101Struct;
void * phead_IEC101Buff;
void * phread_IEC101Down;
void * vipclass_IEC101Buf;
void * firstclass_IEC101Buf;
void * secondclass_IEC101Buf;

INT8U Init_IEC101_Buf() {
    INT16U usTemp_Step1;
    INT8U * pTemp_Buf;
    BufStruct * pTemp_BufList, *pTemp_BufList1;

    vipListHead = (BufStruct *)malloc(IEC101BUFDOWNNUMBER * sizeof(BufStruct));
    if (vipListHead == NULL) {
        return 0;
    }

    vipListFill = vipListHead;
    vipListGet = vipListHead;

    vipclass_IEC101Buf = malloc(IEC101BUFDOWNNUMBER * IEC101BUFUPLENGTH);
    if (vipclass_IEC101Buf == NULL) {
        return 0;
    }
    pTemp_Buf = (INT8U *) vipclass_IEC101Buf;

    pTemp_BufList = vipListHead;
    pTemp_BufList1 = vipListHead;
    usTemp_Step1 = 0;
    while (usTemp_Step1 < IEC101BUFDOWNNUMBER) {
        pTemp_BufList->usStatus = EMPTY;
        pTemp_BufList->buf = pTemp_Buf;
        pTemp_BufList->usLength = 0;
        pTemp_BufList1++;
        pTemp_BufList->next = pTemp_BufList1;
        pTemp_Buf += IEC101BUFUPLENGTH;
        pTemp_BufList++;
        usTemp_Step1++;
    }
    pTemp_BufList--;
    pTemp_BufList->next = vipListHead;

    firstListHead = (BufStruct *)malloc(IEC101BUFDOWNNUMBER * sizeof(BufStruct));
    if (firstListHead == NULL) {
        return 0;
    }

    firstListFill = firstListHead;
    firstListGet = firstListHead;

    firstclass_IEC101Buf = malloc(IEC101BUFDOWNNUMBER * IEC101BUFUPLENGTH);
    if (firstclass_IEC101Buf == NULL) {
        return 0;
    }
    pTemp_Buf = (INT8U *) firstclass_IEC101Buf;

    pTemp_BufList = firstListHead;
    pTemp_BufList1 = firstListHead;
    usTemp_Step1 = 0;
    while (usTemp_Step1 < IEC101BUFDOWNNUMBER) {
        pTemp_BufList->usStatus = EMPTY;
        pTemp_BufList->buf = pTemp_Buf;
        pTemp_BufList->usLength = 0;
        pTemp_BufList1++;
        pTemp_BufList->next = pTemp_BufList1;
        pTemp_Buf += IEC101BUFUPLENGTH;
        pTemp_BufList++;
        usTemp_Step1++;
    }
    pTemp_BufList--;
    pTemp_BufList->next = firstListHead;

    secondListHead = (BufStruct *)malloc(IEC101BUFDOWNNUMBER * sizeof(BufStruct));
    if (secondListHead == NULL) {
        return 0;
    }

    secondListFill = secondListHead;
    secondListGet = secondListHead;


    secondclass_IEC101Buf = malloc(IEC101BUFDOWNNUMBER * IEC101BUFUPLENGTH);
    if (secondclass_IEC101Buf == NULL) {
        return 0;
    }
    pTemp_Buf = (INT8U *) secondclass_IEC101Buf;

    pTemp_BufList = secondListHead;
    pTemp_BufList1 = secondListHead;
    usTemp_Step1 = 0;
    while (usTemp_Step1 < IEC101BUFDOWNNUMBER) {
        pTemp_BufList->usStatus = EMPTY;
        pTemp_BufList->buf = pTemp_Buf;
        pTemp_BufList->usLength = 0;
        pTemp_BufList1++;
        pTemp_BufList->next = pTemp_BufList1;
        pTemp_Buf += IEC101BUFUPLENGTH;
        pTemp_BufList++;
        usTemp_Step1++;
    }
    pTemp_BufList--;
    pTemp_BufList->next = secondListHead;

    phead_IEC101Struct = malloc(sizeof(IEC101Struct));
    if (phead_IEC101Struct == NULL) {
        return 0;
    }

    pIEC101_Struct = (IEC101Struct *) phead_IEC101Struct;

    phead_IEC101Buff = malloc(IEC101BUFDOWNNUMBER * sizeof(BufStruct));
    if (phead_IEC101Buff == NULL) {
        return 0;
    }
    pBufRecvRoot = (BufStruct *) phead_IEC101Buff;
    pBufRecvFill = pBufRecvRoot;
    pBufRecvGet = pBufRecvRoot;

    phread_IEC101Down = malloc(IEC101BUFDOWNNUMBER * IEC101BUFDOWNLENGTH);
    if (phread_IEC101Down == NULL) {
        return 0;
    }
    pTemp_Buf = (INT8U *) phread_IEC101Down;

    pTemp_BufList = pBufRecvRoot;
    pTemp_BufList1 = pBufRecvRoot;
    usTemp_Step1 = 0;
    while (usTemp_Step1 < IEC101BUFDOWNNUMBER) {

        pTemp_BufList->usStatus = EMPTY;
        pTemp_BufList->buf = pTemp_Buf;
        pTemp_BufList->usLength = 0;
        pTemp_BufList1++;
        pTemp_BufList->next = pTemp_BufList1;
        pTemp_Buf += IEC101BUFDOWNLENGTH;
        pTemp_BufList++;
        usTemp_Step1++;
    }
    pTemp_BufList--;
    pTemp_BufList->next = pBufRecvRoot;

    pIEC101_Struct->pSendBuf = (INT8U*) malloc(IEC101BUFUPLENGTH);
    pIEC101_Struct->pAutoSendYXBuf = (INT8U*) malloc(IEC101BUFUPLENGTH);
    pIEC101_Struct->pAutoSendYCBuf = (INT8U*) malloc(IEC101BUFUPLENGTH);
    sem_init(&Sem_Send101, 1, 1);
    return 1;
}


void iec101_destroy() {
    sem_destroy(&Sem_Send101);

    if (pIEC101_Struct->pSendBuf != NULL)
        free(pIEC101_Struct->pSendBuf);
    if (pIEC101_Struct->pAutoSendYXBuf != NULL)
        free(pIEC101_Struct->pAutoSendYXBuf);
    if (pIEC101_Struct->pAutoSendYCBuf != NULL)
        free(pIEC101_Struct->pAutoSendYCBuf);
    if (phead_IEC101Struct != NULL)
        free(phead_IEC101Struct);
    if (phead_IEC101Buff != NULL)
        free(phead_IEC101Buff);
    if (phread_IEC101Down != NULL)
        free(phread_IEC101Down);
}

//提供的初始化函数接口
INT8U iec101_create(INITIEC101VAULE * initiec101Value,
                    unsigned int (*send_callback)(INT8U * pBuffer, INT16U iLen),
                    int (*yk_callback)(int line, int act, int how)) {
    INT8U ucRet;
    ucRet = Init_IEC101_Buf();
    if (0 == ucRet) {
        return 0; //初始化缓存失败返回0
    }

    pIEC101_Struct->ucYK_Limit_Time = initiec101Value->ucYK_Limit_Time;
    pIEC101_Struct->ucTimeOut_t0 = initiec101Value->ucTimeOut_t0;
    pIEC101_Struct->ucTimeOut_t1 = initiec101Value->ucTimeOut_t1;
    pIEC101_Struct->ucTimeOut_t2 = initiec101Value->ucTimeOut_t2;
    pIEC101_Struct->ucTimeOut_t3 = initiec101Value->ucTimeOut_t3;
    pIEC101_Struct->ucMax_k = initiec101Value->ucMax_k;
    pIEC101_Struct->ucMax_w = initiec101Value->ucMax_w;
    IEC101_InitAllFlag();

    pIEC101_Struct->send = send_callback;
    pIEC101_Struct->yk = yk_callback;

    return 1;
}

//初始化101规约里面的状态标志
void IEC101_InitAllFlag() {
    pIEC101_Struct->ucDataSend_Flag = FALSE;
    pIEC101_Struct->ucYK_TimeCount_Flag = FALSE;
    pIEC101_Struct->ucYK_TimeCount = 0x00;

    pIEC101_Struct->ucSendCountOverturn_Flag = FALSE;

    pIEC101_Struct->usRecvNum = 0;
    pIEC101_Struct->usSendNum = 0;

    pIEC101_Struct->usServRecvNum = 0;
    pIEC101_Struct->usServSendNum = 0;

    pIEC101_Struct->ucIdleCount_Flag = FALSE;
    pIEC101_Struct->ucIdleCount = 0;

    pIEC101_Struct->ucNoRecvCount = 0;
    pIEC101_Struct->ucNoRecvT3 = 0;

    pIEC101_Struct->ucWaitServConCount_Flag = FALSE;
    pIEC101_Struct->ucWaitServConCount = 0;

    pIEC101_Struct->w = 0;
    pIEC101_Struct->k = 0;
}

//清空计时器
void iec101_init_timer() {
    pIEC101_Struct->ucWaitServConCount_Flag = FALSE;    //关闭t1计时器
    pIEC101_Struct->ucWaitServConCount = 0;             //t1计时器清零
    pIEC101_Struct->ucNoRecvT3 = 0;                     //t3计时器清零
}

void iec101_data_add(INT8U *pFrameBuffer, INT32U iFrameLen) {
    LX_CharCopy(pBufRecvFill->buf, pFrameBuffer, iFrameLen);
    pBufRecvFill->usStatus = FULL;
    pBufRecvFill->usLength = iFrameLen;
    pBufRecvFill = pBufRecvFill->next;
    return;
}
void iec101_vipData_add(INT8U *pFrameBuffer, INT32U iFrameLen) {
    LX_CharCopy(vipListFill->buf, pFrameBuffer, iFrameLen);
    vipListFill->usStatus = FULL;
    vipListFill->usLength = iFrameLen;
    vipListFill = vipListFill->next;
    printf("iec101_vipData_add\n");
    return;
}
void iec101_firstData_add(INT8U *pFrameBuffer, INT32U iFrameLen) {
    LX_CharCopy(firstListFill->buf, pFrameBuffer, iFrameLen);
    firstListFill->usStatus = FULL;
    firstListFill->usLength = iFrameLen;
    firstListFill = firstListFill->next;
    printf("iec101_firstData_add\n");
    return;
}
void iec101_secondData_add(INT8U *pFrameBuffer, INT32U iFrameLen) {
    LX_CharCopy(secondListFill->buf, pFrameBuffer, iFrameLen);
    secondListFill->usStatus = FULL;
    secondListFill->usLength = iFrameLen;
    secondListFill = secondListFill->next;
    printf("iec102_secondData_add\n");
    return;
}
//101规约处理入口函数


int iec101_verify_handle(INT16U Addr,                          //公共地址
                         INT16U MasterAddr,                    //主站地址
                         INT8U *DealBuf,
                         INT16U BufLength,
                         INT8U YXBuf[],                        //1点遥信YX数据缓冲区,共160路YX
                         INT8U YXBufLength,                    //1点遥信路长度
                         INT8U YXDBuf[],                       //2点遥信YX数据缓冲区,共160路YX
                         INT8U YXDBufLength,                   //2点遥信路长度
                         FP32 YcValue[],                       //遥测采集变量
                         INT16U YcNum,                         //遥测个数
                         INT16U YXAddr[],                      //1点遥信信息体地址
                         INT16U YXDAddr[],                     //2点遥信信息体地址
                         INT16U YCAddr[],                      //遥测信息体地址
                         FP32 YcUnitaryTime[],                 //归一化系数
                         FP32 YcLargerTime[],                  //放大系数
                         struct timeval settime,               //主站对时变量
                         INT32S ClearFlag,                     //缓冲区设置标志
                         INT8U iTranCauseL,                    //传送原因长度
                         INT8U iCommonAddrL,                   //公共地址长度
                         INT8U iInfoAddrL,                     //信息地址长度
                         SharedMemory * RtuDataAddr) {

    INT8U ucTemp_ReturnFlag = iec101_verify(DealBuf, BufLength);

    //请求链路状态
    if (ucTemp_ReturnFlag == IEC101FRAMELOGIN) {
        iec101_data_send(FORMAT_10, 0x0b, OTHERS);
        return 10;
    }
    //正常通信/招测报文
    if ((ucTemp_ReturnFlag == IEC101FRAME68H) || (ucTemp_ReturnFlag == IEC101FRAMERESET)) {
        iec101_handle(DealBuf,
                      BufLength,
                      Addr,
                      MasterAddr,
                      YXBuf,
                      YXBufLength,
                      YXDBuf,
                      YXDBufLength,
                      YcValue,
                      YcNum,
                      YXAddr,
                      YXDAddr,
                      YCAddr,
                      YcUnitaryTime,
                      YcLargerTime,
                      settime,
                      ClearFlag,
                      ucTemp_ReturnFlag,
                      iTranCauseL,
                      iCommonAddrL,
                      iInfoAddrL,
                      RtuDataAddr);
        return 1;
    }

    //请求1类数据
    if (ucTemp_ReturnFlag == IEC101FRAME1REQUEST) {
        //首先检查是否有优先数据需要上送
        if (vipListGet->usStatus == FULL) {
            iec101_data_send(IEC_VIPCLASS_DATA, 0, OTHERS);
            vipListGet->usStatus = EMPTY;
            vipListGet = vipListGet->next;
        }
        //检查是否有1类数据需要上送
        else if (firstListGet->usStatus == FULL) {
            iec101_data_send(IEC_FIRSTCLASS_DATA, 0, OTHERS);
            firstListGet->usStatus = EMPTY;
            firstListGet = firstListGet->next;
        } else {
            //无一级数据
            iec101_data_send(FORMAT_10, 0x09, OTHERS);
        }

    }
    //请求2类数据
    if (ucTemp_ReturnFlag == IEC101FRAME2REQUEST) {
        //首先检查是否有优先数据和1类数据需要上送
        if ((vipListGet->usStatus == FULL) || (firstListGet->usStatus == FULL)) {
            //通知主站有一级数据需要上送。
            iec101_data_send(FORMAT_10, 0x29, OTHERS);
            return 2;
        }
        //检查是否有2类数据需要上送
        if (secondListGet->usStatus == FULL) {
            iec101_data_send(IEC_SECONDCLASS_DATA, 0, OTHERS);
            secondListGet->usStatus = EMPTY;
            secondListGet = secondListGet->next;
        } else {
            //无二级数据
            iec101_data_send(FORMAT_10, 0x09, OTHERS);
        }

    }
    //状态招测报文
    if (ucTemp_ReturnFlag == IEC101FRAME10H) {
        iec101_data_send(FORMAT_10, 0, OTHERS);
        return 1;
    }
    //错误报文
    if (ucTemp_ReturnFlag == IEC101FRAMEERR) {
        return -1;
    }
}

/*
char logpkg[] = { 0x10, 0x49, 0x01, 0x4a, 0x16};   //请求链路状态
char resetpkg[] = { 0x10, 0x40, 0x01, 0x41, 0x16}; //复位链路状态
char requst11[] = { 0x10, 0x5a, 0x01, 0x5b, 0x16}; //请求一级数据帧1
char requst12[] = { 0x10, 0x7a, 0x01, 0x7b, 0x16}; //请求一级数据帧2
char requst21[] = { 0x10, 0x5b, 0x01, 0x5c, 0x16}; //请求二级数据帧1
char requst22[] = { 0x10, 0x7b, 0x01, 0x7c, 0x16}; //请求二级数据帧2
*/
//判断是否为有效报文
INT8U iec101_verify(INT8U * pTemp_Buf, INT16U usTemp_Length) {
    if ((usTemp_Length > 255) || (usTemp_Length < 5)) {
        return IEC101FRAMEERR;
    }

    if (usTemp_Length == 5) {
        switch (pTemp_Buf[1]) {
        case 0x49:
            return IEC101FRAMELOGIN;
        case 0x40:
            return IEC101FRAMERESET;
        case 0x5a:
            return IEC101FRAME1REQUEST;
        case 0x7a:
            return IEC101FRAME1REQUEST;
        case 0x5b:
            return IEC101FRAME2REQUEST;
        case 0x7b:
            return IEC101FRAME2REQUEST;
        default:
            return IEC101FRAME10H;
        }
    }

    if ((0x68 == pTemp_Buf[0]) && (0x68 == pTemp_Buf[3])) {
        INT8U ucTemp_Length = usTemp_Length & 0xff;

        if (pTemp_Buf[2] != (ucTemp_Length - 6)) {
            return IEC101FRAMEERR;
        }

        if (pTemp_Buf[ucTemp_Length - 2] != iecframecheckcode(pTemp_Buf, usTemp_Length, 68))
            return IEC101FRAMEERR;

        iLinkAddress = pTemp_Buf[9];
        return IEC101FRAME68H;
    }
    return IEC101FRAMEERR;
}

//根据主站命令，执行不同操作
void  iec101_handle(INT8U       *        pTemp_Buf,         //缓冲区指针
                    INT8U                ucTemp_Length,     //缓冲区长度
                    INT16U               Addr,              //公共地址
                    INT16U               MasterAddr,        //主站地址
                    INT8U                YXBuf[],           //1点遥信YX数据缓冲区,共160路YX
                    INT8U                YXBufLength,       //1点遥信路长度
                    INT8U                YXDBuf[],          //2点遥信YX数据缓冲区,共160路YX
                    INT8U                YXDBufLength,      //2点遥信路长度
                    FP32                 YcValue[],         //遥测采集变量
                    INT16U               YcNum,             //遥测个数
                    INT16U               YXAddr[],          //1点遥信信息体地址
                    INT16U               YXDAddr[],         //2点遥信信息体地址
                    INT16U               YCAddr[],          //遥测信息体地址
                    FP32                 YcUnitaryTime[],   //归一化系数
                    FP32                 YcLargerTime[],    //放大系数
                    struct               timeval  settime,  //主站对时变量
                    INT32S               ClearFlag,         //缓冲区设置标志
                    INT8U                iPacketKind,       //报文类型
                    INT8U                iTranCauseL,       //传送原因长度
                    INT8U                iCommonAddrL,      //公共地址长度
                    INT8U                iInfoAddrL,        //信息地址长度
                    SharedMemory         *RtuDataAddr
                   ) {

    printf("iec101_handle\n");
    INT8U *pTemp_Pointer;
    INT8U *pTemp_Pointer101;
    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    INT8U ucTemp_Type;
    INT16U usTemp_ServSendNum;

    pTemp_Pointer = pTemp_Buf + 2;
    usTemp_ServSendNum = ChangeShort(pTemp_Pointer);
    pTemp_Pointer += 2;
    ChangeShort(pTemp_Pointer);
    pTemp_Pointer += 2;

    pTemp_Pointer101 = pTemp_Buf + 6;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *) pTemp_Pointer101;
    //68包
    if (iPacketKind == IEC101FRAME68H) {

        ucTemp_Type = pTemp_ASDUFrame->ucType;
        printf("[101] Get message ucType = %d\n", ucTemp_Type);
        switch (ucTemp_Type) {
        case IEC101_C_DC_NA_1: //遥控，双命令
            IEC101_AP_YK2E(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr,
                           iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);

            break;
        case IEC101_C_IC_NA_1: //总召唤命令
            IEC101_AP_IC(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr,
                         YXBuf, YXBufLength, YXDBuf, YXDBufLength, YcValue, YcNum,
                         YXAddr, YXDAddr, YCAddr, YcUnitaryTime, YcLargerTime,
                         iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
            break;
        case IEC101_C_CS_NA_1: //时钟同步命令
            IEC101_AP_CS(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr,
                         settime, iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
            break;
        case IEC101_C_RP_NA_1: //复位进程命令
            IEC101_AP_RP(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr,
                         ClearFlag, iTranCauseL, iCommonAddrL, iInfoAddrL, RtuDataAddr);
            break;
        default:
            IEC101_AP_UnknownType(pTemp_ASDUFrame, ucTemp_Length - 6);
            break;
        }
    }
    if (iPacketKind == IEC101FRAMERESET) { //复位链路
        IEC101_M_EI_NA(pTemp_ASDUFrame, ucTemp_Length - 6, Addr, MasterAddr, iTranCauseL, iCommonAddrL, iInfoAddrL, 1);
    }

}

//发送报文
void iec101_data_send(INT8U ucTemp_FrameFormat, INT8U ucTemp_Value, ENUM sk) {
    INT8U *pTemp_SendBuf;
    int outlen = ucTemp_Value;
    sem_wait(&Sem_Send101);

    //决定使用哪个发送buf
    switch (sk) {
    case AUTOYX:
    case YXBW:
        pTemp_SendBuf = pIEC101_Struct->pAutoSendYXBuf;
        break;
    case AUTOYC:
        pTemp_SendBuf = pIEC101_Struct->pAutoSendYCBuf;
        break;
    case OTHERS:
    default:
        pTemp_SendBuf = pIEC101_Struct->pSendBuf;
        break;
    }

    // printf("a===================\n");
    // for (int i = 0; i < outlen + 8; ++i) {
    //     printf("%02x\n", pTemp_SendBuf[i]);
    // }
    // printf("===================\n");

    switch (ucTemp_FrameFormat) {
    case FORMAT_10:
        *pTemp_SendBuf++ = 0x10;
        *pTemp_SendBuf++ = ucTemp_Value;
        *pTemp_SendBuf++ = 0x01;
        *pTemp_SendBuf++ = 0x0c;
        *pTemp_SendBuf++ = 0x16;
        outlen = 5;
        pTemp_SendBuf -= 5;
        *(pTemp_SendBuf + 3) = iecframecheckcode(pTemp_SendBuf, 5, 10);
        pIEC101_Struct->send(pTemp_SendBuf, 5);
        break;

    case FORMAT_68:

        *pTemp_SendBuf++ = 0x68;
        *pTemp_SendBuf++ = outlen + 2;
        *pTemp_SendBuf++ = outlen + 2;
        *pTemp_SendBuf++ = 0x68;
        *pTemp_SendBuf++ = iLinkControl;
        *pTemp_SendBuf++ = iLinkAddress;
        outlen = outlen + 8;
        pTemp_SendBuf[outlen - 8] = iecframecheckcode(pTemp_SendBuf - 6, outlen, 68);
        printf(">>>>> pTemp_SendBuf = %d %d\n", pTemp_SendBuf[outlen - 9], outlen);
        pTemp_SendBuf[outlen - 7] = 0x16;
        pTemp_SendBuf -= 6;

        if (sk == YXBW)
            iec101_vipData_add(pTemp_SendBuf, outlen);
        else
            iec101_firstData_add(pTemp_SendBuf, outlen);
        break;

    case IEC_VIPCLASS_DATA:
        pIEC101_Struct->send(vipListGet->buf, vipListGet->usLength);
        break;
    case IEC_FIRSTCLASS_DATA:
        pIEC101_Struct->send(firstListGet->buf, firstListGet->usLength);
        break;
    case IEC_SECONDCLASS_DATA:
        pIEC101_Struct->send(secondListGet->buf, secondListGet->usLength);
        break;
    }
    Print(pTemp_SendBuf, outlen, 1);
    sem_post(&Sem_Send101);
}

//发送报文
void iec_ykpacketdata_add(INT8U flag, INT8U InfoValueIndex) {
    printf("iec_ykpacketdata_add\n");
    INT8U *pTemp_SendBuf;

    IEC101_ASDU_Frame *pTemp_ASDUFrame;
    pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);

    pTemp_SendBuf = pIEC101_Struct->pSendBuf;

    *pTemp_SendBuf++ = 0x68;
    *pTemp_SendBuf++ = (5 + InfoValueIndex);
    *pTemp_SendBuf++ = (5 + InfoValueIndex);
    *pTemp_SendBuf++ = 0x68;
    printf("\n\npTemp_ASDUFrame->ucInfoUnit[%d] = %02x\n\n", InfoValueIndex, pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex]);
    if ((pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x80) || (pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x81))
        *pTemp_SendBuf++ = 0x08;
    else if ((pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x00) || (pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x01))
        *pTemp_SendBuf++ = 0x28;
    else
        *pTemp_SendBuf++ = 0x08;

    *pTemp_SendBuf++ = iLinkAddress;

    pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex + 1] = iecframecheckcode((INT8U*) pTemp_ASDUFrame, (5 + InfoValueIndex), 68);
    pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex + 2]  = 0x16;

    iec101_firstData_add(pTemp_SendBuf - 6, 11 + InfoValueIndex);
    printf("\n\npTemp_ASDUFrame->ucInfoUnit[%d] = %02x\n\n", InfoValueIndex, pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex]);
    if ((pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x00) || (pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex] == 0x01)) {
        pTemp_ASDUFrame = (IEC101_ASDU_Frame *)(pIEC101_Struct->pSendBuf + 6);
        pTemp_SendBuf = pIEC101_Struct->pSendBuf;

        *pTemp_SendBuf++ = 0x68;
        *pTemp_SendBuf++ = (5 + InfoValueIndex);
        *pTemp_SendBuf++ = (5 + InfoValueIndex);
        *pTemp_SendBuf++ = 0x68;

        *pTemp_SendBuf++ = 0x08;
        *pTemp_SendBuf++ = iLinkAddress;
        pTemp_ASDUFrame->ucInfoUnit[0] = IEC101_CAUSE_ACTTERM;

        pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex + 1] = iecframecheckcode((INT8U*) pTemp_ASDUFrame, (5 + InfoValueIndex), 68);
        pTemp_ASDUFrame->ucInfoUnit[InfoValueIndex + 2]  = 0x16;

        iec101_firstData_add(pTemp_SendBuf - 6, 11 + InfoValueIndex);
    }
}


INT16U ChangeShort(INT8U *chan) {
    INT8U * pbyt;
    union iii {
        INT16U cshort;
        INT8U byt[2];
    } unshort;
    pbyt = chan;
    unshort.byt[0] = *pbyt;
    pbyt++;
    unshort.byt[1] = *pbyt;
    return (unshort.cshort);
}

void LX_CharCopy(INT8U * buf1, INT8U * buf2, int length) {
    int copystep;
    for (copystep = 0; copystep < length; copystep++)
        *(buf1 + copystep) = *(buf2 + copystep);
}

void Print(INT8U* buf, int len, short io) {
    return ;
    if (len > 0) {
        int i;
        printf(io == 0 ? "rcv:" : "snd:");
        for (i = 0; i < len; i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");
    }
}
