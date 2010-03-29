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

#include "gf_games.h"
#include "gfire.h"
#include <time.h>

// Static game data & configuration for game handling (shared by all Gfire instances)
static guint32 gfire_games_version = 0;
static GList *gfire_games = NULL;
static GList *gfire_games_external = NULL;
static GList *gfire_games_config = NULL;

void gfire_update_games_list_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const gchar *p_buf, gsize p_len, const gchar *p_error_message)
{
	gfire_games_update_done();

	if (!p_data || !p_buf || !p_len)
		purple_debug_error("gfire", "An error occured while updating the games list. Website down?\n");
	else if(purple_util_write_data_to_file("gfire_games.xml", p_buf, p_len))
	{
		gfire_game_load_games_xml();

		gchar *version = gfire_game_get_version_str();
		gchar *msg = g_strdup_printf(_("The Games List has been updated to version: %s."), version);
		g_free(version);
#ifdef USE_NOTIFICATIONS
		if(purple_account_get_bool(purple_connection_get_account(p_data), "use_notify", TRUE))
			gfire_notify_system(_("New Gfire Game List Version"), msg);
		else
#endif // USE_NOTIFICATIONS
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("New Gfire Game List Version"),
								  _("New Gfire Game List Version"), msg, NULL, NULL);
		g_free(msg);
	}
	else
		purple_debug_error("gfire", "An error occured while updating the games list. Website down?\n");
}

void gfire_game_data_reset(gfire_game_data *p_game)
{
	if(!p_game)
		return;

	p_game->id = 0;
	p_game->ip.value = 0;
	p_game->port = 0;
}

void gfire_game_data_ip_from_str(gfire_game_data *p_game, const gchar *p_ipstr)
{
	if(!p_game || !p_ipstr)
		return;

	gchar **ip_octet_strs = g_strsplit(p_ipstr, ".", -1);
	if(!ip_octet_strs)
		return;

	guint8 i = 0;
	for(i = 4; i > 0; i--)
	{
		if(!ip_octet_strs[4 - i])
		{
			p_game->ip.value = 0;
			g_strfreev(ip_octet_strs);
			return;
		}

		p_game->ip.octets[i - 1] = atoi(ip_octet_strs[4 - i]);
	}

	g_strfreev(ip_octet_strs);
}

gboolean gfire_game_data_is_valid(const gfire_game_data *p_game)
{
	return (p_game && (p_game->id != 0));
}

gboolean gfire_game_data_has_addr(const gfire_game_data *p_game)
{
	return (p_game && (p_game->ip.value != 0));
}

gchar *gfire_game_data_ip_str(const gfire_game_data *p_game)
{
	if(!p_game)
		return NULL;

	return g_strdup_printf("%u.%u.%u.%u", p_game->ip.octets[3], p_game->ip.octets[2], p_game->ip.octets[1], p_game->ip.octets[0]);
}

gchar *gfire_game_data_port_str(const gfire_game_data *p_game)
{
	if(!p_game)
		return NULL;

	return g_strdup_printf("%u", p_game->port);
}

gchar *gfire_game_data_addr_str(const gfire_game_data *p_game)
{
	if(!p_game)
		return NULL;

	return g_strdup_printf("%u.%u.%u.%u:%u", p_game->ip.octets[3], p_game->ip.octets[2], p_game->ip.octets[1], p_game->ip.octets[0], p_game->port);
}

const gfire_game *gfire_game_by_id(guint32 p_gameid)
{
	GList *cur = gfire_games;
	for(; cur; cur = g_list_next(cur))
	{
		if(((gfire_game*)cur->data)->id == p_gameid)
			return (gfire_game*)cur->data;
	}

	return NULL;
}

static const gfire_game *gfire_game_by_name(const gchar *p_name)
{
	GList *cur = gfire_games;
	for(; cur; cur = g_list_next(cur))
	{
		if(!purple_utf8_strcasecmp(((gfire_game*)cur->data)->name, p_name))
			return (gfire_game*)cur->data;
	}

	return NULL;
}

guint32 gfire_game_id_by_url(const gchar *p_url)
{
	if(!p_url)
		return 0;

	gchar *url_down = g_ascii_strdown(p_url, -1);

	GList *cur = gfire_games_external;
	for(; cur; cur = g_list_next(cur))
	{
		gfire_game_detection_set *dset = NULL;

		GList *set = ((gfire_game*)cur->data)->detection_sets;
		for(; set; set = g_list_next(set))
		{
			dset = (gfire_game_detection_set*)set->data;
			if(!dset->external)
				continue;

			if(dset->detect_url)
			{
				if(strstr(url_down, dset->detect_url))
				{
					g_free(url_down);
					return ((gfire_game*)cur->data)->id;
				}
			}

			if(dset->launch_url)
			{
				if(strstr(url_down, dset->launch_url))
				{
					g_free(url_down);
					return ((gfire_game*)cur->data)->id;
				}
			}
		}
	}

	g_free(url_down);

	return 0;
}

gboolean gfire_game_have_list()
{
	return (gfire_games != NULL);
}

guint32 gfire_game_get_version()
{
	return gfire_games_version;
}

gchar *gfire_game_get_version_str()
{
	time_t version = gfire_games_version;
	struct tm *time_data = localtime(&version);

	gchar *local_str = g_malloc(100 * sizeof(gchar));
	strftime(local_str, 100, "%d %B %Y", time_data);

	gchar *ret = g_locale_to_utf8(local_str, -1, NULL, NULL, NULL);
	g_free(local_str);

	if(!ret)
		return g_strdup(_("Unknown"));

	return ret;
}

guint32 gfire_game_id(const gchar *p_name)
{
	const gfire_game *game = gfire_game_by_name(p_name);
	if(!game)
		return 0;

	return game->id;
}

