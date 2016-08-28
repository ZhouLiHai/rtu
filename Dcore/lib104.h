#ifndef __LIB104_H_
#define __LIB104_H_

#include "define.h"

#define FORMAT_10               5   //传输格式：bit[0]=0
#define FORMAT_68               6   //监视格式：bit[1]=0 bit[0]=1
#define IEC_One68               8
#define IEC_FourI               7

//ASDU的类型标识
#define IEC101_M_SP_NA_1        1
#define IEC101_M_DP_NA_1        3
#define IEC101_M_ME_NA_1        9
#define IEC101_M_ME_NB_1        11
#define IEC101_M_ME_NC_1        11
#define IEC101_M_IT_NA_1        15
#define IEC101_M_ME_ND_1        21
#define IEC101_M_SP_TB_1        30
#define IEC101_M_DP_TB_1        31
#define IEC101_M_ST_TB_1        32
#define IEC101_M_ME_TD_1        34
#define IEC101_M_ME_TE_1        35
#define IEC101_M_ME_TF_1        36
#define IEC101_M_IT_TB_1        37
#define IEC101_M_EP_TD_1        38
#define IEC101_M_EP_TE_1        39
#define IEC101_M_EP_TF_1        40

#define IEC101_C_DC_NA_1        45
#define IEC101_C_RC_NA_1        47
#define IEC101_C_SC_TA_1        58

#define IEC101_M_EI_NA_1        70

#define IEC101_C_IC_NA_1        100
#define IEC101_C_CI_NA_1        101
#define IEC101_C_RD             102
#define IEC101_C_CS_NA_1        103
#define IEC101_C_RP_NA_1        105

//ASDU的传送原因
#define IEC101_CAUSE_N_BIT                     0x40
#define IEC101_CAUSE_SPONT                     3        //突发（自发）
#define IEC101_CAUSE_INIT                      4        //请求或者被请求
#define IEC101_CAUSE_REQ                       5        //请求或者被请求
#define IEC101_CAUSE_ACT                       6        //激活
#define IEC101_CAUSE_ACTCON                    7        //激活确认
#define IEC101_CAUSE_DEACT                     8        //停止激活
#define IEC101_CAUSE_DEACTCON                  9        //停止激活确认
#define IEC101_CAUSE_ACTTERM                   10       //激活终止
#define IEC101_CAUSE_INTROGEN                  20       //响应站召唤
#define IEC101_CAUSE_COUNTGEN                  37       //响应计数量站总召唤
#define IEC101_CAUSE_UNKNOWNTYPE               44       //未知的类型标识
#define IEC101_CAUSE_UNKNOWNCAUSE              45       //未知的传送原因
#define IEC101_CAUSE_UNKNOWNCOMMADDR           46       //未知的应用服务数据单元公共地址
#define IEC101_CAUSE_UNKNOWNINFOADDR           47       //未知的信息对象地址

#define     YCPacketContain     0x28        //遥测回路数
#define     YXPacketContain     0x28        //遥测回路数
#define     YXZPacketContain    0x16        //遥测回路数
#define     YXDPacketContain    0x28        //遥测回路数
#define     YCTimePacketContain 15          //遥测带时标

#define IEC101FRAMEOK           0
#define IEC101FRAMEERR          1
#define IEC101FRAMESEQERR       2
#define IECFRAMENEEDACK         3
#define IEC101FRAMELOGIN        4
#define IEC101FRAMERESET        5
#define IEC101FRAMECHERR        6
#define IEC101FRAMEOK           0
#define IEC101FRAME_I           0x10
#define IEC101FRAME_S           0x11
#define IEC101FRAME_U           0x13
#define IEC101BUFDOWNNUMBER     160     //16  //接收的命令的最大数量
#define IEC101BUFDOWNLENGTH     700     //256  //接收一个命令的最大长度
#define IEC101BUFUPLENGTH       256

#define IEC101FRAME68H          0x11
#define IEC101FRAME10H          0x12


#define IEC101FRAME1REQUEST        0x13
#define IEC101FRAME2REQUEST        0x14

