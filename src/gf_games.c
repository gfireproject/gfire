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

#include "gfire.h"

void gfire_xml_download_cb(

	PurpleUtilFetchUrlData *url_data,
	gpointer data, 
	const char *buf, 

	gsize len,
	const gchar *error_message
	);


int gfire_game_id(PurpleConnection *gc, char *game_name)
{
	gfire_data *gfire = NULL;
	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_game_id", "GC not set or found.\n");
		return;
	}

	xmlnode *gfire_games;
	int game_id = -1;

	gfire_games = gfire->xml_games_list;
	if (gfire_games != NULL)
	{
		xmlnode *node_child;
		const char *game_name_tmp;
		const char *game_id_tmp;
		
		for (node_child = xmlnode_get_child(gfire_games, "game"); node_child != NULL;
		     node_child = xmlnode_get_next_twin(node_child))
		{
			game_name_tmp = xmlnode_get_attrib(node_child, "name");
			if (g_strcmp0(game_name_tmp, game_name) == NULL) {
				game_id_tmp = xmlnode_get_attrib(node_child, "id");
				game_id = atoi(game_id_tmp);
			}
		}
	}
	
	return game_id;
}


/**
 * Returns via *buffer the name of the game identified by int game
 * if the game doesn't exist, it uses the game number as the name
 *
 * @param buffer			pointer to string return value will be here
 * @param game				integer ID of the game to translate
*/
char *gfire_game_name(PurpleConnection *gc, int game)
{
	xmlnode *node = NULL;
	const char *name = NULL;
	const char *num = NULL;
	char *ret = NULL;
	int found = FALSE;
	int id = 0;
	gfire_data *gfire = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return NULL;

	if (gfire->xml_games_list != NULL) {
		node = xmlnode_get_child(gfire->xml_games_list, "game");
		while (node) {
			name = xmlnode_get_attrib(node, "name");
			num  = xmlnode_get_attrib(node, "id");
			id = atoi((const char *)num);
			if (id == game) {
				found = TRUE;
			}
			if (found) break;
			node = xmlnode_get_next_twin(node);
		}
		/* if we didn't find the game just show game ID */
		if (!found) {
			ret = g_strdup_printf("%d",game);
		} else {
			// Name is non-empty?
			if(name)
				ret = gfire_escape_html(name);
			else
				ret = g_strdup_printf("%d",game);
		}
	}else{
		/* uhh our gfire_games.xml was not found */
		ret = g_strdup_printf("%d",game);
	}
	return ret;
}


/**
 * Parses XML file to convert xfire game ID's to names
 *
 * @param filename		The filename to parse (xml)
 *
 * @return TRUE if parsed ok, FALSE otherwise
*/ 
gboolean gfire_parse_games_file(PurpleConnection *gc, const char *filename)
{
	xmlnode *node = NULL;
	GError *error = NULL;
	gchar *contents = NULL;
	gsize length;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return FALSE;

	purple_debug(PURPLE_DEBUG_INFO, "gfire",
			   "XML Games import, Reading %s\n", NN(filename));

	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
		/* file not found try to grab from url */

		purple_util_fetch_url(GFIRE_GAMES_XML_URL, TRUE, "Purple-xfire", TRUE, gfire_xml_download_cb, (void *)gc);

		return FALSE;
	}

	if (!g_file_get_contents(filename, &contents, &length, &error)) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
				   "XML Games import, Error reading game list: %s\n", NN(error->message));
		g_error_free(error);
		return FALSE;
	}

	node = xmlnode_from_str(contents, length);
	
	if (!node) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
				   "XML Games import, Error parsing XML file: %s\n", NN(filename));
		g_free(contents);
		return FALSE;
	}

	gfire->xml_games_list = node;	
	g_free(contents);
	return TRUE;
};


/**
 *	Parses launchinfo.xml file into memory for launching games
 *	directly from right click menu
 *
 *	@param		filename		filename of the xml file to parse (dir+fname)
 *
 *	@return		boolean		return true on success, false on failure
**/
gboolean gfire_parse_launchinfo_file(PurpleConnection *gc, const char *filename)
{
	gfire_data *gfire = NULL;
	xmlnode *node = NULL;
	GError *error = NULL;
	gchar *contents = NULL;
	gsize length;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !filename) return FALSE;

	purple_debug(PURPLE_DEBUG_INFO, "gfire", "launchinfo import Reading %s\n", NN(filename));
	if (!g_file_get_contents(filename, &contents, &length, &error)) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "launchinfo import Error reading launchinfo list: %s\n"
					, NN(error->message));
		g_error_free(error);
		return FALSE;
	}

	node = xmlnode_from_str(contents, length);
	
	if (!node) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "launchinfo import Error parsing XML file: %s\n", NN(filename));
		g_free(contents);
		return FALSE;
	}

	gfire->xml_launch_info = node;	
	g_free(contents);
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
gboolean gfire_game_playable(PurpleConnection *gc, int game)
{
	gfire_game_launch_info *foo = NULL;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !game) return FALSE;
	
	foo = gfire_game_launch_info_get(gc, game);
	if (!foo) return FALSE;
	gfire_game_launch_info_free(foo);
	return TRUE;

}


