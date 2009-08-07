/*
    Copyright (C) 2009, Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

#include "gf_query.h"

#ifdef _WIN32
	#define close       closesocket
#endif // _WIN32

void freex(void **buff) {
    if(!buff || !*buff) return;
    free(*buff);
    *buff = NULL;
}

int timeout(int sock, int sec) {
    struct  timeval tout;
    fd_set  fd_read;
    int     err;

    tout.tv_sec  = sec;
    tout.tv_usec = 0;
    FD_ZERO(&fd_read);
    FD_SET(sock, &fd_read);
    err = select(sock + 1, &fd_read, NULL, NULL, &tout);
    if(err < 0) return(-1);
    if(!err) return(-1);
    return(0);
}

int udpsocket(void) {
    static const struct linger  ling = {1,1};
    int     sd;

    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sd < 0) return(-1);
    setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
    return(sd);
}

u8 *mychrdup(u8 *str) {
    if(!str || !str[0]) return(NULL);
	return (u8*)strdup((char*)str);
}

void *mychrdupn(u8 *str, int n) {
    u8      *ret;

    if(!str || !str[0]) return(NULL);
    ret = malloc(n + 1);
    if(!ret) return(NULL);
	strncpy((char*)ret, (char*)str, n);
    ret[n] = 0;
    return(ret);
}

// multi_query.h

void show_unicode(u8 *data, u32 size) {
    u8      *limit = data + size;

    while(*data && (data < limit)) {
        fputc(*data, stdout);
        data += 2;
    }
    fputc('\n', stdout);
}


GSLIST_QUERY_PAR_T(handle_query_par) {
    u8      *p;

	p = (u8*)strchr((char*)data, '_');
    if(p) data = p + 1;     /* "sv_" and "si_" */

#define K(x)  !stricmp((char*)data, x)
    if(!(skip & 2) && (
        K("hostname")   || K("name"))) {
        return(1);

    } else if(!(skip & 4) && (
        K("mapname")    || K("map"))) {
        return(2);

    } else if(!(skip & 8) && (
        K("gametype")   || K("gametypeLocal"))) {
        return(3);

    } else if(!(skip & 16) && (
        K("numplayers") || K("clients")  || K("privateClients"))) {
        return(4);

    } else if(!(skip & 32) && (
        K("maxplayers") || K("maxclients"))) {
        return(5);

    } else if(!(skip & 64) && (
        K("gamever")    || K("version")  || K("protocol"))) {
        return(6);

    } else if(!(skip & 128) && (
        K("password")   || K("needpass") || K("usepass")  || K("passworden") || K("pwd") || K("pswrd")  || K("pw"))) {
        return(7);

    } else if(!(skip & 256) && (
        K("dedicated")  || K("type")     || K("ded")      || K("dedic")      || K("serverDedicated") || K("bIsDedicated"))) {
        return(8);

    } else if(!(skip & 512) && (
        K("hostport"))) {
        return(9);

    } else if(!(skip & 1024) && (
        K("game")       || K("gamedir")  || K("modname")  || K("mods")       || K("mod"))) {
        return(10);

    } else if(!(skip & 2048) && (
        K("pb")         || K("punkbuster"))) {
        return(11);

    } else if(!(skip & 4096) && (
        K("gamemode"))) {
        return(12);

    } else if(!(skip & 8192) && (
        K("ranked")     || K("rated"))) {
        return(13);
    }

#undef K
    return(0);
}

GSLIST_QUERY_VAL_T(handle_query_val) {
    int     ret = 0;

    if(!*data) return(-1);
    switch(ipbit) {
        case  1:             ipdata->name    = mychrdup(data);      break;
        case  2:             ipdata->map     = mychrdup(data);      break;
        case  3:             ipdata->type    = mychrdup(data);      break;
		case  4: if(data[0]) ipdata->players = atoi((char*)data);   break;
		case  5: if(data[0]) ipdata->max     = atoi((char*)data);   break;
        case  6:             ipdata->ver     = mychrdup(data);      break;
        case  7: if(data[0]) ipdata->pwd     = *data;               break;
        case  8: if(data[0]) ipdata->ded     = *data;               break;
		case  9: if(data[0]) ipdata->port    = htons(atoi((char*)data));   break;
        case 10:             ipdata->mod     = mychrdup(data);      break;
        case 11: if(data[0]) ipdata->pb      = *data;               break;
        case 12:             ipdata->mode    = mychrdup(data);      break;
        case 13: if(data[0]) ipdata->rank    = *data;               break;
        default: ret = -1; break;
    }
    return(ret);
}

