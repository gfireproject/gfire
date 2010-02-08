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
#include "gf_game_detection.h"
#include "gf_server_detection_linux.h"

gfire_server_detection *gfire_server_detection_new()
{
	gfire_server_detection *ret = g_malloc0(sizeof(gfire_server_detection));
	if(!ret)
		return NULL;

	ret->game_pid = 0;
	ret->netstat_servers = NULL;
	ret->tcpdump_servers = NULL;
	ret->detected_netstat_servers = 0;
	ret->detected_tcpdump_servers = 0;
	ret->udp_detected = FALSE;
	ret->game_information = NULL;

	return ret;
}

void gfire_server_detection_free(gfire_server_detection *p_gfire_server_detection)
{
	if(!p_gfire_server_detection)
		return;

	gfire_server_detection_arrays_clear(p_gfire_server_detection);
	g_free(p_gfire_server_detection);
}

void gfire_server_detection_arrays_clear(gfire_server_detection *p_gfire_server_detection)
{
	gint i, j;

	for(i = 0; i < p_gfire_server_detection->detected_netstat_servers; i++)
	{
		for(j = 0; j < 2; j++)
		{
			if(p_gfire_server_detection->netstat_servers[i][j])
				g_free(p_gfire_server_detection->netstat_servers[i][j]);
		}
		g_free(p_gfire_server_detection->netstat_servers[i]);
	}
	g_free(p_gfire_server_detection->netstat_servers);

	for(i = 0; i < p_gfire_server_detection->detected_tcpdump_servers; i++)
	{
		for(j = 0; j < 2; j++)
		{
			if(p_gfire_server_detection->tcpdump_servers[i][j])
				g_free(p_gfire_server_detection->tcpdump_servers[i][j]);
		}
		g_free(p_gfire_server_detection->tcpdump_servers[i]);
	}
	g_free(p_gfire_server_detection->tcpdump_servers);
}

void gfire_server_detection_detect(gfire_server_detection *p_server_detection_infos)
{
	gchar *server_ip = gfire_server_detection_get(p_server_detection_infos->game_pid, p_server_detection_infos);

	// Abort server detection if game detector aborted too
	if(gfire_game_detector_check_server_detection_abortion() == TRUE)
	{
		g_free(server_ip);
		return;
	}

	// Store found IP
	if(server_ip)
	{
		gchar **server_ip_split = g_strsplit(server_ip, ".", -1);
		if(server_ip_split)
		{
			gchar *server_ip_str = g_strdup_printf("%s.%s.%s.%s", server_ip_split[0], server_ip_split[1], server_ip_split[2], server_ip_split[3]);
			guint16 server_port_tmp = atoi(server_ip_split[4]);
			g_strfreev(server_ip_split);

			gfire_game_data tmp_data;
			gfire_game_data_ip_from_str(&tmp_data, server_ip_str);

			// Abort server detection if game detector aborted too
			if(gfire_game_detector_check_server_detection_abortion() == TRUE)
			{
				g_free(server_ip);
				return;
			}

			// Set found IP
			gfire_game_detector_update_server(tmp_data.ip.value, server_port_tmp);
		}
	}
	// Reset IP if none has been found
	else
	{
		// Abort server detection if game detector aborted too
		if(gfire_game_detector_check_server_detection_abortion() == TRUE)
			return;

		gfire_game_detector_update_server(0, 0);
	}

	gfire_game_detecor_update_server_detection_thread_status(FALSE);
	gfire_server_detection_free(p_server_detection_infos);
	g_free(server_ip);
}

