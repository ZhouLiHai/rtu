#include "NtpTime.h"
#include "shm.h"
#include "libhd.h"
#include "define.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

SYSTEMTIME st;

void GetSystemTime(SYSTEMTIME * stime) {
    struct tm* tm;
    time_t timep;
    time(&timep);
    tm = gmtime(&timep);

    stime->wYear = 1900 + tm->tm_year;
    stime->wMonth = 1 + tm->tm_mon;
    stime->wDay = tm->tm_mday;

    stime->wHour =  tm->tm_hour;
    stime->wMinute = tm->tm_min;
    stime->wSecond = tm->tm_sec;

    DDBG(("DTU time %02d/%02d/%04d, %02d:%02d:%02d"),
         stime->wDay, stime->wMonth, stime->wYear, stime->wHour, stime->wMinute, stime->wSecond);
}

int SetSystemTime(SYSTEMTIME stime) {
    struct tm settimetm;
    DDBG(("Server time %02d/%02d/%04d, %02d:%02d:%02d"), stime.wDay, stime.wMonth, stime.wYear,
         stime.wHour, stime.wMinute, stime.wSecond);

    settimetm.tm_year = stime.wYear - 1900;
    settimetm.tm_mon = stime.wMonth > 0 ? stime.wMonth - 1 : 0;
    settimetm.tm_mday = stime.wDay;
    settimetm.tm_hour = stime.wHour;
    settimetm.tm_min = stime.wMinute;
    settimetm.tm_sec = stime.wSecond;

    settime.tv_sec = mktime(&settimetm);
    settime.tv_usec = 0 * 1000;

    fix_time(settime);

    return 1;
}

/************************************************************************/
/*  函数原型：NTPTIMEPACKET host2network(NTPTIMEPACKET ntp)
 参数类型： NTPTIMEPACKET ntp  本地字节序的时间包
 返回值：   ntp：时间的网络字节序
 基本原理： 在客户端发包之前将时间的本地字节序转换为网络字节序，网络字节序表示整数时，最高位字节在前。                                                    */
/************************************************************************/
NTPTIMEPACKET host2network(NTPTIMEPACKET ntp) { //transform the host byte order to network byte order,before filling the packet
    ntp.m_dwInteger = htonl(ntp.m_dwInteger);
    ntp.m_dwFractional = htonl(ntp.m_dwFractional);
    return ntp;
}

/************************************************************************/
/*  函数原型：NTPTIMEPACKET network2host(NTPTIMEPACKET packet)
 参数类型：NTPTIMEPACKET packet  网络字节序的时间包
 返回值：  packet：时间的网络字节序
 基本原理： 在客户端收包后将时间的网络字节序转换为本地字节序。       */
/************************************************************************/
NTPTIMEPACKET network2host(NTPTIMEPACKET packet) {      //transform the network byte order to host byte order,after receiving the packet
    NTPTIMEPACKET hosttime;

    hosttime.m_dwInteger = ntohl(packet.m_dwInteger);
    hosttime.m_dwFractional = ntohl(packet.m_dwFractional);

    return hosttime;
}

/************************************************************************/
/*  函数原型：NTPTIMEPACKET Systemtime2CNtpTimePacket(SYSTEMTIME st)
 参数类型：SYSTEMTIME st  从本机上获取的系统时间，由年月日时分秒微妙构成。
 返回值：  t：2个32bit的数据包
 基本原理：首先将系统时间的年月日转换成为1个32bit的整数,减去1900年的时间，
 获得秒数赋给包的整数部分，微妙转换为小数部分。            */
/************************************************************************/

NTPTIMEPACKET Systemtime2CNtpTimePacket(SYSTEMTIME st) {
    NTPTIMEPACKET t;

    //Currently this function only operates correctly in
    //the 1900 - 2036 primary epoch defined by NTP

    long Seconds;

    long JD = GetJulianDay(st.wYear, st.wMonth, st.wDay);

    JD -= JAN_1ST_1900;

    //assert(JD >= 0);
    Seconds = JD;
    Seconds = (Seconds * 24) + st.wHour;
    Seconds = (Seconds * 60) + st.wMinute;
    Seconds = (Seconds * 60) + st.wSecond;
    //assert(Seconds <= 0xFFFFFFFF); //NTP Only supports up to 2036
    t.m_dwInteger = Seconds;
    t.m_dwFractional = 0;
    return t;
}