gchar *gfire_game_name(guint32 p_gameid)
{
	const gfire_game *game = gfire_game_by_id(p_gameid);
	if(!game)
		return g_strdup_printf("%u", p_gameid);

	return gfire_escape_html(game->name);
}

gchar *gfire_game_short_name(guint32 p_gameid)
{
	const gfire_game *game = gfire_game_by_id(p_gameid);
	if(!game)
		return g_strdup_printf("%u", p_gameid);

	return gfire_escape_html(game->short_name);
}

GList *gfire_game_excluded_ports_copy(const gfire_game *p_game)
{
	if(!p_game)
		return NULL;

	GList *copy = NULL;
	// Check the first set, servers should be the same for all different detection sets
	gfire_game_detection_set *dset = (gfire_game_detection_set*)p_game->detection_sets->data;
	if(dset)
	{
		GList *port = dset->excluded_ports;
		while(port)
		{
			copy = g_list_append(copy, g_memdup(port->data, sizeof(guint16)));
			port = g_list_next(port);
		}
	}

	return copy;
}

const gchar *gfire_game_server_query_type(guint32 p_gameid)
{
	const gfire_game *game = gfire_game_by_id(p_gameid);
	if(!game)
		return NULL;

	// Check the first set, servers should be the same for all different detection sets
	if(game->detection_sets)
	{
		gfire_game_detection_set *dset = (gfire_game_detection_set*)game->detection_sets->data;
		if(dset)
		 return dset->server_status_type;
	}

	return NULL;
}

gboolean gfire_game_foreach_dset(const gfire_game *p_game, GCallback p_callback, gpointer p_data, gboolean p_external)
{
	if(!p_game || !p_callback)
		return FALSE;

	GList *cur = p_game->detection_sets;
	while(cur)
	{
		gfire_game_detection_set *dset = (gfire_game_detection_set*)cur->data;
		if(!p_external && dset->external)
		{
			cur = g_list_next(cur);
			continue;
		}

		if(((gboolean(*)(const gfire_game*,const gfire_game_detection_set*,gpointer))p_callback)(p_game, dset, p_data))
			return TRUE;

		cur = g_list_next(cur);
	}

	return FALSE;
}

static gfire_game_detection_set *gfire_game_detection_set_create_from_xml(xmlnode *p_node)
{
	gfire_game_detection_set *ret = g_malloc0(sizeof(gfire_game_detection_set));

	gchar *tmp = NULL;

	// Enable server detection
	xmlnode *cur_node = xmlnode_get_child(p_node, "server_detection");
	if(cur_node)
	{
		tmp = xmlnode_get_data_unescaped(cur_node);
		if(tmp)
		{
			if(!g_utf8_collate(tmp, "enabled"))
				ret->detect_server = TRUE;
			g_free(tmp);
		}
	}

	// Excluded ports
	cur_node = xmlnode_get_child(p_node, "server_excluded_ports");
	if(cur_node)
	{
		tmp = xmlnode_get_data_unescaped(cur_node);
		if(tmp)
		{
			gchar **parray = g_strsplit(tmp, ";", -1);
			if(parray)
			{
				int i;
				for(i = 0; i < g_strv_length(parray); i++)
				{
					if(parray[i][0] == 0)
						continue;

					guint16 *port = g_malloc0(sizeof(guint16));
					sscanf(parray[i], "%hu", port);

					ret->excluded_ports = g_list_append(ret->excluded_ports, port);
				}
				g_strfreev(parray);
			}
			g_free(tmp);
		}
	}

	// Broadcast ports
	cur_node = xmlnode_get_child(p_node, "server_broadcast_ports");
	if(cur_node)
	{
		tmp = xmlnode_get_data_unescaped(cur_node);
		if(tmp)
		{
			gchar **parray = g_strsplit(tmp, ";", -1);
			if(parray)
			{
				int i;
				for(i = 0; i < g_strv_length(parray); i++)
				{
					if(parray[i][0] == 0)
						continue;

					ret->server_broadcast_ports = g_list_append(ret->server_broadcast_ports, g_strdup(parray[i]));
				}
				g_strfreev(parray);
			}
			g_free(tmp);
		}
	}

	// Server game name
	cur_node = xmlnode_get_child(p_node, "server_game_name");
	if(cur_node)
	{
		ret->server_game_name = xmlnode_get_data_unescaped(cur_node);
	}

	// Server status type
	cur_node = xmlnode_get_child(p_node, "server_status_type");
	if(cur_node)
	{
		ret->server_status_type = xmlnode_get_data_unescaped(cur_node);
	}

	// Launch PW args
	cur_node = xmlnode_get_child(p_node, "launch_password_args");
	if(cur_node)
	{
		ret->password_args = xmlnode_get_data_unescaped(cur_node);
	}

	// Launch network args
	cur_node = xmlnode_get_child(p_node, "launch_network_args");
	if(cur_node)
	{
		ret->network_args = xmlnode_get_data_unescaped(cur_node);
	}

	// Launch args
	cur_node = xmlnode_get_child(p_node, "launch_args");
	if(cur_node)
	{
		ret->launch_args = xmlnode_get_data_unescaped(cur_node);
	}

	// Arguments
	cur_node = xmlnode_get_child(p_node, "arguments");
	if(cur_node)
	{
		// Invalid args
		if(xmlnode_get_attrib(cur_node, "invalid"))
		{
			gchar **args = g_strsplit(xmlnode_get_attrib(cur_node, "invalid"), ";", -1);
			if(args)
			{
				int i;
				for(i = 0; i < g_strv_length(args); i++)
				{
					if(args[i][0] == 0)
						continue;

					ret->invalid_args = g_list_append(ret->invalid_args, g_strdup(args[i]));
				}

				g_strfreev(args);
			}
		}

		// Required args
		if(xmlnode_get_attrib(cur_node, "required"))
		{
			gchar **args = g_strsplit(xmlnode_get_attrib(cur_node, "required"), ";", -1);
			if(args)
			{
				int i;
				for(i = 0; i < g_strv_length(args); i++)
				{
					if(args[i][0] == 0)
						continue;

					ret->required_args = g_list_append(ret->required_args, g_strdup(args[i]));
				}

				g_strfreev(args);
			}
		}
	}

	// External
	cur_node = xmlnode_get_child(p_node, "external");
	if(cur_node)
	{
		ret->external = TRUE;

		// Detection URL
		if(xmlnode_get_attrib(cur_node, "url"))
		{
			ret->detect_url = g_strdup(xmlnode_get_attrib(cur_node, "url"));
		}

		// Launch URL
		if(xmlnode_get_attrib(cur_node, "launchurl"))
		{
			ret->launch_url = g_strdup(xmlnode_get_attrib(cur_node, "launchurl"));
		}
	}

	return ret;
}

