/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2010  Oliver Ney <oliver@dryder.de>
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

#ifdef _WIN32

#include "gf_server_detection.h"
#include <windows.h>
#include <iphlpapi.h>

#define SIO_RCVALL _WSAIOW(IOC_VENDOR,1)
#define RCVALL_IPLEVEL 3

// Declare some required data types
#ifndef UDP_TABLE_CLASS
typedef enum  {
  UDP_TABLE_BASIC,
  UDP_TABLE_OWNER_PID,
  UDP_TABLE_OWNER_MODULE
} UDP_TABLE_CLASS, *PUDP_TABLE_CLASS;
#endif // UDP_TABLE_CLASS

#ifndef MIB_UDPROW_OWNER_PID
typedef struct _MIB_UDPROW_OWNER_PID {
  DWORD dwLocalAddr;
  DWORD dwLocalPort;
  DWORD dwOwningPid;
} MIB_UDPROW_OWNER_PID, *PMIB_UDPROW_OWNER_PID;
#endif // MIB_UDPROW_OWNER_PID

#ifndef MIB_UDPTABLE_OWNER_PID
typedef struct _MIB_UDPTABLE_OWNER_PID {
  DWORD                dwNumEntries;
  MIB_UDPROW_OWNER_PID table[ANY_SIZE];
} MIB_UDPTABLE_OWNER_PID, *PMIB_UDPTABLE_OWNER_PID;
#endif // MIB_UDPTABLE_OWNER_PID

typedef struct _gfire_server_detection_windows
{
	SOCKET sock;
	guint prpl_input;
	GList *local_ips;
	GTimeVal timeout;
} gfire_server_detection_windows;

static DWORD WINAPI (*ptrGetIpAddrTable)(PMIB_IPADDRTABLE,PULONG,BOOL) = NULL;
static DWORD WINAPI (*ptrGetExtendedTcpTable)(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG) = NULL;
static DWORD WINAPI (*ptrGetExtendedUdpTable)(PVOID,PDWORD,BOOL,ULONG,UDP_TABLE_CLASS,ULONG) = NULL;

static GList *get_local_ips()
{
	if(!ptrGetIpAddrTable)
		return NULL;

	PMIB_IPADDRTABLE iptable = NULL;
	ULONG size = 0;

	DWORD ret = ptrGetIpAddrTable(NULL, &size, FALSE);
	if(ret != ERROR_INSUFFICIENT_BUFFER)
		return NULL;

	iptable = (PMIB_IPADDRTABLE)g_malloc(size);

	ret = ptrGetIpAddrTable(iptable, &size, FALSE);
	if(ret != NO_ERROR)
		return NULL;

	GList *ips = NULL;
	int i = 0;
	for(; i < iptable->dwNumEntries; i++)
	{
		// Only connected devices
		if(!(iptable->table[i].wType & 0x0008))
		{
			guint32 *ip = g_malloc(sizeof(guint32));
			*ip = ntohl(iptable->table[i].dwAddr);
			ips = g_list_append(ips, ip);
		}
	}

	return ips;
}

