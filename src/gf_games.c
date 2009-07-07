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

#include "gf_games.h"

// Static XML nodes for Game handling (shared by all Gfire instances)
static xmlnode *gfire_games_xml = NULL;
static xmlnode *gfire_launch_xml = NULL;

void gfire_xml_download_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const gchar *p_buf, gsize p_len, const gchar *p_error_message)
{
	char *successmsg = NULL;
	PurpleConnection *gc = (PurpleConnection *)p_data;

	if (!p_data || !p_buf || !p_len)
	{
		purple_notify_message(gc, PURPLE_NOTIFY_MSG_ERROR, N_("Gfire Game List Download"), N_("Download failed"), NN(p_error_message), NULL, NULL);
		return;
	}

	if(purple_util_write_data_to_file("gfire_games.xml", p_buf, p_len))
	{
		gfire_game_load_games_xml();

		if(g_strcmp0(gfire_game_name(100), "100") != 0)
			successmsg = g_strdup_printf(N_("New Game List Version: %s"), gfire_game_name(100));
		else
			successmsg = NULL;

		purple_notify_message(gc, PURPLE_NOTIFY_MSG_INFO, N_("Gfire Game List Download"), N_("Download successful"), successmsg, NULL, NULL);

		if(successmsg) g_free(successmsg);

	}
	else
		purple_notify_message(gc, PURPLE_NOTIFY_MSG_ERROR, N_("Gfire Game List Download"), N_("Download failed"), N_("Unable to write gfire_games.xml"), NULL, NULL);
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
	for(i = 0; i < 4; i++)
	{
		if(!ip_octet_strs[i])
		{
			p_game->ip.value = 0;
			g_strfreev(ip_octet_strs);
			return;
		}

		p_game->ip.octets[i] = atoi(ip_octet_strs[i]);
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

	return g_strdup_printf("%u.%u.%u.%u", p_game->ip.octets[0], p_game->ip.octets[1], p_game->ip.octets[2], p_game->ip.octets[3]);
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

	return g_strdup_printf("%u.%u.%u.%u:%u", p_game->ip.octets[0], p_game->ip.octets[1], p_game->ip.octets[2], p_game->ip.octets[3], p_game->port);
}

static xmlnode *gfire_game_node_by_id(guint32 p_gameid)
{
	if(!gfire_games_xml)
		if(!gfire_game_load_games_xml())
			return NULL;

	xmlnode *node_child = NULL;
	for(node_child = xmlnode_get_child(gfire_games_xml, "game"); node_child;
		node_child = xmlnode_get_next_twin(node_child))
	{
		const gchar *game_id_tmp = xmlnode_get_attrib(node_child, "id");
		if(atoi(game_id_tmp) == p_gameid)
			return node_child;
	}

	return NULL;
}

static xmlnode *gfire_game_node_by_name(const gchar *p_name)
{
	if(!gfire_games_xml)
		if(!gfire_game_load_games_xml())
			return NULL;

	xmlnode *node_child = NULL;
	for(node_child = xmlnode_get_child(gfire_games_xml, "game"); node_child;
		node_child = xmlnode_get_next_twin(node_child))
	{
		const gchar *game_name_tmp = xmlnode_get_attrib(node_child, "name");
		if(g_ascii_strcasecmp(game_name_tmp, p_name) == 0)
			return node_child;
	}

	return NULL;
}

xmlnode *gfire_game_node_first()
{
	if(!gfire_games_xml)
		return NULL;

	return xmlnode_get_child(gfire_games_xml, "game");
}

xmlnode *gfire_game_node_next(xmlnode *p_node)
{
	if(!gfire_launch_xml || !p_node)
		return NULL;

	return xmlnode_get_next_twin(p_node);
}

guint32 gfire_game_id(const gchar *p_name)
{
	xmlnode *node = NULL;
	const gchar *game_id_tmp;
		
	node = gfire_game_node_by_name(p_name);
	if(!node)
		return 0;

	game_id_tmp = xmlnode_get_attrib(node, "id");
	if(!game_id_tmp)
		return 0;

	return atoi(game_id_tmp);
}

gchar *gfire_game_name(guint32 p_gameid)
{
	xmlnode *node = NULL;
	const gchar *name = NULL;

	node = gfire_game_node_by_id(p_gameid);
	if(!node)
		return g_strdup_printf("%d", p_gameid);

	name = xmlnode_get_attrib(node, "name");
	if(!name)
		return g_strdup_printf("%d", p_gameid);

	gchar *escaped = gfire_escape_html(name);
	return escaped;
}


/**
 * @return TRUE if parsed ok, FALSE otherwise
*/ 
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

	if(gfire_games_xml)
		xmlnode_free(gfire_games_xml);

	gfire_games_xml = node;
	return TRUE;
}