static void gfire_game_detection_set_free(gfire_game_detection_set *p_dset)
{
	gfire_list_clear(p_dset->excluded_ports);
	gfire_list_clear(p_dset->server_broadcast_ports);

	if(p_dset->server_game_name)
		g_free(p_dset->server_game_name);

	if(p_dset->server_status_type)
		g_free(p_dset->server_status_type);

	if(p_dset->password_args)
		g_free(p_dset->password_args);

	if(p_dset->network_args)
		g_free(p_dset->network_args);

	if(p_dset->launch_args)
		g_free(p_dset->launch_args);

	gfire_list_clear(p_dset->invalid_args);
	gfire_list_clear(p_dset->required_args);

	if(p_dset->detect_url)
		g_free(p_dset->detect_url);

	if(p_dset->launch_url)
		g_free(p_dset->launch_url);

	g_free(p_dset);
}

static gfire_game *gfire_game_create_from_xml(xmlnode *p_node, gboolean *p_external)
{
	gfire_game *ret = g_malloc0(sizeof(gfire_game));

	if(xmlnode_get_attrib(p_node, "id"))
		sscanf(xmlnode_get_attrib(p_node, "id"), "%u", &ret->id);

	if(xmlnode_get_attrib(p_node, "name"))
		ret->name = g_strdup(xmlnode_get_attrib(p_node, "name"));

	if(xmlnode_get_attrib(p_node, "shortname"))
		ret->short_name = g_strdup(xmlnode_get_attrib(p_node, "shortname"));

	if(xmlnode_get_child(p_node, "voice"))
		ret->is_voice = TRUE;

	// Get all detection sets
	xmlnode *dset_node = xmlnode_get_child(p_node, "detection");
	while(dset_node)
	{
		gfire_game_detection_set *dset = gfire_game_detection_set_create_from_xml(dset_node);
		if(dset)
		{
			ret->detection_sets = g_list_append(ret->detection_sets, dset);

			*p_external = (*p_external || dset->external);
		}

		dset_node = xmlnode_get_next_twin(dset_node);
	}

	return ret;
}

static void gfire_game_free(gfire_game *p_game)
{
	if(p_game->name)
		g_free(p_game->name);

	if(p_game->short_name)
		g_free(p_game->short_name);

	GList *cur = p_game->detection_sets;
	while(cur)
	{
		gfire_game_detection_set_free((gfire_game_detection_set*)cur->data);
		cur = g_list_next(cur);
	}

	g_list_free(p_game->detection_sets);

	g_free(p_game);
}

gboolean gfire_game_load_games_xml()
{
	xmlnode *node = NULL;

	gchar *filename = g_build_filename(purple_user_dir(), "gfire_games.xml", NULL);
	if(filename)
	{
		purple_debug(PURPLE_DEBUG_INFO, "gfire", "Loading Game Data from: %s\n", filename);
		g_free(filename);
	}

	node = purple_util_read_xml_from_file("gfire_games.xml", "Gfire Games List");
	if(!node)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_game_load_games_xml: Couldn't load game list.\n");
		return FALSE;
	}

	// Delete all old games
	gfire_game_cleanup();

	// Read the games version
	if(g_utf8_collate(node->name, "games"))
	{
		xmlnode_free(node);
		return FALSE;
	}

	if(!xmlnode_get_attrib(node, "version"))
		gfire_games_version = 0;
	else
		sscanf(xmlnode_get_attrib(node, "version"), "%u", &gfire_games_version);

	// Read all games
	xmlnode *game_node = xmlnode_get_child(node, "game");
	while(game_node)
	{
		gboolean external = FALSE;
		gfire_game *game = gfire_game_create_from_xml(game_node, &external);
		if(game)
		{
			gfire_games = g_list_append(gfire_games, game);
			if(external)
				gfire_games_external = g_list_append(gfire_games_external, game);
		}

		game_node = xmlnode_get_next_twin(game_node);
	}

	xmlnode_free(node);

	return TRUE;
}

void gfire_game_cleanup()
{
	GList *cur = gfire_games;
	while(cur)
	{
		gfire_game_free((gfire_game*)cur->data);

		cur = g_list_next(cur);
	}
	g_list_free(gfire_games);
	g_list_free(gfire_games_external);
	gfire_games = NULL;
	gfire_games_external = NULL;
}

