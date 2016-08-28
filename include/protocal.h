#ifndef PROTOEXTE_H
#define PROTOEXTE_H

#include <time.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mtypes.h"
#include "shm.h"

#define EMPTY 0
#define FULL  1

#define IEC_VIPCLASS_DATA               1
#define IEC_FIRSTCLASS_DATA             2
#define IEC_SECONDCLASS_DATA            3
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

#define YKBase  0x6001

#define YCNum   1         //遥测回路数
#define YCMax   36        //每一回路采集的量
#define YKMax   48        //遥控路数

struct _BufStruct {
    INT16U usStatus;
    INT16U usLength;
    INT8U *buf;
    struct _BufStruct *next;
};

typedef struct _BufStruct BufStruct;

typedef struct {
    INT32S  YKAct;      //YK动作，0XE0:选择/0xE2 执行/0xE3 撤销;
    INT32S  YKNo;       //YK路数,1-8路
    INT32S  YKSta;      //YK状态，0xCC(合闸)，0x55（分闸），0xAA：YK执行，0xff：YK撤销
} YKDATAIEC;

struct _InitIEC101Value {               //说明：初始化101规约常量参数，定义以下结构体
    INT8U ucYK_Limit_Time;              //默认40s
    INT8U ucTimeOut_t0;                 //连接建立超时值,单位s，默认30s
    INT8U ucTimeOut_t1;                 //APDU的发送或测试的超时时间,默认：15s
    INT8U ucTimeOut_t2;                 //无数据报文t2t1情况下发送S-帧的超时时间,默认：10s
    INT8U ucTimeOut_t3;                 //长时间空闲状态下发送测试帧的超时   默认:20s
    INT8U ucMax_k;                      //发送状态变量的最大不同的接收序号
    INT8U ucMax_w;                      //接收w个I格式APDUs之后的最后的认可
};

typedef struct _InitIEC101Value INITIEC101VAULE;

struct _IEC101_ASDU_Struct {
    INT8U ucType;          //类型标识
    INT8U ucRep_Num;       //可变结构限定词
    // INT8U ucCauseL;        //传送原因
    // INT8U ucCauseH;        //传送原因
    // INT8U ucCommonAddrL;   //公共地址
    // INT8U ucCommonAddrH;   //公共地址
    INT8U ucInfoUnit[220]; //信息体单元,length
};

typedef struct _IEC101_ASDU_Struct IEC101_ASDU_Frame;

struct _IEC10X_ASDU_Struct {
    INT8U ucType;               //类型标识
    INT8U ucRep_Num;            //可变结构限定词
    INT8U ucCauseL;             //传送原因
    INT8U ucCauseH;             //传送原因
    INT8U ucCommonAddrL;        //公共地址
    INT8U ucCommonAddrH;        //公共地址
    INT8U ucInfoUnit[220];      //信息体单元,length
};

typedef struct _IEC10X_ASDU_Struct IEC10X_ASDU_Frame;

typedef enum SendDataKind_Enum {
    AUTOYX = 0,
    AUTOYC = 1,
    OTHERS = 2,
    YXBW = 3,
} ENUM;

#define FORMAT_I                0       //传输格式：bit[0]=0
#define FORMAT_S                1       //监视格式：bit[1]=0 bit[0]=1
#define FORMAT_U                3       //控制格式：bit[1]=1 bit[0]=1

#endif