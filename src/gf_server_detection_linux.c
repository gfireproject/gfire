/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009	    Oliver Ney <oliver@dryder.de>
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
#include "gf_server_detection_linux.h"

void gfire_detect_teamspeak_server(guint8 **voip_ip, guint32 *voip_port)
{
	if(!voip_ip || !voip_port)
		return;

	int unix_socket = -1;
	struct sockaddr_un unix_addr;

	unix_addr.sun_family = AF_UNIX;
	strcpy(unix_addr.sun_path, "/tmp/.teamspeakclient");

	unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(unix_socket < 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to create socket.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}

	// Connect to TeamSpeak client
	if(connect(unix_socket, (struct sockaddr*)&unix_addr, strlen(unix_addr.sun_path) + sizeof(unix_addr.sun_family)) != 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to connect to unix socket.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}

	// Send Server Data Request (Format: Message length (incl. zero), 3x zero, Message, zero)
	// Message: "GET_SERVER_INFO" => Length = 16
	char request[20] = {0x10, 0x00, 0x00, 0x00, 'G', 'E', 'T', '_', 'S', 'E', 'R', 'V', 'E', 'R', '_', 'I', 'N', 'F', 'O', 0x00};
	if(send(unix_socket, request, sizeof(request), 0) <= 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to send server data request.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	// Receive response
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));

	int recBytes = 0;
	int r = 0;
	do
	{
		r = recv(unix_socket, &buffer[recBytes], sizeof(buffer) - 1 - recBytes, 0);
		recBytes += r;
	} while(r > 0);

	if(recBytes <= 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to receive server data.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	buffer[recBytes] = 0;
	char *data = &buffer[4]; // Skip the size and zeros
	if(strncmp(data, "ERROR", 5) == 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: TeamSpeak doesn't seem to be connected.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	// Extract Server Address
	char *tmp = data;
	char *strings = NULL;
	int count = 0;
	while((strings = strtok(tmp, "\r\n")))
	{
		if(count == 5)
		{
			char *split = strchr(strings, ':');
			if(!split)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Received invalid data.\n");
				*voip_ip = NULL;
				*voip_port = 0;
				close(unix_socket);
			}

			*split = ' ';
			char ip[16];
			guint32 port;
			sscanf(strings, "%s %d", ip, &port);
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: TeamSpeak Server: %s:%d\n", ip, port);

			// NEEDS TO BE FIXED ! *voip_ip = (guint8*)gfire_ipstr_to_bin(ip);
			*voip_port = port;
		}

		count = count + 1;
		tmp = NULL;
	}

	close(unix_socket);
}

void gfire_detect_mumble_server(const gchar *executable, guint8 **voip_ip, guint32 *voip_port)
{
	if(!voip_ip || !voip_port || !executable)
		return;

	// Build command line
	char cmd[1024];
	sprintf(cmd, "netstat -tnp | grep -i \"%s\"", executable);

	// Read output
	FILE *netstat = popen(cmd, "r");
	char data[100];
	if(fgets(data, 100, netstat) == NULL && feof(netstat))
	{
		// No data. Mumble is not connected.
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_mumble_server: Mumble doesn't seem to be connected.\n");
		pclose(netstat);
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}
	pclose(netstat);

	// Extract "IP:Port"
	char addr[22];
	sscanf(data, "%*s\t%*d\t%*d\t%*s\t%s", addr);
	// Replace ':' with ' ' for sscanf
	(*strchr(addr, ':')) = ' ';
	// Extract IP as a string and port
	char ip[16];
	guint32 port;
	sscanf(addr, "%s %u", ip, &port);
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_mumble_server: Mumble Server: %s:%d\n", ip, port);
	//*voip_ip = (guint8*)gfire_ipstr_to_bin(ip);
	*voip_port = port;
}

gboolean gfire_server_detection_port_allowed(gfire_game_detection_info *p_server_detection_info, int p_port)
{
	gboolean ret = TRUE;

	if (p_server_detection_info->exclude_ports)
	{
		int i;
		for(i = 0; p_server_detection_info->exclude_ports[i]; i++)
		{
			if (p_port == atoi(p_server_detection_info->exclude_ports[i]))
				ret = FALSE;
		}
	}

	return ret;
}