static gfire_game_configuration *gfire_game_configuration_create_from_xml(xmlnode *p_node)
{
	xmlnode *command_node = xmlnode_get_child(p_node, "command");
	if(!command_node)
		return NULL;

	gfire_game_configuration *ret = g_malloc0(sizeof(gfire_game_configuration));

	if(xmlnode_get_attrib(p_node, "id"))
		sscanf(xmlnode_get_attrib(p_node, "id"), "%u", &ret->game_id);

	xmlnode *cur_node = xmlnode_get_child(command_node, "detect");
	if(cur_node)
		ret->detect_file = xmlnode_get_data_unescaped(cur_node);

	cur_node = xmlnode_get_child(command_node, "launch");
	if(cur_node)
		ret->launch_file = xmlnode_get_data_unescaped(cur_node);

	cur_node = xmlnode_get_child(command_node, "prefix");
	if(cur_node)
		ret->launch_prefix = xmlnode_get_data_unescaped(cur_node);

	return ret;
}

static gfire_game_configuration *gfire_game_configuration_create(guint32 p_gameid, const gchar *p_detect_file,
																 const gchar *p_launch_file, const gchar *p_prefix)
{
	gfire_game_configuration *ret = g_malloc0(sizeof(gfire_game_configuration));

	ret->game_id = p_gameid;
	if(p_detect_file)
		ret->detect_file = g_strdup(p_detect_file);
	if(p_launch_file)
		ret->launch_file = g_strdup(p_launch_file);
	if(p_prefix)
		ret->launch_prefix = g_strdup(p_prefix);

	return ret;
}

static void gfire_game_configuration_free(gfire_game_configuration *p_gconf)
{
	if(p_gconf->detect_file)
		g_free(p_gconf->detect_file);

	if(p_gconf->launch_file)
		g_free(p_gconf->launch_file);

	if(p_gconf->launch_prefix)
		g_free(p_gconf->launch_prefix);

	g_free(p_gconf);
}

static xmlnode *gfire_game_configuration_to_xmlnode(const gfire_game_configuration *p_gconf)
{
	xmlnode *ret = xmlnode_new("game");
	gchar *id_str = g_strdup_printf("%u", p_gconf->game_id);
	xmlnode_set_attrib(ret, "id", id_str);
	g_free(id_str);

	xmlnode *command_node = xmlnode_new_child(ret, "command");

	if(p_gconf->detect_file)
	{
		xmlnode *detect_node = xmlnode_new_child(command_node, "detect");
		xmlnode_insert_data(detect_node, p_gconf->detect_file, -1);
	}

	if(p_gconf->launch_file)
	{
		xmlnode *launch_node = xmlnode_new_child(command_node, "launch");
		xmlnode_insert_data(launch_node, p_gconf->launch_file, -1);
	}

	if(p_gconf->launch_prefix)
	{
		xmlnode *prefix_node = xmlnode_new_child(command_node, "prefix");
		xmlnode_insert_data(prefix_node, p_gconf->launch_prefix, -1);
	}

	return ret;
}

static gint gfire_game_configuration_compare(const gfire_game_configuration *p_a, const gfire_game_configuration *p_b)
{
	const gfire_game *game_a = gfire_game_by_id(p_a->game_id);
	const gfire_game *game_b = gfire_game_by_id(p_b->game_id);

	if(!game_a)
		return 1;
	else if(!game_b)
		return -1;
	else
		return g_strcmp0(game_a->name, game_b->name);
}

static void gfire_game_config_sort()
{
	if(!gfire_games_config)
		return;

	gfire_games_config = g_list_sort(gfire_games_config, (GCompareFunc)gfire_game_configuration_compare);
}

gboolean gfire_game_load_config_xml(gboolean p_force)
{
	if(!p_force && gfire_games_config)
		return TRUE;

	xmlnode *node = NULL;

	gchar *filename = g_build_filename(purple_user_dir(), "gfire_game_config.xml", NULL);
	if(filename)
	{
		purple_debug(PURPLE_DEBUG_INFO, "gfire", "Loading Game Launch Data from: %s\n", filename);
		g_free(filename);
	}

	node = purple_util_read_xml_from_file("gfire_game_config.xml", "Gfire Game Config List");
	if(!node)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_game_load_config_xml: Couldn't load game config.\n");
		return FALSE;
	}

	// Check for a valid game config
	if(g_utf8_collate(node->name, "game_config"))
	{
		xmlnode_free(node);
		return FALSE;
	}

	// Check for a valid version
	if(!xmlnode_get_attrib(node, "version") || g_utf8_collate(xmlnode_get_attrib(node, "version"), "2"))
	{
		xmlnode_free(node);
		return FALSE;
	}

	// Delete all old configurations
	gfire_game_config_cleanup();

	// Parse all games
	xmlnode *game_node = xmlnode_get_child(node, "game");
	while(game_node)
	{
		gfire_game_configuration *gconf = gfire_game_configuration_create_from_xml(game_node);
		if(gconf)
			gfire_games_config = g_list_append(gfire_games_config, gconf);

		game_node = xmlnode_get_next_twin(game_node);
	}

	gfire_game_config_sort();

	xmlnode_free(node);

	return TRUE;
}

static void gfire_game_save_config_xml()
{
	xmlnode *game_config = xmlnode_new("game_config");
	xmlnode_set_attrib(game_config, "version", "2");

	GList *cur = gfire_games_config;
	while(cur)
	{
		xmlnode_insert_child(game_config, gfire_game_configuration_to_xmlnode((gfire_game_configuration*)cur->data));

		cur = g_list_next(cur);
	}

	gchar *xml_str = xmlnode_to_formatted_str(game_config, NULL);

	purple_util_write_data_to_file("gfire_game_config.xml", xml_str, -1);
	g_free(xml_str);

	xmlnode_free(game_config);
}

void gfire_game_config_cleanup()
{
	GList *cur = gfire_games_config;
	while(cur)
	{
		gfire_game_configuration_free((gfire_game_configuration*)cur->data);
		cur = g_list_next(cur);
	}
	g_list_free(gfire_games_config);
	gfire_games_config = NULL;
}