gboolean gfire_game_load_launch_xml()
{
	xmlnode *node = NULL;

	gchar *filename = g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL);
	if(filename)
	{
		purple_debug(PURPLE_DEBUG_INFO, "gfire", "Loading Game Launch Data from: %s\n", filename);
		g_free(filename);
	}

	node = purple_util_read_xml_from_file("gfire_launch.xml", "Gfire Game Launch List");
	if(!node)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_game_load_launch_xml: Couldn't load launch config.\n");
		return FALSE;
	}

	if(gfire_launch_xml)
		xmlnode_free(gfire_launch_xml);

	gfire_launch_xml = node;
	return TRUE;
}

/**
 * Determines of a game is configured, and is playable
 *
 * @param gc	Valid PurpleConnection
 * @param game	integer ID of game to check to see if its playable
 *
 * @return TRUE if game is playable, FALSE if not
*/
gboolean gfire_game_playable(guint32 p_gameid)
{
	gfire_game_launch_info *foo = NULL;
	
	foo = gfire_game_launch_info_get(p_gameid);
	if (!foo) return FALSE;
	gfire_game_launch_info_free(foo);
	return TRUE;
}

static xmlnode *gfire_game_launch_node_by_id(guint32 p_gameid)
{
	if(!gfire_launch_xml)
		if(!gfire_game_load_launch_xml())
			return NULL;

	xmlnode *node_child = NULL;
	for(node_child = xmlnode_get_child(gfire_launch_xml, "game"); node_child;
		node_child = xmlnode_get_next_twin(node_child))
	{
		const gchar *game_id_tmp = xmlnode_get_attrib(node_child, "id");
		if(atoi(game_id_tmp) == p_gameid)
			return node_child;
	}

	return NULL;
}

xmlnode *gfire_game_launch_node_first()
{
	if(!gfire_launch_xml)
		return NULL;

	return xmlnode_get_child(gfire_launch_xml, "game");
}

xmlnode *gfire_game_launch_node_next(xmlnode *p_node)
{
	if(!gfire_launch_xml || !p_node)
		return NULL;

	return xmlnode_get_next_twin(p_node);
}

gfire_game_launch_info *gfire_game_launch_info_new()
{
	gfire_game_launch_info *game_launch_info;
	game_launch_info = g_malloc0(sizeof(gfire_game_launch_info));
	return game_launch_info;
}

void gfire_game_launch_info_free(gfire_game_launch_info *p_launch_info)
{
	if(!p_launch_info)
		return;
	if(p_launch_info->game_connect) g_free(p_launch_info->game_connect);
	if(p_launch_info->game_launch) g_free(p_launch_info->game_launch);
	if(p_launch_info->game_name) g_free(p_launch_info->game_name);
	if(p_launch_info->game_path) g_free(p_launch_info->game_path);
	if(p_launch_info->game_prefix) g_free(p_launch_info->game_prefix);
	g_free(p_launch_info);
}