#define FORMAT_I                0       //传输格式：bit[0]=0
#define FORMAT_S                1       //监视格式：bit[1]=0 bit[0]=1
#define FORMAT_U                3       //控制格式：bit[1]=1 bit[0]=1


struct _IEC104Struct {
    unsigned short usRecvNum;           //已经接收到的帧
    unsigned short usSendNum;           //已经发送出的帧
    unsigned short usServRecvNum;       //服务器接收到的帧
    unsigned short usServSendNum;       //服务器已发送的帧
    unsigned char ucSendCountOverturn_Flag;     //发送计数翻转标志
    //unsigned char   ucRecvCountOverturn_Flag;  //接收计数翻转标志
    unsigned short usAckNum;            //已经认可的帧
    unsigned char ucTimeOut_t0;         //连接建立超时值,单位s，默认30s
    unsigned char ucTimeOut_t1;         //APDU的发送或测试的超时时间,默认：15s
    unsigned char ucTimeOut_t2;         //无数据报文t2t1情况下发送S-帧的超时时间,默认：10s
    unsigned char ucTimeOut_t3;         //长时间空闲状态下发送测试帧的超时   默认:20s
    unsigned char k;                    //发送I格式应用规约数据单元的未认可帧数
    unsigned char w;                    //接收I格式应用规约数据单元的帧数
    unsigned char ucMax_k;              //发送状态变量的最大不同的接收序号
    unsigned char ucMax_w;              //接收w个I格式APDUs之后的最后的认可
    unsigned char ucDataSend_Flag;      //是否允许发送标志
    unsigned char ucIdleCount_Flag;     //是否允许t2计数
    unsigned char ucIdleCount;          //t2计数
    unsigned char ucWaitServConCount_Flag;      //是否t1计数
    unsigned char ucWaitServConCount;   //t1计数
    unsigned char ucNoRecvCount;        //t0计数
    unsigned char ucNoRecvT3;           //t3计时器
    unsigned char ucYK_Limit_Time;
    unsigned char ucYK_TimeCount_Flag;
    unsigned char ucYK_TimeCount;

    int sfd;
};

int lib104TypeU(unsigned char *TempBuf, unsigned short BufLen);
int lib104TypeI(unsigned char *TempBuf, unsigned short BufLen, struct _IEC104Struct* str104);

int lib104Verify(unsigned char * TempBuf, unsigned short BufLen);

int IEC104_AP_RQALL_CON(unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104);
int IEC104_AP_RQALL_END(unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104);

int IEC104_AP_SP_NA(unsigned char *SendBuf, struct _net * net, struct _remote * r, struct _IEC104Struct* str104);
int IEC104_AP_DP_NA(unsigned char *SendBuf, struct _net * net, struct _remote * r, struct _IEC104Struct* str104);
int IEC104_AP_ME_ND(unsigned char *SendBuf, struct _net * net, struct _analog * a, struct _IEC104Struct* str104);

int lib104Send(unsigned char * buf, int length, struct _IEC104Struct* str104);
int libCmdSend(unsigned char * buf, int type, int fd);

int IEC104_AP_CS(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct timeval *settime, struct _IEC104Struct* str104);

int IEC104_YK_PO(char * RecvBuf);
int IEC104_YK_AC(char * RecvBuf);
int IEC104_YK_HW(char * RecvBuf);
int IEC104_YK_CA(char * RecvBuf);

int IEC104_YK_RP(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104);
int IEC104_YK_TR(unsigned char * RecvBuf, unsigned char *SendBuf, struct _net * net, struct _IEC104Struct* str104);

int IEC104_SP_AUTO(struct _net * net, int type, int num, int state[], struct _IEC104Struct* str104);

int IEC104_YC_AUTO(struct _net * net, struct _analog * a, struct _IEC104Struct * str104);
int IEC104_SO_AUTO(struct _net * net, int number, int state[0], long long stime, int type,
                   struct _IEC104Struct * str104);


void IEC104_InitAllFlag(struct _IEC104Struct * str104);

#endif