const gfire_game_configuration *gfire_game_config_by_id(guint32 p_gameid)
{
	GList *cur = gfire_games_config;
	while(cur)
	{
		if(((gfire_game_configuration*)cur->data)->game_id == p_gameid)
			return (gfire_game_configuration*)cur->data;

		cur = g_list_next(cur);
	}

	return NULL;
}

gboolean gfire_game_config_foreach(GCallback p_callback, gpointer p_data)
{
	if(!gfire_games_config || !p_callback)
		return FALSE;

	GList *cur = gfire_games_config;
	while(cur)
	{
		if(((gboolean(*)(const gfire_game_configuration*,gpointer))p_callback)((const gfire_game_configuration*)cur->data,
																			   p_data))
			return TRUE;

		cur = g_list_next(cur);
	}

	return FALSE;
}

gboolean gfire_game_playable(guint32 p_gameid)
{
	GList *cur = gfire_games_config;
	while(cur)
	{
		if(((gfire_game_configuration*)cur->data)->game_id == p_gameid)
			return TRUE;

		cur = g_list_next(cur);
	}

	return FALSE;
}

// FIXME: Changes needed
static gchar *gfire_game_config_get_command(const gfire_game_configuration *p_gconf, const gfire_game_data *p_game_data)
{
	if (!p_gconf)
	{
		purple_debug_error("gfire", "Invalid game config info.\n");
		return NULL;
	}

	const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
	if(!game)
	{
		purple_debug_error("gfire", "gfire_game_config_get_command: Game not existing.\n");
		return NULL;
	}

	if (!game->detection_sets || !((gfire_game_detection_set*)game->detection_sets->data)->launch_args ||
		!p_gconf->launch_file)
	{
		purple_debug_error("gfire", "Insufficient information available!\n");
		return NULL;
	}

	// Get the first detection set
	gfire_game_detection_set *dset = (gfire_game_detection_set*)game->detection_sets->data;

	gchar *username_args = NULL;
	/* This is not implemented yet, so let it stay NULL for now
	if(dset->username_args)
	{
		username_args = purple_strreplace(dset->username_args, "%UA_GAME_HOST_USERNAME%", "");
	}*/

	gchar *password_args = NULL;
	/* This is not implemented yet, so let it stay NULL for now
	if(dset->password_args)
	{
		password_args = purple_strreplace(dset->password_args, "%UA_GAME_HOST_PASSWORD%", "");
	}*/

	gchar *tmp, *tmp2;
	gchar *network_args = NULL;

	if(dset->network_args && p_game_data && p_game_data->ip.value)
	{
		gchar *server_ip_tmp = NULL, *server_port_tmp = NULL, *tmp = NULL, *tmp2 = NULL;

		server_port_tmp = gfire_game_data_port_str(p_game_data);
		server_ip_tmp = gfire_game_data_ip_str(p_game_data);

		// Replace IP and port
		tmp = purple_strreplace(dset->network_args, "%UA_GAME_HOST_NAME%", server_ip_tmp);
		tmp2 = purple_strreplace(tmp, "%UA_GAME_HOST_PORT%", server_port_tmp);
		g_free(tmp);
		g_free(server_port_tmp);
		g_free(server_ip_tmp);

		// Replace username
		tmp = purple_strreplace(tmp2, "%UA_LAUNCHER_USERNAME_ARGS%", username_args ? username_args : "");
		g_free(tmp2);

		// Replace password
		network_args = purple_strreplace(tmp, "%UA_LAUNCHER_PASSWORD_ARGS%", password_args ? password_args : "");
		g_free(tmp);
	}
	g_free(username_args);
	g_free(password_args);

	gchar *exe;

#ifdef _WIN32
	exe = g_strdup_printf("\"%s\"", p_gconf->launch_file);
#else
	// Checks for Wine compatibility if needed
	exe = g_strdup_printf("%s \"%s\"", gfire_filetype_use_wine(p_gconf->launch_file) ? "wine": "", p_gconf->launch_file);
#endif

	tmp = purple_strreplace(dset->launch_args, "%UA_LAUNCHER_EXE_PATH%", exe);
	g_free(exe);

	tmp2 = purple_strreplace(tmp, "%UA_LAUNCHER_EXTRA_ARGS%", ""); // Unknown
	g_free(tmp);

	gchar *launch_cmd = purple_strreplace(tmp2, "%UA_LAUNCHER_NETWORK_ARGS%", network_args ? network_args : "");
	g_free(network_args);
	g_free(tmp2);

	return launch_cmd;
}

static void gfire_join_game_parse_prefix(const gchar *p_prefix, gchar **p_exe_prefix, GList **p_env_keys,
										 GList **p_env_values)
{
	if(!p_prefix || !p_exe_prefix || !p_env_keys || !p_env_values)
		return;

	GString *prefix = g_string_new("");

	const gchar *pos = p_prefix;
	const gchar *end = NULL;
	do
	{
		// Find the next space / end of string
		const gchar *space = strchr(pos, ' ');
		if(!space)
			space = p_prefix + strlen(p_prefix);
		// Find the next equal
		const gchar *equal = strchr(pos, '=');

		// We got an env var setting
		if(equal && (equal < space))
		{
			if((equal + 1) <= (p_prefix + strlen(p_prefix)))
			{
				end = equal + 1;

				gchar *key = g_strndup(pos, equal - pos);

				if(*key != 0)
				{
					gchar *value = NULL;
					if(*(equal + 1) == '"')
					{
						const gchar *quote = strchr(equal + 2, '"');
						if(quote)
						{
							value = g_strndup(equal + 2, quote - equal - 2);
							end = quote;
						}
					}
					else
					{
						value = g_strndup(equal + 1, space - equal - 1);
						end = space;
					}

					if(value)
					{
						*p_env_keys = g_list_append(*p_env_keys, key);
						*p_env_values = g_list_append(*p_env_values, value);
					}
					else
						g_free(key);

					if(end == (p_prefix + strlen(p_prefix)))
						end = NULL;
					else
						end += 1;
				}
				else
					g_free(key);
			}
			else
				end = NULL;
		}
		else
		{
			gchar *chunk = g_strndup(pos, space - pos);
			g_string_append_printf(prefix, " %s", chunk);
			g_free(chunk);

			if(space == (p_prefix + strlen(p_prefix)))
				end = NULL;
			else
				end = space + 1;
		}
	} while((pos = end));

	*p_exe_prefix = g_string_free(prefix, FALSE);
}