gfire_game_launch_info *gfire_game_launch_info_get(guint32 p_gameid)
{
	gfire_game_launch_info *l = NULL;
	xmlnode *node = NULL;
	xmlnode *cnode = NULL;
	xmlnode *command = NULL;

	node = gfire_game_launch_node_by_id(p_gameid);
	if(!node)
		return NULL;

	/* got the game */
	l = gfire_game_launch_info_new();
	if (!l)
		return NULL; /* Out of Memory */
		
	l->game_id = p_gameid;
	l->game_name = g_strdup(xmlnode_get_attrib(node, "name"));

	for(cnode = node->child; cnode; cnode = cnode->next)
	{
		if(cnode->type != XMLNODE_TYPE_TAG)
			continue;
			
		if(g_strcmp0(cnode->name, "command") == 0)
		{
			if ((command = xmlnode_get_child(cnode, "prefix")))
				l->game_prefix = xmlnode_get_data(command);

			if ((command = xmlnode_get_child(cnode, "path")))
				l->game_path = xmlnode_get_data(command);

			if ((command = xmlnode_get_child(cnode, "launch")))
				l->game_launch = xmlnode_get_data(command);

			if ((command = xmlnode_get_child(cnode, "connect")))
				l->game_connect = xmlnode_get_data(command);
		}
	}
	
	return l;
}

gchar *gfire_game_launch_info_get_command(gfire_game_launch_info *game_launch_info, const gfire_game_data *p_game_data)
{
	gchar *command_prefixed, *command_launch_args, *command, *game_connect_option = NULL;
	
	if (game_launch_info == NULL) {
		purple_debug_error("gfire_game_launch_info_get_command", "Invalid game launch info.\n");
		return NULL;
	}

	if(!game_launch_info->game_path) {
		purple_debug_error("gfire_game_launch_info_get_command", "Path setting is empty, please fill it out first!\n");
		return NULL;
	}
	
	if(game_launch_info->game_connect && p_game_data)
	{
		gchar *server_ip_tmp, *server_port_tmp, *tmp = NULL;

		server_port_tmp = gfire_game_data_port_str(p_game_data);
		server_ip_tmp = gfire_game_data_ip_str(p_game_data);
		tmp = purple_strreplace(game_launch_info->game_connect, "[ip]", server_ip_tmp);
		game_connect_option = purple_strreplace(tmp, "[port]", server_port_tmp);

		g_free(server_port_tmp);
		g_free(server_ip_tmp);
		g_free(tmp);
	}
	
	if (game_launch_info->game_prefix != NULL)
		command_prefixed = g_strdup_printf("%s \"%s\"", game_launch_info->game_prefix, game_launch_info->game_path);
	else
		command_prefixed = g_strdup_printf("\"%s\"", game_launch_info->game_path);

	if (game_launch_info->game_launch != NULL)
		command_launch_args = g_strdup_printf("%s %s", command_prefixed, game_launch_info->game_launch);
	else
		command_launch_args = g_strdup(command_prefixed);

	g_free(command_prefixed);

	if(game_connect_option)
	{
		command = g_strdup_printf("%s %s", command_launch_args, game_connect_option);
		g_free(game_connect_option);
		g_free(command_launch_args);

		return command;
	}
	else
		return command_launch_args;
}