GSLIST_QUERY_PAR_T(print_query_par) {
    printf("%28s ", data);
    return(0);
}

GSLIST_QUERY_VAL_T(print_query_val) {
    printf("%s\n", data);
    return(0);
}

GSLIST_QUERY_T(generic_query) {
    GSLIST_QUERY_PAR_T(*par);   /* the usage of function pointers */
    GSLIST_QUERY_VAL_T(*val);   /* avoids to use many if() */
    ipdata_t    *ipdata;
    u32     chall;
    int     ipbit   = 0,
            skip    = 0,
            nt;
    u8      minime[32],
            *next,
            *tmp,
            *extra_data,
            *limit;

    if((gqd->type == 11) && (len < 20)) {   // Gamespy 3
		chall = atoi((char*)(data + 5));
        memcpy(minime, GS3_QUERYX, sizeof(GS3_QUERYX) - 1);
        minime[7]  = chall >> 24;
        minime[8]  = chall >> 16;
        minime[9]  = chall >>  8;
        minime[10] = chall;
        sendto(gqd->sock, minime, sizeof(GS3_QUERYX) - 1, 0, (struct sockaddr *)gqd->peer, sizeof(struct sockaddr_in));
        return(-1);
    }

    limit  = data + len - gqd->rear;
    *limit = 0;
    data   += gqd->front;
    nt     = gqd->nt;

    if(gqd->ipdata) {
        ipdata = &gqd->ipdata[gqd->pos];
        par    = handle_query_par;
        val    = handle_query_val;
    } else {
        ipdata = NULL;
        par    = print_query_par;
        val    = print_query_val;
    }

    extra_data = NULL;
    for(next = data; (data < limit) && next; data = next + 1, nt++) {
		next = (u8*)strchr((char*)data, gqd->chr);
        if(next) *next = 0;

                /* work-around for Quake games with players at end, only for -Q */
        if(gqd->ipdata) {
            if(nt & 1) {
                if(skip && (gqd->chr != '\n')) {
                    for(tmp = data; *tmp; tmp++) {
                        if(*tmp != '\n') continue;
                        *tmp = 0;
                        next = limit;
                        break;
                    }
                }
            } else {
                skip |= (1 << ipbit);
            }
        }

        if(nt & 1) {
            val(data, ipbit, ipdata);
            ipbit = 0;
        } else {
			if(!*data || !strcmp((char*)data, "queryid") || !strcmp((char*)data, "final")) break;
            ipbit = par(data, skip);
            skip |= (1 << ipbit);
        }
    }

    if(gqd->ipdata) {
        // country/geoip not handled
    }

    return(0);
}

GSLIST_QUERY_T(source_query) {
    ipdata_t   *ipdata;

    data += 5;
    if(gqd->ipdata) {
        ipdata = &gqd->ipdata[gqd->pos];

		data += strlen((char*)data) + 1;
		ipdata->name    = mychrdup(data);               data += strlen((char*)data) + 1;
		ipdata->map     = mychrdup(data);               data += strlen((char*)data) + 1;
		ipdata->mod     = mychrdup(data);               data += strlen((char*)data) + 1;
		ipdata->type    = mychrdup(data);               data += strlen((char*)data) + 1;
        ipdata->players = *data++;
        ipdata->max     = *data++;
        ipdata->ver     = malloc(6);
		sprintf((char*)ipdata->ver, "%hu", *data++);
        ipdata->ded     = (*data++ == 'd') ? '1' : '0';
        data++;
        ipdata->pwd     = *data;

    } else {
		printf("%28s %s\n",        "Address",          data);   data += strlen((char*)data) + 1;
		printf("%28s %s\n",        "Hostname",         data);   data += strlen((char*)data) + 1;
		printf("%28s %s\n",        "Map",              data);   data += strlen((char*)data) + 1;
		printf("%28s %s\n",        "Mod",              data);   data += strlen((char*)data) + 1;
		printf("%28s %s\n",        "Description",      data);   data += strlen((char*)data) + 1;
        printf("%28s %hhu/%hhu\n", "Players",          data[0], data[1]);  data += 2;
        printf("%28s %hhu\n",      "Version",          *data++);
        printf("%28s %c\n",        "Server type",      *data++);
        printf("%28s %c\n",        "Server OS",        *data++);
        printf("%28s %hhu\n",      "Password",         *data++);
        printf("%28s %hhu\n",      "Secure server",    *data);
    }
    return(0);
}

