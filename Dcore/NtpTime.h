#ifndef  _NTP_TIME
#define  _NTP_TIME

#include "strings.h"
#include "string.h"
#include "stdio.h"
#include "NtpTime.h"
#include "math.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#define  ERROR_NONE            0
#define  ERROR_NOT_FIND_SERVER 1
#define  ERROR_SOCKET          3        //get socket error
#define  ERROR_SOCKET_ADD      4        //get host address error
#define  ERROR_NOT_SEND        5        //send packet error
#define  ERROR_TIMEDOUT        6        //wait retrieve packet timed out

#define  SOCKET_ERROR -1
#define  INVALID_SOCKET -1
int m_hNtpClientSocket;

int g_bSynchroniseClock;

typedef signed char s8_t;

typedef unsigned char u8_t;

typedef unsigned short u16_t;

typedef int s32_t;

typedef unsigned int u32_t;

typedef unsigned long ulong;

typedef struct
{
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
} SYSTEMTIME;

static double NTP_TO_SECOND = (((double) 1.0) / 0xFFFFFFFF);

static long JAN_1ST_1900 = 2415021;

typedef struct
{
    ulong m_dwInteger;
    ulong m_dwFractional;
} NTPTIMEPACKET;

//Representation of an NTP timestamp
//Lookup table to convert from Milliseconds (hence 1000 Entries)
//to fractions of a second expressed as a ulong

typedef struct _NtpServerResponse_
{
    int m_nLeapIndicator;       //0: no warning
    //1: last minute in day has 61 seconds
    //2: last minute has 59 seconds
    //3: clock not synchronized

    int m_nStratum;             //0: unspecified or unavailable
    //1: primary reference (e.g., radio clock)
    //2-15: secondary reference (via NTP or SNTP)
    //16-255: reserved

    NTPTIMEPACKET m_OriginateTime;      //Time when the request was sent from the client to the SNTP server
    NTPTIMEPACKET m_ReceiveTime;        //Time when the request was received by the server
    NTPTIMEPACKET m_TransmitTime;       //Time when the server sent the request back to the client
    NTPTIMEPACKET m_DestinationTime;    //Time when the reply was received by the client
    double m_RoundTripDelay;    //Round trip time in seconds
    double m_LocalClockOffset;  //Local clock offset relative to the server
} NtpServerResponse;

//The mandatory part of an NTP packet
typedef struct _NtpBasicInfo_
{
    u8_t m_LiVnMode;
    u8_t m_Stratum;
    char m_Poll;
    char m_Precision;
    long m_RootDelay;
    long m_RootDispersion;
    char m_ReferenceID[4];
    //4 timestamp in the protocol is 64 bit
    NTPTIMEPACKET m_ReferenceTimestamp;
    NTPTIMEPACKET m_OriginateTimestamp;
    NTPTIMEPACKET m_ReceiveTimestamp;
    NTPTIMEPACKET m_TransmitTimestamp;
} NtpBasicInfo;

//The optional part of an NTP packet
typedef struct _NtpAuthenticationInfo_
{
    unsigned long m_KeyID;
    u8_t m_MessageDigest[16];
} NtpAuthenticationInfo;

//The Full NTP packet
typedef struct _NtpFullPacket_
{
    NtpBasicInfo m_Basic;
    NtpAuthenticationInfo m_Auth;
} NtpFullPacket;

//General functions

SYSTEMTIME NtpTimePacket2SystemTime (NTPTIMEPACKET pNtpTime);

double NtpTimePacket2Double (NTPTIMEPACKET pNtpTime);

NTPTIMEPACKET Double2CNtpTimePacket (double f);

NTPTIMEPACKET Systemtime2CNtpTimePacket (SYSTEMTIME st);

NTPTIMEPACKET MyGetCurrentTime ();

NTPTIMEPACKET NewTime (double nOffset);

double NtpFraction2Second (ulong dwFraction);

long GetJulianDay (u16_t Year, u16_t Month, u16_t Day);

NTPTIMEPACKET host2network (NTPTIMEPACKET ntp);

NTPTIMEPACKET network2host (NTPTIMEPACKET packet);

////////////////////////////////////////////////////////////////////////// 

int send_sntp_packet (char *pszBuf, int nBuf);

int is_readible (int *bReadible);

int rcv_sntp_packet (char *pszBuf, int nBuf);

void close_socket ();

int set_client_time (NTPTIMEPACKET * NewTime);

#endif
