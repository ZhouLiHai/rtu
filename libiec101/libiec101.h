/*
 * libiec101.h
 *
 * Created on: 2013-11-14
 * Author:
 */
#ifndef LIBIEC101_H_
#define LIBIEC101_H_

#include "protocal.h"

IEC101Struct  * pIEC101_Struct;

//信号量
sem_t       Sem_Deal101;
sem_t       Sem_Send101;

//socket监听收到数据指针
BufStruct   *pBufRecvRoot;//头指针
BufStruct   *pBufRecvFill;//（sockt监听收到的数据）未处理数据指针尾（EMPTY->FULL）
BufStruct   *pBufRecvGet; //（sockt监听收到的数据）未处理数据指针头（FULL->EMPTY）

BufStruct* vipListHead;//优先数据头
BufStruct* vipListGet;//优先数据待发送
BufStruct* vipListFill;//优先数据尾
BufStruct* firstListHead;//一级数据
BufStruct* firstListGet;//一级数据待发送
BufStruct* firstListFill;//一级数据尾
BufStruct* secondListHead;//二级数据
BufStruct* secondListGet;//二级数据待发送
BufStruct* secondListFill;//二级数据尾


/**************************************************************************************************
 *                              接口函数定义
 *说明：1.在本动态库函数中iec101_verify_handle（）函数的功能结合了iec101_verify（）,iec101_handle（）的功能，可
 *       单独用iec101_verify_handle（），也可以iec101_verify（）,iec101_handle（）结合起来用；
 *     2.网络标志位： nTCP_ConnectFlag克用来判断网络的连通性，可在以太网中被用来判断；
 *     3.注意iec101_data_add函数用法。
***************************************************************************************************/

/* 说明：本函数主要是完成初始化操作，主要包括101协议规约结构体初始化即：缓冲区、变量、状态标志位的初始化，初始化要求见结构体定义。
 * 参数：1.initiec101Value 为初始化101规约常量参数，见结构体定义;2.INT8S(*psendbuffcom_callback)(INT8U* pBuffer,INT16U iLen);串口回调 3.INT8S(*psendbuffnet_callback)(INT8U* pBuffer,INT16U iLen)；网口回调
 * 返回值：若返回值为0，则表示初始化失败。
 */
INT8U iec101_create(INITIEC101VAULE * initiec101Value,
                    unsigned int (*send_callback)(INT8U * pBuffer, INT16U iLen),
                    int (*yk_callback)(int line, int act, int how));

/* 说明：本函数主要是完成待处理帧的压入操作
 * 参数：1. pFrameBuffer：指向待处理的数据缓冲区
 *      2. iFrameLen：一个完整帧总长度即从帧中读出的apdu总长度加2
 * 返回值：无
 */
void iec101_data_add(INT8U *pFrameBuffer, INT32U iFrameLen);

/* 说明：本函数是协议的处理函数，主要功能是判断报文的有效性和类别，再根据报文的进行不同的操作处理，并具有反馈信息的功能等。
 *      在初始化操作完成后，以太网远方规约以及数据库召唤处理及通讯线程中使用。
 * 参数：见注释
 * 返回值：无，本函数位开放式的，异常情况类型内部已处理。
 */
int iec101_verify_handle(INT16U              Addr,
                         INT16U              MasterAddr,
                         INT8U               *DealBuf,
                         INT16U              BufLength,
                         INT8U               YXBuf[],
                         INT8U               YXBufLength,
                         INT8U               YXDBuf[],
                         INT8U               YXDBufLength,
                         FP32                YcValue[],
                         INT16U              YcNum,
                         INT16U              YXAddr[],
                         INT16U              YXDAddr[],
                         INT16U              YCAddr[],
                         FP32                YcUnitaryTime[],
                         FP32                YcLargerTime[],
                         struct              timeval  settime,
                         INT32S              ClearFlag,
                         INT8U               iTranCauseL,
                         INT8U               iCommonAddrL,
                         INT8U               iInfoAddrL,
                         SharedMemory * RtuDataAddr
                        );

/* 说明：本函数是验证报文帧的有效性，然后返回报文类型，方便后续处理。
 * 参数：1.pTemp_Buf 指向待验证报文的缓冲区
 *      2.usTemp_Length 表示待验证报文的缓冲区长度
 * 返回值：可以为：IEC101FRAMEERR，IEC101FRAMESEQERR，IECFRAMENEEDACK，
 *        IEC101FRAMEOK，IEC101FRAME_I，IEC101FRAME_S，IEC101FRAME_U，
 *        根据返回值作相应处理。
 */
INT8U iec101_verify(INT8U * pTemp_Buf, INT16U usTemp_Length);

