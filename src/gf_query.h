/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 *
 * This file is part of Gfire.
 *
 * Gfire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.
*/

#define MINIQUERY_VERSION   "0.1.1"

#include <stdint.h>

#ifdef _WIN32
    #include <winsock.h>
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include "winerr.h"
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/param.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <sys/times.h>
    #include <sys/timeb.h>
    #include <pthread.h>
#endif

typedef int8_t      i8;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;

#ifdef WIN32
    #define close       closesocket
    #define in_addr_t   u32
    #define TEMPOZ1
    #define TEMPOZ2     GetTickCount()
#else
    #define stristr     strcasestr
    #define stricmp     strcasecmp
    #define strnicmp    strncasecmp
    #define TEMPOZ1     ftime(&timex)
    #define TEMPOZ2     ((timex.time * 1000) + timex.millitm)
#endif

#define QUERYSZ         8192
//#define mychrdup        strdup
#define TIMEOUT         1   // set it to 0 to avoid to wait for other possible replies, not good with gs3

#define QUAKE3_QUERY    "\xff\xff\xff\xff" "getstatus xxx\n"
#define MOH_QUERY       "\xff\xff\xff\xff" "\x02" "getstatus xxx\n"
#define QUAKE2_QUERY    "\xff\xff\xff\xff" "status"
#define HALFLIFE_QUERY  "\xff\xff\xff\xff" "infostring\n\0"
#define DPLAY8_QUERY    "\x00\x02"          /* info/join */     \
                        "\xff\xff"          /* ID tracker */    \
                        "\x02"              /* 01 = need GUID, 02 = no GUID */
#define DOOM3_QUERY     "\xff\xff" "getInfo\0" "\0\0\0\0"
#define ASE_QUERY       "s"
#define GS1_QUERY       "\\status\\"
#define GS2_QUERY       "\xfe\xfd\x00" "\x00\x00\x00\x00"                    "\xff\x00\x00" "\x00"  // 1 for split?
#define GS3_QUERY       "\xfe\xfd\x09" "\x00\x00\x00\x00"
#define GS3_QUERYX      "\xfe\xfd\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\xff\x00\x00" "\x00"  // 1 for split?
#define SOURCE_QUERY    "\xff\xff\xff\xff" "T" "Source Engine Query\0"
#define TRIBES2_QUERY   "\x12\x02\x01\x02\x03\x04"

#define GSLIST_QUERY_T(X)       int (X)(u8 *data, int len, generic_query_data_t *gqd)
#define GSLIST_QUERY_PAR_T(X)   int (X)(u8 *data, int skip)
#define GSLIST_QUERY_VAL_T(X)   int (X)(u8 *data, int ipbit, ipdata_t *ipdata)

#pragma pack(1)             // save tons of memory

typedef struct {
    in_addr_t   ip;     // 0.0.0.0
    u16     port;
    u8      *name;      // string
    u8      *map;       // string
    u8      *type;      // string
    u8      players;    // 0-255
    u8      max;        // 0-255
    u8      *ver;       // string
    u8      *mod;       // string
    u8      *mode;      // string
    u8      ded;        // 0-255
    u8      pwd;        // 0-255
    u8      pb;         // 0-255
    u8      rank;       // 0-255
    u32     ping;
} ipdata_t;

typedef struct {
    u8      nt;         // 0 or 1
    u8      chr;        // the delimiter
    u8      front;      // where the data begins?
    u8      rear;       // where the data ends?
    int     sock;       // scan only
    int     pos;        // scan only
    int     (*func)(u8 *, int, void *gqd);  // GSLIST_QUERY_T(*func);      // scan only
    ipdata_t    *ipdata;    // scan only, if 1
    u8      type;       // query number
    struct sockaddr_in  *peer;
} generic_query_data_t;

#pragma pack()

#define FREEX(X)    freex((void *)&X)