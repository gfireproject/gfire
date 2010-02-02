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

#include "gf_game_detection.h"
#include <time.h>

// Networking includes, required for the "HTTP-server" as libpurple
// offers no way to listen only on the localhost and makes sure that
// that the listening socket stays open
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <netinet/in.h>
#endif // _WIN32

static gfire_game_detector *gfire_detector = NULL;

static void gfire_game_detector_inform_instances()
{
	gchar *game_name = gfire_game_name(gfire_detector->game_data.id);;
	if(gfire_detector->game_data.id != 0)
		purple_debug_info("gfire", "%s is running, sending ingame status.\n", NN(game_name));
	else
		purple_debug_misc("gfire", "Game is not running anymore, sending out-of-game status.\n");
	g_free(game_name);

	GList *cur = gfire_detector->instances;
	while(cur)
	{
		gfire_set_game_status((gfire_data*)cur->data, &gfire_detector->game_data);
		cur = g_list_next(cur);
	}
}

static gboolean gfire_game_detector_detect_cb(void *p_unused)
{
	if(!gfire_detector)
		return FALSE;

	guint32 old_game = gfire_detector->game_data.id;
	guint32 new_game = 0;
	gboolean checked_old_game = FALSE;

	gfire_process_list_update(gfire_detector->process_list);

	xmlnode *gfire_game = gfire_game_config_node_first();
	// Check all configured games until we have checked all,
	// or have checked the old game and found a new one
	for (; (gfire_game != NULL) && (new_game == 0 || !checked_old_game);
		gfire_game = gfire_game_config_node_next(gfire_game))
	{
		xmlnode *command_node = NULL;
		xmlnode *detect_node = NULL;

		command_node = xmlnode_get_child(gfire_game, "command");
		if(!command_node)
			continue;

		detect_node = xmlnode_get_child(command_node, "detect");
		if(!detect_node)
			continue;

		gchar *game_executable;
		const gchar *game_id;
		guint32 game_id_int = 0;

		game_executable = xmlnode_get_data(detect_node);
		if(!game_executable)
			continue;

		game_id = xmlnode_get_attrib(gfire_game, "id");
		if(game_id)
			sscanf(game_id, "%u", &game_id_int);
		else
		{
			// We cannot successfully detect a game if we have no ID
			g_free(game_executable);
			continue;
		}

		// Arguments & required libraries are optional
		const gchar *game_exec_required_args = NULL;
		const gchar *game_exec_invalid_args = NULL;
		gchar *game_required_libraries = NULL;

		if (game_id_int)
		{
			xmlnode *game_config_node = gfire_game_node_by_id(game_id_int);
			xmlnode *game_exec_args_node = NULL;
			xmlnode *game_required_libraries_node = NULL;

			if (game_config_node)
			{
				game_exec_args_node = xmlnode_get_child(game_config_node, "arguments");
				game_required_libraries_node = xmlnode_get_child(game_config_node, "libraries");
			}

			if (game_exec_args_node)
			{
				game_exec_required_args = xmlnode_get_attrib(game_exec_args_node, "required");
				game_exec_invalid_args = xmlnode_get_attrib(game_exec_args_node, "invalid");
			}

			if (game_required_libraries_node)
				game_required_libraries = xmlnode_get_data(game_required_libraries_node);
		}
		else
		{
			purple_debug_error("gfire", "Couldn't get game ID to obtain game arguments and required libraries.\n");
			g_free(game_executable);
			continue;
		}

		gboolean process_running = gfire_process_list_contains(gfire_detector->process_list, game_executable,
															   game_exec_required_args, game_exec_invalid_args,
															   game_required_libraries);

		if (game_required_libraries)
			g_free(game_required_libraries);

		g_free(game_executable);

		// Old game not running anymore
		if(old_game == game_id_int && !process_running)
		{
			gfire_game_data_reset(&gfire_detector->game_data);;
			checked_old_game = TRUE;
		}
		// Old game still running, no need for further detection
		else if(old_game == game_id_int && process_running)
			break;
		// Save the first running new game
		else if(process_running && new_game == 0)
			new_game = game_id_int;
	}

	// Set a new game
	if(gfire_detector->game_data.id == 0)
	{
		gfire_detector->game_data.id = new_game;

		if(new_game != 0)
			gfire_detector->game_type = GFGT_PROCESS;
	}


	// If the game has changed, inform all instances about it
	if(gfire_detector->game_data.id != old_game)
		gfire_game_detector_inform_instances();

	return TRUE;
}

