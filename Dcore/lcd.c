#include "libhd.h"
#include "define.h"
#include "font8x16.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void lcdenable(void) {
    int fd;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        return;
    LCD.lcdAddr = (short *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x14000000);
    LCD.lcdData = LCD.lcdAddr;
    if (LCD.lcdAddr == NULL)
        DDBG("液晶程序内存映射失败");
}

void lcdwrite(void) {

    int i, j, k, m;

    short int l = 0;

    i = j = k = m = 0;
    bzero(LCD.fpgaBuf, sizeof(LCD.fpgaBuf));

    for (i = 0; i < 160; i++) {
        for (j = 0; j < 160; j++) {
            LCD.buf2[i][j] = LCD.LcdBuf[i * 160 + j];
        }
    }

    for (i = 0; i < 160; i++) {
        for (j = 0; j < 160; j++) {
            LCD.buf3[j][-i + 160] = LCD.buf2[i][j];
        }
    }

    for (i = 0; i < 160; i++) {
        for (j = 0; j < 160; j++) {
            LCD.buf4[i * 160 + j] = LCD.buf3[i][j];
        }
    }

    if (LCD.lcdAddr != NULL) {
        for (i = 0; i < 160; i++) {
            for (j = 0; j < 10; j++) {
                for (k = 0; k < 16; k++) {
                    if (LCD.buf4[m + k] == 0xff) {
                        //fprintf(stderr,"%02x ",LcdBuf[m + k]);
                        LCD.fpgaBuf[i][j] |= 1 << k;
                    }
                }
                m += 16;
            }
        }
    }

    for (l = 0; l < 1600; l++) {
        LCD.lcdAddr[LCD_BASE_ADDR] = l;
        LCD.lcdData[LCD_BASE_DATA] = *((short *) LCD.fpgaBuf + l);
    }

}

void lcdopenlight(void) {
    LCD.lcdAddr[LCD_OPEN_LIGHT_ADDR] = 1;
}

void lcdcloselight(void) {
    LCD.lcdAddr[LCD_CLOSE_LIGHT_ADDR] = 0;
}

void lcdsetcontrast(void) {
    LCD.lcdAddr[LCD_CONTRAST_ADDR] = 0xdf;
}

void lcdreadhzbuf(void) {
    int hdrfp;

    hdrfp = open("/opt/12", O_RDONLY);

    if (hdrfp == -1) {
        DDBG("汉字库读取失败，查看/opt是否有[12]文件。");
        exit(1);
    }
    read(hdrfp, LCD.HzDat_12, 198576);
    close(hdrfp);
}

void pixelcolor(Point pt, unsigned char color) {
    unsigned int x = pt.x;

    unsigned int y = pt.y;

    LCD.LcdBuf[y * 160 + x] = (color == 0) ? 0 : 0xff;
    return;
}

