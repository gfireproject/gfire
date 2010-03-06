/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009       Oliver Ney <oliver@dryder.de>
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

#include "gf_server_browser.h"

#ifdef HAVE_GTK
static GtkBuilder *gfire_gtk_builder = NULL;
static GtkTreeStore *server_browser_tree_store = NULL;

static GtkTreeIter recent_serverlist_iter;
static GtkTreeIter fav_serverlist_iter;
static GtkTreeIter friends_fav_serverlist_iter;
static GtkTreeIter serverlist_iter;

static void gfire_server_browser_add_parent_rows()
{
	// Clear tree store
	gtk_tree_store_clear(server_browser_tree_store);

	// Add main (parent) rows
	gtk_tree_store_append(server_browser_tree_store, &recent_serverlist_iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &recent_serverlist_iter, 0, _("Recent servers"), 1, "", -1);

	gtk_tree_store_append(server_browser_tree_store, &fav_serverlist_iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &fav_serverlist_iter, 0, _("Favorite servers"), -1);

	gtk_tree_store_append(server_browser_tree_store, &friends_fav_serverlist_iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &friends_fav_serverlist_iter, 0, _("Friends' favorite servers"), -1);

	gtk_tree_store_append(server_browser_tree_store, &serverlist_iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &serverlist_iter, 0, _("All servers"), -1);
}

// FIXME: not revised
void gfire_server_browser_add_server(gfire_server_browser_server_info *p_server)
{
	// Get parent row
	gint parent = gfire_server_brower_proto_get_parent(p_server);
	if(parent == -1)
		return;

	// Add row
	GtkTreeIter iter;

	if(parent == 0)
		gtk_tree_store_append(server_browser_tree_store, &iter, &recent_serverlist_iter);
	else if(parent == 1)
		gtk_tree_store_append(server_browser_tree_store, &iter, &fav_serverlist_iter);
	else if(parent == 2)
		gtk_tree_store_append(server_browser_tree_store, &iter, &friends_fav_serverlist_iter);
	else if(parent == 3)
		gtk_tree_store_append(server_browser_tree_store, &iter, &serverlist_iter);

	gtk_tree_store_set(server_browser_tree_store, &iter, 0, p_server->ip_full, 1, _("N/A"), 2, _("N/A"), 3, _("N/A"), 4, p_server->ip_full, -1);

	// Set row data
	if(p_server)
	{
		// If we didn't get server name, then remove it
		if(p_server->name)
		{
			gchar *server_name_clean = g_strstrip(gfire_remove_quake3_color_codes(p_server->name));
			gtk_tree_store_set(server_browser_tree_store, &iter, 0, server_name_clean, -1);
			g_free(server_name_clean);
		}

		if(p_server->ping != -1)
		{
			gchar *server_ping = g_strdup_printf("%d", p_server->ping);
			gtk_tree_store_set(server_browser_tree_store, &iter, 1, server_ping, 6, p_server->ping, -1);
			g_free(server_ping);
		}

		if(p_server->players != -1 && p_server->max_players != -1)
		{
			gchar *server_players = g_strdup_printf("%u/%u", p_server->players, p_server->max_players);
			gtk_tree_store_set(server_browser_tree_store, &iter, 2, server_players, 7, p_server->players, -1);
			g_free(server_players);
		}

		if(p_server->map)
			gtk_tree_store_set(server_browser_tree_store, &iter, 3, p_server->map, -1);

		if(p_server->raw_info)
		{
			gchar *server_raw_info = g_strdup(p_server->raw_info);
			gtk_tree_store_set(server_browser_tree_store, &iter, 5, server_raw_info, -1);
			g_free(server_raw_info);
		}
	}
}