/**
 * gives the integer representation of the game the buddy is playing
 *
 * @param gc	Valid PurpleConnection
 * @param b		Buddy to get game number of
 *
 * @return returns the game or 0 if the buddy is not playing
*/
int gfire_get_buddy_game(PurpleConnection *gc, PurpleBuddy *b)
{
		gfire_data *gfire = NULL;
		GList *l = NULL;
		gfire_buddy *gb = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b || !b->name) return 0;

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if (!l || !(l->data)) return 0;
	gb = (gfire_buddy *)l->data;
	return gb->gameid;
}


/**
 * get port number of the game the buddy is playing
 *
 * @param gc	Valid PurpleConnection
 * @param b		Buddy to get game number of
 *
 * @return returns the game or 0 if the buddy is not playing
*/
int gfire_get_buddy_port(PurpleConnection *gc, PurpleBuddy *b)
{
		gfire_data *gfire = NULL;
		GList *l = NULL;
		gfire_buddy *gb = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b || !b->name) return 0;

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if (!l || !(l->data)) return 0;
	gb = (gfire_buddy *)l->data;
	return gb->gameport;
}


/**
 * get ip address of server where buddy is in game
 *
 * @param gc	Valid PurpleConnection
 * @param b		Buddy to get game ip number of
 *
 * @return returns the ip address as a string or NULL if budy isn't playing
*/
const gchar *gfire_get_buddy_ip(PurpleConnection *gc, PurpleBuddy *b)
{
		gfire_data *gfire = NULL;
		GList *l = NULL;
		gfire_buddy *gb = NULL;
		gchar *tmp = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b || !b->name) return 0;

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if (!l || !(l->data)) return NULL;
	gb = (gfire_buddy *)l->data;
	if(gfire_get_buddy_game(gc ,b) != 0){
		tmp = g_malloc0(16);
		g_sprintf(tmp, "%d.%d.%d.%d", gb->gameip[3], gb->gameip[2], gb->gameip[1], gb->gameip[0]);
		return tmp;
 	}
 	return NULL;
}

gfire_game_launch_info *gfire_game_launch_info_new()
{
	gfire_game_launch_info *game_launch_info;
	game_launch_info = g_malloc0(sizeof(gfire_game_launch_info));
	return game_launch_info;
}

void gfire_game_launch_info_free(gfire_game_launch_info *game_launch_info)
{
	if (game_launch_info == NULL) return;
	g_free(game_launch_info);
}

gfire_game_launch_info *gfire_game_launch_info_get(PurpleConnection *gc, int game_id)
{
	gfire_data *gfire = NULL;
	gfire_game_launch_info *l = NULL;
	xmlnode *node = NULL;
	xmlnode *cnode = NULL;
	xmlnode *command = NULL;
	const char *name, *num;
	int found = 0;
	int id = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !game_id) return NULL;
	
	if (gfire->xml_launch_info != NULL) {
		node = xmlnode_get_child(gfire->xml_launch_info, "game");
		while (node) {
			name = xmlnode_get_attrib(node, "name");
			num  = xmlnode_get_attrib(node, "id");
			id = atoi((const char *)num);
// printf("linfo id=%d\tname=\"%s\"\n", id, name);
			if (id == game_id) {
				found = 1;
			}
			if (found) break;
			node = xmlnode_get_next_twin(node);
		}    
		/* if we didn't find the game */
		if (!found) {
			return NULL;
		}
		/* got the game */
		l = gfire_game_launch_info_new();
		if (!l) return NULL; /* Out of Memory */
		
		l->game_id = game_id;
		l->game_name = g_strdup(name);
		for (cnode = node->child; cnode; cnode = cnode->next) {
			if (cnode->type != XMLNODE_TYPE_TAG)
				continue;
			
			if (strcmp(cnode->name, "command") == NULL)
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
	}
	
	return l;
}


