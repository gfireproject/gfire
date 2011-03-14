/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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
// offers no way to listen only on the localhost.
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <fcntl.h>
#endif // _WIN32

#if defined(USE_DBUS_STATUS_CHANGE) && !defined(_WIN32)
#	include <dbus/dbus-glib.h>

// KMess status updates
static void setKMessInstanceStatus(DBusGConnection *p_connection, const gchar *p_instance, const gchar *p_message)
{
	if(!p_connection || !p_instance || !p_message)
		return;

	DBusGProxy *proxy = dbus_g_proxy_new_for_name(p_connection, p_instance, "/remoteControl", "org.kmess.remoteControl");
	if(!proxy)
		return;

	dbus_g_proxy_call_no_reply(proxy, "setPersonalMessage", G_TYPE_STRING, p_message, G_TYPE_INVALID);

	purple_debug_info("gfire", "kmess status: changed to \"%s\" for instance \"%s\"\n", p_message,
					  p_instance);

	g_object_unref(proxy);
}

static void setKMessStatus(const gchar *p_message)
{
	if(!p_message)
		return;

	GError *error = NULL;

	DBusGConnection *connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if(!connection)
	{
		purple_debug_error("gfire", "kmess status: dbus_g_bus_get: %s\n", error->message);
		g_error_free(error);
		return;
	}

	DBusGProxy *proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	if(!proxy)
		return;

	gchar **names = NULL;
	if(!dbus_g_proxy_call(proxy, "ListNames", &error, G_TYPE_INVALID, G_TYPE_STRV, &names, G_TYPE_INVALID))
	{
		purple_debug_error("gfire", "kmess status: dbus_g_proxy_call: %s\n", error->message);
		g_error_free(error);

		g_object_unref(proxy);
		return;
	}

	g_object_unref(proxy);

	if(names)
	{
		int i = 0;
		for(; names[i]; i++)
		{
			if(strncmp(names[i], "org.kmess.kmess-", 16) == 0)
				setKMessInstanceStatus(connection, names[i], p_message);
		}

		g_strfreev(names);
	}
}
#endif // USE_DBUS_STATUS_CHANGE && !_WIN32

static gfire_game_detector *gfire_detector = NULL;