void gfire_join_game(const gfire_game_data *p_game_data)
{
	const gfire_game_configuration *gconf = gfire_game_config_by_id(p_game_data->id);
	if(!gconf)
	{
		purple_debug_error("gfire", "gfire_join_game: Game not configured!\n");
		return;
	}

	gchar *game_launch_command = gfire_game_config_get_command(gconf, p_game_data);
	if (!game_launch_command)
	{
		purple_debug_error("gfire", "gfire_join_game: Couldn't generate game launch command!\n");
		return;
	}

	GString *command = g_string_new(game_launch_command);
	g_free(game_launch_command);

	// Set environment if needed
	gchar **env = NULL;
	gint env_len = 0;
	if(gconf->launch_prefix)
	{
		gchar *prefix = NULL;
		GList *env_keys = NULL;
		GList *env_values = NULL;

		gfire_join_game_parse_prefix(gconf->launch_prefix, &prefix, &env_keys, &env_values);
		if(prefix)
		{
			g_string_prepend_c(command, ' ');
			g_string_prepend(command, prefix);
			g_free(prefix);
		}

		if(env_keys)
		{
			gchar **cur_env = g_listenv();
			gint i = 0;
			for(; i < g_strv_length(cur_env); i++)
			{
				env_len++;
				env = (gchar**)g_realloc(env, sizeof(gchar*) * (env_len + 1));
				env[env_len - 1] = g_strdup_printf("%s=%s", cur_env[i], g_getenv(cur_env[i]));
				env[env_len] = NULL;
			}

			GList *cur_key = env_keys;
			GList *cur_value = env_values;
			while(cur_key)
			{
				for(i = 0; i < g_strv_length(cur_env); i++)
				{
					if(g_strcmp0(cur_env[i], (gchar*)cur_key->data) == 0)
						break;
				}

				if(i == g_strv_length(cur_env))
				{
					env_len++;
					env = (gchar**)g_realloc(env, sizeof(gchar*) * (env_len + 1));
					env[env_len - 1] = g_strdup_printf("%s=%s", (gchar*)cur_key->data, (gchar*)cur_value->data);
					env[env_len] = NULL;
				}
				else
				{
					g_free(env[i]);
					env[i] = g_strdup_printf("%s=%s", (gchar*)cur_key->data, (gchar*)cur_value->data);
				}

				cur_key = g_list_next(cur_key);
				cur_value = g_list_next(cur_value);
			}

			g_strfreev(cur_env);

			gfire_list_clear(env_keys);
			gfire_list_clear(env_values);
		}
	}

	// Launch command
	gchar **argv = NULL;
	if(!g_shell_parse_argv(command->str, NULL, &argv, NULL))
	{
		purple_debug_error("gfire", "g_shell_parse_argv failed!");
		g_string_free(command, TRUE);
		g_strfreev(env);
		return;
	}

	purple_debug_misc("gfire", "Launching game and joining server: %s\n", command->str);
	g_string_free(command, TRUE);

	// Get working directory
	gchar *wd = g_path_get_dirname(argv[0]);

	// Launch
	g_spawn_async(wd, argv, env, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
	g_free(wd);
	g_strfreev(argv);
	g_strfreev(env);
}

#ifdef HAVE_GTK
static GtkBuilder *gfire_gtk_builder = NULL;

static void gfire_game_manager_reload_ui()
{
	if(!gfire_gtk_builder)
		return;

	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_game_entry"));
	GtkWidget *add_detection_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_detection_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_executable_check_button"));
	GtkWidget *add_launch_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_launch_button"));
	GtkWidget *add_advanced_expander = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_advanced_expander"));
	GtkWidget *add_prefix_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_prefix_entry"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_game_combo"));
	GtkListStore *edit_game_list_store = GTK_LIST_STORE(gtk_builder_get_object(gfire_gtk_builder, "edit_game_list_store"));
	GtkWidget *edit_detection_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_detection_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_executable_check_button"));
	GtkWidget *edit_launch_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_launch_button"));
	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_prefix_entry"));

	// Reset widgets on "add" tab
	gtk_entry_set_text(GTK_ENTRY(add_game_entry), "");
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(add_detection_button));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_executable_check_button), TRUE);
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(add_launch_button));
	gtk_expander_set_expanded(GTK_EXPANDER(add_advanced_expander), FALSE);
	gtk_entry_set_text(GTK_ENTRY(add_prefix_entry), "");

	// Reset widgets on "edit" tab
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(edit_detection_button));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), TRUE);
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(edit_launch_button));
	gtk_entry_set_text(GTK_ENTRY(edit_prefix_entry), "");

	// Clear games list combo box
	gtk_list_store_clear(edit_game_list_store);

	// Add all new configured games
	GList *cur = gfire_games_config;
	while(cur)
	{
		const gfire_game *game = gfire_game_by_id(((gfire_game_configuration*)cur->data)->game_id);
		if(game)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(edit_game_combo), game->name);
		}
		cur = g_list_next(cur);
	}
}

