/*
 * libhd.h
 *
 *  Created on: 2013-5-22
 *      Author: Lizhen
 */

#ifndef LIBHD_H_
#define LIBHD_H_

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define delay(A) usleep((A)*1000)

#define DDBG(format, args...) \
		do { \
			char info[512]; \
			sprintf(info,format, ##args); \
			syslog(LOG_DEBUG, info, strlen(info)); \
		} while (0)

#define DCRI(format, args...) \
		do { \
			char info[512]; \
			sprintf(info,format, ##args); \
			syslog(LOG_CRIT, info, strlen(info)); \
		} while (0)

#endif
