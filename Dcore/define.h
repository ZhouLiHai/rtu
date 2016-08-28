/*
 * define.h
 *
 *  Created on: 2014-6-30
 *      Author: root
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#include "shm.h"
#include "libhd.h"
#include "mtypes.h"
#include "protocal.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <termios.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/un.h>

#include "libconfig.h"

static struct timeval settime;
unsigned short *gFpgaic;
unsigned short *ycbase[YC_SIZE];

#define ENABLE 0
#define HZ 1
#define COEF01 2
#define COEF02 3
#define COEF03 4
#define COEF04 5
#define COEF05 6
#define COEF06 7
#define COEF07 8
#define COEF08 9
#define COEF09 10
#define COEF10 11
#define COEF11 12
#define COEF12 13
#define COEF13 14
#define COEF14 15
#define COEF15 16
#define COEF16 17
#define COEF17 18
#define COEF18 19
#define COEF19 20
#define COEF20 21
#define COEF21 22
#define COEF22 23
#define COEF23 24
#define COEF24 25
#define IA 26
#define IB 27
#define IC 28
#define UA 29
#define UB 30
#define UC 31

#define ST1 32
#define ST2 33

#define OP1 0
#define OQ1 2
#define OP2 4
#define OQ2 6

#define INFONUM 128
#define INFOPAGE 8

int SerialOpen(const char *DeviceName, int baund);
int sntp_client(char *lpszAscii);
void judge_time(int);
void fix_time(struct timeval settime);
void setback(void);
void checksetback();

int HEXtoDEC(int a);
int Doubletonum(unsigned short a, unsigned short b);

int get_board_state(int id);
int get_sta(int id, int how);

int getPress(int th, int index);
int getSignal(int index);
float elapse(struct timeval *ptv, struct timeval *tv);

void set_select(int id, int how);
void set_action(int id, int how);
void set_cancel(int id, int how);

void set_led(int id, int o);
void set_power_start(int hw);
void set_power_end(int hw);

int checkstate(void);

long long getDeviceTime();
unsigned short getDc(int type);
float getTemprature();
void mktimeout(struct timespec *tsp, long sec);

int gpioWrite(int number, int v);
int gpioRead(int number);

int revTo10(unsigned short v);

// For now contal
struct _contral {
    int pt;
    int hw;
    int id;
    int ac;
    int timeout;

    int size;
    config_setting_t *setting;

    pthread_t thread_key;
    pthread_mutex_t colock;
    pthread_cond_t sig;
    pthread_mutex_t siglock;

    int (*start)(void);
    void *(*server)(void *);

    int (*select)(int line, int how);
    int (*action)(int line, int how);
    int (*cancel)(int line, int how);
    int (*justdo)(int line, int how);

    int (*getot)(int num);
    int (*getswitch)(int num);

} CONTRAL;

int ctstart(void);
void *ctserver(void *args);
int ctselect(int line, int how);
int ctaction(int line, int how);
int ctcancel(int line, int how);
int ctjustdo(int line, int how);
int cttimeout(int num);
int ctswitch(int num);

struct _battery {
    int state;
    int activate;
    int onbattery;

    pthread_t thread_key;
    pthread_mutex_t colock;
    pthread_cond_t sig;
    pthread_mutex_t siglock;

    int (*start)(void);
    int (*begin)(void);
    int (*end)(void);

    void *(*server)(void *);

} BATTERY;

int pwstart(void);
void *pwserver(void *args);
int pwbegin(void);
int pwend(void);

struct _led {
    /*
     * 0 - on
     * 1 - off
     * 5 - flash
     */
    int state[7];
    int nm[7];

    pthread_t thread_key;
    pthread_cond_t sig;
    pthread_mutex_t siglock;

    int (*start)(void);
    int (*on)(int ind);
    int (*off)(int ind);
    int (*flash)(int index, int num);

    void *(*server)(void *);
    int (*checkself)(void);
} LED;

int ledstart(void);
void *ledserver(void *);
int ledon(int ind);
int ledoff(int ind);
int ledflash(int ind, int num);
int ledcheckself(void);

struct _ipc {
    char *_share_;
    char *_socket_;
    struct sockaddr_un addr;
    socklen_t addrlen;
    pthread_t thread_key;