/* 说明：本函数根据不同的主站命令，执行相应的操作。
 * 参数：1.pTemp_Buf 指向报文的缓冲区
 *      2.usTemp_Length 表示链路层长度
 * 返回值：无
 */
void  iec101_handle(INT8U   *             pTemp_Buf,
                    INT8U                 ucTemp_Length,
                    INT16U                Addr,
                    INT16U                MasterAddr,
                    INT8U                 YXBuf[],
                    INT8U                 YXBufLength,
                    INT8U                 YXDBuf[],
                    INT8U                 YXDBufLength,
                    FP32                  YcValue[],
                    INT16U                YcNum,
                    INT16U                YXAddr[],
                    INT16U                YXDAddr[],
                    INT16U                YCAddr[],
                    FP32                  YcUnitaryTime[],
                    FP32                  YcLargerTime[],
                    struct                timeval  settime,
                    INT32S                ClearFlag,
                    INT8U                 iPacketKind,
                    INT8U                 iTranCauseL,
                    INT8U                 iCommonAddrL,
                    INT8U                 iInfoAddrL,
                    SharedMemory * RtuDataAddr
                   );

/* 说明：本函数主要功能是发送回馈信息，可根据参数类型向主站发送相应的报文
 * 参数：1.ucTemp_FrameFormat 可以为FORMAT_I，FORMAT_S，FORMAT_U，分别表示报文的传输格式，监视格式和控制格式
 *      2.ucTemp_Value 发送缓存填充值，以为3种APDU格式长度
 *      3.sk，表示3种发送数据的类型，分别为AUTOYX,AUTOYC,OTHERS。
 * 返回值：无
 */
void iec101_data_send(INT8U ucTemp_FrameFormat,
                      INT8U ucTemp_Value,
                      ENUM sk);

void iec_ykpacketdata_add(INT8U flag,
                          INT8U InfoValueIndex);

INT8U iec101_ap_sp_tb_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[],
                               long long stime,
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              );

/* 说明：组成多帧报文,不带时标的单点遥信，主动上报，类型标识1。可用于主动上报线程。
 * 参数：ucNum：为一帧报文包含的最大SOE数量，默认为22
 * 返回值：1 表示成功
 */
extern INT8U iec101_ap_sp_na_autosend(INT16U Addr,
                                      INT16U MasterAddr,
                                      int number,
                                      int state[],
                                      INT8U iFlag,
                                      INT8U iTranCauseL,
                                      INT8U iCommonAddrL,
                                      INT8U iInfoAddrL
                                     );


/* 说明：不带品质描述词的归一化值，主动上报，类型标识21，可用于遥测主动上报线程。
 * 参数：见注释,内部处理
 * 返回值：1 表示成功
 */
extern INT8U iec101_ap_yc_autosend(INT16U     Addr,           //公共地址
                                   INT16U     MasterAddr,       //主站地址
                                   FP32       YcValue[],        //DTU两块YC采集模块采集遥测量
                                   INT16U     YcNum,
                                   INT16U     YcObjectAddr[],
                                   FP32     YcUnitaryTime[],
                                   FP32     YcLargerTime[],
                                   FP32     YcCriticalValue[],
                                   INT8U      iTranCauseL,      //传送原因长度
                                   INT8U      iCommonAddrL,     //公共地址长度
                                   INT8U      iInfoAddrL        //信息地址长度
                                  );

/* 说明：清空计时器。
 * 参数：无
 * 返回值：无
 */
void iec101_init_timer();

/* 说明：清空101规约所用缓冲区，可用与进程退出前。
 * 参数：无
 * 返回值：无
 */
void iec101_destroy();

/* 说明：读取遥测值，作为最初值的记录用于判断。
 * 参数：
 * 返回值：无
 * */
void iec101_getycvalue(FP32 YcValue[], INT16U ycNum);

INT8U Init_IEC101_Buf();//初始化101规约的缓冲区        //**隐式调用

void IEC101_InitAllFlag();//初始化101规约里面的状态标志     //**隐式调用

//不带时标的单点遥信，响应总召测，类型标识1
INT8U IEC101_AP_SP_NA(
    IEC101_ASDU_Frame        *pRecv_ASDUFrame,  //指向ASDU信息单位
    INT16U               Addr,
    INT16U               MasterAddr,
    INT8U                    YXBuf[],
    INT8U                    YXBufLength,
    INT16U               YXAddr[],
    INT8U                iTranCauseL,
    INT8U                iCommonAddrL,
    INT8U                iInfoAddrL
);

//不带时标的单点遥信，响应总召测，类型标识1  IEC101_AP_IC//隐式调用
INT8U IEC101_AP_DP_NA(
    IEC101_ASDU_Frame        *pRecv_ASDUFrame,  //指向ASDU信息单位
    INT16U               Addr,
    INT16U               MasterAddr,
    INT8U                    YXDBuf[],
    INT8U                    YXDBufLength,
    INT16U               YXDAddr[],
    INT8U                iTranCauseL,
    INT8U                iCommonAddrL,
    INT8U                iInfoAddrL
);