static void gfire_game_launch_edit_xmlnode(xmlnode *p_node, const gchar *p_game_id, const gchar *p_game_name, const gchar *p_game_executable, const gchar *p_game_argument,
	const gchar *p_game_prefix, const gchar *p_game_path, const gchar *p_game_launch, const gchar *p_game_connect)
{
	if(!p_node)
		return;

	// Set Game ID attribute
	if(p_game_id)
		xmlnode_set_attrib(p_node, "id", p_game_id);

	// Set Game Name attribute
	if(p_game_name)
		xmlnode_set_attrib(p_node, "name", p_game_name);

	// Delete XQF node it already exists
	xmlnode *xqf_node = NULL;
	if((xqf_node = xmlnode_get_child(p_node, "xqf")))
		xmlnode_free(xqf_node);

	// Set XQF name
	xqf_node = xmlnode_new_child(p_node, "xqf");
	if(p_game_name)
		xmlnode_set_attrib(xqf_node, "name", p_game_name);

	// Get old command node or create one
	xmlnode *command_node = NULL;
	if(!(command_node = xmlnode_get_child(p_node, "command")))
		command_node = xmlnode_new_child(p_node, "command");

	// Set executable
	xmlnode *executable_node = NULL;
	if((executable_node = xmlnode_get_child(p_node, "executable")))
		xmlnode_free(executable_node);

	executable_node = xmlnode_new_child(command_node, "executable");
	if(p_game_executable)
		xmlnode_insert_data(executable_node, p_game_executable, -1);
	if(p_game_argument)
		xmlnode_set_attrib(executable_node, "argument", p_game_argument);

	// Set prefix
	xmlnode *prefix_node = NULL;
	if((prefix_node = xmlnode_get_child(p_node, "prefix")))
		xmlnode_free(prefix_node);

	prefix_node = xmlnode_new_child(command_node, "prefix");
	if(p_game_prefix)
		xmlnode_insert_data(prefix_node, p_game_prefix, -1);

	// Set path
	xmlnode *path_node = NULL;
	if((path_node = xmlnode_get_child(p_node, "path")))
		xmlnode_free(path_node);

	path_node = xmlnode_new_child(command_node, "path");
	if(p_game_path)
		xmlnode_insert_data(path_node, p_game_path, -1);

	// Set launch
	xmlnode *launch_node = NULL;
	if((launch_node = xmlnode_get_child(p_node, "launch")))
		xmlnode_free(launch_node);

	launch_node = xmlnode_new_child(command_node, "launch");
	if(p_game_launch)
		xmlnode_insert_data(launch_node, p_game_launch, -1);

	// Set connect
	xmlnode *connect_node = NULL;
	if((connect_node = xmlnode_get_child(p_node, "connect")))
		xmlnode_free(connect_node);

	connect_node = xmlnode_new_child(command_node, "connect");
	if(p_game_connect)
		xmlnode_insert_data(connect_node, p_game_connect, -1);
}

/**
 * creates a new xmlnode containing the game information, the returned node must be inserted
 * in the main launch_info node
 *
 * @param p_game_id: the game ID
 * @param p_game_name: the game name
 * @param p_game_executable: the game executable
 * @param p_game_prefix: the game launch prefix (environment variables in most cases)
 * @param p_game_path: the full path to the game executable/launcher
 * @param p_game_launch: the game launch options
 * @param p_game_connect: the game connection options
 *
 * @return: the new xmlnode
 *
**/
static xmlnode *gfire_game_launch_new_xmlnode(const gchar *p_game_id, const gchar *p_game_name, const gchar *p_game_executable, const gchar *p_game_argument,
	const gchar *p_game_prefix, const gchar *p_game_path, const gchar *p_game_launch, const gchar *p_game_connect)
{
	xmlnode *game_node = xmlnode_new("game");
	gfire_game_launch_edit_xmlnode(game_node, p_game_id, p_game_name, p_game_executable, p_game_argument, p_game_prefix,
								   p_game_path, p_game_launch, p_game_connect);

	return game_node;
}

static void gfire_game_launch_xml_init()
{
	if(gfire_launch_xml)
		return;

	gfire_launch_xml = xmlnode_new("launch_info");
	xmlnode_set_attrib(gfire_launch_xml, "version", "2");
}

static gboolean gfire_game_launch_xml_save()
{
	if(!gfire_launch_xml)
		gfire_game_launch_xml_init();

	gchar *gfire_launch_xml_str = xmlnode_to_formatted_str(gfire_launch_xml, NULL);

	gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_xml_str, -1);
	g_free(gfire_launch_xml_str);

	return write_xml;
}

gfire_game_detection_info *gfire_game_detection_info_create()
{
	gfire_game_detection_info *ret = g_malloc0(sizeof(gfire_game_detection_info));
	if(!ret)
		return NULL;

	memset(ret, 0, sizeof(gfire_game_detection_info));

	return ret;
}

void gfire_game_detection_info_free(gfire_game_detection_info *p_info)
{
	if(!p_info)
		return;

	if(p_info->executable) g_free(p_info->executable);
	if(p_info->arguments) g_free(p_info->arguments);
	if(p_info->exclude_ports) g_free(p_info->exclude_ports);

	g_free(p_info);
}

