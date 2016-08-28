#include "libhd.h"
#include "define.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <net/if.h>
#include <arpa/inet.h>

void GetPassWord(char *Data, unsigned char Len, unsigned char Pos, unsigned char Key) {
    char tar = Data[Pos];

    if (Key == 64) {
        switch (tar) {
        case '9':
            tar = 'A';
            break;
        case 'Z':
            tar = 'a';
            break;
        case 'z':
            tar = '0';
            break;
        default:
            tar++;
        }
    }
    if (Key == 128) {
        switch (tar) {
        case '0':
            tar = 'z';
            break;
        case 'a':
            tar = 'Z';
            break;
        case 'A':
            tar = '9';
            break;
        default:
            tar--;
        }
    }
    Data[Pos] = tar;
}


int checkpasswd() {
    int position = 0;
    char password[8];
    memset(password, '0', sizeof(password));

    Rect rect;

    rect.left = 53;
    rect.right = rect.left + 6;
    rect.top = 67;
    rect.bottom = rect.top + 12;

    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "输入密码");
        LCD.show(showbuf, 56, 50, 0);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "%s", password);
        LCD.show(showbuf, 53, 67, 0);

        LCD.revrect(rect);

        switch (KEY.signal()) {
        case 4:
            if (position != 0) {
                position--;
                rect.left = rect.left - 6;
                rect.right = rect.left + 6;
            }
            break;
        case 8:
            if (position != 7) {
                position++;
                rect.left = rect.left + 6;
                rect.right = rect.left;
            }
            break;
        case 16:
            if (!strcmp("00000000", password)) {
                return 1;
            } else {
                bzero(showbuf, sizeof(showbuf));
                sprintf(showbuf, "密码错误");
                LCD.show(showbuf, 56, 84, LCD_NOREV);
                sleep(1);
                return 0;
            }
        case 64:
            GetPassWord(password, 8, position, 64);
            break;
        case 128:
            GetPassWord(password, 8, position, 128);
            break;
        case 32:
            return 0;
        }
        usleep(200 * 1000);
    }
    return 0;
}

int check_board_state(int bIndex) {
    static int board_state[11];

    int result = -1;

    if (bIndex == -1) {
        int i = 0;

        for (i = 0; i < 11; i++) board_state[i] = get_board_state(i);
        return result;
    }
    if (board_state[bIndex] == 0) {
        result = 0;
    } else {
        if (board_state[bIndex] == get_board_state(bIndex))
            result = 1;
        else
            result = 2;
    }
    return result;
}

//////////////////////////////////////////////
/*
 * 菜单处理函数，负责处理整个菜单逻辑。showMainMenu函数是菜单入口。
 */
//////////////////////////////////////////////
void setFirstBeenSelect(int CurLevel, int father) {
    int i = 0;

    for (i = 0; i < MENU.menusize; ++i) {
        if (MENU.menulist[i].level == MENU.CurLevel &&
                MENU.menulist[i].father == father) {
            MENU.menulist[i].selected = 1;
            break;
        }
    }
}

int foundCurLevSelect(int CurLevel, int father) {
    int i = 0;

    for (i = 0; i < MENU.menusize; ++i) {
        if (MENU.menulist[i].level == CurLevel &&
                MENU.menulist[i].father == father && MENU.menulist[i].selected == 1)
            break;
    }
    return i + 1;
}

int foundFirstItem(int CurLevel, int father) {
    int i = 0;

    for (i = 0; i < MENU.menusize; ++i) {
        if (MENU.menulist[i].level == CurLevel && MENU.menulist[i].father == father)
            break;
    }
    return i;
}

int foundFirstShow(int CurLevel, int father) {
    int head = foundFirstItem(CurLevel, father);

    int i = head;

    int path = 0;

    for (i = head; i < MENU.menusize; ++i) {
        if (i - head + 1 > SHOWMAX) ++path;
        if (MENU.menulist[i].selected == 1) break;
    }
    return path > 0 ? i - path + 1 : head;
}

int selectedup(int CurLevel, int father) {
    int i = foundFirstItem(CurLevel, father);

    if (MENU.menulist[i].selected == 1) return 0;
    for (; i < MENU.menusize; ++i) {
        if (MENU.menulist[i].selected == 1 &&
                MENU.menulist[i - 1].level == CurLevel &&
                MENU.menulist[i - 1].father == father) {
            MENU.menulist[i].selected = 0;
            MENU.menulist[i - 1].selected = 1;
            break;
        }
    }
    return 0;
}