GSLIST_QUERY_T(tribes2_query) {
    ipdata_t    *ipdata;
    int     sz,
            tmp;

#define TRIBES2STR(x)   sz = *data++;   \
                        tmp = data[sz]; \
                        data[sz] = 0;   \
                        x;              \
                        data += sz;     \
                        *data = tmp;

    data += 6;
    if(gqd->ipdata) {
        ipdata = &gqd->ipdata[gqd->pos];

        TRIBES2STR((ipdata->mod  = mychrdup(data)));
        TRIBES2STR((ipdata->type = mychrdup(data)));
        TRIBES2STR((ipdata->map  = mychrdup(data)));
        data++;
        ipdata->players = *data++;
        ipdata->max     = *data++;

    } else {
        TRIBES2STR(printf("%28s %s\n", "Mod",       data));
        TRIBES2STR(printf("%28s %s\n", "Game type", data));
        TRIBES2STR(printf("%28s %s\n", "Map",       data));
        data++;
        printf("%28s %hhu/%hhu\n", "Players", data[0], data[1]);   data += 2;
        printf("%28s %hhu\n",      "Bots",    *data);              data++;
        printf("%28s %hu\n",       "CPU",     *(int16_t *)data);     data += 2;
        TRIBES2STR(printf("%28s %s\n", "Info",      data));
    }
#undef TRIBES2STR
    return(0);
}

GSLIST_QUERY_T(dplay8info) {
    ipdata_t    *ipdata;
    int     i;
    struct myguid {
        u32     g1;
        u16     g2;
        u16     g3;
        u8      g4;
        u8      g5;
        u8      g6;
        u8      g7;
        u8      g8;
        u8      g9;
        u8      g10;
        u8      g11;
    };
    struct dplay8_t {
        u32     pcklen;
        u32     reservedlen;
        u32     flags;
        u32     session;
        u32     max_players;
        u32     players;
        u32     fulldatalen;
        u32     desclen;
        u32     dunno1;
        u32     dunno2;
        u32     dunno3;
        u32     dunno4;
        u32     datalen;
        u32     appreservedlen;
        struct  myguid  instance_guid;
        struct  myguid  application_guid;
    } *dplay8;

    if(gqd->ipdata) {
        ipdata = &gqd->ipdata[gqd->pos];
    } else {
        ipdata = NULL;
    }

    if(len < (sizeof(struct dplay8_t) + 4)) {
        //printf("\nError: the packet received seems invalid\n");
        return(0);
    }

    dplay8 = (struct dplay8_t *)(data + 4);

    if(dplay8->session) {
        if(gqd->ipdata) {
            ipdata->type = malloc(64);
            ipdata->type[0] = 0;
			if(dplay8->session & 1)   strcat((char*)ipdata->type, "client-server ");
			if(dplay8->session & 4)   strcat((char*)ipdata->type, "migrate_host ");
			if(dplay8->session & 64)  strcat((char*)ipdata->type, "no_DPN_server ");
			if(dplay8->session & 128) strcat((char*)ipdata->type, "password ");
        } else {
            printf("\nSession options:     ");
            if(dplay8->session & 1)   printf("client-server ");
            if(dplay8->session & 4)   printf("migrate_host ");
            if(dplay8->session & 64)  printf("no_DPN_server ");
            if(dplay8->session & 128) printf("password ");
        }
    }

    if(gqd->ipdata) {
        ipdata->players = dplay8->players;
        ipdata->max     = dplay8->max_players;
    } else {
        printf("\n"
            "Max players          %u\n"
            "Current players      %u\n",
            dplay8->max_players,
            dplay8->players);
    }

    if(gqd->ipdata) {
    } else {
        printf("\n"
            "Instance GUID        %08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            dplay8->instance_guid.g1, dplay8->instance_guid.g2, dplay8->instance_guid.g3, dplay8->instance_guid.g4,
            dplay8->instance_guid.g5, dplay8->instance_guid.g6, dplay8->instance_guid.g7, dplay8->instance_guid.g8,
            dplay8->instance_guid.g9, dplay8->instance_guid.g10, dplay8->instance_guid.g11);

        printf("\n"
            "Application GUID     %08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx\n",
            dplay8->application_guid.g1, dplay8->application_guid.g2, dplay8->application_guid.g3, dplay8->application_guid.g4,
            dplay8->application_guid.g5, dplay8->application_guid.g6, dplay8->application_guid.g7, dplay8->application_guid.g8,
            dplay8->application_guid.g9, dplay8->application_guid.g10, dplay8->application_guid.g11);
    }

        // removed any security check here

    if(gqd->ipdata) {
        ipdata->name = malloc(128 + 1);
        for(i = 0; i < ((len - dplay8->fulldatalen) >> 1); i++) {
            if(i >= 128) break;
            ipdata->name[i] = data[4 + dplay8->fulldatalen + (i << 1)];
            if(!ipdata->name[i]) break;
        }
        ipdata->name[i] = 0;
    } else {
        printf("\nGame description/Session name:\n\n  ");
        show_unicode(
            data + 4 + dplay8->fulldatalen,
            len - dplay8->fulldatalen);
    }

    if(dplay8->appreservedlen) {
        //printf("\nHexdump of the APPLICATION RESERVED data (%u) sent by the server:\n\n",
            //dplay8->appreservedlen);
        //show_dump(
            //data + 4 + dplay8->datalen,
            //dplay8->appreservedlen,
            //stdout);
    }

    if(dplay8->reservedlen) {
        //printf("\nHexdump of the RESERVED data (%u) sent by the server:\n\n",
            //dplay8->reservedlen);
        //show_dump(
            //data + 4 + dplay8->fulldatalen + dplay8->desclen,
            //dplay8->reservedlen,
            //stdout);
    }

    return(0);
}