    config_t *cfg;

    int sp;

    struct {
        int act;
        int main;
        int how;
    } msg;

    int (*start)(void);
    int (*read)(void);
    void (*send)(int res);

    void *(*server)(void *);
    int (*checkself)(void);
} IPC;

int ipcstart(void);
int ipcread(void);
void ipcsend(int res);
void *ipcserver(void *);

struct _remote {

    int press, size;
    config_setting_t *setting;
    unsigned short yxoriginal;
    SoeUnit records;

    //遥信硬件状态缓冲区
    unsigned short state[6];
    //soe存储缓冲区
    _Soe soe;

    //单点遥信和双点遥信数据缓冲区，供规约使用
    unsigned short SId[REMOTE_SIZE];
    unsigned char SBuf[REMOTE_SIZE];
    unsigned char Slength;

    unsigned short DId[REMOTE_SIZE];
    unsigned char DBuf[REMOTE_SIZE];
    unsigned char Dlength;

    pthread_mutex_t soelock;

    int (*start)(void);
    int (*get)(void);
    int (*add)(int id, int sign, int type, long long tv);
    void *(*server)(void *);
    int (*search)(int type, int board, int shift);

} REMOTE;

int remstart(void);
int remget(void);
int remadd(int id, int sign, int type, long long tv);
void *remserver(void *);
int remSearch(int type, int board, int shift);

struct _loop {
    int step;
    unsigned char board_state_org[12];
    unsigned char board_state[12];

    char ip[64];

    int (*start)(void);
    void *(*server)(void *);
    int (*checkstate)(void);

} LOOP;

int lostart(void);
void *loserver(void *);

struct _analog {
    config_setting_t *setting;
    int size;
    int state[ANALOG_SIZE];
    struct timeval tv[ANALOG_SIZE];

    config_setting_t *prsetting;
    int prsize;
    int prstate[ANALOG_SIZE];
    int rebackstate[ANALOG_SIZE];
    int prtimes[ANALOG_SIZE];
    struct timeval prtv[ANALOG_SIZE];
    struct timeval rebacktv[ANALOG_SIZE];

    double dcCoe1;
    double dcCoe2;

    float threshold;
    int otime;

    int rebacktime;
    int reback;
    int loop;
    int warn;

    unsigned short fpgaver;
    float deviceTemp;
    long long devicetv;

    //遥测系数
    _YcCoeffect Coe[YC_SIZE];
    //遥测实时数据，无修正。
    _YcValue Val[YC_SIZE];

    //遥测测量值缓冲区，
    unsigned short buflength;
    float Form[ANALOG_SIZE];
    unsigned short Id[ANALOG_SIZE];

    float Unit[ANALOG_SIZE];
    float Larg[ANALOG_SIZE];
    float Dead[ANALOG_SIZE];

    void (*start)(void);
    void *(*server)(void* job);
    void (*ultralimit)(void);
    void (*protection)(void);
    int (*initCoeffect)(void);
    void *(*refresh)(void *args);
    float (*point)(int board, int shift);

} ANALOG;

void anaStart(void);
void *anaServer(void *job);
void anaUltralimit(void);
void anaProtection(void);
int anaInitCoeffect(void);
void *anaRefresh(void *args);
float anaPoint(int board, int shift);

typedef struct {
    int upper;
    int board;
    int shift;
    // 1代表越限 2代表故障
    int type;
} eunit;

struct _error {
    eunit config[INFONUM];
    eunit msg[INFONUM];

    int config_length;
    int soeHead;
    int ErrorInfoShow;
    int ShowHead;

    int (*isShowing)(void);

    void (*Show)(void);
    void (*Update)(void);
} ERINFO;

void clearCRTbody(void);
int FoundError(int point);
void UpdateError(void);
int ErrorShowing(void);
void ShowError(void);
void intiErr(config_t *cfg);

//液晶点坐标
typedef struct {
    int x;
    int y;
} Point;

//液晶区域
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} Rect;

#define LCM_X 160
#define LCM_Y 160

#define LCD_NOREV 0
#define LCD_REV 1

struct _lcd {
    short int *lcdAddr;
    short int *lcdData;
    unsigned char buf2[160][160];
    unsigned char buf3[160][160];
    unsigned char buf4[160 * 160];
    short int fpgaBuf[160][10];