static void get_tcp_connections(gfire_server_detector *p_detector)
{
	if(!ptrGetExtendedTcpTable)
		return;

	PMIB_TCPTABLE_OWNER_PID tcptable = NULL;
	DWORD size = 0;

	DWORD ret = ptrGetExtendedTcpTable(NULL, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
	if(ret != ERROR_INSUFFICIENT_BUFFER)
		return;

	tcptable = (PMIB_TCPTABLE_OWNER_PID)g_malloc(size);
	ret = ptrGetExtendedTcpTable(tcptable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
	if(ret != NO_ERROR)
	{
		g_free(tcptable);
		return;
	}

	int i = 0;
	for(; i < tcptable->dwNumEntries; i++)
	{
		// Only store active connections which belong to the requested PID
		if(tcptable->table[i].dwState == MIB_TCP_STATE_ESTAB && tcptable->table[i].dwOwningPid == p_detector->pid)
		{
			// Store the IPs
			gfire_server *server = (gfire_server*)g_malloc0(sizeof(gfire_server));
			server->ip = ntohl(tcptable->table[i].dwRemoteAddr);
			server->port = ntohs(tcptable->table[i].dwRemotePort);
			p_detector->tcp_servers = g_list_append(p_detector->tcp_servers, server);
		}
	}

	g_free(tcptable);
}

static void get_udp_connections(gfire_server_detector *p_detector)
{
	if(!ptrGetExtendedUdpTable)
		return;

	PMIB_UDPTABLE_OWNER_PID udptable = NULL;
	DWORD size = 0;

	DWORD ret = ptrGetExtendedUdpTable(NULL, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
	if(ret != ERROR_INSUFFICIENT_BUFFER)
		return;

	udptable = (PMIB_UDPTABLE_OWNER_PID)g_malloc(size);
	ret = ptrGetExtendedUdpTable(udptable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
	if(ret != NO_ERROR)
	{
		g_free(udptable);
		return;
	}

	int i = 0;
	for(; i < udptable->dwNumEntries; i++)
	{
		// Only store connections which belong to the requested PID
		if(udptable->table[i].dwOwningPid == p_detector->pid)
		{
			// Store the ports
			guint16 *port = (guint16*)g_malloc(sizeof(guint16));
			*port = ntohs(udptable->table[i].dwLocalPort);

			p_detector->local_udp_connections = g_list_append(p_detector->local_udp_connections, port);
		}
	}

	g_free(udptable);
}

static void cleanup(gfire_server_detector *p_detector)
{
	gfire_server_detection_windows *data = (gfire_server_detection_windows*)p_detector->os_data;

	if(data->sock != INVALID_SOCKET)
		closesocket(data->sock);

	if(data->prpl_input)
		purple_input_remove(data->prpl_input);

	gfire_list_clear(data->local_ips);
	g_free(data);

	gfire_list_clear(p_detector->excluded_ports);
	p_detector->excluded_ports = NULL;
	gfire_list_clear(p_detector->local_udp_connections);
	p_detector->local_udp_connections = NULL;
	gfire_list_clear(p_detector->tcp_servers);
	p_detector->tcp_servers = NULL;
	gfire_list_clear(p_detector->udp_servers);
	p_detector->udp_servers = NULL;

	p_detector->running = FALSE;
}

static void read_sniffed_packet(gfire_server_detector *p_detector, gint p_fd, PurpleInputCondition p_cond)
{
	if(p_cond != PURPLE_INPUT_READ)
		return;

	gfire_server_detection_windows *data = (gfire_server_detection_windows*)p_detector->os_data;

	GTimeVal cur_time;
	g_get_current_time(&cur_time);

	if((cur_time.tv_sec - data->timeout.tv_sec) >= 5)
	{
		gfire_server_detection_remove_invalid_servers(p_detector, data->local_ips);
		const gfire_server *server = gfire_server_detection_guess_server(p_detector);

		if(server)
			((void(*)(guint32,guint16))p_detector->server_callback)(server->ip, server->port);
		else
			((void(*)(guint32,guint16))p_detector->server_callback)(0, 0);

		p_detector->finished = TRUE;
		cleanup(p_detector);
		return;
	}

	static guint8 buffer[8192];

	int len = recv(data->sock, (char*)buffer, 8192, 0);
	if(len <= 0)
	{
		cleanup(p_detector);
		return;
	}
	else if(len >= 24)
	{
		guint8 len = (buffer[0] & 0x7F) * 4;
		guint8 proto = buffer[9];

		if(proto == 6 /*TCP*/ || proto == 17 /*UDP*/)
		{
			guint16 source_port = *((guint16*)(buffer + len));
			guint16 dest_port = *((guint16*)(buffer + len + 2));

			guint32 source_ip = ntohl(*((guint32*)(buffer + 12)));
			guint32 dest_ip = ntohl(*((guint32*)(buffer + 16)));

			guint32 local_ip = 0, remote_ip = 0;
			guint16 local_port = 0, remote_port = 0;

			GList *lip = data->local_ips;
			while(lip)
			{
				if(source_ip == *((guint32*)lip->data))
				{
					local_ip = source_ip;
					local_port = source_port;
					remote_ip = dest_ip;
					remote_port = dest_port;
					break;
				}
				else if(dest_ip == *((guint32*)lip->data))
				{
					local_ip = dest_ip;
					local_port = dest_port;
					remote_ip = source_ip;
					remote_port = source_port;
					break;
				}
				lip = g_list_next(lip);
			}

			if(local_ip != 0)
			{
				if(proto == 6)
				{
					GList *tcp = p_detector->tcp_servers;
					while(tcp)
					{
						gfire_server *server = tcp->data;
						if(server->ip == remote_ip && server->port == dest_port)
						{
							server->priority++;
							break;
						}
						tcp = g_list_next(tcp);
					}
				}
				else if(proto == 17)
				{
					GList *udp = p_detector->udp_servers;
					while(udp)
					{
						gfire_server *server = udp->data;
						if(server->ip == remote_ip && server->port == dest_port)
						{
							server->priority++;
							break;
						}
						udp = g_list_next(udp);
					}

					if(!udp)
					{
						GList *udpcon = p_detector->local_udp_connections;
						while(udpcon)
						{
							if(local_port == *((guint16*)udpcon->data))
							{
								gfire_server *server = g_malloc(sizeof(gfire_server));
								server->ip = remote_ip;
								server->port = remote_port;
								server->priority = 1;
								p_detector->udp_servers = g_list_append(p_detector->udp_servers, server);
								break;
							}

							udpcon = g_list_next(udpcon);
						}
					}
				}
			}
		}
	}
}

static gboolean start_sniffing_connections(gfire_server_detector *p_detector)
{
	gfire_server_detection_windows *data = (gfire_server_detection_windows*)p_detector->os_data;
	data->sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if(data->sock == INVALID_SOCKET)
		return FALSE;

	char host[255];
	gethostname(host, 255);
	struct hostent *dns = gethostbyname(host);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = *((u_long*)dns->h_addr_list[0]);
	addr.sin_port = 0;

	if(bind(data->sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0)
	{
		closesocket(data->sock);
		return FALSE;
	}

	DWORD outlen, val = RCVALL_IPLEVEL;
	if(WSAIoctl(data->sock, SIO_RCVALL, &val, sizeof(DWORD), NULL, 0, &outlen, NULL, NULL) != 0)
	{
		if(WSAGetLastError() == WSAEACCES)
			purple_debug_error("gfire", "server detection: No permission to listen on device, "
							   "please make sure you launch Pidgin with enough permissions");

		closesocket(data->sock);
		return FALSE;
	}

	g_get_current_time(&data->timeout);

	data->prpl_input = purple_input_add(data->sock, PURPLE_INPUT_READ, (PurpleInputFunction)read_sniffed_packet,
										p_detector);

	return TRUE;
}

void gfire_server_detector_start(gfire_server_detector *p_detector, guint32 p_gameid, guint32 p_pid)
{
	if(!p_detector || p_gameid == 0 || p_pid == 0 || p_detector->running)
		return;

	if(!ptrGetExtendedTcpTable || !ptrGetExtendedUdpTable)
	{
		// Load the "netstat" functions dynamically
		HINSTANCE lib = LoadLibrary("iphlpapi.dll");
		if(!lib)
		{
			purple_debug_error("gfire", "server detection: couldn't load library\n");
			return;
		}

		ptrGetIpAddrTable = (DWORD WINAPI(*)(PMIB_IPADDRTABLE,PULONG,BOOL))GetProcAddress(lib, "GetIpAddrTable");
		ptrGetExtendedTcpTable = (DWORD WINAPI(*)(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG))
								 GetProcAddress(lib, "GetExtendedTcpTable");
		ptrGetExtendedUdpTable = (DWORD WINAPI(*)(PVOID,PDWORD,BOOL,ULONG,UDP_TABLE_CLASS,ULONG))
								 GetProcAddress(lib, "GetExtendedUdpTable");
	}

	p_detector->gameid = p_gameid;
	p_detector->pid = p_pid;
	p_detector->quit = FALSE;
	p_detector->finished = FALSE;
	p_detector->running = TRUE;

	gfire_server_detection_windows *data = g_malloc0(sizeof(gfire_server_detection_windows));
	data->sock = INVALID_SOCKET;
	data->local_ips = get_local_ips();
	p_detector->os_data = data;

	get_tcp_connections(p_detector);
	gfire_server_detection_remove_invalid_servers(p_detector, data->local_ips);

	get_udp_connections(p_detector);
	if(!start_sniffing_connections(p_detector))
	{
		((void(*)(guint32,guint16))p_detector->server_callback)(0, 0);
		cleanup(p_detector);
	}
}

void gfire_server_detector_stop(gfire_server_detector *p_detector)
{
	if(p_detector->running && p_detector->os_data)
	{
		cleanup(p_detector);
	}
}

#endif // _WIN32