GSLIST_QUERY_T(ase_query) {
    ipdata_t    *ipdata;
    int     num = 0;
    u8      *limit;

    if(memcmp(data, "EYE1", 4)) {
        //fwrite(data + 1, len - 1, 1, stdout);
        //fputc('\n', stdout);
        return(0);
    }
    limit = data + len;
    data += 4;

    if(gqd->ipdata) {
        ipdata = &gqd->ipdata[gqd->pos];
    } else {
        ipdata = NULL;
    }

    while((data < limit) && (*data > 1)) {
        if(gqd->ipdata) {
            switch(num) {
                case 0: ipdata->type    = mychrdupn(data + 1, *data - 1);   break;
				case 1: ipdata->port    = atoi((char*)(data + 1));          break;
                case 2: ipdata->name    = mychrdupn(data + 1, *data - 1);   break;
                case 3: ipdata->mode    = mychrdupn(data + 1, *data - 1);   break;
                case 4: ipdata->map     = mychrdupn(data + 1, *data - 1);   break;
                case 5: ipdata->ver     = mychrdupn(data + 1, *data - 1);   break;
				case 6: ipdata->pwd     = atoi((char*)(data + 1));          break;
				case 7: ipdata->players = atoi((char*)(data + 1));          break;
				case 8: ipdata->max     = atoi((char*)(data + 1));          break;
                default: break;
            }

        } else {
            switch(num) {
                case 0: printf("\n%28s ", "Running game"); break;
                case 1: printf("\n%28s ", "Game port");    break;
                case 2: printf("\n%28s ", "Server name");  break;
                case 3: printf("\n%28s ", "Battle mode");  break;
                case 4: printf("\n%28s ", "Map name");     break;
                case 5: printf("\n%28s ", "Version");      break;
                case 6: printf("\n%28s ", "Password");     break;
                case 7: printf("\n%28s ", "Players");      break;
                case 8: printf("\n%28s ", "Max players");  break;
                default: {
                    if(num & 1) fputc('\n', stdout);
                    } break;
            }
            fwrite(data + 1, *data - 1, 1, stdout);
        }
        data += *data;
        num++;
    }

    if(gqd->ipdata) {
    } else {
        fputc('\n', stdout);
    }
    num = limit - data;
    //if(num > 1) {
        //printf("\nHex dump of the rest of the packet:\n", stdout);
        //show_dump(data, num, stdout);
    //}
    return(0);
}