gchar*** gfire_server_detection_netstat(guint32 p_pid, gfire_server_detection *p_gfire_server_detection)
{
	// Initialize netstat array
	gchar ***servers_array = NULL;

	gchar command[128], tcp_output[1024];
	tcp_output[0] = 0;

	// Fetch network connections using netstat
	sprintf(command, "netstat -tuanp 2> /dev/null | grep %u", p_pid);
	FILE *command_pipe = popen(command, "r");

	if(!command_pipe)
		return servers_array;
	else
	{
		GRegex *regex = g_regex_new("(\\d{1,3}\\.){3}\\d{1,3}:\\d{4,5}", G_REGEX_OPTIMIZE, 0, NULL);
		GMatchInfo *regex_matches;

		while(fgets(tcp_output, sizeof(tcp_output), command_pipe))
		{
			p_gfire_server_detection->detected_netstat_servers++;

			servers_array = realloc(servers_array, p_gfire_server_detection->detected_netstat_servers * sizeof(gchar*));
			servers_array[p_gfire_server_detection->detected_netstat_servers - 1] = malloc(3 * sizeof(gchar *));

			gchar *server_ip_tmp = NULL;
			gchar **server_ip_split;

			if(strstr(tcp_output, "udp"))
			{
				if(g_regex_match(regex, tcp_output, 0, &regex_matches))
					server_ip_tmp = g_match_info_fetch(regex_matches, 0);
				else
					server_ip_tmp = "0.0.0.0:*";

				servers_array[p_gfire_server_detection->detected_netstat_servers - 1][2] = "udp";
				p_gfire_server_detection->udp_detected = TRUE;
			}
			else
			{
				if(g_regex_match(regex, tcp_output, 0, &regex_matches))
				{
					// Try to get second IP, because first IP is user IP
					while(g_match_info_matches(regex_matches))
					{
						server_ip_tmp = g_match_info_fetch(regex_matches, 0);
						g_match_info_next(regex_matches, NULL);
					}

					g_match_info_free(regex_matches);
				}

				servers_array[p_gfire_server_detection->detected_netstat_servers - 1][2] = "tcp";
			}

			server_ip_tmp = purple_strreplace(server_ip_tmp, ":", ".");
			server_ip_split = g_strsplit(server_ip_tmp, ".", -1);
			g_free(server_ip_tmp);

			servers_array[p_gfire_server_detection->detected_netstat_servers - 1][0] = g_strdup(server_ip_split[4]);
			servers_array[p_gfire_server_detection->detected_netstat_servers - 1][1] = g_strdup_printf("%s.%s.%s.%s", server_ip_split[0], server_ip_split[1],
																									   server_ip_split[2], server_ip_split[3]);
			g_strfreev(server_ip_split);
		}

		g_regex_unref(regex);
	}

	pclose(command_pipe);

	// Return servers array
	return servers_array;
}

gchar*** gfire_server_detection_tcpdump(gfire_server_detection *p_gfire_server_detection)
{
	// Initialize netstat array
	gchar ***servers_array = NULL;

	gchar command[128], tcp_output[1024];
	tcp_output[0] = 0;

	// Detect network connections using tcpdump
	sprintf(command, "/usr/sbin/tcpdump -f -n -c 5 2> /dev/null");
	FILE *command_pipe = popen(command, "r");

	if(!command_pipe)
		return servers_array;
	else
	{
		GRegex *regex = g_regex_new("(\\d{1,3}\\.){4}\\d{4,5}", G_REGEX_OPTIMIZE, 0, NULL);
		GMatchInfo *regex_matches;

		while(fgets(tcp_output, sizeof(tcp_output), command_pipe))
		{
			p_gfire_server_detection->detected_tcpdump_servers++;

			servers_array = realloc(servers_array, p_gfire_server_detection->detected_tcpdump_servers * sizeof(gchar*));
			servers_array[p_gfire_server_detection->detected_tcpdump_servers - 1] = malloc(3 * sizeof(gchar *));

			gchar *server_ip_tmp;
			gchar **server_ip_split;

			if(g_regex_match(regex, tcp_output, 0, &regex_matches))
				server_ip_tmp = g_match_info_fetch(regex_matches, 0);
			else
				server_ip_tmp = "0.0.0.0.*";

			g_match_info_free(regex_matches);

			server_ip_split = g_strsplit(server_ip_tmp, ".", -1);
			servers_array[p_gfire_server_detection->detected_tcpdump_servers - 1][2] = "udp";
			servers_array[p_gfire_server_detection->detected_tcpdump_servers - 1][0] = g_strdup(server_ip_split[4]);
			servers_array[p_gfire_server_detection->detected_tcpdump_servers - 1][1] = g_strdup_printf("%s.%s.%s.%s", server_ip_split[0], server_ip_split[1],
																									   server_ip_split[2], server_ip_split[3]);
			g_strfreev(server_ip_split);
		}

		g_regex_unref(regex);
	}

	pclose(command_pipe);

	// Return servers array
	return servers_array;
}

gchar *gfire_server_detection_get(guint32 p_pid, gfire_server_detection *p_gfire_server_detection)
{
	gchar *server_ip = NULL;

	// Fetch connections using netstat
	p_gfire_server_detection->netstat_servers = gfire_server_detection_netstat(p_pid, p_gfire_server_detection);
	if(!p_gfire_server_detection->netstat_servers)
		return NULL;

	if(p_gfire_server_detection->udp_detected == TRUE)
	{
		// Fetch connections using tcpdump
		p_gfire_server_detection->tcpdump_servers = gfire_server_detection_tcpdump(p_gfire_server_detection);
		if(!p_gfire_server_detection->tcpdump_servers)
			return NULL;
	}

	// Remove invalid IP's & guess correct IP
	gfire_server_detection_remove_invalid_ips(p_gfire_server_detection);
	server_ip = gfire_server_detection_guess_server(p_gfire_server_detection);

	return server_ip;
}

