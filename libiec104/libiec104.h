/*
 * libiec104.h
 *
 * Created on: 2013-11-14
 * Author:
 */
#ifndef LIBIEC10X_H_
#define LIBIEC10X_H_

#include "protocal.h"

/**************************************************************************************************
 *                              接口函数定义
 *说明：1.在本动态库函数中iec10x_verify_handle（）函数的功能结合了iec101_verify（）,iec101_handle（）的功能，可
 *       单独用iec101_verify_handle（），也可以iec101_verify（）,iec101_handle（）结合起来用；
 *     2.网络标志位： nTCP_ConnectFlag克用来判断网络的连通性，可在以太网中被用来判断；
 *     3.注意iec101_data_add函数用法。
***************************************************************************************************/

//初始化10x缓冲区，指定遥控回调函数和发送回调函数。
INT8U iec10x_create(INITIEC101VAULE * initiec101Value,
                    unsigned int (*send_callback)(INT8U * pBuffer, INT16U iLen),
                    int (*yk_callback)(int line, int act, int how));

/* 说明：本函数是协议的处理函数，主要功能是判断报文的有效性和类别，再根据报文的进行不同的操作处理，并具有反馈信息的功能等。
 *      在初始化操作完成后，以太网远方规约以及数据库召唤处理及通讯线程中使用。
 * 参数：见注释
 * 返回值：无，本函数位开放式的，异常情况类型内部已处理。
 */
int iec10x_verify_handle(INT16U Addr,   //公共地址
                         INT16U MasterAddr,    //主站地址
                         INT8U * DealBuf,
                         INT16U BufLength,
                         YKDATAIEC YK,      //YK下发命令
                         INT8U YXBuf[],        //1点遥信YX数据缓冲区,共160路YX
                         INT8U YXBufLength,    //1点遥信路长度
                         INT8U YXDBuf[],       //2点遥信YX数据缓冲区,共160路YX
                         INT8U YXDBufLength,   //2点遥信路长度
                         FP32 YcValue[],       //遥测采集变量
                         INT16U YcNum, //遥测个数
                         INT16U YXAddr[],      //1点遥信信息体地址
                         INT16U YXDAddr[],     //2点遥信信息体地址
                         INT16U YCAddr[],      //遥测信息体地址
                         FP32 YcUnitaryTime[], //归一化系数
                         FP32 YcLargerTime[],  //放大系数
                         struct timeval *settime,       //主站对时变量
                         INT32S ClearFlag,     //缓冲区设置标志
                         INT8U iTranCauseL,    //传送原因长度
                         INT8U iCommonAddrL,   //公共地址长度
                         INT8U iInfoAddrL,     //信息地址长度
                         SharedMemory * RtuDataAddr);

/* 说明：本函数是验证报文帧的有效性，然后返回报文类型，方便后续处理。
 * 参数：1.pTemp_Buf 指向待验证报文的缓冲区
 *      2.usTemp_Length 表示待验证报文的缓冲区长度
 * 返回值：可以为：IEC101FRAMEERR，IEC101FRAMESEQERR，IECFRAMENEEDACK，
 *        IEC101FRAMEOK，IEC101FRAME_I，IEC101FRAME_S，IEC101FRAME_U，
 *        根据返回值作相应处理。
 */
INT8U iec10x_verify(INT8U * pTemp_Buf, INT16U usTemp_Length);

/* 说明：本函数根据不同的主站命令，执行相应的操作。
 * 参数：1.pTemp_Buf 指向报文的缓冲区
 *      2.usTemp_Length 表示链路层长度
 * 返回值：无
 */
int iec10x_handle(INT8U * pTemp_Buf,    //缓冲区指针
                  INT8U ucTemp_Length, //缓冲区长度
                  INT16U Addr, //公共地址
                  INT16U MasterAddr,   //主站地址
                  YKDATAIEC YK,        //YK下发命令
                  INT8U YXBuf[],       //1点遥信YX数据缓冲区,共160路YX
                  INT8U YXBufLength,   //1点遥信路长度
                  INT8U YXDBuf[],      //2点遥信YX数据缓冲区,共160路YX
                  INT8U YXDBufLength,  //2点遥信路长度
                  FP32 YcValue[],      //遥测采集变量
                  INT16U YcNum,        //遥测个数
                  INT16U YXAddr[],     //1点遥信信息体地址
                  INT16U YXDAddr[],    //2点遥信信息体地址
                  INT16U YCAddr[],     //遥测信息体地址
                  FP32 YcUnitaryTime[],        //归一化系数
                  FP32 YcLargerTime[], //放大系数
                  struct timeval *settime,      //主站对时变量
                  INT32S ClearFlag,    //缓冲区设置标志
                  INT8U iPacketKind,   //报文类型
                  INT8U iTranCauseL,   //传送原因长度
                  INT8U iCommonAddrL,  //公共地址长度
                  INT8U iInfoAddrL,    //信息地址长度
                  SharedMemory * RtuDataAddr);

/* 说明：本函数主要功能是发送回馈信息，可根据参数类型向主站发送相应的报文
 * 参数：1.ucTemp_FrameFormat 可以为FORMAT_I，FORMAT_S，FORMAT_U，分别表示报文的传输格式，监视格式和控制格式
 *      2.ucTemp_Value 发送缓存填充值，以为3种APDU格式长度
 *      3.sk，表示3种发送数据的类型，分别为AUTOYX,AUTOYC,OTHERS。
 * 返回值：无
 */