gfire_game_detection_info *gfire_game_detection_info_get(guint32 p_gameid)
{
	// Launch info
	if(!gfire_launch_xml)
		return NULL;

	xmlnode *game_node = gfire_game_launch_node_by_id(p_gameid);
	if(!game_node)
		return NULL;

	xmlnode *command_node = xmlnode_get_child(game_node, "command");
	if(!command_node)
		return NULL;

	xmlnode *executable_node = xmlnode_get_child(command_node, "executable");
	if(!executable_node)
		return NULL;

	gfire_game_detection_info *ret = gfire_game_detection_info_create();
	if(!ret)
		return NULL;

	ret->executable = xmlnode_get_data(executable_node);
	ret->arguments  = g_strdup(xmlnode_get_attrib(executable_node, "argument"));
	ret->id = p_gameid;

	// Game info
	if(!gfire_games_xml)
		return ret;

	game_node = gfire_game_node_by_id(p_gameid);
	if(!game_node)
		return ret;

	xmlnode *detect_node = xmlnode_get_child(game_node, "server_detect");
	if(!detect_node)
		return ret;

	gchar *detect = xmlnode_get_data(detect_node);
	if(g_ascii_strcasecmp(detect, "true") == 0)
		ret->detect = TRUE;
	else
		ret->detect = FALSE;

	g_free(detect);

	xmlnode *exclude_ports = xmlnode_get_child(game_node, "server_exclude_ports");
	if(!exclude_ports)
		return ret;

	gchar *exclude_ports_str = xmlnode_get_data(exclude_ports);
	ret->exclude_ports = g_strsplit(exclude_ports_str, ",", -1);
	g_free(exclude_ports_str);

	return ret;
}

static void gfire_game_manager_update_executable_toggled_cb(GtkBuilder *p_builder, GtkWidget *p_executable_check_button)
{
	if (p_builder == NULL)
	{
		purple_debug_error("gfire", "Couldn't access interface.");
		return;
	}

	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_button"));

	gboolean check_button_state;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button)) == TRUE)
		check_button_state = FALSE;
	else
		check_button_state = TRUE;

	gtk_widget_set_sensitive(add_executable_button, check_button_state);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button)) == TRUE)
		check_button_state = FALSE;
	else
		check_button_state = TRUE;

	gtk_widget_set_sensitive(edit_executable_button, check_button_state);
}

/**
 * adds a game by getting the values from the manage games window
 *
**/
static void gfire_game_manager_add_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{
	if(!p_builder)
	{
		purple_debug_error("gfire", "gfire_game_manager_add_cb: GC not set and/or couldn't access interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(p_builder, "manage_games_window"));
	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_game_entry"));
	GtkWidget *add_path_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_path_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_executable_button"));
	GtkWidget *add_connect_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_connect_entry"));

	GtkWidget *add_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_prefix_entry"));
	GtkWidget *add_launch_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_launch_entry"));
	GtkWidget *add_argument_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "add_argument_entry"));

	const gchar *game_name = gtk_entry_get_text(GTK_ENTRY(add_game_entry));
	gchar *game_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_path_button));
	gboolean game_executable_use_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button));
	gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_executable_button));
	const gchar *game_connect = gtk_entry_get_text(GTK_ENTRY(add_connect_entry));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(add_prefix_entry));
	const gchar *game_launch = gtk_entry_get_text(GTK_ENTRY(add_launch_entry));
	const gchar *game_argument = gtk_entry_get_text(GTK_ENTRY(add_argument_entry));

	if (game_name != NULL && game_path != NULL && game_connect != NULL
		&& ((game_executable_use_path == FALSE && game_executable != NULL) || game_executable_use_path == TRUE))
	{
		guint32 game_id_tmp;
		gchar *game_id;

		game_id_tmp = gfire_game_id(game_name);
		game_id = g_strdup_printf("%u", game_id_tmp);

		xmlnode *game_node = NULL;

		if(!gfire_launch_xml || !gfire_game_launch_node_by_id(game_id_tmp))
		{
			if(!gfire_launch_xml)
				gfire_game_launch_xml_init();

			if(game_executable_use_path == TRUE)
			{
				game_node = gfire_game_launch_new_xmlnode(game_id, game_name, game_path, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
			else
			{
				game_node = gfire_game_launch_new_xmlnode(game_id, game_name, game_executable, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
		}
		else
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: warning"), N_("Game already added"),
								  N_("This game is already added, you can configure it if you want."), NULL, NULL);
			g_free(game_id);
			g_free(game_path);
			g_free(game_executable);
			return;
		}

		g_free(game_id);
		g_free(game_path);
		g_free(game_executable);

		if(!game_node)
		{
			purple_debug_error("gfire: gfire_add_game_cb", "Couldn't create the new game node.\n");
			return;
		}
		else
		{
			xmlnode_insert_child(gfire_launch_xml, game_node);
			if(!gfire_game_launch_xml_save())
			{
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Couldn't add game"),
									  N_("Please try again. An error occured while adding the game."), NULL, NULL);
				return;
			}
			else
			{
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game added"),
									  game_name, N_("The game has been successfully added."), NULL, NULL);
			}
		}
	}
	else
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
							  N_("Couldn't add game"), N_("Please try again. Make sure you fill in all fields."), NULL, NULL);
		return;
	}

	gtk_widget_destroy(manage_games_window);
}