static void gfire_game_detector_inform_instances_game()
{
	gchar *game_name = gfire_game_name(gfire_detector->game_data.id, TRUE);

	if(gfire_detector->game_data.id != 0)
	{
		gchar *addr = gfire_game_data_addr_str(&gfire_detector->game_data);
		purple_debug_info("gfire", "%s is running, sending ingame status. (%s)\n", game_name, addr);
		g_free(addr);
	}
	else
		purple_debug_misc("gfire", "Game is not running anymore, sending out-of-game status.\n");

	GList *cur = gfire_detector->instances;
	gboolean do_global_status = FALSE;
	while(cur)
	{
		gfire_set_game_status((gfire_data*)cur->data, &gfire_detector->game_data);

		// Set to true if at least one account wants that
		do_global_status = (do_global_status || gfire_wants_global_status_change((gfire_data*)cur->data));
		cur = g_list_next(cur);
	}

	// Change the status of non-Xfire accounts as well if requested
	if(do_global_status)
	{
		GList *accounts = purple_accounts_get_all_active();
		GList *cur = accounts;
		while(cur)
		{
			PurpleAccount *account = (PurpleAccount*)cur->data;

			// Only change the status for non-Xfire protocols
			if(!purple_utf8_strcasecmp(purple_account_get_protocol_id(account), GFIRE_PRPL_ID))
			{
				cur = g_list_next(cur);
				continue;
			}

			// Set gaming status
			if(gfire_detector->game_data.id != 0)
			{
				gchar *msg = g_strdup_printf(_("Playing %s"), game_name);
				PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account,
																							  PURPLE_STATUS_UNAVAILABLE);
				// Set the status to unavailable/busy/dnd
				if(status_type)
				{
					PurplePresence *presence = purple_account_get_presence(account);
					if(presence)
					{
						PurpleStatus *status = purple_presence_get_status(presence,
																		  purple_status_type_get_id(status_type));
						if(status)
						{
							if(purple_status_type_get_attr(status_type, "message"))
							{
								purple_debug_info("gfire", "detection: Setting %s status to: %s\n",
												  purple_account_get_username(account), msg);

								GList *attrs = NULL;
								attrs = g_list_append(attrs, "message");
								attrs = g_list_append(attrs, g_strdup(msg));
								purple_status_set_active_with_attrs_list(status, TRUE, attrs);
								g_list_free(attrs);
							}
							else
								purple_status_set_active(status, TRUE);
						}
					}
				}
				// No unavailable/busy/dnd status supported, just change the current status' message
				else
				{
					PurpleStatus *status = purple_account_get_active_status(account);
					if(purple_status_type_get_attr(purple_status_get_type(status), "message"))
					{
						purple_debug_info("gfire", "detection: Setting %s status to: %s\n",
										  purple_account_get_username(account), msg);

						GList *attrs = NULL;
						attrs = g_list_append(attrs, "message");
						attrs = g_list_append(attrs, g_strdup(msg));
						purple_status_set_active_with_attrs_list(status, TRUE, attrs);
						g_list_free(attrs);
					}
				}
				g_free(msg);
			}
			// Restore old status
			else
			{
				PurpleSavedStatus *savedstatus = purple_savedstatus_get_current();
				if(savedstatus)
				{
					purple_debug_info("gfire", "detection: Resetting %s status\n",
										  purple_account_get_username(account));

					purple_savedstatus_activate_for_account(savedstatus, account);
				}
				else
					purple_debug_warning("gfire", "detection: no status for status reset found\n");
			}

			cur = g_list_next(cur);
		}

		g_list_free(accounts);

		// Set external gaming status
#if defined(USE_DBUS_STATUS_CHANGE) && !defined(_WIN32)
		if(gfire_detector->game_data.id != 0)
		{
			gchar *msg = g_strdup_printf(_("Playing %s"), game_name);
			// KMess
			setKMessStatus(msg);
			g_free(msg);
		}
		else
			// KMess
			setKMessStatus("");
#endif // USE_DBUS_STATUS_CHANGE && !_WIN32
	}

	g_free(game_name);
}

static void gfire_game_detector_inform_instances_voip()
{
	if(gfire_detector->voip_data.id != 0)
	{
		gchar *voip_name = gfire_game_name(gfire_detector->voip_data.id, FALSE);
		gchar *addr = gfire_game_data_addr_str(&gfire_detector->voip_data);
		purple_debug_info("gfire", "%s is running, sending VoIP status. (%s)\n", voip_name, addr);
		g_free(addr);
		g_free(voip_name);
	}
	else
		purple_debug_misc("gfire", "VoIP application is not running anymore, sending status.\n");

	GList *cur = gfire_detector->instances;
	while(cur)
	{
		gfire_set_voip_status((gfire_data*)cur->data, &gfire_detector->voip_data);
		cur = g_list_next(cur);
	}
}

typedef struct _gfire_process_detection_data
{
	const gfire_game_configuration *gconf;

	// Game
	const gfire_game_detection_set *g_dset;
	gboolean g_checked_old;
	gboolean g_old_running;
	guint32 g_new_game;
	guint32 g_pid;

	// VoIP
	const gfire_game_detection_set *v_dset;
	gboolean v_checked_old;
	gboolean v_old_running;
	guint32 v_new_voip;
	guint32 v_pid;
} gfire_process_detection_data;