// Both passed values must be freed with g_free() when no longer used
int gfire_server_detection_get_ips(gchar **p_local_ip, gchar **p_remote_ip)
{
	struct ifaddrs *if_addresses, *if_adresses_tmp;
	gint address_family, address;
	gchar address_ip[NI_MAXHOST];

	// Fetch IP addresses
	if(getifaddrs(&if_addresses) == -1)
		return -1;

	// Get IP's: first IP is local IP & second IP is remote IP
	int i = 0;
	for (if_adresses_tmp = if_addresses; if_adresses_tmp; if_adresses_tmp = if_adresses_tmp->ifa_next)
	{
		address_family = if_adresses_tmp->ifa_addr->sa_family;
		if(address_family == AF_INET)
		{
			address = getnameinfo(if_adresses_tmp->ifa_addr, (address_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
						  address_ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(address)
				return -1;
			if(i == 0)
				  *p_local_ip = g_strdup(address_ip);
			else if(i == 1)
				  *p_remote_ip = g_strdup(address_ip);

			i++;
		}
	}

	freeifaddrs(if_addresses);

	// Return error if one or both could not be retrieved
	if(!p_local_ip || !p_remote_ip)
		return -1;

	// Return success
	return 0;
}

void gfire_server_detection_remove_invalid_ips(gfire_server_detection *p_gfire_server_detection)
{
	// Get local & remote IP
	gchar *local_ip = NULL;
	gchar *remote_ip = NULL;

	if(gfire_server_detection_get_ips(&local_ip, &remote_ip) != 0)
		return;

	gint i, j;
	gboolean port_allowed = TRUE;

	for (i = 0; i < p_gfire_server_detection->detected_netstat_servers; i++)
	{
		// Check for invalid ports
		if(p_gfire_server_detection->game_information->excluded_ports)
		{
			for (j = 0; j < g_strv_length(p_gfire_server_detection->game_information->excluded_ports); j++)
			{
				if(!g_strcmp0(p_gfire_server_detection->game_information->excluded_ports[j], p_gfire_server_detection->netstat_servers[i][0]))
				{
					port_allowed = FALSE;
					break;
				}
			}
		}

		if(!g_strcmp0(p_gfire_server_detection->netstat_servers[i][1], local_ip) ||
		   !g_strcmp0(p_gfire_server_detection->netstat_servers[i][1], remote_ip) || !port_allowed)
		{
			p_gfire_server_detection->netstat_servers[i][0] = NULL;
			p_gfire_server_detection->netstat_servers[i][1] = NULL;
			p_gfire_server_detection->netstat_servers[i][2] = NULL;
		}
	}

	for (i = 0; i < p_gfire_server_detection->detected_tcpdump_servers; i++)
	{
		if(!g_strcmp0(p_gfire_server_detection->tcpdump_servers[i][1], local_ip) ||
		   !g_strcmp0(p_gfire_server_detection->tcpdump_servers[i][1], remote_ip) || !port_allowed)
		{
			p_gfire_server_detection->tcpdump_servers[i][0] = NULL;
			p_gfire_server_detection->tcpdump_servers[i][1] = NULL;
			p_gfire_server_detection->tcpdump_servers[i][2] = NULL;
		}
	}
}

gchar *gfire_server_detection_guess_server(gfire_server_detection *p_gfire_server_detection)
{
	gchar *server_ip = NULL;
	gint i, j;

	// Server uses UDP, get first server in tcpdump array with port present in netstat array
	if(p_gfire_server_detection->udp_detected == TRUE)
	{
		for (i = 0; i < p_gfire_server_detection->detected_tcpdump_servers; i++)
		{
			if(p_gfire_server_detection->tcpdump_servers[i][0] != NULL)
			{
				gchar *server_port_tcpdump_tmp = p_gfire_server_detection->tcpdump_servers[i][0];
				gchar *server_port_netstat_tmp;

				for (j = 0; j < p_gfire_server_detection->netstat_servers; j++)
				{
					server_port_netstat_tmp = p_gfire_server_detection->netstat_servers[j][0];

					if(g_strcmp0(server_port_tcpdump_tmp, server_port_netstat_tmp))
					{
						server_ip = g_strdup_printf("%s.%s", p_gfire_server_detection->tcpdump_servers[i][1], p_gfire_server_detection->tcpdump_servers[i][0]);
						break;
					}
				}
			}

			if(server_ip)
				break;
		}

	}
	// Server uses TCP, get first server in netstat array
	else
	{
		for (i = 0; i < p_gfire_server_detection->detected_netstat_servers; i++)
		{
			if(p_gfire_server_detection->netstat_servers[i][1] != NULL)
			{
				server_ip = g_strdup_printf("%s.%s", p_gfire_server_detection->netstat_servers[i][1], p_gfire_server_detection->netstat_servers[i][0]);
				break;
			}
		}
	}

	// Return server IP, if none found this will return NULL ofc!
	return server_ip;
}