void gfire_server_detection_detect(gfire_data *p_gfire)
{
	if (!p_gfire)
	{
		purple_debug_error("gfire", "Couldn't access gfire data.\n");
		return;
	}

	// Fetch local and remote IP
	gchar *local_ip = NULL;
	gchar *remote_ip = NULL;

	struct ifaddrs *if_addresses, *if_adresses_tmp;
	int address_family, address;
	char address_ip[NI_MAXHOST];

	if (getifaddrs(&if_addresses) == -1)
	{
		purple_debug_error("gfire", "Couldn't get interface addresses.\n");
		return;
	}

	int i = 0;
	for (if_adresses_tmp = if_addresses; if_adresses_tmp != NULL; if_adresses_tmp = if_adresses_tmp->ifa_next)
	{
		address_family = if_adresses_tmp->ifa_addr->sa_family;
		if (address_family == AF_INET)
		{
			address = getnameinfo(if_adresses_tmp->ifa_addr,
					      (address_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
					      address_ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (address)
				return;

			if (i == 0)
				local_ip = g_strdup(address_ip);
			else if (i == 1)
				remote_ip = g_strdup(address_ip);

			i++;
		}
	}

	freeifaddrs(if_addresses);

	if (!local_ip || !remote_ip)
	{
		purple_debug_error("gfire", "Couldn't get interface addresses.\n");
		return;
	}

	// Define needed regex patterns
	char *regex_ip_netstat = "(\\d{1,3}\\.){3}\\d{1,3}:\\d{4,5}";
	char *regex_ip_tcpdump = "(\\d{1,3}\\.){4}\\d{4,5}";
	char *regex_ip_sender_tcpdump = "(\\d{1,3}\\.){4}\\d{4,5} >";
	char *regex_ip_receiver_tcpdump = "> (\\d{1,3}\\.){4}\\d{4,5}";

	// Don't continue server detection if user isn't playing anymore
        if (!gfire_is_playing(p_gfire))
                return;

	const gfire_game_data *game_data = gfire_get_game_data(p_gfire);

	gchar *server_ip = NULL;
	gboolean server_detect_udp = FALSE;
	int server_port = 0;

	// Get all infos needed for detection
	gfire_game_detection_info *detection_info = gfire_game_detection_info_get(game_data->id);
	if (!detection_info->detect || !detection_info->executable)
		return;

	// Shorten process name to make it compatible with netstat
	gchar *process_name_short = g_strndup(g_path_get_basename(detection_info->executable), 13);

	// Server detection begins here
	char command[128], line[2048];
	memset(line, 0, sizeof(line));

        gchar current_line[5 * 1024];

	// Detect server IP using TCP with netstat
	sprintf(command, "netstat -tuanp | grep %s", process_name_short);

	FILE *command_pipe = popen(command, "r");
	if (!command_pipe)
	{
		purple_debug_error("gfire", "Couldn't get TCP adresses for server detection.\n");
		pclose(command_pipe);
		return;
	}
	else
	{
		GRegex *regex;
		GMatchInfo *regex_matches;

		// Detect if UDP is used, if so get server port from first occurence
		server_detect_udp = FALSE;

		char tcp_output[1024];
		memset(tcp_output, 0, sizeof(tcp_output));

		i = 0;
		while(fgets(tcp_output, sizeof(tcp_output), command_pipe))
		{
                        strcat(current_line, tcp_output);

			if (strstr(tcp_output, "udp"))
			{
				gchar *server_ip_tmp;

				regex = g_regex_new(regex_ip_netstat, G_REGEX_OPTIMIZE, 0, NULL);
				if (g_regex_match(regex, tcp_output, 0, &regex_matches))
                                        server_ip_tmp = g_match_info_fetch(regex_matches, 0);
				else
					return;

				server_ip_tmp = purple_strreplace(server_ip_tmp, ":", ".");
				gchar **server_ip_split = g_strsplit(server_ip_tmp, ".", -1);
				server_port = atoi(server_ip_split[4]);

				if (gfire_server_detection_port_allowed(detection_info, server_port))
				{
					server_detect_udp = TRUE;
					break;
				}
			}
		}

		// If TCP is used, get server IP and port
		if (!server_detect_udp)
		{
			GRegex *regex;
			GMatchInfo *regex_matches;

			regex = g_regex_new(regex_ip_netstat, G_REGEX_OPTIMIZE, 0, NULL);
			if (g_regex_match(regex, current_line, 0, &regex_matches))
			{
				// Try to get second IP, because first IP is user IP
				while(g_match_info_matches(regex_matches))
				{
					gchar *server_ip_tmp = g_match_info_fetch(regex_matches, 0);
					if (server_ip_tmp != NULL)
						server_ip = server_ip_tmp;

					g_match_info_next(regex_matches, NULL);
				}

				server_ip = purple_strreplace(server_ip, ":", ".");
				gchar **server_ip_split = g_strsplit(server_ip, ".", -1);
				server_port = atoi(server_ip_split[4]);
			}
		}
	}

	pclose(command_pipe);

	// Continue using UDP
	if (server_detect_udp)
	{
            printf("I thinks this is the port: %d\n", server_port);
		sprintf(command, "tcpdump -f -n -c 5");
		command_pipe = popen(command, "r");

		if (!command_pipe)
		{
			purple_debug_error("gfire", "Couldn't get UDP adresses for server detection.\n");
			pclose(command_pipe);
			return;
		}
		else
		{
			GRegex *regex;
			GMatchInfo *regex_matches;

			gchar *sender_ip_full, *receiver_ip_full;

			char udp_output[1024];
			memset(udp_output, 0, sizeof(udp_output));

			while(fgets(udp_output, sizeof(udp_output), command_pipe))
			{
				regex = g_regex_new(regex_ip_sender_tcpdump, G_REGEX_OPTIMIZE, 0, NULL);
				if (g_regex_match(regex, udp_output, 0, &regex_matches))
				{
					sender_ip_full = g_match_info_fetch(regex_matches, 0);
					sender_ip_full = purple_strreplace(sender_ip_full, " >", "");

					g_regex_unref(regex);
					g_match_info_free(regex_matches);

					gchar **sender_ip_full_split = g_strsplit(sender_ip_full, ".", -1);
					int sender_port = atoi(sender_ip_full_split[4]);

					if (sender_port != server_port)
						continue;

					regex = g_regex_new(regex_ip_receiver_tcpdump, G_REGEX_OPTIMIZE, 0, NULL);
					if (g_regex_match(regex, udp_output, 0, &regex_matches))
					{
						receiver_ip_full = g_match_info_fetch(regex_matches, 0);
						receiver_ip_full = purple_strreplace(receiver_ip_full, "> ", "");

						g_regex_unref(regex);
						g_match_info_free(regex_matches);

						gchar **receiver_ip_split = g_strsplit(receiver_ip_full, ".", -1);
						gchar *receiver_ip_wo_port = g_strdup_printf("%s.%s.%s.%s", receiver_ip_split[0],
											     receiver_ip_split[1], receiver_ip_split[2],
											     receiver_ip_split[3]);

						// Check if found IP is not remote IP
						if ((g_strcmp0(receiver_ip_wo_port, remote_ip) != 0))
						{
							server_ip = receiver_ip_full;
							break;
						}
					}
				}
			}
		}

		pclose(command_pipe);
	}

        // Store found IP
	if (server_ip)
	{
		gchar **server_ip_split = g_strsplit(server_ip, ".", -1);
		if (server_ip_split)
		{
                        gchar *server_ip_str = g_strdup_printf("%s.%s.%s.%s", server_ip_split[0], server_ip_split[1],
                                                               server_ip_split[2], server_ip_split[3]);
			guint32 server_port_tmp = atoi(server_ip_split[4]);

			gfire_game_data tmp_data;
			gfire_game_data_ip_from_str(&tmp_data, server_ip_str);

			// Set found IP
			g_mutex_lock(p_gfire->server_mutex);
                        if ((p_gfire->game_data.ip.value != tmp_data.ip.value) || (p_gfire->game_data.port != server_port_tmp))
				p_gfire->server_changed = TRUE;

			p_gfire->game_data.ip.value = tmp_data.ip.value;
			p_gfire->game_data.port = server_port_tmp;
			g_mutex_unlock(p_gfire->server_mutex);
		}
	}
        // Reset IP as none has been found
	else
	{
		g_mutex_lock(p_gfire->server_mutex);
		if (gfire_game_data_is_valid(&p_gfire->game_data))
			p_gfire->server_changed = TRUE;

                p_gfire->game_data.ip.value = 0;
                p_gfire->game_data.port = 0;
		g_mutex_unlock(p_gfire->server_mutex);
	}

}