static gboolean gfire_game_detector_detect_game_dset_cb(const gfire_game *p_game, const gfire_game_detection_set *p_dset,
														gpointer p_data)
{
	gfire_process_detection_data *data = (gfire_process_detection_data*)p_data;

	// Check if the current detection set matches our environment
	guint32 pid = gfire_process_list_contains(gfire_detector->process_list, data->gconf->detect_file,
											  p_dset->required_args, p_dset->invalid_args/*, NULL*/);

	// VoIP app
	if(p_game->is_voice)
	{
		if((gfire_detector->voip_data.id == p_game->id) && !data->v_old_running)
		{
			data->v_checked_old = TRUE;
			data->v_old_running = (pid != 0);
			if(pid)
			{
				data->v_pid = pid;
				data->v_dset = p_dset;
			}
		}
		else if(pid)
		{
			data->v_new_voip = p_game->id;
			data->v_pid = pid;
			data->v_dset = p_dset;
		}
	}
	// Game
	else
	{
		if((gfire_detector->game_data.id == p_game->id) && !data->g_old_running)
		{
			data->g_checked_old = TRUE;
			data->g_old_running = (pid != 0);
			if(pid)
			{
				data->g_pid = pid;
				data->g_dset = p_dset;
			}
		}
		else if(pid)
		{
			data->g_new_game = p_game->id;
			data->g_pid = pid;
			data->g_dset = p_dset;
		}
	}

	// Check if we can abort
		// Old game still running / not running anymore but new one available
	if(((gfire_detector->game_data.id && data->g_checked_old && (data->g_old_running || data->g_new_game)) ||
		// No old game present, but new one found
		(!gfire_detector->game_data.id && data->g_new_game)) &&
	   // Old VoIP app still running / not running anymore but new one available
	   ((gfire_detector->voip_data.id && data->v_checked_old && (data->v_old_running || data->v_new_voip)) ||
		// No old VoIP app present, but new one found
		(!gfire_detector->voip_data.id && data->v_new_voip)))
		return TRUE;

	return FALSE;
}

static gboolean gfire_game_detector_detect_conf_game_cb(const gfire_game_configuration *p_gconf, gpointer p_data)
{
	const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
	if(game)
	{
		gfire_process_detection_data *data = (gfire_process_detection_data*)p_data;
		data->gconf = p_gconf;

		// If a detection callback aborted the loop we abort as well to stop detection
		if(gfire_game_foreach_dset(game, G_CALLBACK(gfire_game_detector_detect_game_dset_cb), data, FALSE))
			return TRUE;
	}

	return FALSE;
}

