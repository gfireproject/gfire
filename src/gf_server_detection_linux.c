/*
* purple - Xfire Protocol Plugin
*
* Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
* Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
* Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
* Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
* Copyright (C) 2009	   Oliver Ney <oliver@dryder.de>
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

#include "gf_games.h"
#include "gf_server_detection.h"
#include "gf_game_detection.h"
#include <dirent.h>
#include <ifaddrs.h>

typedef struct _gfire_server_detection_linux
{
	gfire_server_detector *detector;

	GList *tcp_servers;
	GList *local_udp_connections;
	GList *udp_servers;

	GList *excluded_ports;

	GThread *thread;
} gfire_server_detection_linux;

static gfire_server_detection_linux *gfire_server_detection_linux_create(gfire_server_detector *p_detector, GList *p_ports)
{
	if(!p_detector)
		return NULL;

	gfire_server_detection_linux *ret = g_malloc0(sizeof(gfire_server_detection_linux));
	if(!ret)
		return NULL;

	ret->detector = p_detector;
	ret->excluded_ports = p_ports;

	return ret;
}

static void gfire_server_detection_linux_free(gfire_server_detection_linux *p_detection)
{
	if(!p_detection)
		return;

	gfire_list_clear(p_detection->tcp_servers);
	gfire_list_clear(p_detection->local_udp_connections);
	gfire_list_clear(p_detection->udp_servers);
	gfire_list_clear(p_detection->excluded_ports);

	g_free(p_detection);
}

static void gfire_server_detection_netstat(gfire_server_detection_linux *p_detection)
{
	gchar *fd_dir_path = g_strdup_printf("/proc/%u/fd", p_detection->detector->pid);
	DIR *fd_dir = opendir(fd_dir_path);

	// No such dir, netstat won't work
	if(!fd_dir)
	{
		g_free(fd_dir_path);
		return;
	}

	GList *inodes = NULL;

	struct dirent *fd;
	while((fd = readdir(fd_dir)))
	{
		gchar *fd_path = g_strdup_printf("%s/%s", fd_dir_path, fd->d_name);

		gchar real_path[PATH_MAX];
		int len = readlink(fd_path, real_path, PATH_MAX - 1);
		g_free(fd_path);

		// Skip on error
		if(len == -1)
			continue;

		real_path[len] = 0;

		if(strlen(real_path) > 9 && !strncmp(real_path, "socket:", 7))
		{
			if(real_path[7] == '[' && real_path[strlen(real_path) - 1] == ']')
			{
				gchar *inode_str = real_path + 8;
				real_path[strlen(real_path) - 1] = 0;

				unsigned long *inode = g_malloc(sizeof(unsigned long));
				if(!sscanf(inode_str, "%lu", inode))
				{
					g_free(inode);
				}
				else
					inodes = g_list_append(inodes, inode);
			}
		}
		else if(strlen(real_path) > 7 && !strncmp(real_path, "[0000]:", 7))
		{
			unsigned long *inode = g_malloc(sizeof(unsigned long));
			if(!sscanf(real_path + 7, "%lu", inode))
			{
				g_free(inode);
			}
			else
				inodes = g_list_append(inodes, inode);
		}
	}

	closedir(fd_dir);
	g_free(fd_dir_path);

	// Get the socket data for each inode
	gchar buffer[8192];
	//	TCP
	FILE *file = fopen("/proc/net/tcp", "r");
	if(file)
	{
		gboolean first = TRUE;
		do
		{
			if(fgets(buffer, 8192, file))
			{
				// Skip the header
				if(first)
				{
					first = FALSE;
					continue;
				}

				guint32 ip;
				guint16 port;
				unsigned long inode;
				guint16 state;
				if(sscanf(buffer, "%*d: %*X:%*X %8X:%4hX %hX %*X:%*X %*X:%*X %*X %*d %*d %lu"
						  "%*s\n", &ip, &port, &state, &inode) < 4)
					continue;

				if(state != 0x01) // ESTABLISHED
					continue;

				GList *cur = inodes;
				while(cur)
				{
					if(*((guint32*)cur->data) == inode)
					{
						gfire_server *server = g_malloc0(sizeof(gfire_server));
						server->ip = ip;
						server->port = port;

						p_detection->tcp_servers = g_list_append(p_detection->tcp_servers, server);

						break;
					}
					cur = g_list_next(cur);
				}
			}
		} while(!feof(file));

		fclose(file);
	}

	//	UDP
	file = fopen("/proc/net/udp", "r");
	if(file)
	{
		gboolean first = TRUE;
		do
		{
			if(fgets(buffer, 8192, file))
			{
				// Skip the header
				if(first)
				{
					first = FALSE;
					continue;
				}

				guint32 ip;
				guint16 port;
				unsigned long inode;
				guint16 state;
				if(sscanf(buffer, "%*d: %8X:%4hX %*X:%*X %hX %*X:%*X %*X:%*X %*X %*d %*d %lu "
						  "%*s\n", &ip, &port, &state, &inode) < 4)
					continue;

				if(state != 0x07) // LISTENING
					continue;

				GList *cur = inodes;
				while(cur)
				{
					if(*((guint32*)cur->data) == inode)
					{
						gfire_server *server = g_malloc0(sizeof(gfire_server));
						server->ip = ip;
						server->port = port;

						p_detection->local_udp_connections = g_list_append(p_detection->local_udp_connections, server);

						break;
					}
					cur = g_list_next(cur);
				}
			}
		} while(!feof(file));

		fclose(file);
	}

	gfire_list_clear(inodes);
}

static void gfire_server_detection_tcpdump(gfire_server_detection_linux *p_detection, const GList *p_local_ips)
{
	gchar tcp_output[8192];

	// Detect network connections using tcpdump
	gchar *command = g_strdup("/usr/sbin/tcpdump -i wlan0 -ftnc 20 2>/dev/null");
	FILE *command_pipe = popen(command, "r");
	g_free(command);

	if(!command_pipe)
		return;

	do
	{
		if(fgets(tcp_output, 8192, command_pipe))
		{
			// Check if we should abort
			g_mutex_lock(p_detection->detector->mutex);
			if(p_detection->detector->quit)
			{
				g_mutex_unlock(p_detection->detector->mutex);
				pclose(command_pipe);
				return;
			}
			else
				g_mutex_unlock(p_detection->detector->mutex);

			guint16 sip1[4];
			guint16 port1;
			guint16 sip2[4];
			guint16 port2;

			if(sscanf(tcp_output, "IP %3hu.%3hu.%3hu.%3hu.%5hu > %3hu.%3hu.%3hu.%3hu.%5hu:",
					  &sip1[0], &sip1[1], &sip1[2], &sip1[3], &port1,
					  &sip2[0], &sip2[1], &sip2[2], &sip2[3], &port2) < 10)
				continue;

			guint32 ip1 = sip1[0] << 24 | sip1[1] << 16 | sip1[2] << 8 | sip1[3];
			guint32 ip2 = sip2[0] << 24 | sip2[1] << 16 | sip2[2] << 8 | sip2[3];

			guint32 local_ip = 0, remote_ip = 0;
			guint16 local_port = 0, remote_port = 0;
			const GList *lip = p_local_ips;
			while(lip)
			{
				if(ip1 == *((guint32*)lip->data))
				{
					local_ip = ip1;
					local_port = port1;
					remote_ip = ip2;
					remote_port = port2;

					break;
				}
				else if(ip2 == *((guint32*)lip->data))
				{
					local_ip = ip2;
					local_port = port2;
					remote_ip = ip1;
					remote_port = ip1;

					break;
				}
				lip = g_list_next(lip);
			}

			// No such IP found
			if(local_ip == 0)
			{
				printf("no local found\n");
				continue;
			}

			// Check TCP servers
			GList *tcp = p_detection->tcp_servers;
			while(tcp)
			{
				gfire_server *server = (gfire_server*)tcp->data;
				// If we got that server, increase its priority
				if(server->ip == remote_ip && server->port == remote_port)
				{
					server->priority++;
					break;
				}
				tcp = g_list_next(tcp);
			}

			// Check UPD servers
			GList *udp = p_detection->udp_servers;
			while(udp)
			{
				gfire_server *server = (gfire_server*)udp->data;
				// If we got that server, increase its priority
				if(server->ip == remote_ip && server->port == remote_port)
				{
					server->priority++;
					break;
				}
				udp = g_list_next(udp);
			}

			// Check UDP connections
			if(!udp)
			{
				udp = p_detection->local_udp_connections;
				while(udp)
				{
					gfire_server *con = (gfire_server*)udp->data;
					// If the connection listens on that IP & Port, add the server
					if((con->ip == 0 || con->ip == local_ip) && con->port == local_port)
					{
						gfire_server *server = g_malloc(sizeof(gfire_server));
						server->ip = remote_ip;
						server->port = remote_port;
						server->priority = 1;

						p_detection->udp_servers = g_list_append(p_detection->udp_servers, server);
						break;
					}
					udp = g_list_next(udp);
				}
			}
		}
	} while(!feof(command_pipe));

	pclose(command_pipe);
}

static void gfire_server_detection_remove_invalid_servers(gfire_server_detection_linux *p_detection,
														  const GList *p_local_ips)
{
	// Check detected TCP servers
	GList *cur = p_detection->tcp_servers;
	while(cur)
	{
		gboolean deleted = FALSE;
		gfire_server *server = (gfire_server*)cur->data;

		// Check against local IPs
		const GList *lip = p_local_ips;
		while(lip)
		{
			if(server->ip == *((guint32*)lip->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->tcp_servers = g_list_delete_link(p_detection->tcp_servers, to_delete);

				break;
			}
			lip = g_list_next(lip);
		}
		if(deleted)
			continue;

		// Check against invalid ports
		GList *port = p_detection->excluded_ports;
		while(port)
		{
			if(server->port == *((guint16*)port->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->tcp_servers = g_list_delete_link(p_detection->tcp_servers, to_delete);

				break;
			}
			port = g_list_next(port);
		}
		if(deleted)
			continue;

		cur = g_list_next(cur);
	}

	// Check detected UDP servers
	cur = p_detection->udp_servers;
	while(cur)
	{
		gboolean deleted = FALSE;
		gfire_server *server = (gfire_server*)cur->data;

		// Check against local IPs
		const GList *lip = p_local_ips;
		while(lip)
		{
			if(server->ip == *((guint32*)lip->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->udp_servers = g_list_delete_link(p_detection->udp_servers, to_delete);

				break;
			}
			lip = g_list_next(lip);
		}
		if(deleted)
			continue;

		// Check against invalid ports
		GList *port = p_detection->excluded_ports;
		while(port)
		{
			if(server->port == *((guint16*)port->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->udp_servers = g_list_delete_link(p_detection->udp_servers, to_delete);

				break;
			}
			port = g_list_next(port);
		}
		if(deleted)
			continue;

		cur = g_list_next(cur);
	}
}

// Sorts from highest to lowest priority
static gint gfire_server_cmp(const gfire_server *p_server1, const gfire_server *p_server2)
{
	if(p_server1->priority < p_server2->priority)
		return 1;
	else if(p_server1->priority == p_server2->priority)
		return 0;
	else
		return -1;
}

static const gfire_server *gfire_server_detection_guess_server(gfire_server_detection_linux *p_detection)
{
	// Get best TCP server
	gfire_server *tcp = NULL;
	if(p_detection->tcp_servers)
	{
		p_detection->tcp_servers = g_list_sort(p_detection->tcp_servers, (GCompareFunc)gfire_server_cmp);
		tcp = p_detection->tcp_servers->data;
	}

	// Get best UDP server
	gfire_server *udp = NULL;
	if(p_detection->udp_servers)
	{
		p_detection->udp_servers = g_list_sort(p_detection->udp_servers, (GCompareFunc)gfire_server_cmp);
		udp = p_detection->udp_servers->data;
	}

	if(!tcp)
		return udp;
	else if(!udp)
		return tcp;
	else
		// Return TCP only if it has better priority than UDP (most servers use UDP anyway)
		return (tcp->priority > udp->priority) ? tcp : udp;
}

static GList *gfire_server_detection_get_local_ips()
{
	struct ifaddrs *if_addresses, *if_addresses_tmp;
	GList *ret = NULL;

	// Fetch IP addresses
	if(getifaddrs(&if_addresses) == -1)
		return NULL;

	for (if_addresses_tmp = if_addresses; if_addresses_tmp; if_addresses_tmp = if_addresses_tmp->ifa_next)
	{
		// Store all IPv4 addresses of all local devices (there's no remote info available)
		if(if_addresses_tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *addr = (struct sockaddr_in*)if_addresses_tmp->ifa_addr;

			guint32 *ip = g_malloc(sizeof(guint32));
			*ip = ntohl(addr->sin_addr.s_addr);

			ret = g_list_append(ret, ip);
		}
	}

	freeifaddrs(if_addresses);

	return ret;
}

static void gfire_server_detection_thread(gfire_server_detection_linux *p_detection)
{
	GList *local_ips = gfire_server_detection_get_local_ips();

	// Get all TCP servers and local UDP connections
	gfire_server_detection_netstat(p_detection);

	// Check if we've been requested to stop already
	g_mutex_lock(p_detection->detector->mutex);
	if(p_detection->detector->quit)
	{
		p_detection->detector->running = FALSE;
		g_mutex_unlock(p_detection->detector->mutex);
		gfire_server_detection_linux_free(p_detection);
		gfire_list_clear(local_ips);
		return;
	}
	else
		g_mutex_unlock(p_detection->detector->mutex);

	// Filter all found TCP servers
	gfire_server_detection_remove_invalid_servers(p_detection, local_ips);

	// Check their occurrence and the remote data for local UPD sockets
	gfire_server_detection_tcpdump(p_detection, local_ips);

	// Check if we've been requested to stop already
	g_mutex_lock(p_detection->detector->mutex);
	if(p_detection->detector->quit)
	{
		p_detection->detector->running = FALSE;
		g_mutex_unlock(p_detection->detector->mutex);
		gfire_server_detection_linux_free(p_detection);
		gfire_list_clear(local_ips);
		return;
	}
	else
		g_mutex_unlock(p_detection->detector->mutex);

	// Filter all found TCP & UDP servers
	gfire_server_detection_remove_invalid_servers(p_detection, local_ips);

	// Get the best IP
	const gfire_server *server = gfire_server_detection_guess_server(p_detection);
	// Tell the games detection about the server if there is one and it's priority isn't 0
	if(server && server->priority)
		gfire_game_detector_update_server(server->ip, server->port);
	else
		// Tell the games detection there is no server
		gfire_game_detector_update_server(0, 0);

	// Finalize the thread
	g_mutex_lock(p_detection->detector->mutex);
	p_detection->detector->running = FALSE;
	p_detection->detector->finished = TRUE;
	g_mutex_unlock(p_detection->detector->mutex);
	gfire_server_detection_linux_free(p_detection);
	gfire_list_clear(local_ips);
}

void gfire_server_detector_start(gfire_server_detector *p_detector, guint32 p_gameid, guint32 p_pid)
{
	if(!p_detector || p_gameid == 0 || p_pid == 0)
		return;

	g_mutex_lock(p_detector->mutex);
	if(p_detector->running)
	{
		g_mutex_unlock(p_detector->mutex);
		return;
	}
	g_mutex_unlock(p_detector->mutex);

	// Get all infos needed for detection
	const gfire_game *game = gfire_game_by_id(p_gameid);

	gfire_server_detection_linux *detection_data = gfire_server_detection_linux_create(p_detector,
																					   gfire_game_excluded_ports_copy(game));
	if(!detection_data)
	{
		gfire_server_detection_linux_free(detection_data);
		return;
	}
	p_detector->os_data = detection_data;

	p_detector->pid = p_pid;

	p_detector->finished = FALSE;
	p_detector->quit = FALSE;
	p_detector->running = TRUE;

	// Create server detection thread
	detection_data->thread = g_thread_create((GThreadFunc)gfire_server_detection_thread, detection_data, TRUE, NULL);
}

void gfire_server_detector_stop(gfire_server_detector *p_detector)
{
	if(!p_detector)
		return;

	g_mutex_lock(p_detector->mutex);
	// Check if the thread is running
	if(!p_detector->running || !p_detector->os_data)
	{
		g_mutex_unlock(p_detector->mutex);
		return;
	}

	gfire_server_detection_linux *data = (gfire_server_detection_linux*)p_detector->os_data;

	// Tell the thread to terminate
	p_detector->quit = TRUE;
	g_mutex_unlock(p_detector->mutex);

	// Wait until the thread terminates
	g_thread_join(data->thread);
}