static void gfire_game_manager_update_executable_toggled_cb(GtkBuilder *p_builder, GtkWidget *p_executable_check_button)
{
	if (!p_builder)
	{
		purple_debug_error("gfire", "Couldn't access game manager interface.");
		return;
	}

	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_check_button"));
	GtkWidget *add_launch_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_launch_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_launch_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_launch_button"));

	gboolean check_button_state;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button)))
		check_button_state = FALSE;
	else
		check_button_state = TRUE;

	gtk_widget_set_sensitive(add_launch_button, check_button_state);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button)))
		check_button_state = FALSE;
	else
		check_button_state = TRUE;

	gtk_widget_set_sensitive(edit_launch_button, check_button_state);
}

static void gfire_game_manager_add_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{
	if(!p_builder)
	{
		purple_debug_error("gfire", "Couldn't access game manager interface.\n");
		return;
	}

	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_game_entry"));
	GtkWidget *add_detection_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_detection_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_check_button"));
	GtkWidget *add_launch_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_launch_button"));

	GtkWidget *add_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_prefix_entry"));

	const gchar *game_name = gtk_entry_get_text(GTK_ENTRY(add_game_entry));
	gchar *game_detect = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_detection_button));
	gboolean game_launch_use_detect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button));
	gchar *game_launch = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_launch_button));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(add_prefix_entry));

	if (game_name && game_detect && ((!game_launch_use_detect && game_launch) || game_launch_use_detect))
	{
		guint32 game_id;

		game_id = gfire_game_id(game_name);

		if(game_id == 0)
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"), _("Couldn't add game"),
							  _("There's no such game, please try again."), NULL, NULL);
			g_free(game_detect);
			g_free(game_launch);
			return;
		}

		if(!gfire_game_config_by_id(game_id))
		{
			gfire_game_configuration *gconf = gfire_game_configuration_create(game_id, game_detect,
																			  game_launch_use_detect ? game_detect :
																			  game_launch, game_prefix);

			gfire_games_config = g_list_append(gfire_games_config, gconf);
			gfire_game_config_sort();
			gfire_game_save_config_xml();

			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("Manage Games: game added"),
							  game_name, _("The game has been successfully added."), NULL, NULL);
		}
		else
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("Manage Games: warning"), _("Game already added"),
						  _("This game is already added, you can configure it if you want."), NULL, NULL);

			g_free(game_launch);
			g_free(game_detect);

			return;
		}

		g_free(game_launch);
		g_free(game_detect);
	}
	else
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"),
					  _("Couldn't add game"), _("Please try again. Make sure you fill in all fields."), NULL, NULL);
		return;
	}

	gfire_game_manager_reload_ui(p_builder);
}

static void gfire_game_manager_edit_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{

	if (!p_builder)
	{
		purple_debug_error("gfire", "Couldn't access game manager interface.\n");
		return;
	}

	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));
	GtkWidget *edit_detection_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_detection_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_launch_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_launch_button"));
	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_prefix_entry"));

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	gchar *game_detect = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_detection_button));
	gboolean game_launch_use_detect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button));
	gchar *game_launch = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_launch_button));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(edit_prefix_entry));

	if (game_name && game_detect && ((!game_launch_use_detect && game_launch) || game_launch_use_detect))
	{
		guint32 game_id;

		game_id = gfire_game_id(game_name);

		gfire_game_configuration *gconf = (gfire_game_configuration*)gfire_game_config_by_id(game_id);

		if (gconf)
		{
			if(gconf->detect_file)
				g_free(gconf->detect_file);

			gconf->detect_file = g_strdup(game_detect);

			if(gconf->launch_file)
				g_free(gconf->launch_file);

			gconf->launch_file = g_strdup(game_launch_use_detect ? game_detect : game_launch);

			if(gconf->launch_prefix)
				g_free(gconf->launch_prefix);

			gconf->launch_prefix = g_strdup(game_prefix);
		}
		else
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("Manage Games: warning"), _("Game launch data not found"),
						  _("This game is not yet added as it seems, please add it first!"), NULL, NULL);
			g_free(game_launch);
			g_free(game_detect);
			g_free(game_name);

			return;
		}

		g_free(game_launch);
		g_free(game_detect);
		g_free(game_name);

		gfire_game_save_config_xml();

		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("Manage Games: game edited"), _("Game edited"), _("The game has been successfully edited."), NULL, NULL);
	}
	else
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"),
					  _("Couldn't edit game"), _("Please try again. Make sure you fill in all fields."), NULL, NULL);
		return;
	}

	gfire_game_manager_reload_ui(p_builder);
}

static void gfire_game_manager_remove_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{
	if (!p_builder)
	{
		purple_debug_error("gfire", "Couldn't build game manager interface.\n");
		return;
	}

	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	if (selected_game)
	{
		guint32 game_id = gfire_game_id(selected_game);
		g_free(selected_game);

		if(game_id == 0)
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"),
								  _("Couldn't remove game"), _("No such game, please try again!"), NULL, NULL);
			return;
		}

		GList *cur = gfire_games_config;
		while(cur)
		{
			if(((gfire_game_configuration*)cur->data)->game_id == game_id)
				break;
			cur = g_list_next(cur);
		}

		if(!cur)
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"),
								  _("Couldn't remove game"), _("This game is not yet added as it seems, please add it first!"), NULL, NULL);
			return;
		}

		gfire_game_configuration_free((gfire_game_configuration*)cur->data);
		gfire_games_config = g_list_delete_link(gfire_games_config, cur);

		gfire_game_save_config_xml();

		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, _("Manage Games: game removed"),
							  _("Game removed"), _("The game has been successfully removed."), NULL, NULL);
	}
	else
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, _("Manage Games: error"),
							  _("Couldn't remove game"), _("Please try again. Make sure you select a game to remove."), NULL, NULL);

	gfire_game_manager_reload_ui(p_builder);
}