static void gfire_server_browser_add_fav_server_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!gfire_server_browser_proto_can_add_fav_server())
	{
		purple_notify_error(NULL, _("Server Browser: error"), _("Can't add favorite server"),
							_("You've reached the limit of favorite servers, you can however still remove favorite servers in order to add new ones."));

		return;
	}

	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "servers_tree_view"));
	GtkWidget *add_favorite_dialog = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_favorite_dialog"));
	GtkWidget *ip_address_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "ip_address_entry"));
	GtkWidget *port_entry = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "port_entry"));

	// Eventually get selected server
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(selected_server)
		{
			gchar **server_split;
			server_split = g_strsplit(selected_server, ":", -1);
			g_free(selected_server);

			if(server_split)
			{
				gchar *ip_str = g_strdup(server_split[0]);
				gchar *port_str = g_strdup(server_split[1]);

				gtk_entry_set_text(GTK_ENTRY(ip_address_entry), ip_str);
				gtk_entry_set_text(GTK_ENTRY(port_entry), port_str);

				g_free(ip_str);
				g_free(port_str);

				g_strfreev(server_split);
			}
		}
	}

	gint result = gtk_dialog_run(GTK_DIALOG(add_favorite_dialog));
	if(result == 0)
	{
		// Get user input
		GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "game_combo"));

		const gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
		guint32 gameid = gfire_game_id(game_name);

		const gchar *ip_str = gtk_entry_get_text(GTK_ENTRY(ip_address_entry));
		const gchar *port_str = gtk_entry_get_text(GTK_ENTRY(port_entry));

		// Check given IP
		guint16 ip1, ip2, ip3, ip4;

		if(sscanf(ip_str, "%3hu.%3hu.%3hu.%3hu", &ip1, &ip2, &ip3, &ip4) == 4)
		{
			// Check every IP part
			if((ip1 < 256) || (ip2 < 256) || (ip3 < 256) || (ip4 < 256))
			{
				// Check port
				guint16 port;

				if(sscanf(port_str, "%hu", &port) == 1)
				{
					// Add fav server to user Xfire's profile
					gfire_server_browser_proto_add_fav_server(p_gfire, gameid, ip_str, port_str);

					// Get IP in correct datatype
					gfire_game_data game_tmp;
					gfire_game_data_ip_from_str(&game_tmp, ip_str);

					guint32 ip = game_tmp.ip.value;

					// Add to server to local list
					gfire_server_browser_server *server = gfire_server_browser_server_new();
					gfire_server_browser_add_fav_server_local(gameid, ip, port);

					// Push fav server to fav queue
					server->protocol = (gchar *)gfire_game_server_query_type(gameid);
					server->ip = ip;
					server->port = port;
					server->parent = 1;

					// Push server struct to queue
					gfire_server_browser_proto_push_server(server);
				}
			}
		}
	}

	gtk_widget_hide(add_favorite_dialog);
}

static void gfire_server_browser_remove_fav_server_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "servers_tree_view"));

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	guint32 gameid = gfire_game_id(game_name);
	g_free(game_name);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(!selected_server)
			return;

		gchar **server_split;
		server_split = g_strsplit(selected_server, ":", -1);
		g_free(selected_server);

		if(!server_split)
			return;

		gchar *ip_str = g_strdup(server_split[0]);
		gchar *port_str = g_strdup(server_split[1]);

		// Get IP & port in correct datatype
		gfire_game_data game_tmp;
		gfire_game_data_ip_from_str(&game_tmp, ip_str);

		// Put server in struct
		gfire_server_browser_server_info *server_info = gfire_server_browser_server_info_new();

		server_info->ip = game_tmp.ip.value;
		server_info->port =  atoi(port_str);

		g_strfreev(server_split);

		// Check if server is fav server
		if(!gfire_server_browser_proto_is_fav_server(server_info->ip, server_info->port))
		{
			purple_notify_error(NULL, _("Server Browser: error"), _("Can't remove favorite server"),
								_("The selected server is not a favorite server and thereby can't be removed."));

			return;
		}

		// Remove fav server from user Xfire's profile & local list
		gfire_server_browser_proto_remove_fav_server(p_gfire, gameid, ip_str, port_str);
		gfire_server_browser_remove_fav_server_local(gameid, server_info->ip, server_info->port);

		// Remove server from tree view
		gtk_tree_store_remove(server_browser_tree_store, &iter);

		g_free(ip_str);
		g_free(port_str);
	}
	else
		purple_debug_error("gfire", "Couldn't get selected favorite server to remove.\n");
}