/**
 * edits a game in gfire_launch.xml by getting the new values from
 * the manage games window
 *
**/
static void gfire_game_manager_edit_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{

	if(!p_builder)
	{
		purple_debug_error("gfire", "gfire_game_manager_edit_cb: Couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(p_builder, "manage_games_window"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));
	GtkWidget *edit_path_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_path_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_button"));
	GtkWidget *edit_connect_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_connect_entry"));

	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_prefix_entry"));
	GtkWidget *edit_launch_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_launch_entry"));
	GtkWidget *edit_argument_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_argument_entry"));

	const gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	gchar *game_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_path_button));
	gboolean game_executable_use_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button));
	gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_executable_button));
	const gchar *game_connect = gtk_entry_get_text(GTK_ENTRY(edit_connect_entry));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(edit_prefix_entry));
	const gchar *game_launch = gtk_entry_get_text(GTK_ENTRY(edit_launch_entry));
	const gchar *game_argument = gtk_entry_get_text(GTK_ENTRY(edit_argument_entry));

	if (game_name != NULL && game_path != NULL && game_connect != NULL
		&& ((game_executable_use_path == FALSE && game_executable != NULL) || game_executable_use_path == TRUE))
	{
		guint32 game_id_tmp;
		gchar *game_id;

		game_id_tmp = gfire_game_id(game_name);
		game_id = g_strdup_printf("%u", game_id_tmp);

		xmlnode *game_node = gfire_game_launch_node_by_id(game_id_tmp);

		if(game_node)
		{
			if(game_executable_use_path == TRUE)
			{
				gfire_game_launch_edit_xmlnode(game_node, game_id, game_name, game_path, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
			else
			{
				gfire_game_launch_edit_xmlnode(game_node, game_id, game_name, game_executable, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
		}
		else
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: warning"), N_("Game launch data not found"),
								  N_("This game is not yet added as it seems, please add it first!"), NULL, NULL);
			g_free(game_id);
			g_free(game_path);
			g_free(game_executable);
			return;
		}

		g_free(game_id);
		g_free(game_path);
		g_free(game_executable);

		if(!gfire_game_launch_xml_save())
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Couldn't add game"),
								  N_("Please try again. An error occured while editing the game."), NULL, NULL);
			return;
		}
		else
		{
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game edited"),
								  N_("Game edited"), N_("The game has been successfully edited."), NULL, NULL);
		}
	}
	else
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
							  N_("Couldn't edit game"), N_("Please try again. Make sure you fill in all fields."), NULL, NULL);
		return;
	}

	gtk_widget_destroy(manage_games_window);
}