int selecteddown(int CurLevel, int father) {
    int i = foundFirstItem(CurLevel, father);

    if (i == MENU.menusize) return 0;
    for (; i < MENU.menusize; ++i) {
        if (MENU.menulist[i].selected == 1 &&
                MENU.menulist[i + 1].level == CurLevel &&
                MENU.menulist[i + 1].father == father) {
            MENU.menulist[i].selected = 0;
            MENU.menulist[i + 1].selected = 1;
            break;
        }
    }
    return 0;
}

void *showMainMenu(void *args) {
    char titlebuf[32];

    memset(titlebuf, 32, 0);
    MENU.CurLevel = 0;
    int fatherItem = 0;

    int firstItem = 0;

    MENU.setFirstBeenSelect(MENU.CurLevel, fatherItem);
    while (1) {
        if (MENU.pass == 0 && (fatherItem == 2 || fatherItem == 3)) {
            MENU.pass = checkpasswd();
            // printf("check -- %d\n", passowrdisok);
            if (MENU.pass == 0) {
                MENU.CurLevel = 0;
                fatherItem = 0;
                MENU.setFirstBeenSelect(MENU.CurLevel, fatherItem);
                continue;
            } else {
                MENU.pass = 1;
            }
        }
        if (fatherItem == 0) {
            MENU.pass = 0;
        }
        firstItem = foundFirstShow(MENU.CurLevel, fatherItem);
        // printf("CurLevel %d , fatherItem %d\n", CurLevel, fatherItem);
        if (MENU.CurLevel == 0) {
            LCD.show((char *)"GK-600D站所终端", 30, 20, 0);
        } else {
            LCD.show(MENU.menulist[fatherItem - 1].title, 42, 20, 0);
        }

        int i = 0;

        for (i = 0; i < SHOWMAX; i++) {
            if (MENU.menulist[firstItem + i].level != MENU.CurLevel ||
                    MENU.menulist[firstItem + i].father != fatherItem)
                break;
            memset(titlebuf, 32, 0);
            sprintf(titlebuf, "%d. %s", MENU.menulist[firstItem + i].titlenum,
                    MENU.menulist[firstItem + i].title);
            LCD.show(titlebuf, 42, SHOWBASE + i * SHOWSTEP,
                     MENU.menulist[firstItem + i].selected ? 1 : 0);
        }

        int select;

        switch (KEY.signal()) {
        case 1:
            MENU.selectedup(MENU.CurLevel, fatherItem);

            break;
        case 2:
            MENU.selecteddown(MENU.CurLevel, fatherItem);

            break;
        case 16:
            select = MENU.foundCurLevSelect(MENU.CurLevel, fatherItem);
            if (MENU.menulist[select - 1].fun != NULL) {
                // printf ("IN----CurLevel %d , fatherItem %d\n", CurLevel,
                // fatherItem);
                MENU.menulist[select - 1].fun();
            } else {
                if (MENU.CurLevel < LEVELMAX) {
                    MENU.menulist[select - 1].selected = 0;
                    ++MENU.CurLevel;
                    fatherItem = select;
                    MENU.setFirstBeenSelect(MENU.CurLevel, fatherItem);
                }
            }
            // printf ("----CurLevel %d , fatherItem %d\n", CurLevel, fatherItem);
            break;
        case 32:
            select = MENU.foundCurLevSelect(MENU.CurLevel, fatherItem);
            if (MENU.CurLevel > LEVELMIN) {
                MENU.menulist[select - 1].selected = 0;
                --MENU.CurLevel;
                fatherItem = MENU.menulist[fatherItem - 1].father;
                MENU.setFirstBeenSelect(MENU.CurLevel, fatherItem);
            }
        }
        usleep(200 * 1000);
    }
    return NULL;
}

/*
 * 划线、显示时间、判断是否需要弹出故障页面。
 */
void *menuserver(void *args) {
    Point line;

    char str_time_year[50];

    struct timeval tv;

    line.x = 6;
    line.y = 16;

    while (1) {
        tv.tv_sec = ANALOG.devicetv / 1000;
        tv.tv_usec = ANALOG.devicetv % 1000;
        sprintf(str_time_year, "%s", &(ctime((time_t *)&tv)[4]));

        LCD.show(str_time_year, 25, 2, 1);
        LCD.drawhline(line, 150);

        ERINFO.Update();
        ERINFO.Show();

        LCD.write();
        LCD.clear();

        usleep(200 * 1000);
    }
    return NULL;
}