u8 *switch_type_query(int type, int *querylen, generic_query_data_t *gqd, GSLIST_QUERY_T(**func), int info) {
    u8      *query,
            *msg;

    if(gqd) memset(gqd, 0, sizeof(generic_query_data_t));

#define ASSIGN(t,x,y,n,c,f,r,z) {                           \
			msg   = (u8*)x;                                      \
			query = (u8*)y;                                      \
            if(querylen) *querylen  = sizeof(y) - 1;        \
            if(gqd) {                                       \
                memset(gqd, 0, sizeof(generic_query_data_t)); \
                gqd->type  = t;                             \
                gqd->nt    = n;                             \
                gqd->chr   = c;                             \
                gqd->front = f;                             \
                gqd->rear  = r;                             \
            }                                               \
            if(func) *func = z;                             \
        } break;

    switch(type) {
        case  0: ASSIGN(0,  "Gs \\status\\",  GS1_QUERY,      1, '\\',  0, 0, generic_query);
        case  1: ASSIGN(1,  "Quake 3 engine", QUAKE3_QUERY,   1, '\\',  4, 0, generic_query);
        case  2: ASSIGN(2,  "Medal of Honor", MOH_QUERY,      1, '\\',  5, 0, generic_query);
        case  3: ASSIGN(3,  "Quake 2 engine", QUAKE2_QUERY,   1, '\\',  4, 0, generic_query);
        case  4: ASSIGN(4,  "Half-Life",      HALFLIFE_QUERY, 1, '\\', 23, 0, generic_query);
        case  5: ASSIGN(5,  "DirectPlay 8",   DPLAY8_QUERY,   0, '\0',  0, 0, dplay8info);
        case  6: ASSIGN(6,  "Doom 3 engine",  DOOM3_QUERY,    0, '\0', 23, 8, generic_query);
        case  7: ASSIGN(7,  "All-seeing-eye", ASE_QUERY,      0, '\0',  0, 0, ase_query);
        case  8: ASSIGN(8,  "Gamespy 2",      GS2_QUERY,      0, '\0',  5, 0, generic_query);
        case  9: ASSIGN(9,  "Source",         SOURCE_QUERY,   0, '\0',  0, 0, source_query);
        case 10: ASSIGN(10, "Tribes 2",       TRIBES2_QUERY,  0, '\0',  0, 0, tribes2_query);
        case 11: ASSIGN(11, "Gamespy 3",      GS3_QUERY,      0, '\0',  5, 0, generic_query);
        default: return(NULL);
    }
#undef ASSIGN

    switch(info) {
        //case 1: fprintf(stderr, "- query type: %s\n", msg); break;  /* details */
        //case 2: printf(" %2d %-22s", type, msg);            break;  /* list */
        case 3: return(msg);                                break;  /* type */
    }
    return(query);
}

/* this is the code called when you use the -i, -I or -d option */
int miniquery(ipdata_t *gip, int type, in_addr_t ip, u16 port) {
#ifdef WIN32
#else
    struct  timeb   timex;
#endif
    GSLIST_QUERY_T(*func);
    generic_query_data_t  gqd;
    struct  sockaddr_in peer;
    u32     start_ping  = -1;
    int     sd,
            len,
            querylen,
            pcks        = 0;
    u8      buff[QUERYSZ],
            *query;

    query = switch_type_query(type, &querylen, &gqd, (void *)&func, 1);
    if(!query) return(pcks);

    peer.sin_addr.s_addr = ip;
    peer.sin_port        = htons(port);
    peer.sin_family      = AF_INET;

    //fprintf(stderr, "- target   %s : %hu\n",
        //inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

    sd = udpsocket();
    if(sd < 0) return(-1);

    gqd.sock     = sd;
    gqd.func     = (void *)func;
    gqd.peer     = &peer;

    gqd.ipdata   = gip;
    if(gip) {
        memset(gip, 0, sizeof(ipdata_t));
        gip->ip    = ip;
        gip->port  = port;
        TEMPOZ1;
        start_ping = TEMPOZ2;
        gip->ping  = -1;
    }

    sendto(sd, query, querylen, 0, (struct sockaddr *)&peer, sizeof(struct sockaddr_in));
    while(!timeout(sd, TIMEOUT)) {
        len = recvfrom(sd, buff, sizeof(buff) - 1, 0, NULL, NULL);
        if(len > 0) {
            pcks++;
            if(gip && (gip->ping == -1)) {
                TEMPOZ1;
                gip->ping = TEMPOZ2 - start_ping;
            }
            (*func)(buff, len, &gqd);
        }
	// break;
    }
    close(sd);
    return(pcks);
}

void ipdata_free(ipdata_t *ipdata) {
    FREEX(ipdata->name);
    FREEX(ipdata->map);
    FREEX(ipdata->type);
    FREEX(ipdata->ver);
    FREEX(ipdata->mod);
    FREEX(ipdata->mode);
}