static gboolean gfire_game_detector_detect_cb(void *p_unused)
{
	if(!gfire_detector)
		return FALSE;

	gfire_process_list_update(gfire_detector->process_list);

	gfire_process_detection_data *data = g_malloc0(sizeof(gfire_process_detection_data));
	gfire_game_config_foreach(G_CALLBACK(gfire_game_detector_detect_conf_game_cb), data);

	// We have detected a game
	if(data->g_pid)
	{
		// We got a new game?
		if(!gfire_detector->game_data.id || !data->g_old_running)
		{
			gfire_server_detector_stop(gfire_detector->g_server_detector);
			gfire_detector->g_server_changed = FALSE;
			gfire_detector->g_server_ip = 0;
			gfire_detector->g_server_port = 0;

			gfire_game_data_reset(&gfire_detector->game_data);
			gfire_detector->game_data.id = data->g_new_game;
			gfire_detector->game_type = GFGT_PROCESS;

			gfire_game_detector_inform_instances_game();

			// Start new server detection if allowed
			if(gfire_detector->server_detection_ref && data->g_dset->detect_server)
				gfire_server_detector_start(gfire_detector->g_server_detector, gfire_detector->game_data.id,
											data->g_pid);
		}
		// We do still play the old game, start a server detection cycle
		else
		{
			// Check for changed server data
			g_mutex_lock(gfire_detector->server_mutex);
			if(gfire_detector->g_server_changed)
			{
				gfire_detector->g_server_changed = FALSE;

				gfire_detector->game_data.ip.value = gfire_detector->g_server_ip;
				gfire_detector->game_data.port = gfire_detector->g_server_port;
				g_mutex_unlock(gfire_detector->server_mutex);

				gfire_game_detector_inform_instances_game();
			}
			else
				g_mutex_unlock(gfire_detector->server_mutex);

			// Start new server detection if allowed
			if(gfire_detector->server_detection_ref && data->g_dset->detect_server)
				gfire_server_detector_start(gfire_detector->g_server_detector, gfire_detector->game_data.id,
											data->g_pid);
		}
	}
	// No game playing
	else
	{
		if(gfire_detector->game_data.id)
		{
			gfire_server_detector_stop(gfire_detector->g_server_detector);
			gfire_detector->g_server_changed = FALSE;
			gfire_detector->g_server_ip = 0;
			gfire_detector->g_server_port = 0;

			gfire_game_data_reset(&gfire_detector->game_data);
			gfire_game_detector_inform_instances_game();
		}
	}

	// We have detected a VoIP app
	if(data->v_pid)
	{
		// We got a new VoIP app?
		if(!gfire_detector->voip_data.id || !data->v_old_running)
		{
			gfire_server_detector_stop(gfire_detector->v_server_detector);
			gfire_detector->v_server_changed = FALSE;
			gfire_detector->v_server_ip = 0;
			gfire_detector->v_server_port = 0;

			gfire_game_data_reset(&gfire_detector->voip_data);
			gfire_detector->voip_data.id = data->v_new_voip;

			gfire_game_detector_inform_instances_voip();

			// Start new server detection if allowed
			if(gfire_detector->server_detection_ref && data->v_dset->detect_server)
				gfire_server_detector_start(gfire_detector->v_server_detector, gfire_detector->voip_data.id,
											data->v_pid);
		}
		// Still the old one
		else
		{
			// Check for changed server data
			g_mutex_lock(gfire_detector->server_mutex);
			if(gfire_detector->v_server_changed)
			{
				gfire_detector->v_server_changed = FALSE;

				gfire_detector->voip_data.ip.value = gfire_detector->v_server_ip;
				gfire_detector->voip_data.port = gfire_detector->v_server_port;
				g_mutex_unlock(gfire_detector->server_mutex);

				gfire_game_detector_inform_instances_voip();
			}
			else
				g_mutex_unlock(gfire_detector->server_mutex);

			// Start new server detection if allowed
			if(gfire_detector->server_detection_ref && data->v_dset->detect_server)
				gfire_server_detector_start(gfire_detector->v_server_detector, gfire_detector->voip_data.id,
											data->v_pid);
		}
	}
	// No VoIP app running
	else
	{
		if(gfire_detector->voip_data.id)
		{
			gfire_server_detector_stop(gfire_detector->v_server_detector);
			gfire_detector->v_server_changed = FALSE;
			gfire_detector->v_server_ip = 0;
			gfire_detector->v_server_port = 0;

			gfire_game_data_reset(&gfire_detector->voip_data);
			gfire_game_detector_inform_instances_voip();
		}
	}

	g_free(data);

	return TRUE;
}

static void gfire_game_detector_update_game_server(guint32 p_server_ip, guint16 p_server_port)
{
	g_mutex_lock(gfire_detector->server_mutex);
	// Only apply changes
	if(gfire_detector->g_server_ip != p_server_ip || gfire_detector->g_server_port != p_server_port)
	{
		gfire_detector->g_server_changed = TRUE;
		gfire_detector->g_server_ip = p_server_ip;
		gfire_detector->g_server_port = p_server_port;
	}
	g_mutex_unlock(gfire_detector->server_mutex);
}

static void gfire_game_detector_update_voip_server(guint32 p_server_ip, guint16 p_server_port)
{
	g_mutex_lock(gfire_detector->server_mutex);
	// Only apply changes
	if(gfire_detector->v_server_ip != p_server_ip || gfire_detector->v_server_port != p_server_port)
	{
		gfire_detector->v_server_changed = TRUE;
		gfire_detector->v_server_ip = p_server_ip;
		gfire_detector->v_server_port = p_server_port;
	}
	g_mutex_unlock(gfire_detector->server_mutex);
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

		gfire_game_detector_inform_instances_game();

		return FALSE;
	}

	return TRUE;
}