void menurun(void) {
    int res;

    pthread_attr_init(&MENU.attr_t);
    pthread_attr_setdetachstate(&MENU.attr_t, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&MENU.attr_t, 256000);
    res = pthread_create(&MENU.thread_main, &MENU.attr_t, MENU.run, NULL);
    if (res != 0) {
        DDBG("液晶程序菜单模块线程启动失败。");
    }

    res = pthread_create(&MENU.thread_server, &MENU.attr_t, MENU.server, NULL);
    if (res != 0) {
        DDBG("液晶程序后台服务模块线程启动失败。");
    }
}

//////////////////////////////////////////////
/*
 * 以下为具体显示的页面，每个页面都陷入页面循环。
 */
//////////////////////////////////////////////
int getyxshowdown(int line) {
    int i = 0;

    for (i = 0; i < 12; i++) {
        line = (line + 1) % 12;
        int chip = line / 4;

        if (ANALOG.Val[chip].Enable == 1) return line;
    }
    return 0;
}

int getyxshowup(int line) {
    int i = 0;

    for (i = 0; i < 12; i++) {
        line = (line - 1) % 12;
        int chip = line / 4;

        if (ANALOG.Val[chip].Enable == 1) return line;
    }
    return 0;
}

void yc_show_5() {
    int line = 0, chip = 0, l = 0;

    _YcValue *mycache = (_YcValue *) & (ANALOG.Val[0]);

    while (1) {
        chip = line / 4;
        l = line % 4;
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "板%d 第%02d回路, 二次值", chip, l + 1);
        LCD.show(showbuf, 20, 24, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "Uab %06.2fV Uab %06.2fV",
                (mycache[chip].Ua > 0.5) ? mycache[chip].Ua : 0,
                (mycache[chip].Uc > 0.5) ? mycache[chip].Uc : 0);
        LCD.show(showbuf, 10, 42, LCD_NOREV);
        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "Ucb %06.2fV Ucb %06.2fV",
                (mycache[chip].Ub > 0.5) ? mycache[chip].Ub : 0,
                (mycache[chip].Ud > 0.5) ? mycache[chip].Ud : 0);
        LCD.show(showbuf, 10, 54, LCD_NOREV);
        switch (l) {
        case 0:
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IA  %06.3fA IB  %06.3fA",
                    (mycache[chip].Ia1 > 0.06) ? mycache[chip].Ia1 : 0,
                    (mycache[chip].Ib1 > 0.06) ? mycache[chip].Ib1 : 0);
            LCD.show(showbuf, 10, 70, LCD_NOREV);
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IC  %06.3fA",
                    (mycache[chip].Ic1 > 0.06) ? mycache[chip].Ic1 : 0);
            LCD.show(showbuf, 10, 82, LCD_NOREV);
            break;
        case 1:
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IA  %06.3fA IB  %06.3fA",
                    (mycache[chip].Ia2 > 0.06) ? mycache[chip].Ia2 : 0,
                    (mycache[chip].Ib2 > 0.06) ? mycache[chip].Ib2 : 0);
            LCD.show(showbuf, 10, 70, LCD_NOREV);
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IC  %06.3fA",
                    (mycache[chip].Ic2 > 0.06) ? mycache[chip].Ic2 : 0);
            LCD.show(showbuf, 10, 82, LCD_NOREV);
            break;
        case 2:
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IA  %06.3fA IB  %06.3fA",
                    (mycache[chip].Ia3 > 0.06) ? mycache[chip].Ia3 : 0,
                    (mycache[chip].Ib3 > 0.06) ? mycache[chip].Ib3 : 0);
            LCD.show(showbuf, 10, 70, LCD_NOREV);
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IC  %06.3fA",
                    (mycache[chip].Ic3 > 0.06) ? mycache[chip].Ic3 : 0);
            LCD.show(showbuf, 10, 82, LCD_NOREV);
            break;
        case 3:
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IA  %06.3fA IB  %06.3fA",
                    (mycache[chip].Ia4 > 0.06) ? mycache[chip].Ia4 : 0,
                    (mycache[chip].Ib4 > 0.06) ? mycache[chip].Ib4 : 0);
            LCD.show(showbuf, 10, 70, LCD_NOREV);
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "IC  %06.3fA",
                    (mycache[chip].Ic4 > 0.06) ? mycache[chip].Ic4 : 0);
            LCD.show(showbuf, 10, 82, LCD_NOREV);
            break;
        }

        float p = 0, q = 0, s = 0;

        Point linehead;

        linehead.x = 10;
        linehead.y = 95;
        LCD.drawhline(linehead, 150);

        switch (l) {
        case 0:
            bzero(showbuf, sizeof(showbuf));
            p = mycache[chip].P1;
            q = mycache[chip].Q1;
            p = (p < 5 && p > -5) ? 0 : p;
            q = (q < 5 && q > -5) ? 0 : q;
            sprintf(showbuf, "P %07.2fW  Q %07.2fW", p, q);
            LCD.show(showbuf, 10, 104, LCD_NOREV);
            s = sqrt(p * p + q * q);
            s = (s == 0) ? 0.001 : s;
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "S %07.2f", s);
            LCD.show(showbuf, 10, 116, LCD_NOREV);
            break;
        case 1:
            bzero(showbuf, sizeof(showbuf));
            p = mycache[chip].P2;
            q = mycache[chip].Q2;
            p = (p < 5 && p > -5) ? 0 : p;
            q = (q < 5 && q > -5) ? 0 : q;
            sprintf(showbuf, "P %07.2fW  Q %07.2fW", p, q);
            LCD.show(showbuf, 10, 104, LCD_NOREV);
            s = sqrt(p * p + q * q);
            s = (s == 0) ? 0.001 : s;
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "S %07.2f", s);
            LCD.show(showbuf, 10, 116, LCD_NOREV);
            break;
        case 2:
            bzero(showbuf, sizeof(showbuf));
            p = mycache[chip].P3;
            q = mycache[chip].Q3;
            p = (p < 5 && p > -5) ? 0 : p;
            q = (q < 5 && q > -5) ? 0 : q;
            sprintf(showbuf, "P %07.2fW  Q %07.2fW", p, q);
            LCD.show(showbuf, 10, 104, LCD_NOREV);
            s = sqrt(p * p + q * q);
            s = (s == 0) ? 0.001 : s;
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "S %07.2f", s);
            LCD.show(showbuf, 10, 116, LCD_NOREV);
            break;
        case 3:
            bzero(showbuf, sizeof(showbuf));
            p = mycache[chip].P4;
            q = mycache[chip].Q4;
            p = (p < 5 && p > -5) ? 0 : p;
            q = (q < 5 && q > -5) ? 0 : q;
            sprintf(showbuf, "P %07.2fW  Q %07.2fW", p, q);
            LCD.show(showbuf, 10, 104, LCD_NOREV);
            s = sqrt(p * p + q * q);
            s = (s == 0) ? 0.001 : s;
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "S %07.2f", s);
            LCD.show(showbuf, 10, 116, LCD_NOREV);
            break;
        }

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "COS:%05.3f   频率:%05.1f", p / s,
                (mycache[chip].Ua > 0.5) ? mycache[chip].Hz1 : 0);
        LCD.show(showbuf, 10, 128, LCD_NOREV);

        memset(mycache, 0, sizeof(mycache));

        switch (KEY.signal()) {
        case 1:
            if (line == 0) break;
            line = getyxshowup(line);
            break;
        case 2:
            if (line == 11) break;
            line = getyxshowdown(line);
            break;
        case 32:
            return;
        }
    }
}