/************************************************************************/
/*  函数原型：SYSTEMTIME NtpTimePacket2SystemTime(NTPTIMEPACKET pNtpTime)
 参数类型：NTPTIMEPACKET pNtpTime  2个32bit组成的时间包
 返回值：  st：系统时间
 基本原理： 时间包转换成年月日时分秒的系统时间。                     */
/************************************************************************/

SYSTEMTIME NtpTimePacket2SystemTime(NTPTIMEPACKET pNtpTime) {
    //Currently this function only operates correctly in
    //the 1900 - 2036 primary epoch defined by NTP

    SYSTEMTIME st;

    ulong s = pNtpTime.m_dwInteger;

    long JD;

    long d;

    long m;

    long j;

    long y;

    st.wSecond = (u16_t)(s % 60);
    s /= 60;
    st.wMinute = (u16_t)(s % 60);
    s /= 60;
    st.wHour = (u16_t)(s % 24);
    s /= 24;
    JD = s + JAN_1ST_1900;

    j = JD - 1721119;
    y = (4 * j - 1) / 146097;
    j = 4 * j - 1 - 146097 * y;
    d = j / 4;
    j = (4 * d + 3) / 1461;
    d = 4 * d + 3 - 1461 * j;
    d = (d + 4) / 4;
    m = (5 * d - 3) / 153;
    d = 5 * d - 3 - 153 * m;
    d = (d + 5) / 5;
    y = 100 * y + j;
    if (m < 10)
        m = m + 3;
    else {
        m = m - 9;
        y = y + 1;
    }

    st.wYear = (u16_t) y;
    st.wMonth = (u16_t) m;
    st.wDay = (u16_t) d;

    return st;
}

/************************************************************************/
/*  函数原型：GetJulianDay(u16_t Year, u16_t Month, u16_t Day)
 参数类型：u16_t Year, u16_t Month, u16_t Day：系统时间中用的16bit无符号整数表示年月日
 返回值：  j：1个32bit的整数
 基本原理： 年月日3个数表示成1个32bit的天数
 */
/************************************************************************/

long GetJulianDay(u16_t Year, u16_t Month, u16_t Day) {
    long c;

    long ya;

    long j;

    long y = (long) Year;

    long m = (long) Month;

    long d = (long) Day;

    if (m > 2)
        m = m - 3;
    else {
        m = m + 9;
        y = y - 1;
    }
    c = y / 100;
    ya = y - 100 * c;
    j = (146097L * c) / 4 + (1461L * ya) / 4 + (153L * m + 2) / 5 + d + 1721119L;
    return j;
}

/************************************************************************/
/*  函数原型：NtpTimePacket2Double(NTPTIMEPACKET pNtpTime)
 参数类型：NTPTIMEPACKET pNtpTime：2个32bit的包
 返回值：  t：双精度时间
 基本原理： 2个32bit的包转换成双精度时间，由整数部分左移32bit后加上分数部分转换成的秒数组成*/
/************************************************************************/
double NtpTimePacket2Double(NTPTIMEPACKET pNtpTime) {
    double t;

    t = (pNtpTime.m_dwInteger) + NtpFraction2Second(pNtpTime.m_dwFractional);
    return t;
}

/************************************************************************/
/*  函数原型：Double2CNtpTimePacket(double dt)
 参数类型：double dt：双精度时间
 返回值：  t：2个32bit的包
 基本原理： 双精度时间转换成2个32bit的包，将双精度数强制转换成32bit无符号长整型，
 构成包的整数部分，将双精度数的后32bit转换成包的小数部分。*/
/************************************************************************/
NTPTIMEPACKET Double2CNtpTimePacket(double dt) { //transform double packet Round trip delay and c-s clock offset to 2 packets of 32 bit
    NTPTIMEPACKET t;

    t.m_dwInteger = (ulong)(dt);        //force dt to 1 integer of 32 bit，drop the fractional part
    //dt -= t.m_dwInteger << 32;
    //只精确到秒,m_dwFractional没用
    t.m_dwFractional = 0;       //(ulong) (dt / NTP_TO_SECOND);
    return t;
}