/**
 * removes the selected game from gfire_launch.xml in the manage games window
 *
**/
static void gfire_game_manager_remove_cb(GtkBuilder *p_builder, GtkWidget *p_button)
{
	if (!p_builder)
	{
		purple_debug_error("gfire", "Couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(p_builder, "manage_games_window"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	if(selected_game != NULL)
	{
		if(gfire_launch_xml != NULL)
		{
			guint32 gameid = gfire_game_id(selected_game);

			xmlnode *game_node = gfire_game_launch_node_by_id(gameid);

			// Remove the node
			xmlnode_free(game_node);

			if(gfire_game_launch_xml_save())
			{
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game removed"),
					N_("Game removed"), N_("The game has been successfully removed."), NULL, NULL);
			}
			else
			{
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
					N_("Couldn't remove game"), N_("Please try again. An error occured while removing the game."), NULL, NULL);
			}
		}

		g_free(selected_game);
	}
	else
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
			N_("Couldn't remove game"), N_("Please try again. Make sure you select a game to remove."), NULL, NULL);
	}

	gtk_widget_destroy(manage_games_window);
}

/**
 * gets and shows the current values from gfire_launch.xml
 * for editing the selected game in the manage games window
 *
**/
static void gfire_game_manager_edit_update_fields_cb(GtkBuilder *p_builder, GtkWidget *p_edit_game_combo)
{
	if(!p_builder)
	{
		purple_debug_error("gfire", "gfire_game_manager_edit_update_fields_cb: Couldn't access interface.\n");
		return;
	}

	if(!gfire_launch_xml)
		return;

	GtkWidget *edit_path_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_path_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_executable_button"));
	GtkWidget *edit_connect_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_connect_entry"));

	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_prefix_entry"));
	GtkWidget *edit_launch_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_launch_entry"));
	GtkWidget *edit_argument_entry = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_argument_entry"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(p_builder, "edit_game_combo"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	//gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_executable_button));

	guint32 gameid = gfire_game_id(selected_game);
	xmlnode *game_node = gfire_game_launch_node_by_id(gameid);

	g_free(selected_game);

	if(!game_node)
		return;

	xmlnode *command_node = xmlnode_get_child(game_node, "command");
	xmlnode *executable_node = xmlnode_get_child(command_node, "executable");
	xmlnode *prefix_node = xmlnode_get_child(command_node, "prefix");
	xmlnode *path_node = xmlnode_get_child(command_node, "path");
	xmlnode *launch_node = xmlnode_get_child(command_node, "launch");
	xmlnode *connect_node = xmlnode_get_child(command_node, "connect");

	gchar *game_executable = xmlnode_get_data(executable_node);
	gchar *game_prefix = xmlnode_get_data(prefix_node);
	gchar *game_path = xmlnode_get_data(path_node);
	gchar *game_launch = xmlnode_get_data(launch_node);
	const gchar *game_argument = xmlnode_get_attrib(executable_node, "argument");
	gchar *game_connect = xmlnode_get_data(connect_node);

	if (g_strcmp0(game_path, game_executable) == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), FALSE);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_executable_button), game_executable);
	gtk_entry_set_text(GTK_ENTRY(edit_prefix_entry), game_prefix);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_path_button), game_path);
	gtk_entry_set_text(GTK_ENTRY(edit_launch_entry), game_launch);
	gtk_entry_set_text(GTK_ENTRY(edit_argument_entry), game_argument);
	gtk_entry_set_text(GTK_ENTRY(edit_connect_entry), game_connect);

	if(game_executable) g_free(game_executable);
	if(game_prefix) g_free(game_prefix);
	if(game_path) g_free(game_path);
	if(game_launch) g_free(game_launch);
	if(game_connect) g_free(game_connect);
}

static void gfire_game_manager_update_executable_cb(GtkWidget *p_executable_button, GtkWidget *p_path_button)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p_executable_button), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p_path_button)));
}