static void gfire_game_detector_web_http_input_cb(gpointer p_con, gint p_fd, PurpleInputCondition p_condition)
{
	if(!p_con  || !(p_condition & PURPLE_INPUT_READ))
		return;

	gfire_game_detection_http_connection *connection = p_con;
	if(connection->socket != p_fd)
		return;

	int bytes = recv(connection->socket, connection->buffer + connection->reclen, 8192 - connection->reclen, 0);
	// Connection closed
	if(bytes <= 0)
	{
		purple_input_remove(connection->input);
		close(connection->socket);

		g_free(connection);

		GList *node = g_list_find(gfire_detector->connections, connection);
		if(node)
			gfire_detector->connections = g_list_delete_link(gfire_detector->connections, node);

#ifdef DEBUG
		purple_debug_misc("gfire", "detection: http: client connection closed\n");
#endif // DEBUG

		return;
	}
	connection->reclen += bytes;
	connection->buffer[connection->reclen] = 0;

	// VERY BASIC HTTP request parsing
	gint result = 200;
	gchar *url = NULL;
	gchar *end = NULL;
	gboolean persistent = TRUE;

	// Request not finished yet, wait for more...
	if(!(end = strstr(connection->buffer, "\r\n\r\n")) && connection->reclen < 8192)
		return;

	end += 4;

	if(connection->reclen == 8192)
		result = 413;
	else
	{
		gchar **lines = g_strsplit(connection->buffer, "\r\n", -1);
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

					if(strcmp(parts[2], "HTTP/1.1"))
						persistent = FALSE;

					g_strfreev(parts);
				}
			}

			gchar **cur = lines + 1;
			while(*cur)
			{
				if(!strcasecmp(*cur, "Connection: keep-alive"))
				{
					persistent = TRUE;
					break;
				}
				cur++;
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

							gfire_game_detector_inform_instances_game();
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

			gchar *game_name = gfire_game_name(gfire_detector->game_data.id, TRUE);
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
	case 413:
		response_text = "Request Entity Too Large";
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
	g_string_append_printf(response, "HTTP/1.1 %d %s\r\n", result, response_text);

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

	// Keep-Alive
	if(result == 200 && persistent)
		g_string_append(response, "Connection: keep-alive\r\n");
	else
		g_string_append(response, "Connection: close\r\n");

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

	if(result == 413)
	{
		purple_input_remove(connection->input);
		close(connection->socket);

		g_free(connection);

		GList *node = g_list_find(gfire_detector->connections, connection);
		if(node)
			gfire_detector->connections = g_list_delete_link(gfire_detector->connections, node);

#ifdef DEBUG
		purple_debug_warning("gfire", "detection: http: client connection closed because of error\n");
#endif // DEBUG

		return;
	}

	// Be ready for another request
	guint extralen = connection->reclen - (end - connection->buffer);
	memmove(connection->buffer, end, extralen);
	connection->buffer[extralen] = 0;
	connection->reclen = extralen;
}

static void gfire_game_detector_web_http_accept_cb(gpointer p_unused, gint p_fd, PurpleInputCondition p_condition)
{
	if(!gfire_detector || (gfire_detector->socket != p_fd) || !(p_condition & PURPLE_INPUT_READ))
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

#ifdef DEBUG
	purple_debug_misc("gfire", "detection: http: new client connection\n");
#endif // DEBUG

	gfire_game_detection_http_connection *connection = g_malloc0(sizeof(gfire_game_detection_http_connection));
	connection->socket = client;
	connection->input = purple_input_add(client, PURPLE_INPUT_READ, gfire_game_detector_web_http_input_cb, connection);

	gfire_detector->connections = g_list_append(gfire_detector->connections, connection);
}