void check_self_9() {
    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "电源      正常");
        LCD.show(showbuf, 15, 30, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "主板      正常");
        LCD.show(showbuf, 15, 45, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "遥测");
        LCD.show(showbuf, 15, 60, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "遥信");
        LCD.show(showbuf, 15, 75, LCD_NOREV);

        char okbuf[256];

        bzero(okbuf, sizeof(okbuf));
        sprintf(okbuf, "正常");

        char errbuf[256];

        bzero(errbuf, sizeof(errbuf));
        sprintf(errbuf, "异常");

        char nobuf[256];

        bzero(nobuf, sizeof(nobuf));
        sprintf(nobuf, "(空)");

        int bindex;

        int starty = 75;

        for (bindex = 0; bindex < 6; bindex++) {
            if (bindex > 2) starty = 90;
            switch (check_board_state(bindex)) {
            case 0:
                LCD.show(nobuf, (60 + 30 * (bindex % 3)), starty, LCD_NOREV);
                break;
            case 1:
                LCD.show(okbuf, (60 + 30 * (bindex % 3)), starty, LCD_NOREV);
                break;
            case 2:
                LCD.show(errbuf, (60 + 30 * (bindex % 3)), starty, LCD_NOREV);
                break;
            }
        }
        for (bindex = 8; bindex < 11; bindex++) {
            switch (check_board_state(bindex)) {
            case 0:
                LCD.show(nobuf, (60 + 30 * (bindex - 8)), 60, LCD_NOREV);
                break;
            case 1:
                LCD.show(okbuf, (60 + 30 * (bindex - 8)), 60, LCD_NOREV);
                break;
            case 2:
                LCD.show(errbuf, (60 + 30 * (bindex - 8)), 60, LCD_NOREV);
                break;
            }
        }

        switch (KEY.signal()) {
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void soe_record_29() {

    int head = REMOTE.soe.info.head;
    int number = REMOTE.soe.info.number;
    const int linenum = 3;

    int pagenum = 0;

    while (1) {
        if (head != REMOTE.soe.info.head) {
            head = REMOTE.soe.info.head;
            number = REMOTE.soe.info.number;
        }

        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "-%03d- 条  -%02d- 页", number, pagenum + 1);
        LCD.show(showbuf, 28, 22, LCD_NOREV);

        int i = 0;

        for (i = 0; i < linenum && (pagenum * linenum + i) < number; ++i) {
            // printf("%d\t#\t%d\n", head, head - i - pagenum * linenum);
            // i = i % 512;
            struct timeval t;

            char timebuf[64];

            bzero(timebuf, sizeof(timebuf));
            t.tv_sec =
                REMOTE.soe.unit[head - i - pagenum * linenum - 1].time / 1000;
            t.tv_usec =
                REMOTE.soe.unit[head - i - pagenum * linenum - 1].time % 1000;

            int m = REMOTE.soe.unit[head - i - pagenum * linenum - 1].time % 1000;

            sprintf(timebuf, "%s", &(ctime((time_t *)&t)[4]));

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "路号 %02d 状态 %02d  [%03d]",
                    REMOTE.soe.unit[head - i - pagenum * linenum - 1].number,
                    REMOTE.soe.unit[head - i - pagenum * linenum - 1].state, m);
            LCD.show(showbuf, 8, 45 + 35 * i, LCD_NOREV);
            LCD.show(timebuf, 8, 45 + 35 * i + 15, LCD_NOREV);
        }

        switch (KEY.signal()) {
        case 1:
            if (pagenum != 0) pagenum--;
            break;
        case 2:
            if (pagenum != 64) pagenum++;
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}


int getstate(int index) {
    int offset = index % 16;
    return REMOTE.state[index / 16] & (1 << offset);
}

void yx_all_28() {
    int page = 0;

    while (1) {
        char showbuf[50];

        if (page != 6) {
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "全遥测状态 板%02d状态", page);
            LCD.show(showbuf, 23, 30, LCD_NOREV);

            char fenbuf[256];

            bzero(fenbuf, sizeof(fenbuf));
            sprintf(fenbuf, "分态");

            char hebuf[256];

            bzero(hebuf, sizeof(hebuf));
            sprintf(hebuf, "合态");

            int index = 0;

            while (index < 12) {
                bzero(showbuf, sizeof(showbuf));
                LCD.show(getstate(page * 12 + index) ? hebuf : fenbuf,
                         23 + (index % 4) * (29), 60 + (index / 4) * 17,
                         getstate(page * 12 + index) ? LCD_REV : LCD_NOREV);
                index++;
            }
        }
        if (page == 6) {
            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "   门节点:%s",
                    REMOTE.state[4] & 0x100 ? "合" : "分");
            LCD.show(showbuf, 30, 28, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, " 活化状态:%s",
                    REMOTE.state[4] & 0x200 ? "合" : "分");
            LCD.show(showbuf, 30, 43, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, " 电池欠压:%s",
                    REMOTE.state[4] & 0x400 ? "合" : "分");
            LCD.show(showbuf, 30, 58, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "交流1失电:%s",
                    REMOTE.state[4] & 0x800 ? "合" : "分");
            LCD.show(showbuf, 30, 73, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "交流2失电:%s",
                    REMOTE.state[4] & 0x1000 ? "合" : "分");
            LCD.show(showbuf, 30, 88, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "     就地:%s",
                    REMOTE.state[4] & 0x2000 ? "合" : "分");
            LCD.show(showbuf, 30, 103, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, " 电源故障:%s",
                    REMOTE.state[4] & 0x4000 ? "合" : "分");
            LCD.show(showbuf, 30, 118, LCD_NOREV);

            bzero(showbuf, sizeof(showbuf));
            sprintf(showbuf, "     远方:%s",
                    REMOTE.state[4] & 0x8000 ? "合" : "分");
            LCD.show(showbuf, 30, 133, LCD_NOREV);
        }

        switch (KEY.signal()) {
        case 1:
            if (page != 0) {
                page--;
            }
            break;
        case 2:
            if (page != 6) {
                page++;
            }
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void sys_info_8() {
    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "软件版本:SV01.200");
        LCD.show(showbuf, 22, 45, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "FPGA版本:%d", 100);
        LCD.show(showbuf, 22, 65, LCD_NOREV);

        int sock_get_ip;

        char ipaddr0[50], ipaddr1[50];

        struct sockaddr_in *sin;

        struct ifreq ifr_ip0, ifr_ip1;

        if ((sock_get_ip = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
            memset(&ifr_ip0, 0, sizeof(ifr_ip0));
            memset(&ifr_ip1, 0, sizeof(ifr_ip1));

            strcpy(ifr_ip1.ifr_name, "eth1");
            strcpy(ifr_ip0.ifr_name, "eth0");

            if (ioctl(sock_get_ip, SIOCGIFADDR, &ifr_ip0) >= 0) {
                sin = (struct sockaddr_in *)&ifr_ip0.ifr_addr;
                struct in_addr __in = sin->sin_addr;

                strcpy(ipaddr0, inet_ntoa(__in));
                sprintf(showbuf, "物理网口:%s", ipaddr0);
                LCD.show(showbuf, 22, 85, LCD_NOREV);
            }
            if (ioctl(sock_get_ip, SIOCGIFADDR, &ifr_ip1) >= 0) {
                sin = (struct sockaddr_in *)&ifr_ip1.ifr_addr;
                struct in_addr __in = sin->sin_addr;

                strcpy(ipaddr1, inet_ntoa(__in));
                sprintf(showbuf, "物理网口:%s", ipaddr1);
                LCD.show(showbuf, 22, 105, LCD_NOREV);
            }
            close(sock_get_ip);
        }

        switch (KEY.signal()) {
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void str_show_25() {
    float a = 0, b = 0, c = 0;

    while (1) {
        a = getDc(0) / ANALOG.dcCoe1;
        if (a < 1) a = 0;
        c = getDc(1) / ANALOG.dcCoe2;
        if (c < 4) c = 0;

        b = (getDc(1) / ANALOG.dcCoe2 - 4) * 6.25;

        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "直流量显示");
        LCD.show(showbuf, 50, 30, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "电压 %4.2f   伏", a);
        LCD.show(showbuf, 26, 60, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "直流 %4.2f   毫安", c);
        LCD.show(showbuf, 26, 77, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "温度 %4.2f   度", (b < 0) ? 0 : b);
        LCD.show(showbuf, 26, 94, LCD_NOREV);

        switch (KEY.signal()) {
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void yk_ctrl_17() {
    int board = 0, line = 1, left_right = 1;

    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "遥控板 [%02d]", board + 1);
        LCD.show(showbuf, 15, 25, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "线路一      线路二");
        LCD.show(showbuf, 15, 40, LCD_NOREV);
        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "分预选");
        LCD.show(showbuf, 15, 58,
                 (!(line - 1) && !(left_right - 1)) ? LCD_REV : LCD_NOREV);
        sprintf(showbuf, "分预选");
        LCD.show(showbuf, 90, 58,
                 (!(line - 1) && !(left_right - 2)) ? LCD_REV : LCD_NOREV);

        sprintf(showbuf, "合预选");
        LCD.show(showbuf, 15, 70,
                 (!(line - 2) && !(left_right - 1)) ? LCD_REV : LCD_NOREV);
        sprintf(showbuf, "合预选");
        LCD.show(showbuf, 90, 70,
                 (!(line - 2) && !(left_right - 2)) ? LCD_REV : LCD_NOREV);

        sprintf(showbuf, "执行");
        LCD.show(showbuf, 15, 82,
                 (!(line - 3) && !(left_right - 1)) ? LCD_REV : LCD_NOREV);
        sprintf(showbuf, "执行");
        LCD.show(showbuf, 90, 82,
                 (!(line - 3) && !(left_right - 2)) ? LCD_REV : LCD_NOREV);

        sprintf(showbuf, "取消");
        LCD.show(showbuf, 15, 94,
                 (!(line - 4) && !(left_right - 1)) ? LCD_REV : LCD_NOREV);
        sprintf(showbuf, "取消");
        LCD.show(showbuf, 90, 94,
                 (!(line - 4) && !(left_right - 2)) ? LCD_REV : LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (line != 1) line--;
            break;
        case 2:
            if (line != 4) line++;
            break;
        case 4:
            if (left_right == 2) left_right = 1;
            break;
        case 8:
            if (left_right == 1) left_right = 2;
            break;
        case 16:
            switch (line) {
            case 1:
                CONTRAL.select(board * 2 + left_right - 1, 0);
                break;
            case 2:
                CONTRAL.select(board * 2 + left_right - 1, 1);
                break;
            case 3:
                CONTRAL.action(board * 2 + left_right - 1, 1);
                break;
            case 4:
                CONTRAL.cancel(board * 2 + left_right - 1, 1);
                break;
            }
            break;
        case 64:
            if (board != 0) board--;
            break;
        case 128:
            if (board != 5) board++;
            break;
        case 32:

            return;
        }
        usleep(200 * 1000);
    }
}

void setup(int n) {
    char showbuf[50];
    int next = 0;
    int position = 1;

    Rect rect;
    rect.left = 50;
    rect.right = rect.left + 24;
    rect.top = 67;
    rect.bottom = rect.top + 12;

    config_setting_t *r = config_setting_get_elem(MENU.prsetting, n);

    double threshold;
    int open, t1, contral, how, times;

    config_setting_lookup_float(r, "th", &threshold);
    config_setting_lookup_int(r, "s", &open);
    config_setting_lookup_int(r, "t", &t1);
    config_setting_lookup_int(r, "c", &contral);
    config_setting_lookup_int(r, "w", &how);
    config_setting_lookup_int(r, "r", &times);

    while (1) {
        if (next == 1) {
            next = 0;
            break;
        }

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "保护状态： %d\n", open);
        LCD.show(showbuf, 24, 55, LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (open == 1) open = 0;
            break;
        case 2:
            if (open == 0) open = 1;
            break;
        case 16:
            next = 1;
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }

    while (1) {
        if (next == 1) {
            next = 0;
            break;
        }
        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "保护定值： %6.2f\n", threshold);
        LCD.show(showbuf, 24, 70, LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (threshold < 30) threshold += 0.1;
            break;
        case 2:
            if (threshold > 0) threshold -= 0.1;
            break;
        case 16:
            next = 1;
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }

    while (1) {
        if (next == 1) {
            next = 0;
            break;
        }
        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "动作时间： %d\n", t1);
        LCD.show(showbuf, 24, 85, LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (t1 < 2000) t1 += 50;
            break;
        case 2:
            if (t1 > 0) t1 -= 50;
            break;
        case 16:
            next = 1;
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }

    while (1) {
        if (next == 1) {
            next = 0;
            break;
        }
        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "重合闸次数： %d\n", times);
        LCD.show(showbuf, 24, 115, LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (times < 3) times += 1;
            break;
        case 2:
            if (times > 0) times -= 1;
            break;
        case 16:
            next = 1;
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }

    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "是否保存设置？");
        LCD.show(showbuf, 42, 50, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "取消   确定");
        LCD.show(showbuf, 50, 67, LCD_NOREV);

        LCD.revrect(rect);

        switch (KEY.signal()) {
        case 4:
            if (position == 0) {
                position = 1;
                rect.left = rect.left - 42;
                rect.right = rect.left + 24;
            }
            break;
        case 8:
            if (position == 1) {
                position = 0;
                rect.left = rect.left + 42;
                rect.right = rect.left + 24;
            }
            break;
        case 16:
            if (position == 0) {
                config_setting_t *subr;
                subr = config_setting_lookup(r, "th");
                config_setting_set_float(r, threshold);

                subr = config_setting_lookup(r, "s");
                config_setting_set_int(r, open);

                subr = config_setting_lookup(r, "t");
                config_setting_set_int(r, t1);

                subr = config_setting_lookup(r, "c");
                config_setting_set_int(r, contral);

                subr = config_setting_lookup(r, "w");
                config_setting_set_int(r, how);

                subr = config_setting_lookup(r, "r");
                config_setting_set_int(r, times);

                return;
            } else
                return;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void pro_show_list() {
    int i = 0;

    char showbuf[50];

    while (1) {

        int open, t1, contral, how, times;
        double threshold;
        config_setting_t *r = config_setting_get_elem(MENU.prsetting, i);
        config_setting_lookup_int(r, "s", &open);
        config_setting_lookup_float(r, "th", &threshold);
        config_setting_lookup_int(r, "t", &t1);
        config_setting_lookup_int(r, "c", &contral);
        config_setting_lookup_int(r, "w", &how);
        config_setting_lookup_int(r, "r", &times);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "保护序列： %d\n", i);
        LCD.show(showbuf, 24, 40, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "保护状态： %d\n", open);
        LCD.show(showbuf, 24, 55, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "保护定值： %6.2f\n", threshold);
        LCD.show(showbuf, 24, 70, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "动作时间： %d\n", t1);
        LCD.show(showbuf, 24, 85, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "动作开关： %d-%d\n", contral,
                how);
        LCD.show(showbuf, 24, 100, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "重合闸次数： %d\n", times);
        LCD.show(showbuf, 24, 115, LCD_NOREV);

        switch (KEY.signal()) {
        case 1:
            if (i + 1 < MENU.prsize) i++;
            break;
        case 2:
            if (i - 1 >= 0) i--;
            break;
        case 16:
            setup(i);
            break;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

void sys_reset_21() {
    int position = 0;

    Rect rect;

    rect.left = 50;
    rect.right = rect.left + 24;
    rect.top = 67;
    rect.bottom = rect.top + 12;

    while (1) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "警告：系统将复位");
        LCD.show(showbuf, 20, 50, LCD_NOREV);

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "确定  取消");
        LCD.show(showbuf, 50, 67, LCD_NOREV);

        LCD.revrect(rect);

        switch (KEY.signal()) {
        case 4:
            if (position == 1) {
                position = 0;
                rect.left = rect.left - 42;
                rect.right = rect.left + 24;
            }
            break;
        case 8:
            if (position == 0) {
                position = 1;
                rect.left = rect.left + 42;
                rect.right = rect.left + 24;
            }
            break;
        case 16:
            if (position == 0) {
                system("reboot");
                sprintf(showbuf, "系统复位成功...");
                LCD.show(showbuf, 30, 90, LCD_NOREV);
                sleep(1);
                return;
            } else
                return;
        case 32:
            return;
        }
        usleep(200 * 1000);
    }
}

//////////////////////////////////////////////
/*
 * 以上为具体显示的页面，每个页面都陷入页面循环。页面函数结束。
 */
//////////////////////////////////////////////

/*
 * 菜单初始化函数：构建菜单结构，初始化函数指针。
 */
void MENUINIT(config_t *cfg) {
    memset(&MENU, 0, sizeof(MENU));

    struct menuitem menulist[] = {
        {1, 0, 0, "显示信息", 1, NULL, 1, 0},
        {2, 0, 0, "参数设置", 2, NULL, 1, 0},
        {3, 0, 0, "控制操作", 3, NULL, 1, 0},
        {4, 1, 1, "遥测", 1, NULL, 1, 0},
        {5, 1, 1, "遥信", 2, NULL, 1, 0},
        {6, 1, 1, "系统信息", 3, sys_info_8, 1, 0},
        {7, 1, 1, "状态自检", 4, check_self_9, 1, 0},
        {8, 1, 2, "保护列表", 1, pro_show_list, 1, 0},
        {9, 1, 3, "遥控", 1, yk_ctrl_17, 1, 0},
        {11, 1, 3, "系统复位", 3, sys_reset_21, 1, 0},
        {12, 2, 4, "线路量显示", 1, yc_show_5, 1, 0},
        {13, 2, 4, "直流量显示", 2, str_show_25, 1, 0},
        {14, 2, 5, "全遥信状态", 1, yx_all_28, 1, 0},
        {15, 2, 5, "SOE事件记录", 2, soe_record_29, 1, 0},
    };

    memcpy(&MENU.menulist, menulist, sizeof(menulist));

    MENU.menusize = sizeof(menulist) / sizeof(struct menuitem);
    config_lookup_int(cfg, "go.misc.password", &MENU.password);

    MENU.prsetting = config_lookup(cfg, "go.protect");
    MENU.prsize = config_setting_length(MENU.prsetting);

    MENU.foundCurLevSelect = foundCurLevSelect;
    MENU.foundFirstItem = foundFirstItem;
    MENU.foundFirstShow = foundFirstShow;
    MENU.selectedup = selectedup;
    MENU.selecteddown = selecteddown;
    MENU.setFirstBeenSelect = setFirstBeenSelect;

    MENU.start = menurun;
    MENU.run = showMainMenu;
    MENU.server = menuserver;

    //初始化各个硬件板状态，以备[系统自检]页面使用。
    check_board_state(-1);
}