static void gfire_server_browser_request_serverlist_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire)
		return;

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "game_combo"));
	GtkWidget *list_store = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "servers_list_store"));

	gtk_list_store_clear(GTK_LIST_STORE(list_store));

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	guint32 gameid = gfire_game_id(game_name);
	g_free(game_name);

	if(gameid != 0)
	{
		// Set requested gameid
		gfire_server_browser_proto_set_curr_gameid(gameid);

		// Add parent rows (also clears tree store)
		gfire_server_browser_add_parent_rows();

		// Show fav serverlist (local)
		gfire_server_browser_proto_fav_serverlist_request(gameid);

		// Make serverlist requests to Xfire
		guint16 packet_len = 0;

		// Request serverlist
		packet_len = gfire_server_browser_proto_create_serverlist_request(gameid);
		if(packet_len > 0)
			gfire_send(gfire_get_connection(p_gfire), packet_len);

		// Request friends' fav serverlist
		packet_len = gfire_server_browser_proto_create_friends_fav_serverlist_request(gameid);
		if(packet_len > 0)
			gfire_send(gfire_get_connection(p_gfire), packet_len);
	}
}

static void gfire_server_browser_connect_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire)
		return;

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "servers_tree_view"));

	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));

	// Get selected server
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(!selected_server)
			return;

		gchar **server_split;
		server_split = g_strsplit(selected_server, ":", -1);
		g_free(selected_server);

		if(!server_split)
			return;

		// Put server in struct
		gfire_game_data game;

		game.id = gfire_game_id(game_name);
		gfire_game_data_ip_from_str(&game, server_split[0]);
		game.port = atoi(server_split[1]);

		g_free(game_name);
		g_strfreev(server_split);

		// Finally join server
		gfire_join_game(&game);
	}
	else
	{
		g_free(game_name);
		purple_debug_error("gfire", "Couldn't get selected server to join.\n");
	}
}

static void gfire_server_browser_close(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire)
		return;

	if(gfire_gtk_builder)
	{
		// Destroy window
		GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "server_browser_window"));
		gtk_widget_destroy(server_browser_window);

		// Free builder
		g_object_unref(G_OBJECT(gfire_gtk_builder));
		gfire_gtk_builder = NULL;
	}
}

static gboolean gfire_server_browser_add_configured_games_cb(const gfire_game_configuration *p_gconf, GtkWidget *p_game_combo)
{
	const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
	if(game)
		gtk_combo_box_append_text(GTK_COMBO_BOX(p_game_combo), game->name);

	return FALSE;
}

void gfire_server_browser_show(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	if(!gc || !gfire)
		return;

	// Only one server browser at a time
	if(gfire_gtk_builder)
		return;

	gfire_gtk_builder = gtk_builder_new();

	if(!gfire_gtk_builder)
	{
		purple_debug_error("gfire", "Couldn't build game manager interface.\n");
		return;
	}

	gtk_builder_set_translation_domain(gfire_gtk_builder, GETTEXT_PACKAGE);

	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(gfire_gtk_builder, builder_file, NULL);
	g_free(builder_file);

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "server_browser_window"));
	GtkWidget *tree_view = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "servers_tree_view"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "refresh_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "connect_button"));
	GtkWidget *add_fav_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "add_favorite_button"));
	GtkWidget *remove_fav_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "remove_favorite_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "quit_button"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire_gtk_builder, "game_combo"));

	g_signal_connect_swapped(server_browser_window, "destroy", G_CALLBACK(gfire_server_browser_close), gfire);
	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gfire_server_browser_close), gfire);

	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_server_browser_request_serverlist_cb), gfire);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), gfire);
	g_signal_connect_swapped(add_fav_button, "clicked", G_CALLBACK(gfire_server_browser_add_fav_server_cb), gfire);
	g_signal_connect_swapped(remove_fav_button, "clicked", G_CALLBACK(gfire_server_browser_remove_fav_server_cb), gfire);

	// Connect too when user double clicks on a server
	g_signal_connect_swapped(tree_view, "row-activated", G_CALLBACK(gfire_server_browser_connect_cb), gfire);

	// Add configured games & initialize tree store
	gfire_game_config_foreach(G_CALLBACK(gfire_server_browser_add_configured_games_cb), game_combo);
	server_browser_tree_store = GTK_TREE_STORE(gtk_builder_get_object(gfire_gtk_builder, "servers_list_tree_store"));

	// Show window
	gtk_widget_show_all(server_browser_window);
}
#endif // HAVE_GTK