/************************************************************************/
/*  函数原型：NtpFraction2Second(ulong dwFraction)
 参数类型：ulong dwFraction：时间包的32bit分数部分
 返回值：  秒值，0.xxxxxx s
 基本原理： 由分数部分的值转换成秒值                                 */
/************************************************************************/
double NtpFraction2Second(ulong dwFraction) {
    double d = (double) dwFraction;     //这里就有问题，0.40600000005355102--1743756722

    d *= NTP_TO_SECOND;         //NTP_TO_SECOND十进制的1代表多少秒，乘上十进制的值就得到秒值。0.xxxs
    return d;
}

/************************************************************************/
/*  函数原型：NTPTIMEPACKET myGetCurrentTime()
 参数类型：无
 返回值：  时间包
 基本原理：                                                          */
/************************************************************************/
NTPTIMEPACKET MyGetCurrentTime() {
    SYSTEMTIME st;

    GetSystemTime(&st);
    return Systemtime2CNtpTimePacket(st);
}

/************************************************************************/
/*  函数原型：set_client_time(NTPTIMEPACKET *NewTime)
 参数类型：NTPTIMEPACKET *NewTime：指向时间包类型的指针
 返回值：  bSuccess：FALSE表示获取新时间失败，!FALSE则表示成功
 基本原理：                                                          */
/************************************************************************/
int set_client_time(NTPTIMEPACKET * NewTime) {
    int bSuccess = 0;

    SYSTEMTIME st = NtpTimePacket2SystemTime(*NewTime);

    bSuccess = SetSystemTime(st);
    if (!bSuccess)
        DDBG("Failed in call to set the system time\n");

    return bSuccess;
}

/************************************************************************/
/*  函数原型：NTPTIMEPACKET NewTime()
 参数类型：
 返回值：  time: 更新后的时间，2个32bit数
 基本原理：将客户端当前时间变换成双精度时间值加上计算出的偏移量即得准确时间                                                          */
/************************************************************************/
NTPTIMEPACKET NewTime(double nOffset) {
    NTPTIMEPACKET time;

    double a;

    NTPTIMEPACKET t = MyGetCurrentTime();

    a = NtpTimePacket2Double(t) + nOffset;

    a += 8 * 3600;

    time = Double2CNtpTimePacket(a);    //问题在这个函数处
    return time;
}

/************************************************************************/
/*  函数原型：is_readible(int  bReadible)
 参数类型：int  bReadible
 返回值：TRUE：真 FALSE：假
 基本原理：等待服务器返回数据包，等待超时：返回FALSE，收到包返回TRUE */
/************************************************************************/
int is_readible(int *bReadible) {
    struct timeval timeout;

    fd_set fds;

    int nStatus;

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(m_hNtpClientSocket, &fds);
    nStatus = select(m_hNtpClientSocket + 1, &fds, NULL, NULL, &timeout);
    if (nStatus == SOCKET_ERROR) {
        return 0;
    } else {
        *bReadible = !(nStatus == 0);
        return 1;
    }

}

/************************************************************************/
/*  函数原型：send_sntp_packet(char * pszBuf, int nBuf)
 参数类型：char * pszBuf：指向将发送的数据缓冲区的指针 int nBuf 缓冲区大小
 返回值： 返回是否发送成功的值,0为发送失败，1为发送成功
 基本原理：调用send函数，把要发送的数据复制到操作系统内核的缓冲区 */
/************************************************************************/
int send_sntp_packet(char *pszBuf, int nBuf) {
    int rc = 0;

    //assert(m_hNtpClientSocket != INVALID_SOCKET);
    rc = send(m_hNtpClientSocket, pszBuf, nBuf, 0);

    return (rc != SOCKET_ERROR);
}

/************************************************************************/
/*  函数原型：rcv_sntp_packet(char * pszBuf, int nBuf)
 参数类型：char * pszBuf：指向将发送的数据缓冲区的指针 int nBuf 缓冲区大小
 返回值： 返回是否成功的值,0为接收失败，1为接收成功
 基本原理：调用recv函数，从使用UDP的套接字中接收报文 */
/************************************************************************/
int rcv_sntp_packet(char *pszBuf, int nBuf) {
    int length;

//      assert(m_hNtpClientSocket != INVALID_SOCKET);
    length = recv(m_hNtpClientSocket, pszBuf, nBuf, 0);
    return length;
}