static gboolean gfire_game_detector_bind_http(gpointer p_unused)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(39123); // Xfire WebGame port

	if(bind(gfire_detector->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
	{
		purple_debug_warning("gfire", "detection: http: could not bind to 127.0.0.1:39123: %s\n", g_strerror(errno));
		return TRUE;
	}

	if(listen(gfire_detector->socket, 5) == -1)
	{
		purple_debug_error("gfire", "detection: http: could not listen on 127.0.0.1:39123: %s\n", g_strerror(errno));
		close(gfire_detector->socket);
		gfire_detector->socket = -1;
		gfire_detector->bind_timeout = 0;
		return FALSE;
	}

	int flags = fcntl(gfire_detector->socket, F_GETFL);
	fcntl(gfire_detector->socket, F_SETFL, flags | O_NONBLOCK);
#ifndef _WIN32
	fcntl(gfire_detector->socket, F_SETFD, FD_CLOEXEC);
#endif

	gfire_detector->accept_input = purple_input_add(gfire_detector->socket, PURPLE_INPUT_READ,
													gfire_game_detector_web_http_accept_cb, NULL);

	purple_debug_info("gfire", "detection: http: started listening on 127.0.0.1:39123\n");

	gfire_detector->bind_timeout = 0;
	return FALSE;
}

static void gfire_game_detector_start_web_http()
{
	gfire_detector->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(gfire_detector->socket < 0)
		return;

	int on = 1;
	if(setsockopt(gfire_detector->socket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) == -1)
		purple_debug_warning("gfire", "detection: http: SO_REUSEADDR: %s\n", g_strerror(errno));

	if(gfire_game_detector_bind_http(NULL))
		gfire_detector->bind_timeout = g_timeout_add_seconds(5, gfire_game_detector_bind_http, NULL);
}

static void gfire_game_detector_stop_web_http()
{
	if(gfire_detector->socket >= 0)
	{
		// Abort all binding retries
		if(gfire_detector->bind_timeout)
		{
			g_source_remove(gfire_detector->bind_timeout);
			gfire_detector->bind_timeout = 0;
		}

		// Close the listening socket
		if(gfire_detector->accept_input)
			purple_input_remove(gfire_detector->accept_input);

		close(gfire_detector->socket);
		gfire_detector->socket = -1;

		// Close all client connections
		while(gfire_detector->connections)
		{
			gfire_game_detection_http_connection *connection = gfire_detector->connections->data;
			purple_input_remove(connection->input);
			close(connection->socket);

			g_free(connection);

			gfire_detector->connections = g_list_delete_link(gfire_detector->connections, gfire_detector->connections);
		}

		purple_debug_info("gfire", "detection: http: stopped listening\n");
	}
}

static void gfire_game_detector_init()
{
	if(gfire_detector)
		return;

	gfire_detector = g_malloc0(sizeof(gfire_game_detector));

	gfire_detector->server_mutex = g_mutex_new();
	gfire_detector->g_server_detector = gfire_server_detector_create(G_CALLBACK(gfire_game_detector_update_game_server));
	gfire_detector->v_server_detector = gfire_server_detector_create(G_CALLBACK(gfire_game_detector_update_voip_server));

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

	// Stop server detection
	gfire_server_detector_stop(gfire_detector->g_server_detector);
	gfire_server_detector_stop(gfire_detector->v_server_detector);
	gfire_server_detector_free(gfire_detector->g_server_detector);
	gfire_server_detector_free(gfire_detector->v_server_detector);
	g_mutex_free(gfire_detector->server_mutex);

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
	if(gfire_wants_server_detection(p_gfire))
		gfire_detector->server_detection_ref++;

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

	if(gfire_wants_server_detection(p_gfire))
		gfire_detector->server_detection_ref--;
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

		gfire_game_detector_inform_instances_game();
	}
	else if(gfire_detector->game_data.id != 0 && p_gameid == 0)
	{
		// Stopped playing an external game
		gfire_game_data_reset(&gfire_detector->game_data);

		// Start process detection
		gfire_detector->det_source = g_timeout_add_seconds(GFIRE_DETECTION_INTERVAL,
														   (GSourceFunc)gfire_game_detector_detect_cb, NULL);

		gfire_game_detector_inform_instances_game();
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

guint32 gfire_game_detector_current_gameid()
{
	if(gfire_detector)
		return gfire_detector->game_data.id;
	else
		return 0;
}

const gfire_game_data *gfire_game_detector_current_game()
{
	if(gfire_detector)
		return &gfire_detector->game_data;
	else
		return NULL;
}

guint32 gfire_game_detector_current_voipid()
{
	if(gfire_detector)
		return gfire_detector->voip_data.id;
	else
		return 0;
}

const gfire_game_data *gfire_game_detector_current_voip()
{
	if(gfire_detector)
		return &gfire_detector->voip_data;
	else
		return NULL;
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

process_info *gfire_process_info_new(const gchar *p_exe, const guint32 p_pid, const gchar *p_args)
{
	if (!p_exe || !p_pid)
		return NULL;

	process_info *ret = g_malloc0(sizeof(process_info));

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

guint32 gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_exe, const GList *p_required_args,
									const GList *p_invalid_args/*, const GList *p_required_libraries*/)
{
	if(!p_list || !p_list->processes || !p_exe)
		return FALSE;

#ifdef DEBUG_VERBOSE
	purple_debug_info("gfire", "gfire_process_list_contains: checking for \"%s\"\n", p_exe);
#endif // DEBUG_VERBOSE

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

#ifdef DEBUG_VERBOSE
		purple_debug_info("gfire", "gfire_process_list_contains: comparing with \"%s\"\n", info->exe);
#endif // DEBUG_VERBOSE

#ifdef _WIN32
		// Windows handles file names case-insensitive in contrast to most other OSes
		if(purple_utf8_strcasecmp(info->exe, p_exe) == 0)
#else
		if(g_strcmp0(info->exe, p_exe) == 0)
#endif // _WIN32
		{
			// First check invalid arguments
			gboolean process_invalid_args = FALSE;

			const GList *cur_arg = p_invalid_args;
			while(cur_arg)
			{
				if (strstr(info->args, cur_arg->data) != NULL)
				{
					process_invalid_args = TRUE;
					break;
				}
				cur_arg = g_list_next(cur_arg);
			}

			// Then check required args
			gboolean process_required_args = TRUE;

			if (process_invalid_args != TRUE)
			{
				cur_arg = p_required_args;
				while(cur_arg)
				{
					if (strstr(info->args, cur_arg->data) == NULL)
					{
						process_required_args = FALSE;
						break;
					}
					cur_arg = g_list_next(cur_arg);
				}
			}

			// Finally check required libraries
			/*gboolean process_required_libraries = TRUE;
#ifndef _WIN32
			if (p_required_libraries)
			{
				process_required_libraries = FALSE;

				// Get process libs
				GList *process_libs = gfire_game_detection_get_process_libraries(info->pid);
				GList *cur_lib = process_libs;

				for(; cur_lib != NULL; cur_lib = g_list_next(cur))
				{
					const GList *cur_req_lib = p_required_libraries;
					while(cur_req_lib)
					{
						if (!g_strcmp0(g_strchomp(cur_lib->data), cur_req_lib->data))
						{
							process_required_libraries = TRUE;
							break;
						}
						cur_req_lib = g_list_next(cur_req_lib);
					}

					if (process_required_libraries)
						break;
				}

				gfire_game_detection_process_libraries_clear(process_libs);
				g_list_free(process_libs);
			}
#endif // _WIN32*/

			// Return as found if valid
			if ((process_invalid_args == FALSE) && (process_required_args == TRUE) /*&& process_required_libraries == TRUE*/)
			{
#ifdef DEBUG_VERBOSE
				purple_debug_info("gfire", "gfire_process_list_contains: MATCH!\n");
#endif // DEBUG_VERBOSE
				return info->pid;
			}
#ifdef DEBUG_VERBOSE
			else
				purple_debug_info("gfire", "gfire_process_list_contains: failed args test: %d/%d\n", process_invalid_args, process_required_args);
#endif // DEBUG_VERBOSE

		}

		cur = g_list_next(cur);
	}

#ifdef DEBUG_VERBOSE
	purple_debug_info("gfire", "gfire_process_list_contains: no match!\n");
#endif // DEBUG_VERBOSE
	return 0;
}

guint32 gfire_process_list_get_pid(const gfire_process_list *p_list, const gchar *p_exe)
{
	if (!p_list || !p_exe)
		return 0;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

		if(!g_strcmp0(info->exe, p_exe))
			return info->pid;

		cur = g_list_next(cur);
	}

	// Return nothing found
	return 0;
}
