/*
 * shm.h
 *
 *  Created on: 2014-6-29
 *      Author: root
 */

#ifndef SHM_H_
#define SHM_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include "mtypes.h"

/*
 *  共享内存结构
 *
 *  POSIX标准的共享内存
 *  连接时需要加 rt  库
 *
 */

#define REMOTE_SIZE 256     // 遥信最大配置量
#define DOMAIN_SIZE 32      // 遥控最大配置量
#define ANALOG_SIZE 256     // 遥测最大配置量
#define YC_SIZE 3               // 遥测板数
#define YX_SIZE 128             // 遥信事件空间容量
#define WLDEVICE_NUM    32      // 接受故障指示器数量
#define WLSIG_ON 0x11           //故障指示器上电
#define WLSIG_OFF 0x12          //故障指示器断电
#define WLSIG_SHORT 0x16        //故障指示器短路
#define WLSIG_SHORTRECOVER 0x17 //故障指示器短路恢复
#define WLSIG_GROUND 0x18       //故障指示器接地
#define WLSIG_GROUNDRECOVER 0x1A        //故障指示器接地恢复

typedef void * SharedMemory;

//遥测数据结构
typedef struct {
    int Enable;                 // 板子存在标识

    float Hz1;                  // 频率
    float Hz2;                  // 频率

    float Ua;                   // 对应原结构　Uab1
    float Ub;                   // 对应原结构　Uab2
    float Uc;                   // 对应原结构　Ubc1
    float Ud;                   // 对应原结构　Ubc2

    float Ia1, Ib1, Ic1;        // 第一回路　电流值
    float Ia2, Ib2, Ic2;        // 第二回路　电流值
    float Ia3, Ib3, Ic3;        // 第三回路　电流值
    float Ia4, Ib4, Ic4;        // 第四回路　电流值

    float P1, Q1;               // 第一回路　有功　和　无功
    float P2, Q2;               // 第二回路　有功　和　无功
    float P3, Q3;               // 第三回路　有功　和　无功
    float P4, Q4;               // 第四回路　有功　和　无功

    // 计算方法 atan2(虚部,实部)
    // 单位：弧度
    float Ua_phase;             // 对应原结构　Uab1_imag,Uab1_real
    float Ub_phase;             // 对应原结构　Uab2_imag,Uab2_real
    float Uc_phase;             // 对应原结构　Ubc1_imag,Ubc2_real
    float Ud_phase;             // 对应原结构　Ubc2_imag,Ubc2_real

    float Ia1_phase, Ib1_phase, Ic1_phase;
    float Ia2_phase, Ib2_phase, Ic2_phase;
    float Ia3_phase, Ib3_phase, Ic3_phase;
    float Ia4_phase, Ib4_phase, Ic4_phase;
} _YcValue;

// 遥测系数
typedef struct {
    float CUa, CUb, CUc, CUd;   // 电压系数
    float CIa1, CIb1, CIc1;     // 第一回路　电流系数
    float CIa2, CIb2, CIc2;     // 第二回路　电流系数
    float CIa3, CIb3, CIc3;     // 第三回路　电流系数
    float CIa4, CIb4, CIc4;     // 第四回路　电流系数
    float CP1, CQ1;             // 第一回路　有功无功系数
    float CP2, CQ2;             // 第二回路　有功无功系数
    float CP3, CQ3;             // 第三回路　有功无功系数
    float CP4, CQ4;             // 第四回路　有功无功系数
} _YcCoeffect;

typedef struct {
    int state[2];
    int number;
    int type;
    unsigned long long time;
} SoeUnit;

typedef struct {
    int head;
    int number;
} SoeAttr;

typedef struct {
    SoeUnit unit[YX_SIZE];
    SoeAttr info;
} _Soe;

typedef struct {
    int type;
    int board;
    float u, i , p, q;
} _cmd;

#endif /* SHM_H_ */