INT8U IEC101_AP_ME_ND(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    FP32                    YcUnitaryTime[],
    FP32                    YcLargerTime[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL
);//不带品质描述词不带时标的归一化值，响应总召测，类型标识21  IEC101_AP_IC//隐式调用
INT8U IEC101_AP_ME_TD(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    FP32                    YcUnitaryTime[],
    FP32                    YcLargerTime[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL,
    SharedMemory * RtuDataAddr
);//带品质描述词带时标的归一化值，响应总召测，类型标识34

INT8U IEC101_AP_ME_NB(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL
);//测量值，不带时标 带品质描述词的标度化值
INT8U IEC101_AP_ME_TE(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL,
    SharedMemory * RtuDataAddr
);//测量值，带时标 带品质描述词的标度化值
INT8U IEC101_AP_ME_NC(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL
);//测量值，不带时标 带品质描述词的浮点值
INT8U IEC101_AP_ME_TF(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT16U              Addr,
    INT16U              MasterAddr,
    FP32                    YcValue[],
    INT16U              YcNum,
    INT16U              YCAddr[],
    INT8U               iTranCauseL,
    INT8U               iCommonAddrL,
    INT8U               iInfoAddrL,
    SharedMemory * RtuDataAddr
);//测量值，带时标 带品质描述词的浮点值

//以下根据主站命令，执行不同操作数据来自IEC101_ProcessAPDU（）；将全部封装
//遥控，双命令，类型标识46
void IEC101_AP_YK2E(
    IEC101_ASDU_Frame *     pRecv_ASDUFrame,
    INT8U                   ucTemp_Length,
    INT16U                  Addr,
    INT16U                  MasterAddr,
    INT8U                   iTranCauseL,
    INT8U                   iCommonAddrL,
    INT8U                   iInfoAddrL,
    SharedMemory * RtuDataAddr
);

void IEC101_AP_IC(
    IEC101_ASDU_Frame   *pRecv_ASDUFrame,
    INT8U              ucTemp_Length,
    INT16U             Addr,
    INT16U             MasterAddr,
    INT8U                  YXBuf[],
    INT8U                  YXBufLength,
    INT8U                  YXDBuf[],
    INT8U                  YXDBufLength,
    FP32                   YcValue[],
    INT16U             YcNum,
    INT16U               YXAddr[],
    INT16U             YXDAddr[],
    INT16U             YCAddr[],
    FP32                   YcUnitaryTime[],
    FP32                   YcLargerTime[],
    INT8U              iTranCauseL,
    INT8U              iCommonAddrL,
    INT8U              iInfoAddrL,
    SharedMemory * RtuDataAddr
); //总召唤，类型标识100

void IEC101_AP_CS(
    IEC101_ASDU_Frame *pRecv_ASDUFrame,
    INT8U            ucTemp_Length,
    INT16U           Addr,
    INT16U           MasterAddr,
    struct             timeval  settime,
    INT8U            iTranCauseL,
    INT8U            iCommonAddrL,
    INT8U            iInfoAddrL,
    SharedMemory * RtuDataAddr
);//此处长度为ASDU长度

//复位远方链路，类型标识105
void IEC101_AP_RP(IEC101_ASDU_Frame *pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  INT32S ClearFlag,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr);          //此处长度为ASDU长度
void IEC101_M_EI_NA(
    IEC101_ASDU_Frame  *pRecv_ASDUFrame,
    INT8U             ucTemp_Length,
    INT16U            Addr,
    INT16U            MasterAddr,
    INT8U                 iTranCauseL,
    INT8U                 iCommonAddrL,
    INT8U             iInfoAddrL,
    INT8U             initkind
); //此处长度为ASDU长度
void IEC101_AP_UnknownType(IEC101_ASDU_Frame *, INT8U);         //未知的类型标识IEC101_ProcessAPDU

void IEC101_AP_UnknownCause(IEC101_ASDU_Frame *, INT8U);        //未知的传送原因IEC101_ProcessAPDU

void IEC101_AP_UnknownCommAddr(IEC101_ASDU_Frame *, INT8U);     //未知的公共地址IEC101_AP_UnknownInfoAddr

void IEC101_AP_UnknownInfoAddr(IEC101_ASDU_Frame *, INT8U);     //未知的信息对象IEC101_AP_UnknownInfoAddr

void Print(INT8U* buf, int len, short io);
INT16U ChangeShort(INT8U *chan);
void LX_CharCopy(INT8U * buf1, INT8U * buf2, int length);

#endif /* LIBIEC101_H_ */