/**
 * shows the manage games window
 *
 * @param p_action: the menu action, passed by the signal connection function
 *
**/
void gfire_game_manager_show(PurplePluginAction *p_action)
{
	GtkBuilder *builder = gtk_builder_new();
	gchar *builder_file;

	builder_file = g_build_filename(DATADIR, "purple", "gfire", "games.glade", NULL);
	gtk_builder_add_from_file(builder, builder_file, NULL);
	g_free(builder_file);

	if (builder == NULL)
	{
		purple_debug_error("gfire", "gfire_game_manager_show: Couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_game_entry"));
	GtkWidget *add_path_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_path_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_button"));
	GtkWidget *add_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_close_button"));
	GtkWidget *add_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_add_button"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "edit_game_combo"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_check_button"));
	GtkWidget *edit_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_close_button"));
	GtkWidget *edit_apply_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_apply_button"));
	GtkWidget *edit_remove_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_remove_button"));

	g_signal_connect_swapped(add_path_button, "current-folder-changed", G_CALLBACK(gfire_game_manager_update_executable_cb), add_executable_button);
	g_signal_connect_swapped(add_executable_check_button, "toggled", G_CALLBACK(gfire_game_manager_update_executable_toggled_cb), builder);
	g_signal_connect_swapped(add_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(add_add_button, "clicked", G_CALLBACK(gfire_game_manager_add_cb), builder);
	g_signal_connect_swapped(edit_game_combo, "changed", G_CALLBACK(gfire_game_manager_edit_update_fields_cb), builder);
	g_signal_connect_swapped(edit_executable_check_button, "toggled", G_CALLBACK(gfire_game_manager_update_executable_toggled_cb), builder);
	g_signal_connect_swapped(edit_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(edit_apply_button, "clicked", G_CALLBACK(gfire_game_manager_edit_cb), builder);
	g_signal_connect_swapped(edit_remove_button, "clicked", G_CALLBACK(gfire_game_manager_remove_cb), builder);

	// Reload XML before we edit it
	gfire_game_load_launch_xml();

	if(!gfire_launch_xml)
		gfire_game_launch_xml_init();

	const gchar *manager_version = xmlnode_get_attrib(gfire_launch_xml, "version");

	if(g_strcmp0(manager_version, "2") != 0)
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Incompatible games configuration"),
							  N_("Your current games configuration is incompatible with this version of Gfire. Please remove it and try again."), NULL, NULL);

		gtk_widget_destroy(manage_games_window);
		return;
	}
	else
	{
		xmlnode *node_child;
		for(node_child = xmlnode_get_child(gfire_launch_xml, "game"); node_child != NULL;
		node_child = xmlnode_get_next_twin(node_child))
		{
			const gchar *game_name = xmlnode_get_attrib(node_child, "name");
			gtk_combo_box_append_text(GTK_COMBO_BOX(edit_game_combo), game_name);
		}
	}

	GtkEntryCompletion *add_game_completion;
	GtkListStore *add_game_list_store;
	GtkTreeIter add_game_iter;

	add_game_list_store = gtk_list_store_new(1, G_TYPE_STRING);

	if(gfire_games_xml)
	{
		xmlnode *node_child;
		for(node_child = xmlnode_get_child(gfire_games_xml, "game"); node_child != NULL;
			node_child = xmlnode_get_next_twin(node_child))
		{
			const gchar *game_name = xmlnode_get_attrib(node_child, "name");

			gtk_list_store_append(add_game_list_store, &add_game_iter);
			gtk_list_store_set(add_game_list_store, &add_game_iter, 0,  game_name, -1);
		}

		add_game_completion = gtk_entry_completion_new();
		gtk_entry_completion_set_model(add_game_completion, GTK_TREE_MODEL(add_game_list_store));
		gtk_entry_completion_set_text_column(add_game_completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(add_game_entry), add_game_completion);
	}
	else
	{
		purple_debug_error("gfire", "gfire_game_manager_show: Couldn't get games list.\n");
		gtk_widget_destroy(manage_games_window);
		return;
	}

	gtk_widget_show_all(manage_games_window);
}
