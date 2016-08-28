#include "define.h"

/*
 * 清除状态栏以外的所有屏幕显示信息，以备故障信息显示。
 */
void clearCRTbody() {
    Rect TmpRect;

    TmpRect.left = 0;
    TmpRect.top = 20;
    TmpRect.right = 160;
    TmpRect.bottom = 160;

    LCD.clrrect(TmpRect);
}

//////////////////////////////////////////////
/*
 * 故障弹出显示模块初始化，函数指针初始化、降配置文件中的故障信息导入到ERINFO.config，并记录故障信息数量，用作比对。
 */
//////////////////////////////////////////////

int FoundError(int point) {
    int i = 0;

    for (i = 0; i < ERINFO.config_length; i++) {
        if (point == ERINFO.config[i].upper) {
            DDBG("故障、越限产生，查找错误类型，点号： %d", ERINFO.config[i].upper);
            return i;
        }
    }
    return -1;
}

void UpdateError(void) {
    if (ERINFO.soeHead != REMOTE.soe.info.head) {
        int i = ERINFO.soeHead;

        ERINFO.soeHead = REMOTE.soe.info.head;

        for (; i < ERINFO.soeHead; i++) {
            i = i % YX_SIZE;
            int found = FoundError(REMOTE.soe.unit[i].number);

            if (-1 != found) {
                ERINFO.msg[ERINFO.ShowHead] = ERINFO.config[found];
                ERINFO.ErrorInfoShow = 1;
                LCD.openlight();
                ERINFO.ShowHead = (ERINFO.ShowHead + 1) % INFONUM;
            }
        }
    }
}

//////////////////////////////////////////////
/*
 * 用于按键进程判断是否有故障页面弹出，如果有，将把按键信号传递给故障页面显示线程。
 */
//////////////////////////////////////////////
int ErrorShowing(void) {
    //printf("ERINFO.ErrorInfoShow %d\n", ERINFO.ErrorInfoShow);
    return ERINFO.ErrorInfoShow;
}

void ShowError(void) {
    if (!ERINFO.isShowing())
        return;
    clearCRTbody();
    int i = 0;

    int Firstshow = (ERINFO.ShowHead - 1 + INFONUM) % INFONUM;

    //printf("ERINFO.msg[Firstshow].board %d\n", ERINFO.msg[Firstshow].board);

    for (i = 0; i < INFOPAGE; i++) {
        char showbuf[50];

        bzero(showbuf, sizeof(showbuf));
        sprintf(showbuf, "[%d] %s 第%d块板 第%d路", i, (ERINFO.msg[Firstshow].type == 1) ? "越限" : "故障",
                ERINFO.msg[Firstshow].board, ERINFO.msg[Firstshow].shift);

        if (ERINFO.msg[Firstshow].board == 0)
            break;

        LCD.show(showbuf, 12, 20 + i * 16, 0);

        Firstshow = (Firstshow - 1 + INFONUM) % INFONUM;
    }

    switch (KEY.errsignal()) {
    case 32:
        ERINFO.ErrorInfoShow = 0;
        for (int i = 0; i < INFONUM; ++i) {
            ERINFO.msg[i].board = 0;
        }
        LCD.closelight();
        break;
    }
}