static void gfire_game_manager_edit_update_fields_cb(GtkBuilder *p_builder, GtkWidget *p_edit_game_combo)
{
	if (!p_builder)
	{
		purple_debug_error("gfire", "Couldn't access game manager interface.\n");
		return;
	}

	GtkWidget *edit_detection_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_detection_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_launch_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_launch_button"));

	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_prefix_entry"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));

	guint32 game_id = gfire_game_id(selected_game);
	g_free(selected_game);
	if(game_id == 0)
	{
		// ERROR
		return;
	}

	const gfire_game_configuration *gconf = gfire_game_config_by_id(game_id);

	if(!gconf)
	{
		// ERROR
		return;
	}

	if (!g_utf8_collate(gconf->detect_file ? gconf->detect_file : "", gconf->launch_file ? gconf->launch_file : ""))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), FALSE);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_detection_button), gconf->detect_file ? gconf->detect_file : "");
	gtk_entry_set_text(GTK_ENTRY(edit_prefix_entry), gconf->launch_prefix ? gconf->launch_prefix : "");
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_launch_button), gconf->launch_file ? gconf->launch_file : "");
}

static void gfire_game_manager_update_executable_cb(GtkWidget *p_launch_button, GtkWidget *p_detect_button)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p_launch_button), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_detect_button)));
}

static void gfire_game_manager_closed_cb(GtkObject *p_object, gpointer *p_data)
{
	if(gfire_gtk_builder)
	{
		g_object_unref(G_OBJECT(gfire_gtk_builder));
		gfire_gtk_builder = NULL;
	}
}

void gfire_game_manager_show(PurplePluginAction *p_action)
{
	// Only one manager at a time
	if(gfire_gtk_builder)
		return;

	gfire_gtk_builder = gtk_builder_new();

	if(!gfire_gtk_builder)
	{
		purple_debug_error("gfire", "Couldn't build game manager interface.\n");
		return;
	}

	gtk_builder_set_translation_domain(gfire_gtk_builder, GETTEXT_PACKAGE);

	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "games.glade", NULL);
	gtk_builder_add_from_file(gfire_gtk_builder, builder_file, NULL);
	g_free(builder_file);

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "manage_games_window"));
	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_game_entry"));
	GtkWidget *add_detection_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_detection_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_executable_check_button"));
	GtkWidget *add_launch_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_launch_button"));
	GtkWidget *add_close_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_close_button"));
	GtkWidget *add_add_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_add_button"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_game_combo"));
	GtkWidget *edit_detection_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_detection_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_executable_check_button"));
	GtkWidget *edit_launch_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_launch_button"));
	GtkWidget *edit_close_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_close_button"));
	GtkWidget *edit_apply_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_apply_button"));
	GtkWidget *edit_remove_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "edit_remove_button"));

	g_signal_connect(manage_games_window, "destroy", G_CALLBACK(gfire_game_manager_closed_cb), NULL);
	g_signal_connect_swapped(add_detection_button, "current-folder-changed", G_CALLBACK(gfire_game_manager_update_executable_cb), add_launch_button);
	g_signal_connect_swapped(add_executable_check_button, "toggled", G_CALLBACK(gfire_game_manager_update_executable_toggled_cb), gfire_gtk_builder);
	g_signal_connect_swapped(add_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(add_add_button, "clicked", G_CALLBACK(gfire_game_manager_add_cb), gfire_gtk_builder);
	g_signal_connect_swapped(edit_game_combo, "changed", G_CALLBACK(gfire_game_manager_edit_update_fields_cb), gfire_gtk_builder);
	g_signal_connect_swapped(edit_executable_check_button, "toggled", G_CALLBACK(gfire_game_manager_update_executable_toggled_cb), gfire_gtk_builder);
	g_signal_connect_swapped(edit_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(edit_apply_button, "clicked", G_CALLBACK(gfire_game_manager_edit_cb), gfire_gtk_builder);
	g_signal_connect_swapped(edit_remove_button, "clicked", G_CALLBACK(gfire_game_manager_remove_cb), gfire_gtk_builder);

	// Add filters to filechoosers
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Executable files");

	gtk_file_filter_add_mime_type(filter, "application/x-ms-dos-executable");
	gtk_file_filter_add_mime_type(filter, "application/x-executable");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(add_detection_button), filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(add_launch_button), filter);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(edit_detection_button), filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(edit_launch_button), filter);

	GtkFileFilter *filter_all = gtk_file_filter_new();
	gtk_file_filter_set_name(filter_all, "All files");

	gtk_file_filter_add_pattern (filter_all, "*");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(add_detection_button), filter_all);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(add_launch_button), filter_all);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(edit_detection_button), filter_all);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(edit_launch_button), filter_all);

	// Add all games to the combo box
	GtkListStore *add_list_store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkTreeIter iter;

	GList *cur = gfire_games;
	while(cur)
	{
		gtk_list_store_append(add_list_store, &iter);
		gtk_list_store_set(add_list_store, &iter, 0,
						   ((gfire_game*)cur->data)->name ? ((gfire_game*)cur->data)->name : "", -1);
		cur = g_list_next(cur);
	}

	GtkEntryCompletion *add_completion = gtk_entry_completion_new();
	gtk_entry_completion_set_model(add_completion, GTK_TREE_MODEL(add_list_store));
	gtk_entry_completion_set_text_column(add_completion, 0);
	gtk_entry_set_completion(GTK_ENTRY(add_game_entry), add_completion);

	gfire_game_manager_reload_ui();

	gtk_widget_show_all(manage_games_window);
}
#endif // HAVE_GTK