/************************************************************************/
/*  函数原型：close_socket()
 参数类型：
 返回值：
 基本原理：关闭socket */
/************************************************************************/
void close_socket() {
    if (m_hNtpClientSocket != INVALID_SOCKET) {
//              assert(SOCKET_ERROR != close(m_hNtpClientSocket));
        m_hNtpClientSocket = INVALID_SOCKET;
    }
}

/************************************************************************/
/*  函数原型：sntp_client(char *lpszAscii)
 参数类型：char *lpszAscii ：服务器地址
 返回值：  ERROR_NONE 无错误 ，其他错误类型定义时已说明
 基本原理： 初始化套接字，获取服务器地址，填包，发包，等待服务器回复，
 收包，取出数据，根据sntp协议计算往返时间和偏移时间并校正 */
/************************************************************************/
int sntp_client(char *lpszAscii) {
    struct sockaddr_in sockAddr;

    int nPort = 0;

    int nSendSize;

    int bReadable;

    int nConnect;

    int nReceiveSize;

    NtpBasicInfo nbi;

    NtpFullPacket nfp;

    NtpServerResponse response;

    m_hNtpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    nPort = 123;

    if (m_hNtpClientSocket == INVALID_SOCKET) {
        DDBG("Failed to create client socket\n");
        return ERROR_SOCKET;
    }

    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons((short) nPort);
    sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);

    if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *lphost;

        lphost = gethostbyname(lpszAscii);
        if (lphost != NULL)
            sockAddr.sin_addr.s_addr = ((struct in_addr *) lphost->h_addr)->s_addr;
        else {
            DDBG("[DBG] No such service.\n");
            return ERROR_SOCKET_ADD;
        }
    }

    nConnect = connect(m_hNtpClientSocket, (struct sockaddr *) &sockAddr, sizeof(sockAddr));

    if (nConnect == -1) {
        return 0;
    } else {
        nSendSize = sizeof(NtpBasicInfo);
        memset(&nbi, 0, nSendSize);
        nbi.m_LiVnMode = 27;

        nbi.m_TransmitTimestamp = host2network(MyGetCurrentTime());

        if (!send_sntp_packet((char *) &nbi, nSendSize)) {
            DDBG("发送SNTP请求报文出错。");
            return ERROR_NOT_SEND;
        }

        if (!is_readible(&bReadable) || !bReadable) {
            DDBG("接收SNTP应答报文失败。");
            return ERROR_TIMEDOUT;
        }

        nReceiveSize = sizeof(NtpFullPacket);
        memset(&nfp, 0, nReceiveSize);

        response.m_DestinationTime = MyGetCurrentTime();
        if (!rcv_sntp_packet((char *) &nfp, nReceiveSize)) {
            DDBG("未收到SNTP应答报文。");
            return 0;
        }

        response.m_nStratum = nfp.m_Basic.m_Stratum;
        if (response.m_nStratum == 0) {
            DDBG("未收到SNTP应答报文。");
            return 0;
        }
        response.m_nLeapIndicator = (nfp.m_Basic.m_LiVnMode & 0xC0) >> 6;

        response.m_OriginateTime = network2host(nfp.m_Basic.m_OriginateTimestamp);
        response.m_ReceiveTime = network2host(nfp.m_Basic.m_ReceiveTimestamp);
        response.m_TransmitTime = network2host(nfp.m_Basic.m_TransmitTimestamp);

        response.m_RoundTripDelay =
            (NtpTimePacket2Double(response.m_DestinationTime)
             - NtpTimePacket2Double(response.m_OriginateTime))
            - (NtpTimePacket2Double(response.m_ReceiveTime) - NtpTimePacket2Double(response.m_TransmitTime));

        response.m_LocalClockOffset = ((NtpTimePacket2Double(response.m_ReceiveTime)
                                        - NtpTimePacket2Double(response.m_OriginateTime))
                                       + (NtpTimePacket2Double(response.m_TransmitTime)
                                          - NtpTimePacket2Double(response.m_DestinationTime))) / 2;

        NTPTIMEPACKET true_local = NewTime(response.m_LocalClockOffset);

        if (set_client_time(&true_local))
            DDBG("本地时间同步成功。");
        else
            DDBG("本地时间同步失败。");

    }
    close_socket();
    return ERROR_NONE;
}