static gboolean gfire_game_detector_web_timeout_cb(void *p_unused)
{
	if(!gfire_detector || gfire_detector->game_type != GFGT_WEB)
		return FALSE;

	GTimeVal cur_time;
	g_get_current_time(&cur_time);

	if((cur_time.tv_sec - gfire_detector->web_timeout) >= GFIRE_WEB_DETECTION_TIMEOUT)
	{
		// Webgame timed out, kill it
		gfire_game_data_reset(&gfire_detector->game_data);

		// Kill this timeout check
		g_source_remove(gfire_detector->timeout_check);

		// Start process detection again
		gfire_detector->det_source = g_timeout_add_seconds(GFIRE_DETECTION_INTERVAL,
														   (GSourceFunc)gfire_game_detector_detect_cb, NULL);

		gfire_game_detector_inform_instances();

		return FALSE;
	}

	return TRUE;
}

static void gfire_game_detector_web_http_input_cb(gpointer p_con, gint p_fd, PurpleInputCondition p_condition)
{
	if(!p_con  || (p_condition != PURPLE_INPUT_READ))
		return;

	gfire_game_detection_http_connection *connection = p_con;
	if(connection->socket != p_fd)
		return;

	static gchar buffer[8193]; // 8 KiB + 1B for 0
	int bytes = recv(connection->socket, buffer, 8192, 0);
	// Connection closed
	if(bytes <= 0)
	{
		purple_input_remove(connection->input);
		close(connection->socket);

		g_free(connection);

		GList *node = g_list_find(gfire_detector->connections, connection);
		if(node)
			gfire_detector->connections = g_list_delete_link(gfire_detector->connections, node);

		purple_debug_info("gfire", "detection: http: client connection closed\n");

		return;
	}
	buffer[bytes] = 0;

	// VERY BASIC HTTP request parsing
	gint result = 200;
	gchar *url = NULL;

	if(!strstr(buffer, "\r\n\r\n"))
		result = 400;

	if(result == 200)
	{
		gchar **lines = g_strsplit(buffer, "\r\n", -1);
		if(lines)
		{
			// Check first line "GET <URI> HTTP/<major>.<minor>"
			if(strncmp(lines[0], "GET ", 4))
				result = 501;
			else
			{
				gchar **parts = g_strsplit(lines[0], " ", 3);
				if(parts)
				{
					if(g_strv_length(parts) != 3)
						result = 400;
					else if(strcmp(parts[0], "GET"))
						result = 501;
					else if(strncmp(parts[2], "HTTP/", 5))
						result = 400;
					else
						url = g_strdup(parts[1]);

					g_strfreev(parts);
				}
			}

			g_strfreev(lines);
		}
		else
			result = 400;
	}

	static gchar content[8192];
	content[0] = 0;

	if(url)
	{
		if(strncmp(url, "/url", 4))
			result = 404;
		else if(result == 200)
		{
			const gchar *args = strstr(url, "?");
			if(args)
			{
				if(!strncmp(args + 1, "url=", 4))
				{
					args += 5;

					gchar *unescaped = g_uri_unescape_string(args, "");
					if(unescaped)
					{
						purple_debug_info("gfire", "detection: http: new url request: %s\n", unescaped);

						guint32 gameid = gfire_game_id_by_url(unescaped);
						g_free(unescaped);

						// our game! :)
						if(gameid != 0 && gfire_detector->game_data.id == 0)
						{
							// Set new game
							gfire_detector->game_data.id = gameid;
							gfire_detector->game_type = GFGT_WEB;

							// Stop process checks
							g_source_remove(gfire_detector->det_source);
							gfire_detector->det_source = 0;

							// Add a timeout (browser might be just closed)
							gfire_detector->timeout_check =
									g_timeout_add_seconds(GFIRE_WEB_DETECTION_TIMEOUT,
														  (GSourceFunc)gfire_game_detector_web_timeout_cb, NULL);

							gfire_game_detector_inform_instances();
						}

						// Update the timeout
						if(gameid != 0 && gfire_detector->game_data.id == gameid)
						{
							GTimeVal cur_time;
							g_get_current_time(&cur_time);
							gfire_detector->web_timeout = cur_time.tv_sec;
						}
					}
				}
			}

			gchar *game_name = gfire_game_name(gfire_detector->game_data.id);
			gchar *game_short_name = gfire_game_short_name(gfire_detector->game_data.id);
			g_sprintf(content, "var result = {};\n"
					  "result[\"gameid\"] = \"%u\";\n"
					  "result[\"gameshortname\"] = \"%s\";\n"
					  "result[\"gamelongname\"] = \"%s\";\n"
					  "result[\"nickname\"] = \"%s\";\n"
					  "result[\"result\"] = \"ok\";\n"
					  "result[\"username\"] = \"%s\";\n"
					  "result[\"is_gfire\"] = true;\n"
					  "if (document.onSuccess) document.onSuccess(result);\n\n",
					  gfire_detector->game_data.id,
					  (gfire_detector->game_data.id != 0) ? NN(game_short_name) : "",
					  (gfire_detector->game_data.id != 0) ? NN(game_name) : "",
					  gfire_get_nick(gfire_detector->instances->data) ?
					  gfire_get_nick(gfire_detector->instances->data) : "",
					  gfire_get_name(gfire_detector->instances->data));
			if(game_name)
				g_free(game_name);
			if(game_short_name)
				g_free(game_short_name);
		}
	}

	const gchar *response_text = NULL;
	switch(result)
	{
	case 200:
		response_text = "OK";
		break;
	case 400:
		response_text = "Bad Request";
		break;
	case 404:
		response_text = "Not Found";
		break;
	case 501:
		response_text = "Not Implemented";
		break;
	default:
		response_text = "Not Implemented";
		break;
	}

	// Response header
	GString *response = g_string_new(NULL);
	g_string_append_printf(response, "HTTP/1.0 %d %s\r\n", result, response_text);

	// Location
	if(url)
	{
		g_string_append_printf(response, "Location: %s\r\n", url);
		g_free(url);
	}

	// Server
	g_string_append_printf(response, "Server: Gfire %s\r\n", GFIRE_VERSION_STRING);

	// Content-Length
	g_string_append_printf(response, "Content-Length: %u\r\n", (unsigned int)strlen(content));

	if(strlen(content) > 0)
	{
		// Date
		static gchar timebuf[300];
		time_t rawtime = time(NULL);
		struct tm *cur_time = localtime(&rawtime);

		strftime(timebuf, 300, "%a, %d %b %Y %H:%M:%S %Z", cur_time);
		g_string_append_printf(response, "Date: %s\r\n", timebuf);

		// Content-Type
		g_string_append(response, "Content-Type: text/javascript; charset=utf-8\r\n");
		//g_string_append(response, "Content-Type: text/html; charset=utf-8\r\n");
	}

	// Content
	g_string_append_printf(response, "\r\n%s", content);

	// Send response
	send(connection->socket, response->str, strlen(response->str), 0);
	g_string_free(response, TRUE);
}

