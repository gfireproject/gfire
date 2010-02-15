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
static GtkBuilder *server_browser_builder = NULL;
static GtkTreeStore *server_browser_tree_store;

static void gfire_server_browser_close(gfire_data *p_gfire, GtkWidget *p_sender);
static void gfire_server_browser_request_list_cb(gfire_data *p_gfire, GtkWidget *p_sender);
static void gfire_server_browser_connect_cb(gfire_data *p_gfire, GtkWidget *p_sender);
static void gfire_server_browser_server_information_cb(gfire_data *p_gfire, GtkWidget *p_sender);

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
	gfire_data *gfire = NULL;

	if(!gc || !(gfire = (gfire_data *)gc->proto_data))
	{
		purple_debug_error("gfire", "Purple connection not set and/or couldn't access gfire data.\n");
		return;
	}

	// Only one server browser at a time
	if(server_browser_builder)
		return;

	server_browser_builder = gtk_builder_new();

	if(!server_browser_builder)
	{
		purple_debug_error("gfire", "Couldn't build server browser interface.\n");
		return;
	}

	gtk_builder_set_translation_domain(server_browser_builder, GETTEXT_PACKAGE);

	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(server_browser_builder, builder_file, NULL);
	g_free(builder_file);

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "server_browser_window"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "servers_tree_view"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "refresh_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "connect_button"));
	GtkWidget *information_button = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "information_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "quit_button"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "game_combo"));

	g_signal_connect_swapped(server_browser_window, "destroy", G_CALLBACK(gfire_server_browser_close), gfire);
	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gfire_server_browser_close), gfire);

	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_server_browser_request_list_cb), gfire);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), gfire);
	g_signal_connect_swapped(information_button, "clicked", G_CALLBACK(gfire_server_browser_server_information_cb), gfire);

	// Connect too when user double clicks on a server
	g_signal_connect_swapped(servers_tree_view, "row-activated", G_CALLBACK(gfire_server_browser_connect_cb), gfire);

	// Add configured games & initialize tree store
	gfire_game_config_foreach(G_CALLBACK(gfire_server_browser_add_configured_games_cb), game_combo);
	server_browser_tree_store = GTK_TREE_STORE(gtk_builder_get_object(server_browser_builder, "servers_list_tree_store"));

	// Show window
	gtk_widget_show_all(server_browser_window);
}

static void gfire_server_browser_close(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire)
		return;

	if(servers_list_thread_pool)
		g_thread_pool_free(servers_list_thread_pool, TRUE, FALSE);

	if(p_gfire->server_browser_pool > 0)
		g_source_remove(p_gfire->server_browser_pool);

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "server_browser_window"));
	gtk_widget_destroy(server_browser_window);

	if(server_browser_builder)
	{
		g_object_unref(G_OBJECT(server_browser_builder));
		server_browser_builder = NULL;
	}
}

static void gfire_server_browser_request_list_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire)
		return;

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "game_combo"));
	GtkWidget *servers_list_store = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "servers_list_store"));

	gtk_list_store_clear(GTK_LIST_STORE(servers_list_store));

	gchar *game_name;
	guint32 game_id;

	game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	game_id = gfire_game_id(game_name);
	g_free(game_name);

	if(game_id != 0)
	{
		// Define game id, to be able to determine query types later
		servers_list_queried_game_id = game_id;

		guint16 packet_len = 0;
		packet_len = gfire_server_browser_proto_create_serverlist_request(game_id);

		if(packet_len > 0)
			gfire_send(gfire_get_connection(p_gfire), packet_len);
	}
}

static void gfire_server_browser_server_information_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "servers_tree_view"));
	GtkWidget *server_browser_information_window = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "server_browser_information"));
	GtkWidget *close_button = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "sbi_close_button"));
	GtkWidget *sbi_text_view = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "sbi_text_view"));

	g_signal_connect_swapped(server_browser_information_window, "destroy", G_CALLBACK(gtk_widget_destroy), server_browser_information_window);
	g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_widget_destroy), server_browser_information_window);

	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *server_raw_info;

		gtk_tree_model_get(model, &iter, 5, &server_raw_info, -1);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sbi_text_view));
		gtk_text_buffer_set_text(buffer, g_strescape(server_raw_info, "\n"), -1);
	}
	else
		purple_debug_error("gfire", "Couldn't get selected server to display raw info.\n");

	gtk_widget_show_all(server_browser_information_window);
}

// FIXME: Must be revised!
static void gfire_server_browser_connect_cb(gfire_data *p_gfire, GtkWidget *p_sender)
{
	if(!p_gfire || !server_browser_builder)
	{
		purple_debug_error("gfire", "Purple connection not set and/or couldn't access server browser interface.\n");
		return;
	}

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(server_browser_builder, "servers_tree_view"));

	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_data game;
		gchar *server;
		gchar **server_tok;

		gtk_tree_model_get(model, &iter, 4, &server, -1);
		server_tok = g_strsplit(server, ":", -1);
		g_free(server);

		if(!server_tok)
			return;

		game.id = gfire_game_id(game_name);
		gfire_game_data_ip_from_str(&game, server_tok[0]);
		game.port = atoi(server_tok[1]);

		g_strfreev(server_tok);

		gfire_join_game(&game);
	}
	else
		purple_debug_error("gfire", "Couldn't get selected server to join.\n");
}

void gfire_server_browser_add_parent_rows()
{
	// Add main (parent) rows
	GtkTreeIter iter;

	gtk_tree_store_append(server_browser_tree_store, &iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &iter, 0, "Recent servers", -1);

	gtk_tree_store_append(server_browser_tree_store, &iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &iter, 0, "Favorite servers", -1);

	gtk_tree_store_append(server_browser_tree_store, &iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &iter, 0, "Friends' favorite servers", -1);

	gtk_tree_store_append(server_browser_tree_store, &iter, NULL);
	gtk_tree_store_set(server_browser_tree_store, &iter, 0, "All servers", -1);
}

GtkTreeIter gfire_server_browser_add_server_row(gchar *p_addr)
{
	GtkTreeIter all_servers_iter, iter;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(server_browser_tree_store), &all_servers_iter, "3");

	gtk_tree_store_append(server_browser_tree_store, &iter, &all_servers_iter);
	gtk_tree_store_set(server_browser_tree_store, &iter, 0, p_addr, 1, _("N/A"), 2, _("N/A"), 3, _("N/A"), 4, p_addr, -1);

	return iter;
}

void gfire_server_browser_set_server(gfire_server_info *p_server)
{
	if(p_server)
	{
		GtkTreeIter iter = p_server->server_list_iter;

		if(p_server->name)
		{
			gchar *server_name_clean = g_strstrip(gfire_remove_quake3_color_codes(p_server->name));
			gtk_tree_store_set(server_browser_tree_store, &iter, 0, server_name_clean, -1);
			g_free(server_name_clean);
		}

		if(p_server->ping)
		{
			gchar *server_ping = g_strdup_printf("%u", p_server->ping);
			gtk_tree_store_set(server_browser_tree_store, &iter, 1, server_ping, -1);
			g_free(server_ping);
		}

		if(p_server->players && p_server->max_players)
		{
			gchar *server_players = g_strdup_printf("%u/%u", p_server->players, p_server->max_players);
			gtk_tree_store_set(server_browser_tree_store, &iter, 2, server_players, -1);
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
#endif // HAVE_GTK
