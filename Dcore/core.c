/*
 * core.c
 *
 *  Created on: 2014-6-30
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "define.h"
#include "libconfig.h"

#include "../libiec104/libiec104.h"

void whenExit(int sig) {
    switch (sig) {
    case SIGTERM:
        DDBG("捕获退出信号，程序退出。");
        NET.exit();
        COM.exit();
        DDBG("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
        exit(1);
    default:
        DDBG("获取信号%d, （忽略）", sig);
        break;
    }
}

void intiSig() {
    signal(SIGHUP, whenExit);
    signal(SIGINT, whenExit);
    signal(SIGTERM, whenExit);
    signal(SIGQUIT, whenExit);
    DDBG("信号量捕获函数注册成功。");
}

void initArch(config_setting_t *misc) {
    int fd = 0;

    //建立硬件地址影射
    if (-1 == (fd = open("/dev/mem", O_RDWR | O_SYNC))) {
        DDBG("硬件通信初始化失败");
    }
    gFpgaic = (unsigned short *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x14000000);
    if (-1 == (int)gFpgaic) DDBG("内存地址映射失败");
    close(fd);

    //初始化遥测基地址
    ycbase[0] = (unsigned short *)&gFpgaic[0x500];
    ycbase[1] = (unsigned short *)&gFpgaic[0x600];
    ycbase[2] = (unsigned short *)&gFpgaic[0x700];

    int delay, contrast;
    config_setting_lookup_int(misc, "delay", &delay);
    config_setting_lookup_int(misc, "contrast", &contrast);

    //遥信分辨时间
    gFpgaic[0x401] = delay;
    // 关掉所有的灯
    gFpgaic[0x41C] = 0xff;

    //3.3和24V电源初始化
    gpioWrite(105, 1);
    gpioWrite(106, 1);
    // //初始液晶对比度
    // gFpgaic[0x464] = contrast;
    //FPGA版本
    // ANALOG.fpgaver = gFpgaic[0x437];

    // 对时
    judge_time(0);

    //打开复归按钮设备
    config_setting_t * initScript = config_setting_lookup(misc, "init");
    int numScript = config_setting_length(initScript);
    for (int i = 0; i < numScript; i++) {
        const char * cmd = config_setting_get_string_elem(initScript, i);
        DDBG(cmd);
        system(cmd);
    }
}

void initIpc(config_t *cfg) {
    const char *socketName;
    config_lookup_string(cfg, "go.misc.socket", &socketName);
    memset(&IPC.msg, 0, sizeof(IPC.msg));

    IPC.start = ipcstart;
    IPC.server = ipcserver;

    IPC.cfg = cfg;

    //设置一个socket地址结构server_addr,代表服务器internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr)); //把一段内存区的内容全部设置为0
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(8888);

    //创建用于internet的流协议(TCP)socket,用server_socket代表服务器socket
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Create Socket Failed!");
        exit(1);
    }

    //把socket和socket地址结构联系起来
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        printf("Server Bind Port : %d Failed!", 8888);
        exit(1);
    }

    //server_socket用于监听
    if (listen(server_socket, 1)) {
        printf("Server Listen Failed!");
        exit(1);
    }

    IPC.sp = server_socket;
}

void initRemote(config_t *cfg) {

    config_lookup_int(cfg, "go.misc.pressure", &REMOTE.press);
    DDBG("有压鉴别 %d\n", REMOTE.press);
    REMOTE.setting = config_lookup(cfg, "go.remote");
    REMOTE.size = config_setting_length(REMOTE.setting);

    REMOTE.yxoriginal = gFpgaic[0x400];
    pthread_mutex_init(&REMOTE.soelock, NULL);

    REMOTE.start = remstart;
    REMOTE.get = remget;
    REMOTE.add = remadd;
    REMOTE.server = remserver;
    REMOTE.search = remSearch;

    memset((void *)&REMOTE.records, 0, sizeof(SoeUnit));

    int fd = open("/opt/soe.log", O_RDWR);
    if (fd != -1) {
        read(fd, (void *)&REMOTE.soe, sizeof(_Soe));
        close(fd);
    }

    DDBG("遥信模块初始化完毕");
}

void initErr(config_t *cfg) {
    memset(ERINFO.config, 0, sizeof(ERINFO.config));
    memset(ERINFO.msg, 0, sizeof(ERINFO.msg));
    ERINFO.soeHead = REMOTE.soe.info.head;
    ERINFO.ErrorInfoShow = 0;

    ERINFO.isShowing = ErrorShowing;
    ERINFO.Show = ShowError;
    ERINFO.Update = UpdateError;

    int i, j = 0;

    config_setting_t *setting = config_lookup(cfg, "go.remote");
    int length = config_setting_length(setting);

    int n, t, l0, l1;
    for (int i = 0; i < length; ++i) {
        config_setting_t *r = config_setting_get_elem(setting, i);
        config_setting_lookup_int(r, "n", &n);
        config_setting_lookup_int(r, "t", &t);
        config_setting_lookup_int(r, "l0", &l0);
        config_setting_lookup_int(r, "l1", &l1);

        if (t == 8) {
            ERINFO.config[j].upper = n;
            ERINFO.config[j].board = l0;
            ERINFO.config[j].shift = l1;
            ERINFO.config[j].type = 1;
            if (++j > INFONUM) break;
        }
    }

    setting = config_lookup(cfg, "go.protect");
    length = config_setting_length(setting);

    int b, l;
    for (int i = 0; i < length; ++i) {
        config_setting_t *r = config_setting_get_elem(setting, i);
        config_setting_lookup_int(r, "n", &n);
        config_setting_lookup_int(r, "b", &b);
        config_setting_lookup_int(r, "l", &l);

        ERINFO.config[j].upper = n;
        ERINFO.config[j].board = b;
        ERINFO.config[j].shift = l;
        ERINFO.config[j].type = 2;
        if (++j > INFONUM) break;
    }

    ERINFO.config_length = j;
}

void initLed(void) {

    for (int index = 0; index < 7; index++) {
        LED.state[index] = -1;
        LED.nm[index] = 0;
    }

    pthread_cond_init(&LED.sig, NULL);
    pthread_mutex_init(&LED.siglock, NULL);

    LED.start = ledstart;
    LED.server = ledserver;
    LED.on = ledon;
    LED.off = ledoff;
    LED.flash = ledflash;
    LED.checkself = ledcheckself;

    DDBG("面板指示灯模块初始化完毕");
}

void initBattery(config_setting_t *setting) {
    BATTERY.state = 0;

    config_setting_lookup_int(setting, "activate", &BATTERY.activate);
    config_setting_lookup_int(setting, "onbattery", &BATTERY.onbattery);

    pthread_mutex_init(&BATTERY.colock, NULL);
    pthread_cond_init(&BATTERY.sig, NULL);
    pthread_mutex_init(&BATTERY.siglock, NULL);

    BATTERY.start = pwstart;
    BATTERY.server = pwserver;
    BATTERY.begin = pwbegin;
    BATTERY.end = pwend;

    DDBG("电池模块初始化完毕");
}

void initContral(config_t *cfg) {
    CONTRAL.setting = config_lookup(cfg, "go.domain");
    CONTRAL.size = config_setting_length(CONTRAL.setting);

    CONTRAL.pt = 0;
    CONTRAL.id = 0;
    CONTRAL.hw = 0;
    CONTRAL.ac = 0;
    CONTRAL.timeout = 0;

    pthread_mutex_init(&CONTRAL.colock, NULL);
    pthread_cond_init(&CONTRAL.sig, NULL);
    pthread_mutex_init(&CONTRAL.siglock, NULL);

    CONTRAL.start = ctstart;
    CONTRAL.server = ctserver;
    CONTRAL.select = ctselect;
    CONTRAL.action = ctaction;
    CONTRAL.cancel = ctcancel;
    CONTRAL.justdo = ctjustdo;

    CONTRAL.getot = cttimeout;
    CONTRAL.getswitch = ctswitch;

    DDBG("遥控模块初始化完毕");
}

void initLoop(config_t *cfg) {

    const char * ip;
    config_lookup_string(cfg, "go.net.SntpIP", &ip);

    strcpy(LOOP.ip, ip);
    LOOP.step = 0;
    int i;

    for (i = 0; i < 11; i++) {
        LOOP.board_state[i] = get_board_state(i);
        LOOP.board_state_org[i] = LOOP.board_state[i];
    }

    LOOP.start = lostart;
    LOOP.server = loserver;
    LOOP.checkstate = checkstate;
}

void initAnalog(config_t *cfg) {

    ANALOG.setting = config_lookup(cfg, "go.analog");
    ANALOG.size = config_setting_length(ANALOG.setting);

    ANALOG.prsetting = config_lookup(cfg, "go.protect");
    ANALOG.prsize = config_setting_length(ANALOG.prsetting);

    config_lookup_float(cfg, "go.misc.dcCoe1", &ANALOG.dcCoe1);
    config_lookup_float(cfg, "go.misc.dcCoe2", &ANALOG.dcCoe2);
    config_lookup_int(cfg, "go.misc.rebacktime", &ANALOG.rebacktime);
    config_lookup_int(cfg, "go.misc.reback", &ANALOG.reback);
    config_lookup_int(cfg, "go.misc.loop", &ANALOG.loop);
    config_lookup_int(cfg, "go.misc.warn", &ANALOG.warn);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (int i = 0; i < 255; i++) {
        ANALOG.state[i] = 0;
        ANALOG.tv[i] = tv;

        ANALOG.prstate[i] = 0;
        ANALOG.prtimes[i] = 0;
        ANALOG.rebackstate[i] = 0;
        ANALOG.prtv[i] = tv;
        ANALOG.rebacktv[i] = tv;
    }

    int tmp = revTo10(gFpgaic[0x41B]);
    ANALOG.threshold = (tmp / 100) * 0.5;
    if (ANALOG.threshold == 0) {
        ANALOG.threshold = 9.9;
    }
    ANALOG.otime = (tmp % 100) * 100;
    if (ANALOG.otime == 0) {
        ANALOG.otime = 9.9;
    }
    printf("ANALOG.threshold = %f ANALOG.otime = %d\n", ANALOG.threshold , ANALOG.otime);


    for (int i = 0; i < ANALOG.size; i++) {
        int n;
        double unit, larg, dead;
        config_setting_t *r = config_setting_get_elem(ANALOG.setting, i);
        config_setting_lookup_int(r, "n", &n); ANALOG.Id[i] = (unsigned short)n;
        config_setting_lookup_float(r, "f", &unit); ANALOG.Unit[i] = (float)unit;
        config_setting_lookup_float(r, "la", &larg); ANALOG.Larg[i] = (float)larg;
        config_setting_lookup_float(r, "th", &dead); ANALOG.Dead[i] = (float)dead;

    }

    ANALOG.start = anaStart;
    ANALOG.server = anaServer;
    ANALOG.ultralimit = anaUltralimit;
    ANALOG.protection = anaProtection;
    ANALOG.initCoeffect = anaInitCoeffect;
    ANALOG.refresh = anaRefresh;
    ANALOG.point = anaPoint;

    ANALOG.initCoeffect();
}

void initCom(config_setting_t *setting) {

    config_setting_lookup_int(setting, "Addr", &COM.Addr);
    config_setting_lookup_int(setting, "MasterAddr", &COM.MasterAddr);
    config_setting_lookup_int(setting, "iTranCauseL", &COM.iTranCauseL);
    config_setting_lookup_int(setting, "iCommonAddrL", &COM.iCommonAddrL);
    config_setting_lookup_int(setting, "iInfoAddrL", &COM.iInfoAddrL);

    COM.initiec101Value.ucYK_Limit_Time = 40;
    COM.initiec101Value.ucTimeOut_t0 = 30;
    COM.initiec101Value.ucTimeOut_t1 = 15;
    COM.initiec101Value.ucTimeOut_t2 = 10;
    COM.initiec101Value.ucTimeOut_t3 = 20;
    COM.initiec101Value.ucMax_k = 30;
    COM.initiec101Value.ucMax_w = 8;

    COM.devLst = config_setting_lookup(setting, "device");
    COM.devNum = config_setting_length(COM.devLst);

    for (int i = 0; i < COM.devNum; ++i) {
        COM.devName[i] = config_setting_get_string_elem(COM.devLst, i);
    }

    for (int i = 0; i < SERICOM_NUM; i++) {
        COM.SeriCom_fd[i] = -1;
    }

    COM.start = comStart;
}

void initWirel(config_t *cfg) {
    WIREL.setting = config_lookup(cfg, "go.wl");
    WIREL.curIndex = 0;
    WIREL.size = config_setting_length(WIREL.setting);

    WIREL.start = wlstart;
    WIREL.server = wlserver;
    WIREL.index = wlgetIndex;
}

void initNet(config_t *cfg) {

    config_lookup_int(cfg, "go.protocol.Addr", &NET.Addr);
    config_lookup_int(cfg, "go.protocol.MasterAddr", &NET.MasterAddr);

    config_lookup_int(cfg, "go.protocol.iTranCauseL", &NET.iTranCauseL);
    config_lookup_int(cfg, "go.protocol.iInfoAddrL", &NET.iInfoAddrL);
    config_lookup_int(cfg, "go.protocol.iCommonAddrL", &NET.iCommonAddrL);
    config_lookup_int(cfg, "go.net.port", &NET.servport);

    NET.initiec101Value.ucYK_Limit_Time = 40;
    NET.initiec101Value.ucTimeOut_t0 = 30;
    NET.initiec101Value.ucTimeOut_t1 = 15;
    NET.initiec101Value.ucTimeOut_t2 = 10;
    NET.initiec101Value.ucTimeOut_t3 = 20;
    NET.initiec101Value.ucMax_k = 30;
    NET.initiec101Value.ucMax_w = 8;

    // int res = iec10x_create(&NET.initiec101Value, netSend, netControl);
    // if (res == 0) {
    //     DDBG("10x规约初始化失败");
    //     exit(1);
    // }

    NET.lsnSocket = 0;

    NET.start = netstart;
    NET.exit = netexit;
    NET.server = netserver;
    // NET.autosend = netautosend;

}

void initTel(config_t *cfg) {
    const char * ip;
    config_lookup_string(cfg, "go.net.telnIP", &ip);

    strcpy(TEL.ip, ip);

    TEL.nTCP_ConnectFlag = 0;
    TEL.lsnSocket = 0;
    TEL.servport = 2404;

    TEL.start = telstart;
    TEL.server = telserver;

    sem_init(&TEL.Sem_Send, 1, 1);
    sem_post(&TEL.Sem_Send);
}

int main(int argc, char *argv[]) {

    DDBG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

    pthread_rwlock_init(&CONFIG.rwlock, NULL);

    config_t cfg;
    config_setting_t *setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, "./config.cfg")) {
        printf("e\n");
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return (EXIT_FAILURE);
    }

    config_write_file(&cfg, "a.cfg");

    intiSig();
    initArch(config_lookup(&cfg, "go.misc"));
    initIpc(&cfg);
    initRemote(&cfg);
    initLed();
    initBattery(config_lookup(&cfg, "go.misc"));


    initLoop(&cfg);
    initAnalog(&cfg);
    initCom(config_lookup(&cfg, "go.protocol"));
    //initWirel(&cfg);
    initContral(&cfg);
    initNet(&cfg);

    //initTel(&cfg);


    //液晶显示部分作为独立模组，方便程序后续拆分。
    //LCDINIT();
    //KEYINIT();
    //initErr(&cfg);
    //传入password参数和保护列表
    //MENUINIT(&cfg);

    LED.start();
    IPC.start();
    REMOTE.start();
    BATTERY.start();
    CONTRAL.start();
    LOOP.start();

    //KEY.start();
    //MENU.start();


    ANALOG.start();
    COM.start();
    //WIREL.start();
    NET.start();
    // TEL.start();
    //
    // while (1) {
    //     char ch = gpioRead(76);
    //     printf("%c\n", ch);
    //     usleep(100 * 1000);
    // }

    while (1) {
        sleep(5);

        // if (!LED.checkself()) {
        //     DDBG("指示灯模块异常，重启。");
        //     LED.start();
        // }
    }

    return 0;
}