static void gfire_game_detector_web_http_accept_cb(gpointer p_unused, gint p_fd, PurpleInputCondition p_condition)
{
	if(!gfire_detector || (gfire_detector->socket != p_fd) || (p_condition != PURPLE_INPUT_READ))
		return;

	struct sockaddr_in client_addr;
#ifndef _WIN32
	socklen_t len = sizeof(struct sockaddr_in);
	int client = accept(p_fd, (struct sockaddr*)&client_addr, &len);
	if(client < 0)
#else
	int len = sizeof(struct sockaddr_in);
	int client = accept(p_fd, (struct sockaddr*)&client_addr, &len);
	if(client == INVALID_SOCKET)
#endif // !_WIN32
	{
		return;
	}

	purple_debug_info("gfire", "detection: http: new client connection\n");

	gfire_game_detection_http_connection *connection = g_malloc0(sizeof(gfire_game_detection_http_connection));
	connection->socket = client;
	connection->input = purple_input_add(client, PURPLE_INPUT_READ, gfire_game_detector_web_http_input_cb, connection);

	gfire_detector->connections = g_list_append(gfire_detector->connections, connection);
}

static void gfire_game_detector_start_web_http()
{
	gfire_detector->socket = socket(PF_INET, SOCK_STREAM, 0);
	if(gfire_detector->socket < 0)
		return;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(39123); // Xfire WebGame port

	int on = 1;
	setsockopt(gfire_detector->socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	if(bind(gfire_detector->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0)
	{
		purple_debug_error("gfire", "detection: http: could not bind to 127.0.0.1:39123: %s\n", strerror(errno));
		close(gfire_detector->socket);
		gfire_detector->socket = -1;
		return;
	}

	if(listen(gfire_detector->socket, 5) != 0)
	{
		purple_debug_error("gfire", "detection: http: could not listen on 127.0.0.1:39123: %s\n", strerror(errno));
		close(gfire_detector->socket);
		gfire_detector->socket = -1;
		return;
	}

	gfire_detector->accept_input = purple_input_add(gfire_detector->socket, PURPLE_INPUT_READ,
													gfire_game_detector_web_http_accept_cb, NULL);

	purple_debug_info("gfire", "detection: http: started listening on 127.0.0.1:39123\n");
}

static void gfire_game_detector_stop_web_http()
{
	if(gfire_detector->socket >= 0)
	{
		// Close all client connection
		while(gfire_detector->connections)
		{
			gfire_game_detection_http_connection *connection = gfire_detector->connections->data;
			purple_input_remove(connection->input);
			close(connection->socket);

			g_free(connection);

			gfire_detector->connections = g_list_delete_link(gfire_detector->connections, gfire_detector->connections);
		}

		// Close the listening connection
		purple_input_remove(gfire_detector->accept_input);

		close(gfire_detector->socket);
		gfire_detector->socket = -1;

		purple_debug_info("gfire", "detection: http: stopped listening\n");
	}
}

static void gfire_game_detector_init()
{
	if(gfire_detector)
		return;

	gfire_detector = g_malloc0(sizeof(gfire_game_detector));
	gfire_detector->process_list = gfire_process_list_new();

	gfire_game_detector_start_web_http();

	// Start detection timer for processes
	gfire_detector->det_source = g_timeout_add_seconds(GFIRE_DETECTION_INTERVAL,
													   (GSourceFunc)gfire_game_detector_detect_cb, NULL);

	purple_debug_info("gfire", "detection: Detector started\n");
}

static void gfire_game_detector_free()
{
	if(!gfire_detector)
		return;

	// Remove detection timer
	if(gfire_detector->det_source != 0)
		g_source_remove(gfire_detector->det_source);

	gfire_game_detector_stop_web_http();

	gfire_process_list_free(gfire_detector->process_list);

	g_free(gfire_detector);
	gfire_detector = NULL;

	purple_debug_info("gfire", "detection: Detector freed\n");
}

void gfire_game_detector_register(gfire_data *p_gfire)
{
	if(!p_gfire)
		return;

	if(!gfire_detector)
		gfire_game_detector_init();

	gfire_detector->instances = g_list_append(gfire_detector->instances, p_gfire);

	purple_debug_info("gfire", "detection: Gfire instance added (new count: %u)\n",
					  g_list_length(gfire_detector->instances));
}

void gfire_game_detector_unregister(gfire_data *p_gfire)
{
	if(!gfire_detector || !p_gfire)
		return;

	GList *node = g_list_find(gfire_detector->instances, p_gfire);
	if(!node)
		return;

	gfire_detector->instances = g_list_delete_link(gfire_detector->instances, node);

	purple_debug_info("gfire", "detection: Gfire instance removed (new count: %u)\n",
					  g_list_length(gfire_detector->instances));

	if(!gfire_detector->instances)
		gfire_game_detector_free();
}

void gfire_game_detector_set_external_game(guint32 p_gameid)
{
	if(!gfire_detector)
		return;

	// Ignore external game while playing a non-external game
	if(gfire_detector->game_data.id != 0 && gfire_detector->game_type != GFGT_EXTERNAL)
		return;

	if(gfire_detector->game_data.id == 0 && p_gameid != 0)
	{
		// Playing an external game
		gfire_detector->game_data.id = p_gameid;
		gfire_detector->game_type = GFGT_EXTERNAL;

		// Stop process detection
		g_source_remove(gfire_detector->det_source);
		gfire_detector->det_source = 0;

		gfire_game_detector_inform_instances();
	}
	else if(gfire_detector->game_data.id != 0 && p_gameid == 0)
	{
		// Stopped playing an external game
		gfire_game_data_reset(&gfire_detector->game_data);

		// Start process detection
		gfire_detector->det_source = g_timeout_add_seconds(GFIRE_DETECTION_INTERVAL,
														   (GSourceFunc)gfire_game_detector_detect_cb, NULL);

		gfire_game_detector_inform_instances();
	}
}

gboolean gfire_game_detector_is_playing()
{
	return (gfire_detector && gfire_detector->game_data.id != 0);
}

gboolean gfire_game_detector_is_voiping()
{
	return (gfire_detector && gfire_detector->voip_data.id != 0);
}

guint32 gfire_game_detector_current_game()
{
	if(gfire_detector)
		return gfire_detector->game_data.id;
	else
		return 0;
}

guint32 gfire_game_detector_current_voip()
{
	if(gfire_detector)
		return gfire_detector->voip_data.id;
	else
		return 0;
}

gfire_process_list *gfire_process_list_new()
{
	gfire_process_list *ret = g_malloc0(sizeof(gfire_process_list));
	if(!ret)
		return NULL;

	return ret;
}

void gfire_process_list_free(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	gfire_process_list_clear(p_list);
	g_free(p_list);
}

process_info *gfire_process_info_new(const gchar *p_name, const gchar *p_exe, const guint32 p_pid, const gchar *p_args)
{
	if (!p_name || !p_exe || !p_pid)
		return NULL;

	process_info *ret = g_malloc0(sizeof(process_info));

	ret->name = g_strdup(p_name);
	ret->exe = g_strdup(p_exe);
	ret->pid = p_pid;

	if(p_args)
		ret->args = g_strdup(p_args);

	return ret;
}

static void gfire_process_info_free(process_info *p_info)
{
	if(!p_info)
		return;

	if(p_info->name)
		g_free(p_info->name);

	if(p_info->exe)
		g_free(p_info->exe);

	if(p_info->args)
		g_free(p_info->args);

	g_free(p_info);
}

void gfire_process_list_clear(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	while(p_list->processes)
	{
		gfire_process_info_free((process_info*)p_list->processes->data);
		p_list->processes = g_list_delete_link(p_list->processes, p_list->processes);
	}
}

gboolean gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_name, const gchar *p_required_args, const gchar *p_invalid_args, const gchar *p_required_libraries)
{
	if(!p_list || !p_list->processes || !p_name)
		return FALSE;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

		if(g_strcmp0(info->name, p_name) == 0)
		{
			// First check invalid arguments
			gboolean process_invalid_args = FALSE;

			if(p_invalid_args && strlen(p_invalid_args) > 0)
			{
				gchar **p_invalid_args_parts = g_strsplit(p_invalid_args, ";", -1);
				if (p_invalid_args_parts)
				{
					int i;
					for(i = 0; i < g_strv_length(p_invalid_args_parts); i++)
					{
						if (g_strrstr(info->args, p_invalid_args_parts[i]) != NULL)
						{
							process_invalid_args = TRUE;
							break;
						}
					}

					g_strfreev(p_invalid_args_parts);
				}
			}

			// Then check required args
			gboolean process_required_args = TRUE;

			if (process_invalid_args != TRUE)
			{
				if(p_required_args && strlen(p_required_args) > 0)
				{
					gchar **p_required_args_parts = g_strsplit(p_required_args, ";", -1);
					if (p_required_args_parts)
					{
						int i;
						for(i = 0; i < g_strv_length(p_required_args_parts); i++)
						{
							if (g_strrstr(info->args, p_required_args_parts[i]) == NULL)
							{
								process_required_args = FALSE;
								break;
							}
						}

						g_strfreev(p_required_args_parts);
					}
				}
			}

			// Finally check required libraries
			gboolean process_required_libraries = TRUE;
#ifndef _WIN32
			if (p_required_libraries && strlen(p_required_libraries) > 0)
			{
				process_required_libraries = FALSE;
				gchar **required_libraries_split = g_strsplit(p_required_libraries, ";", -1);

				// Get process libs
				GList *process_libs = gfire_game_detection_get_process_libraries(info->pid);
				GList *cur = process_libs;

				for(; cur != NULL; cur = g_list_next(cur))
				{
					int i;
					for(i = 0; i < g_strv_length(required_libraries_split); i++)
					{
						if (!g_strcmp0(g_strchomp(cur->data), required_libraries_split[i]))
						{
							process_required_libraries = TRUE;
							break;
						}
					}

					if (process_required_libraries)
						break;
				}

				g_strfreev(required_libraries_split);

				gfire_game_detection_process_libraries_clear(process_libs);
				g_list_free(process_libs);
			}
#endif // _WIN32

			// Return as found if valid
			if (process_invalid_args == FALSE && process_required_args == TRUE && process_required_libraries == TRUE)
				return TRUE;
		}

		cur = g_list_next(cur);
	}

	return FALSE;
}

guint32 gfire_process_list_get_pid(const gfire_process_list *p_list, const gchar *p_name)
{
	if (!p_list || !p_name)
		return 0;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

		if(!g_strcmp0(info->name, p_name))
			return info->pid;

		cur = g_list_next(cur);
	}

	// Return nothing found
	return 0;
}

gchar *gfire_process_list_get_exe(const gfire_process_list *p_list, const gchar *p_name)
{
	if (!p_list || !p_name)
		return NULL;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

		if(!g_strcmp0(info->name, p_name))
			return info->exe;

		cur = g_list_next(cur);
	}

	// Return error (-1) if nothing found
	return NULL;
}