    unsigned char LcdBuf[LCM_X * LCM_Y];

    char HzBuf[32];
    unsigned char HzDat_12[198576];

    void (*enable)(void);
    void (*write)(void);

    void (*openlight)(void);
    void (*closelight)(void);

    void (*setcontrast)(void);
    void (*readhzbuf)(void);

    void (*show)(char *str, int x, int y, char rev_flg);
    void (*textshow)(char *str, Point pos, char rev_flg);
    void (*pixelcolor)(Point pt, unsigned char color);
    void (*clrrect)(Rect rect);
    void (*drawhline)(Point start, int x_pos);
    void (*clear)(void);
    void (*revrect)(Rect rect);

} LCD;

void lcdenable(void);
void lcdwrite(void);
void lcdopenlight(void);
void lcdcloselight(void);
void lcdsetcontrast(void);
void lcdreadhzbuf(void);
void pixelcolor(Point pt, unsigned char color);
void textshow(char *str, Point pos, char rev_flg);
void lcdtextshow(char *str, int x, int y, char rev_flg);
void clrrect(Rect rect);
void drawhline(Point start, int x_pos);
void lcdclear(void);
void LCDINIT(void);

//按键定义
#define PAGE_SIZE 4096
#define LCD_BASE_ADDR 0x460
#define LCD_BASE_DATA 0x461
#define LCD_OPEN_LIGHT_ADDR 0x462
#define LCD_CLOSE_LIGHT_ADDR 0x463
#define LCD_CONTRAST_ADDR 0x464
#define LCD_KEY_ADDR 0x41A

#define OFFSET_U 0
#define OFFSET_D 1
#define OFFSET_L 2
#define OFFSET_R 3
#define OFFSET_O 4
#define OFFSET_E 5
#define OFFSET_A 6
#define OFFSET_J 7
#define OFFSET_S 8
#define NOKEY 0

#define UP_VALUE 0x1f7
#define DOWN_VALUE 0x1ef
#define LEFT_VALUE 0x1df
#define RIGHT_VALUE 0x1fe
#define OK_VALUE 0x1fd
#define ESC_VALUE 0x0ff
#define RST_VALUE 0x1fb
#define ADD_VALUE 0x1bf
#define JIAN_VALUE 0x17f

#define UP 1 << OFFSET_U
#define DOWN 1 << OFFSET_D
#define LEFT 1 << OFFSET_L
#define RIGHT 1 << OFFSET_R
#define OK 1 << OFFSET_O
#define ESC 1 << OFFSET_E
#define RST 1 << OFFSET_S
#define ADD 1 << OFFSET_A
#define JIAN 1 << OFFSET_J

struct _key {
    pthread_t key;
    pthread_attr_t attr_t;

    int PressKey;
    int KeyBeenPressed;

    short int *keyAddr;

    int (*get)(void);
    void (*enable)(void);
    void (*setbit)(int *val, char bit, char bit_offset);

    void (*start)(void);
    void *(*server)(void *args);

    int (*signal)(void);
    int (*errsignal)(void);

} KEY;

void keysetbit(int *val, char bit, char bit_offset);
void keyenable();
int keyget();
void keystart(void);
void *keyserver(void *args);
void KEYINIT(void);

#define SHOWMAX 5
#define SHOWBASE 60
#define SHOWSTEP 16

#define LEVELMIN 0
#define LEVELMAX 2

struct menuitem {
    int number;
    int level;
    int father;
    char title[32];
    int titlenum;
    void (*fun)(void);
    int ishow;
    int selected;
};

struct _menu {
    //页面通行标志
    int pass;
    //液晶密码
    int password;
    //保护参数列表
    config_setting_t *prsetting;
    int prsize;

    int CurLevel;
    int menusize;
    int LevelNumSelect[12];

    struct menuitem menulist[16];

    pthread_t thread_main;
    pthread_t thread_server;
    pthread_attr_t attr_t;

    void (*start)(void);
    void *(*run)(void *args);
    void *(*server)(void *args);

    int (*foundCurLevSelect)(int CurLevel, int father);
    int (*foundFirstItem)(int CurLevel, int father);
    int (*foundFirstShow)(int CurLevel, int father);
    int (*selectedup)(int CurLevel, int father);
    int (*selecteddown)(int CurLevel, int father);
    void (*setFirstBeenSelect)(int CurLevel, int father);

} MENU;