void textshow(char *str, Point pos, char rev_flg) {
    unsigned char st[64];

    unsigned int i = 0, m, Len;

    unsigned int k, l, yp, xp;

    unsigned long int rec_offset;

    unsigned long int b1, b2;

    Point pixel;

    unsigned int tmp = 0;       //坐标转换

    tmp = pos.x;
    pos.x = pos.y;
    pos.y = tmp;
    Len = strlen((char *) str);
    for (l = 0; l < Len; l++)
        st[l] = str[l];
    yp = pos.y;

    while (i < Len) {
        if (st[i] < 0x80) {
            b1 = st[i];
            b1 -= 0x20;         //区码
            rec_offset = b1 * 24;
            memcpy((void *) &LCD.HzBuf[0], (void *) &LCD.HzDat_12[rec_offset], 24);
            xp = pos.x;
            k = 0;
            for (m = 0; m < 12; m++) {
                for (k = 0; k < 6; k++) {
                    pixel.x = yp + k;
                    pixel.y = xp;
                    if (rev_flg)
                        LCD.pixelcolor(pixel, ((LCD.HzBuf[m * 2] >> (7 - k)) & 0x01) ^ 0x01);
                    else
                        LCD.pixelcolor(pixel, (LCD.HzBuf[m * 2] >> (7 - k)) & 0x01);
                }
                xp = (xp + 1) % LCM_Y;
            }
            yp = (yp + 6) % LCM_X;
            i++;
        } else {
            b1 = st[i];
            b2 = st[i + 1];
            b1 -= 0xa0;         //区码
            b2 -= 0xa0;         //位码
            rec_offset = (94 * (b1 - 1) + (b2 - 1)) * 24L;      //- 0xb040;
            rec_offset += 96 * 24;
            memcpy((void *) &LCD.HzBuf[0], (void *) &LCD.HzDat_12[rec_offset], 24);
            k = 0;
            xp = pos.x;
            for (m = 0; m < 12; m++) {
                for (k = 0; k < 8; k++) {
                    pixel.x = yp + k;
                    pixel.y = xp;
                    if (rev_flg)
                        LCD.pixelcolor(pixel, ((LCD.HzBuf[m * 2] >> (7 - k)) & 0x01) ^ 0x01);
                    else
                        LCD.pixelcolor(pixel, (LCD.HzBuf[m * 2] >> (7 - k)) & 0x01);
                }
                for (k = 0; k < 4; k++) {
                    pixel.x = yp + k + 8;
                    pixel.y = xp;
                    if (rev_flg)
                        LCD.pixelcolor(pixel, ((LCD.HzBuf[m * 2 + 1] >> (7 - k)) & 0x01) ^ 0x01);
                    else
                        LCD.pixelcolor(pixel, (LCD.HzBuf[m * 2 + 1] >> (7 - k)) & 0x01);
                }
                xp = (xp + 1) % LCM_Y;
            }
            yp = (yp + 12) % LCM_X;
            i += 2;
        }
    }
    return;
}

void lcdtextshow(char *str, int x, int y, char rev_flg) {
    Point TmpPos;

    TmpPos.x = x;
    TmpPos.y = y;
    LCD.textshow(str, TmpPos, rev_flg);
    //pthread_mutex_unlock (&lcd_mutex);
}

void clrrect(Rect rect) {
    int i = 0, j = 0;

    for (j = rect.top; j < rect.bottom; j++) {
        for (i = rect.left; i < rect.right; i++)
            LCD.LcdBuf[j * 160 + i] = 0;
    }
}

void lcdhline(Point start, int x_pos) {
    int i = 0, tmp = 0;

    Point pt;

    pt.y = start.y;
    if (x_pos < start.x) {
        tmp = start.x;
        start.x = x_pos;
        x_pos = tmp;
    }
    for (i = start.x; i < x_pos; i++) {
        pt.x = i;
        LCD.pixelcolor(pt, 1);
    }
}

void lcdrevrect(Rect rect) {
    int i = 0, j = 0;

    for (j = rect.top; j < rect.bottom; j++) {
        for (i = rect.left; i < rect.right; i++) {
            if (LCD.LcdBuf[j * 160 + i] == 0)
                LCD.LcdBuf[j * 160 + i] = 0xff;
            else if (LCD.LcdBuf[j * 160 + i] == 0xff)
                LCD.LcdBuf[j * 160 + i] = 0;
        }
        //LcdBuf[j*160+i] = (~LcdBuf[j*160+i]);
    }
}

void lcdclear(void) {
    Rect all;

    all.bottom = 160;
    all.top = 0;
    all.left = 0;
    all.right = 160;

    LCD.clrrect(all);
}

void LCDINIT(void) {
    memset(&LCD, 0, sizeof(LCD));
    LCD.enable = lcdenable;
    LCD.write = lcdwrite;
    LCD.openlight = lcdopenlight;
    LCD.closelight = lcdcloselight;
    LCD.setcontrast = lcdsetcontrast;
    LCD.readhzbuf = lcdreadhzbuf;
    LCD.show = lcdtextshow;
    LCD.textshow = textshow;
    LCD.pixelcolor = pixelcolor;
    LCD.clrrect = clrrect;
    LCD.drawhline = lcdhline;
    LCD.clear = lcdclear;
    LCD.revrect = lcdrevrect;

    LCD.enable();
    LCD.readhzbuf();
}
