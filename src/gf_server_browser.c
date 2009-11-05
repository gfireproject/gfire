/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
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

#include "gf_query.h"
#include "gf_server_browser.h"

#ifdef HAVE_GTK

static void gfire_server_browser_connect_cb(server_browser_callback_args *p_args, GtkWidget *p_sender)
{
	gfire_data *gfire = p_args->gfire;
	GtkBuilder *builder = p_args->builder;

	if (!gfire || !builder) {
		purple_debug_error("gfire_server_browser_connect_cb", "Couldn't access gfire data and/or couldn't get interface.\n");
		return;
	}

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(builder, "servers_tree_view"));

	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_data game;
		gchar *server;
		gchar **server_tok;

		gtk_tree_model_get(model, &iter, 1, &server, -1);
		server_tok = g_strsplit(server, ":", -1);
		g_free(server);
		if(!server_tok)
			return;

		game.id = gfire_game_id(game_name);
		gfire_game_data_ip_from_str(&game, server_tok[0]);
		game.port = atoi(server_tok[1]);

		g_strfreev(server_tok);

		gfire_join_game(gfire, &game);
	}
	else purple_debug_error("gfire_server_browser_connect_cb", "Couldn't get selected server.\n");
}

static void gfire_server_browser_request_list_cb(server_browser_callback_args *p_args, GtkWidget *p_button)
{
	if(!p_args)
		return;

	gfire_data *gfire = p_args->gfire;
	GtkBuilder *builder = p_args->builder;

	if (!gfire || !builder) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_server_browser_request_list_cb: gfire not set and/or couldn't get interface.\n");
		return;
	}

	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));

	gchar *game_name;
	guint32 game_id;

	game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	game_id = gfire_game_id(game_name);
	g_free(game_name);

	if(game_id != 0)
	{
		guint16 packet_len = 0;

		gfire->server_browser = builder;
		packet_len = gfire_server_browser_proto_create_serverlist_request(game_id);
		if(packet_len > 0) gfire_send(gfire_get_connection(gfire), packet_len);
	}
}

void *gfire_server_browser_update_server_list_thread(GtkListStore *p_server_list_store)
{
	GtkTreeIter iter;
	gboolean valid_iter;

	// ipdata_t server_data;
	gchar **server_tok;
	int query_result;

	for (valid_iter = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(p_server_list_store), &iter); valid_iter == TRUE;
		 valid_iter = gtk_tree_model_iter_next(GTK_TREE_MODEL(p_server_list_store), &iter))
	{
		/*
		gchar *server_ip;

		gtk_tree_model_get(GTK_TREE_MODEL(p_server_list_store), &iter, 0, &server_ip, -1);
		server_tok = g_strsplit(server_ip, ":", -1);
		g_free(server_ip);

		if(!server_tok)
			continue;

		query_result = miniquery(&server_data, 1, inet_addr(server_tok[0]), atoi(server_tok[1]));

		if(query_result != 0)
		{
			gchar *server_name, *server_ping, *server_players;

			server_name = g_strdup((gchar*)server_data.name);
			server_ping = g_strdup_printf("%u", server_data.ping);
			server_players = g_strdup_printf("%u", server_data.players);

			if (server_name != NULL && server_ping != NULL && server_players != NULL)
			{
				gchar *server_name_tmp = gfire_escape_color_codes(server_name);
				g_free(server_name);
				if (server_name_tmp != NULL)
				{
					gtk_list_store_set(p_server_list_store, &iter, 0, server_name_tmp, 2, server_ping,
							   3, server_players, -1);

					g_free(server_name_tmp);
				}

				g_free(server_ping);
				g_free(server_players);
			}
		}

		ipdata_free(&server_data);
		g_strfreev(server_tok);
		*/
	}

	return NULL;
}

void gfire_server_browser_show(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "GC not set and/or couldn't access gfire data.\n");
		return;
	}

	GtkBuilder *builder = gtk_builder_new();
	gchar *builder_file;

	gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

	builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(builder, builder_file, NULL);
	g_free(builder_file);

	if (builder == NULL) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_server_browser_show: Couldn't build interface.\n");
		return;
	}

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(builder, "server_browser_window"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(builder, "refresh_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(builder, "quit_button"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));

	server_browser_callback_args *args;

	args = g_malloc0(sizeof(server_browser_callback_args));
	args->gfire = gfire;
	args->builder = builder;

	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gtk_widget_destroy), server_browser_window);
	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_server_browser_request_list_cb), args);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), args);

	xmlnode *node_child = gfire_game_config_node_first();
	for(; node_child; node_child = gfire_game_config_node_next(node_child))
	{
		const gchar *game_name = xmlnode_get_attrib(node_child, "name");
		gtk_combo_box_append_text(GTK_COMBO_BOX(game_combo), game_name);
	}

	gtk_widget_show_all(server_browser_window);
}

#endif // HAVE_GTK