void MENUINIT(config_t *cfg);

#define SERICOM_NUM 5
#define MAXRECLEN 1024
#define SERICOM_BUFSIZE 255

INT16S CopyYcValueD[128];

struct _com {
    int Addr;
    int MasterAddr;

    int iTranCauseL;
    int iCommonAddrL;
    int iInfoAddrL;

    INITIEC101VAULE initiec101Value;

    int devNum;
    const char * devName[SERICOM_NUM];
    config_setting_t * devLst;

    int hRS232Down;
    int SeriCom_fd[SERICOM_NUM];
    pthread_t pCom[SERICOM_NUM];

    int (*start)(void);
    int (*exit)(void);
} COM;

struct Sunit {
    int len;
    int type;
    unsigned char buf[255];
};

void COMINIT(void);

int comStart(void);
int comExit(void);

void *comServer(void *);

void *wlserver(void *);

int comControl(int line, int act, int how);
struct _wireless {
    config_setting_t *setting;
    int size;

    int curIndex;
    float value[32];

    pthread_t ptWlcom;
    int SeriCom_fd;

    int (*start)(void);
    void *(*server)(void *);

    int (*index)(unsigned short addr);
} WIREL;

int wlstart(void);
void *wlserver(void *args);
int wlgetIndex(unsigned short addr);

int comdealmsg(int fd);
INT8S comdatasend(INT8U * pBuffer, INT16U iLen);
INT8S ykdatasend(YKDATAIEC pBuffer, INT16U iLen);
void prtsyslog(char *prefix, char *buf, int len);

struct _net {
    int Addr;
    int MasterAddr;

    int iTranCauseL;
    int iCommonAddrL;
    int iInfoAddrL;

    INITIEC101VAULE initiec101Value;

    int lsnSocket;
    int acceptSocket;
    int servport;

    fd_set readfd;
    struct sockaddr_in Local_addr;
    struct sockaddr_in Svr_Addr;

    pthread_t threadkey;
    pthread_t autokey;

    char ip0[64], ip1[64];
    char gateway0[64], gateway1[64];
    char netmask[64], default_gw[64];

    int (*start)(void);
    int (*exit)(void);
    void *(*server)(void *);
    void *(*autosend)(void *);

} NET;

int netstart(void);
int netexit(void);
void *netserver(void *);
void *netautosend(void *);
int netdealmsg(int accSocket);

int netControl(int line, int act, int how);
unsigned int netSend(INT8U * pBuffer, INT16U iLen);

struct _telnet {
    int nTCP_ConnectFlag;

    int servport;
    int lsnSocket;
    int acceptSocket;

    fd_set readfd;
    struct sockaddr_in Local_addr;
    struct sockaddr_in Svr_Addr;

    int destfd;
    int runningflag;
    struct sockaddr_in dest_addr;

    sem_t Sem_Send;
    pthread_t threadkey;
    pthread_t autokey;

    char ip[64];

    void (*start)(void);
    void *(*server)(void *);
} TEL;

void telstart(void);
void *telserver(void *);

struct _config {
    pthread_rwlock_t rwlock;
} CONFIG;

typedef struct {
    int line;
    int timeout;
    int switchs;
} control_elem;

typedef struct {
    int name;
    int type;
    int line0;
    int line1;
} remote_elem;

typedef struct {
    int name;
    int addr;
} wireless_elem;

typedef struct {
    double down;
    double up;
    int board;
    int shift;
    int open;
    int otime;
} analog_elem;

typedef struct {
    double threshold;
    int control;
    int how;
    int t1;
    int t2;
    int times;
    int upper;
    int board;
    int shift;
    int open;
} protect_elem;

int read_control_elem(config_setting_t * parent, int index, control_elem *e);
int read_remote_elem(config_setting_t * parent, int index, remote_elem *e);
int read_wireless_elem(config_setting_t * parent, int index, wireless_elem *e);
int read_analog_elem(config_setting_t * parent, int index, analog_elem *e);
int read_protect_elem(config_setting_t * parent, int index, protect_elem *e);

#endif /* DEFINE_H_ */