/**
 *	Parses launch field for delim and replaces it with data. Returns pointer to newly
 *	created string with all subsitutions.
 *
 *	@param		src		string to search for delimeters (usually @xxxx@)
 *	@param		data	string to replace delimeter with. (ip address, ports, etc)
 *	@param		delim	the matching string that needs to be replaced (ex: @port@)
 *
 *	@return				pointer to string with delimeter replaced with data. memory
 *						is allocated using glib, free with g_free when done.
*/
gchar *gfire_launch_parse(const char *src, const char *data, const char *delim)
{
	char **sv;
	gchar *tmp = NULL;
	
	if (!data || !delim || !src) {
		/* no data or delim return empty string */
		return g_strdup("");
	}
	
	sv = g_strsplit(src, delim, 0);
	if (g_strv_length(sv) <= 1) {
		/* no matches */
		return g_strdup(src);
	}

	if (g_strv_length(sv) > 2 ) {
		return g_strdup("");
//		fprintf(stderr, "ERR: (xfire_launch_parse) Only 1 delimeter allowed per subsitution!\n");
	}
	tmp = g_strjoinv(data, sv);	
	g_strfreev(sv);
	return tmp;

}

gchar *gfire_game_launch_info_get_command(gfire_game_launch_info *game_launch_info, const guint8 *server_ip, int server_port)
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
	
	if(game_launch_info->game_connect && server_ip)
	{
		gchar *server_ip_tmp, *server_port_tmp, *tmp = NULL;

		server_port_tmp = g_strdup_printf("%d", server_port);
		server_ip_tmp = g_strdup_printf("%d.%d.%d.%d", server_ip[3], server_ip[2], server_ip[1], server_ip[0]);
		tmp = str_replace(game_launch_info->game_connect, "[ip]", server_ip_tmp);
		game_connect_option = str_replace(tmp, "[port]", server_port_tmp);

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

/**
 *	Return ip address struct initialized with data supplied by argument
 *	When done with structure, it should be freed with xfire_ip_struct_free()
 *
 *	@param		ip		string ip address to fill struct with ex ("127.0.0.1")
 *
 *	@return				returns NULL if no string provided. Otherwise returns
 *						pointer to struct.  ex: s = struct,
 *						
 *						s->octet = { 127, 0, 0, 1 }
 *						s->ipstr = "127.0.0.1"
 *
 *						Structure should be freed with xfire_ip_struct_free()
*/
gchar *gfire_ipstr_to_bin(const gchar *ip)
{
	gchar **ss = NULL;
	int i = 0; int j = 3;
	gchar *ret = NULL;
	
	if (strlen(ip) <= 0) return NULL;
	
	ss = g_strsplit(ip,".", 0);
	if (g_strv_length(ss) != 4) {
		g_strfreev(ss);
		return NULL;
	}
	ret = g_malloc0(sizeof(gchar) * XFIRE_GAMEIP_LEN);
	if (!ret) {
		g_strfreev(ss);
		return NULL;
	}
	j = 3;
	for (i=0; i < 4; i++) {
		ret[j--] = atoi(ss[i]);
	}
	g_strfreev(ss);
	return ret;
}



void gfire_xml_download_cb( PurpleUtilFetchUrlData *url_data, gpointer data, const char *buf, gsize len, const gchar *error_message)
{	
	char *successmsg = NULL;
	PurpleConnection *gc = NULL;
	const char *filename = g_build_filename(purple_user_dir(), "gfire_games.xml", NULL);

	if (! data || !buf || !len) {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("XFire games Download"), N_("Will attempt to download gfire_games.xml from the Gfire server."), N_("Unable to download gfire_games.xml"), NULL, NULL);
		return;
	}	 
	gc = (PurpleConnection *)data;
	if ( purple_util_write_data_to_file("gfire_games.xml", buf, len) ) {
		/* we may be called when we are no longer connected gfire-> may not be vaild */
		if ( PURPLE_CONNECTION_IS_VALID(gc) && PURPLE_CONNECTION_IS_CONNECTED(gc) ) {
			gfire_parse_games_file(gc, filename);
		}
		if(strcmp(gfire_game_name(gc, 100), "100")) {
			successmsg = g_strdup_printf(N_("Successfully downloaded gfire_games.xml\nNew Games List Version: %s"), gfire_game_name(gc, 100));
		} else {
			successmsg = g_strdup_printf(N_("Successfully downloaded gfire_games.xml"));
		}
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("XFire games Download"), "Will attempt to download gfire_games.xml from the Gfire server.", successmsg, NULL, NULL);
		
	} else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("XFire games Download"), N_("Will attempt to download gfire_games.xml from the Gfire server."), N_("Unable to write gfire_games.xml"), NULL, NULL);
	}
}