void iec10x_data_send(INT8U ucTemp_FrameFormat,
                      INT8U ucTemp_Value,
                      ENUM sk);

void iec10x_ykpacketdata_send(INT8U ucIEC_FrameKind,
                              INT8U ucTemp_Value,
                              INT8U ucActValue,
                              INT8U iTranCauseL,
                              INT8U iCommonAddrL,
                              INT8U iInfoAddrL);

/* 说明：组成多帧报文,不带时标的单点遥信，主动上报，类型标识1。可用于主动上报线程。
 * 参数：ucNum：为一帧报文包含的最大SOE数量，默认为22
 * 返回值：1 表示成功
 */
INT8U iec10x_ap_sp_na_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[],
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              );

/* 说明：组成多帧报文，单点遥信SOE，主动上报，类型标识30。可用于主动上报线程。
 * 参数：ucNum:为一帧报文包含的最大SOE数量，默认为22
 */
INT8U iec10x_ap_sp_tb_autosend(INT16U Addr,
                               INT16U MasterAddr,
                               int number,
                               int state[0],
                               long long stime,
                               INT8U iFlag,
                               INT8U iTranCauseL,
                               INT8U iCommonAddrL,
                               INT8U iInfoAddrL
                              );


/* 说明：不带品质描述词的归一化值，主动上报，类型标识21，可用于遥测主动上报线程。
 * 参数：见注释,内部处理
 * 返回值：1 表示成功
 */
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
                    );

/* 说明：清空计时器。
 * 参数：无
 * 返回值：无
 */
void iec10x_init_timer();

/* 说明：清空101规约所用缓冲区，可用与进程退出前。
 * 参数：无
 * 返回值：无
 */
void iec10x_destroy();

/* 说明：初始化101规约的缓冲区
 * 参数：无
 * 返回值：无
 */
INT8U Init_IEC10X_Buf();

/* 说明：初始化101规约里面的状态标志
 * 参数：无
 * 返回值：无
 */
void IEC10X_InitAllFlag();

//不带时标的单点遥信，响应总召测，类型标识1
INT8U IEC10X_AP_SP_NA(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                      INT16U Addr,
                      INT16U MasterAddr,
                      INT8U YXBuf[],
                      INT8U YXBufLength,
                      INT16U YXAddr[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL);

//不带时标的单点遥信，响应总召测，类型标识2
INT8U IEC10X_AP_DP_NA(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                      INT16U Addr,
                      INT16U MasterAddr,
                      INT8U YXDBuf[],
                      INT8U YXDBufLength,
                      INT16U YXDAddr[],
                      INT8U iTranCauseL,
                      INT8U iCommonAddrL,
                      INT8U iInfoAddrL);

//不带时标的遥测值，响应总召测，类型标识21
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
                      INT8U iInfoAddrL);

//遥控，双命令，类型标识46
void IEC10X_AP_YK2E(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                    INT8U ucTemp_Length,
                    INT16U Addr,
                    INT16U MasterAddr,
                    YKDATAIEC YK,
                    INT8U iTranCauseL,
                    INT8U iCommonAddrL,
                    INT8U iInfoAddrL,
                    SharedMemory * RtuDataAddr);

//总招命令，类型标识100
void IEC10X_AP_IC(IEC10X_ASDU_Frame * pRecv_ASDUFrame, INT8U ucTemp_Length, INT16U Addr, INT16U MasterAddr,
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
                  SharedMemory * RtuDataAddr);

//对时命令，类型标识103
void IEC10X_AP_CS(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  struct timeval *settime,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL,
                  SharedMemory * RtuDataAddr);


//复位远方链路，类型标识105
void IEC10X_AP_RP(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                  INT8U ucTemp_Length,
                  INT16U Addr,
                  INT16U MasterAddr,
                  INT32S ClearFlag,
                  INT8U iTranCauseL,
                  INT8U iCommonAddrL,
                  INT8U iInfoAddrL);

void IEC10X_AP_UnknownType(IEC10X_ASDU_Frame *, INT8U);
void IEC10X_AP_UnknownCause(IEC10X_ASDU_Frame *, INT8U);
void IEC10X_AP_UnknownCommAddr(IEC10X_ASDU_Frame *, INT8U);
void IEC10X_AP_UnknownInfoAddr(IEC10X_ASDU_Frame *, INT8U);

//总招唤确认命令
void IEC10X_AP_RQALL_CON(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL);
//总招唤结束命令
void IEC10X_AP_RQALL_END(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
                         INT16U Addr,
                         INT16U MasterAddr,
                         INT8U iTranCauseL,
                         INT8U iCommonAddrL,
                         INT8U iInfoAddrL);
//遥控处理命令
void YcHandle(IEC10X_ASDU_Frame * pRecv_ASDUFrame,
              YKDATAIEC YK,
              INT16U PointNo,
              INT8U iCauseIndex,
              INT8U iUnitIndex,
              INT8U iCheckLen,
              INT8U ucTemp_Length,
              SharedMemory * RtuDataAddr);

INT16U ChangeShort(INT8U * chan);
void LX_CharCopy(INT8U *, INT8U *, int);
void Print(INT8U * buf, int len, short io);

#endif /* LIBIEC10X_H_ */
