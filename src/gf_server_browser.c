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

#include "gf_server_browser.h"
#include "gf_server_browser_proto.h"
#include "gf_games.h"
#include "gfire.h"

// Prototypes
static void gfire_server_browser_close(gfire_server_browser *p_browser, GtkWidget *p_sender);

// Creation & freeing
gfire_server_browser *gfire_server_browser_create(PurpleConnection *p_gc)
{
	if(!p_gc)
		return NULL;

	gfire_server_browser *ret = (gfire_server_browser*)g_malloc0(sizeof(gfire_server_browser));

	ret->gc = p_gc;
	g_datalist_init(&ret->favourites);
	g_datalist_init(&ret->recent);

	return ret;
}

void gfire_server_browser_free(gfire_server_browser *p_browser)
{
	if(!p_browser)
		return;

	gfire_server_browser_close(p_browser, NULL);

	g_datalist_clear(&p_browser->recent);
	g_datalist_clear(&p_browser->favourites);

	g_free(p_browser);
}

// GUI functions (displayal)
static void gfire_server_browser_add_parent_rows(gfire_server_browser *p_browser)
{
	// Add main (parent) rows
	gchar *label = g_strdup_printf(_("Recent servers (%u)"), 0);
	gtk_tree_store_append(p_browser->tree_store, &(p_browser->recent_iter), NULL);
	gtk_tree_store_set(p_browser->tree_store, &(p_browser->recent_iter), 0, label, 4, NULL, -1);
	g_free(label);

	label = g_strdup_printf(_("Favourite servers (%u)"), 0);
	gtk_tree_store_append(p_browser->tree_store, &(p_browser->fav_iter), NULL);
	gtk_tree_store_set(p_browser->tree_store, &(p_browser->fav_iter), 0, label, 4, NULL, -1);
	g_free(label);

	label = g_strdup_printf(_("Friends' favourite servers (%u)"), 0);
	gtk_tree_store_append(p_browser->tree_store, &(p_browser->ffav_iter), NULL);
	gtk_tree_store_set(p_browser->tree_store, &(p_browser->ffav_iter), 0, label, 4, NULL, -1);
	g_free(label);

	label = g_strdup_printf(_("All servers (%u)"), 0);
	gtk_tree_store_append(p_browser->tree_store, &(p_browser->serverlist_iter), NULL);
	gtk_tree_store_set(p_browser->tree_store, &(p_browser->serverlist_iter), 0, label, 4, NULL, -1);
	g_free(label);
}

static gboolean gfire_server_browser_free_cached_server(GtkTreeModel *p_model, GtkTreePath *p_path,
													GtkTreeIter *p_iter, gpointer p_data)
{
	gfire_game_server *server = NULL;
	gtk_tree_model_get(p_model, p_iter, 4, &server, -1);

	if(server)
		gfire_game_server_free(server);

	return TRUE;
}

static void gfire_server_browser_clear(gfire_server_browser *p_browser)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(p_browser->tree_store), gfire_server_browser_free_cached_server, p_browser);
	gtk_tree_store_clear(p_browser->tree_store);
}

static void gfire_server_browser_free_server_list(gpointer p_data)
{
	GSList *servers = (GSList*)p_data;
	while(servers)
	{
		gfire_game_server_free((gfire_game_server*)servers->data);
		servers = g_slist_delete_link(servers, servers);
	}
}

void gfire_server_browser_add_recent(gfire_server_browser *p_browser, guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	if(!p_browser)
		return;

	// Check whether we already got that server
	GSList *servers = (GSList*)g_datalist_id_get_data(&p_browser->recent, p_gameid);
	GSList *cur = servers;
	while(cur)
	{
		gfire_game_server *server = (gfire_game_server*)cur->data;
		if(server->ip == p_ip && server->port == p_port)
			return;
		cur = g_slist_next(cur);
	}

	// No, so add it
	gfire_game_server *server = g_malloc0(sizeof(gfire_game_server));
	server->ip = p_ip;
	server->port = p_port;

	g_datalist_id_remove_no_notify(&p_browser->recent, p_gameid);
	g_datalist_id_set_data_full(&p_browser->recent, p_gameid, g_slist_append(servers, server),
								gfire_server_browser_free_server_list);

	if(p_browser->query && p_browser->query_game == p_gameid)
		gfire_server_query_add_server(p_browser->query, p_ip, p_port, GUINT_TO_POINTER(GFSBT_RECENT));
}

gboolean gfire_server_browser_add_favourite(gfire_server_browser *p_browser, guint32 p_gameid,
											guint32 p_ip, guint16 p_port, gboolean p_remote)
{
	if(!p_browser || p_browser->num_fav >= p_browser->max_fav)
		return FALSE;

	// Check whether we already got that server
	GSList *servers = (GSList*)g_datalist_id_get_data(&p_browser->favourites, p_gameid);
	GSList *cur = servers;
	while(cur)
	{
		gfire_game_server *server = (gfire_game_server*)cur->data;
		if(server->ip == p_ip && server->port == p_port)
			return FALSE;
		cur = g_slist_next(cur);
	}

	// No, so add it
	gfire_game_server *server = g_malloc0(sizeof(gfire_game_server));
	server->ip = p_ip;
	server->port = p_port;

	g_datalist_id_remove_no_notify(&p_browser->favourites, p_gameid);
	g_datalist_id_set_data_full(&p_browser->favourites, p_gameid, g_slist_append(servers, server),
								gfire_server_browser_free_server_list);

	p_browser->num_fav++;

	if(p_remote)
	{
		guint16 len = gfire_server_browser_proto_create_add_fav_server(p_gameid, p_ip, p_port);
		if(len > 0) gfire_send(p_browser->gc, len);
	}

	return TRUE;
}

void gfire_server_browser_remove_favourite(gfire_server_browser *p_browser, guint32 p_gameid,
										   guint32 p_ip, guint16 p_port)
{
	if(!p_browser)
		return;

	GSList *servers = (GSList*)g_datalist_id_get_data(&p_browser->favourites, p_gameid);
	GSList *cur = servers;
	while(cur)
	{
		gfire_game_server *server = (gfire_game_server*)cur->data;
		if(server->ip == p_ip && server->port == p_port)
		{
			gfire_game_server_free(server);
			g_datalist_id_remove_no_notify(&p_browser->favourites, p_gameid);
			g_datalist_id_set_data_full(&p_browser->favourites, p_gameid, g_slist_delete_link(servers, cur),
										gfire_server_browser_free_server_list);

			p_browser->num_fav--;

			guint16 len = gfire_server_browser_proto_create_remove_fav_server(p_gameid, p_ip, p_port);
			if(len > 0) gfire_send(p_browser->gc, len);
			return;
		}
		cur = g_slist_next(cur);
	}
}

void gfire_server_browser_max_favs(gfire_server_browser *p_browser, guint p_max)
{
	if(p_browser) p_browser->max_fav = p_max;
}

void gfire_server_browser_add_server(gfire_server_browser *p_browser, gfire_server_browser_type p_type,
									 guint32 p_ip, guint16 p_port)
{
	if(!p_browser || !p_browser->query)
		return;

	gfire_server_query_add_server(p_browser->query, p_ip, p_port, GUINT_TO_POINTER(p_type));
}

static void gfire_server_browser_show_details_cb(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	// Get selected server
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(gtk_builder_get_object(p_browser->builder, "servers_tree_view")));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_server *server;
		gtk_tree_model_get(model, &iter, 4, &server, -1);

		if(server)
		{
			gchar *content = gfire_game_server_details(server);
			if(!content)
				content = g_strdup(_("No detailed information available!"));

			gchar *title = g_strdup_printf(_("Server Details for %u.%u.%u.%u:%u"),
										   (server->ip & 0xff000000) >> 24,
										   (server->ip & 0xff0000) >> 16,
										   (server->ip & 0xff00) >> 8,
										   server->ip & 0xff, server->port);
			purple_notify_formatted(p_browser->gc, _("Server Details"), title, NULL, content, NULL, NULL);
			g_free(content);
			g_free(title);
		}
	}
}

static void gfire_server_browser_add_fav_server_cb(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	if(p_browser->num_fav >= p_browser->max_fav)
	{
		purple_notify_error(NULL, _("Server Browser: Error"), _("Can't add favourite server"),
							_("You've reached the limit of favourite servers, you can however still remove favourite servers in order to add new ones."));

		return;
	}

	GtkComboBox *game_combo = GTK_COMBO_BOX(gtk_builder_get_object(p_browser->builder, "game_combo"));
	GtkDialog *add_favorite_dialog = GTK_DIALOG(gtk_builder_get_object(p_browser->builder, "add_favorite_dialog"));
	GtkEntry *ip_address_entry = GTK_ENTRY(gtk_builder_get_object(p_browser->builder, "ip_address_entry"));
	GtkEntry *port_entry = GTK_ENTRY(gtk_builder_get_object(p_browser->builder, "port_entry"));

	// Get the currently selected game id
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(game_combo, &iter);
	guint32 game_id;
	gtk_tree_model_get(gtk_combo_box_get_model(game_combo), &iter, 1, &game_id, -1);

	// Eventually get selected server
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(gtk_builder_get_object(p_browser->builder, "servers_tree_view")));
	GtkTreeModel *model;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_server *server;
		gtk_tree_model_get(model, &iter, 4, &server, -1);

		if(server)
		{
			gchar *ip_str = g_strdup_printf("%u.%u.%u.%u",
											(server->ip & 0xff000000) >> 24,
											(server->ip & 0xff0000) >> 16,
											(server->ip & 0xff00) >> 8,
											server->ip & 0xff);
			gchar *port_str = g_strdup_printf("%u", server->port);

			gtk_entry_set_text(ip_address_entry, ip_str);
			gtk_entry_set_text(port_entry, port_str);

			g_free(ip_str);
			g_free(port_str);
		}
	}

	while(TRUE)
	{
		if(gtk_dialog_run(add_favorite_dialog) == 0)
		{
			const gchar *ip_str = gtk_entry_get_text(ip_address_entry);
			const gchar *port_str = gtk_entry_get_text(port_entry);

			// Check given IP
			guint16 ip1, ip2, ip3, ip4;

			if(sscanf(ip_str, "%3hu.%3hu.%3hu.%3hu", &ip1, &ip2, &ip3, &ip4) == 4)
			{
				// Check every IP part
				if((ip1 < 256) && (ip2 < 256) && (ip3 < 256) && (ip4 < 256))
				{
					// Check port
					guint16 port;

					if(sscanf(port_str, "%hu", &port) == 1 && port > 0)
					{
						guint32 ip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;

						// Add fav server to user Xfire's profile
						if(!gfire_server_browser_add_favourite(p_browser, game_id, ip, port, TRUE))
						{
							purple_notify_error(NULL, _("Server Browser: Error"), _("Can't add favourite server"),
												_("This server is already one of your favourites!"));
							continue;
						}

						// Display the server if we are currently querying this game
						if(p_browser->query && p_browser->query_game == game_id)
							gfire_server_query_add_server(p_browser->query, ip, port, GUINT_TO_POINTER(GFSBT_FAVOURITE));

						break;
					}
					else
						purple_notify_error(NULL, _("Server Browser: Error"), _("Can't add favourite server"),
											_("The port you've entered is invalid!"));
				}
				else
					purple_notify_error(NULL, _("Server Browser: Error"), _("Can't add favourite server"),
										_("The IP address you've entered is invalid!"));
			}
			else
				purple_notify_error(NULL, _("Server Browser: Error"), _("Can't add favourite server"),
									_("The IP address you've entered is invalid!"));
		}
		else
			break;
	}

	gtk_widget_hide(GTK_WIDGET(add_favorite_dialog));
}

static void gfire_server_browser_remove_fav_server_cb(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	// Get selected server
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(gtk_builder_get_object(p_browser->builder, "servers_tree_view")));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_server *server = NULL;
		gtk_tree_model_get(model, &iter, 4, &server, -1);

		// Remove it
		if(server)
		{
			gfire_server_browser_remove_favourite(p_browser, p_browser->query_game, server->ip, server->port);

			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			gchar *label = g_strdup_printf(_("Favourite servers (%u)"),
										   gtk_tree_model_iter_n_children(model, &p_browser->fav_iter));
			gtk_tree_store_set(GTK_TREE_STORE(model), &p_browser->fav_iter, 0, label, -1);
			g_free(label);
		}
	}
}

static void gfire_server_browser_add_queried_server(gfire_game_server *p_server, gpointer p_server_data, gpointer p_data)
{
	gfire_server_browser *browser = (gfire_server_browser*)p_data;
	gfire_server_browser_type type = GPOINTER_TO_UINT(p_server_data);

	if(!p_server->data)
	{
		// Don't display servers which could not be queried and are no favourites
		if(type != GFSBT_FAVOURITE)
		{
			// Remove recent servers since they don't seem to exist
			if(type == GFSBT_RECENT)
			{
				GSList *servers = (GSList*)g_datalist_id_get_data(&browser->recent, browser->query_game);
				GSList *cur = servers;
				while(cur)
				{
					gfire_game_server *server = (gfire_game_server*)cur->data;
					if(server->ip == p_server->ip && server->port == p_server->port)
					{
						gfire_game_server_free(server);
						g_datalist_id_remove_no_notify(&browser->recent, browser->query_game);
						g_datalist_id_set_data_full(&browser->recent, browser->query_game,
													g_slist_delete_link(servers, cur),
													gfire_server_browser_free_server_list);
						break;
					}
					cur = g_slist_next(cur);
				}
			}
			gfire_game_server_free(p_server);
			return;
		}
	}

	gchar *label = NULL;
	GtkTreeIter iter;
	switch(type)
	{
	case GFSBT_RECENT:
		gtk_tree_store_append(browser->tree_store, &iter, &browser->recent_iter);
		label = g_strdup_printf(_("Recent servers (%u)"),
								gtk_tree_model_iter_n_children(GTK_TREE_MODEL(browser->tree_store),
															   &browser->recent_iter));
		gtk_tree_store_set(browser->tree_store, &browser->recent_iter, 0, label, -1);
		break;
	case GFSBT_FAVOURITE:
		gtk_tree_store_append(browser->tree_store, &iter, &browser->fav_iter);
		label = g_strdup_printf(_("Favourite servers (%u)"),
								gtk_tree_model_iter_n_children(GTK_TREE_MODEL(browser->tree_store),
															   &browser->fav_iter));
		gtk_tree_store_set(browser->tree_store, &browser->fav_iter, 0, label, -1);
		break;
	case GFSBT_FFAVOURITE:
		gtk_tree_store_append(browser->tree_store, &iter, &browser->ffav_iter);
		label = g_strdup_printf(_("Friends' favourite servers (%u)"),
								gtk_tree_model_iter_n_children(GTK_TREE_MODEL(browser->tree_store),
															   &browser->ffav_iter));
		gtk_tree_store_set(browser->tree_store, &browser->ffav_iter, 0, label, -1);
		break;
	case GFSBT_GENERAL:
		gtk_tree_store_append(browser->tree_store, &iter, &browser->serverlist_iter);
		label = g_strdup_printf(_("All servers (%u)"),
								gtk_tree_model_iter_n_children(GTK_TREE_MODEL(browser->tree_store),
															   &browser->serverlist_iter));
		gtk_tree_store_set(browser->tree_store, &browser->serverlist_iter, 0, label, -1);
		break;
	}
	g_free(label);

	// Format the output
	gchar *name = (p_server->data && p_server->data->name) ? g_strdup(p_server->data->name)
		: g_strdup_printf("%u.%u.%u.%u:%u",
						  (p_server->ip & 0xff000000) >> 24,
						  (p_server->ip & 0xff0000) >> 16,
						  (p_server->ip & 0xff00) >> 8,
						  p_server->ip & 0xff,
						  p_server->port);
	gchar *players = p_server->data ? g_strdup_printf("%u/%u", p_server->data->players, p_server->data->max_players)
		: g_strdup(_("N/A"));
	gchar *ping = p_server->data ? g_strdup_printf("%u", p_server->data->ping) : g_strdup(_("N/A"));
	gchar *map = (p_server->data && p_server->data->map) ? g_strdup(p_server->data->map) : g_strdup(_("N/A"));

	gtk_tree_store_set(browser->tree_store, &iter, 0, name, 1, ping, 2, players, 3, map, 4, p_server, -1);

	g_free(name);
	g_free(ping);
	g_free(players);
	g_free(map);
}

static void gfire_server_browser_request_serverlist_cb(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	GtkComboBox *game_combo = GTK_COMBO_BOX(gtk_builder_get_object(p_browser->builder, "game_combo"));

	gfire_server_browser_clear(p_browser);

	gfire_server_query_free(p_browser->query);
	p_browser->query = NULL;

	// Get the currently selected game id
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(game_combo, &iter);
	guint32 game_id;
	gtk_tree_model_get(gtk_combo_box_get_model(game_combo), &iter, 1, &game_id, -1);

	if(game_id != 0)
	{
		// Start the querying
		p_browser->query_game = game_id;
		p_browser->query = gfire_server_query_create();
		if(!gfire_server_query_start(p_browser->query, gfire_game_server_query_type(game_id), TRUE,
									 gfire_server_browser_add_queried_server, p_browser))
		{
			purple_notify_error(p_browser->gc, _("Game Server Type not supported"),
								_("Game Server Type not supported"),
								_("Sorry, but the server query protocol for this game is not yet supported by Gfire!"));
			gfire_server_query_free(p_browser->query);
			p_browser->query = NULL;
			return;
		}

		// Add parent rows (also clears tree store)
		gfire_server_browser_add_parent_rows(p_browser);

		// Request recent and favourite servers
		GSList *servers = g_datalist_id_get_data(&p_browser->recent, game_id);
		while(servers)
		{
			gfire_game_server *server = (gfire_game_server*)servers->data;
			gfire_server_query_add_server(p_browser->query, server->ip, server->port, GUINT_TO_POINTER(GFSBT_RECENT));
			servers = g_slist_next(servers);
		}

		servers = g_datalist_id_get_data(&p_browser->favourites, game_id);
		while(servers)
		{
			gfire_game_server *server = (gfire_game_server*)servers->data;
			gfire_server_query_add_server(p_browser->query, server->ip, server->port, GUINT_TO_POINTER(GFSBT_FAVOURITE));
			servers = g_slist_next(servers);
		}

		// Request serverlist from Xfire
		guint16 packet_len = gfire_server_browser_proto_create_serverlist_request(game_id);
		if(packet_len > 0)
			gfire_send(p_browser->gc, packet_len);

		// Request friends' fav serverlist from Xfire
		packet_len = gfire_server_browser_proto_create_friends_fav_serverlist_request(game_id);
		if(packet_len > 0)
			gfire_send(p_browser->gc, packet_len);
	}
}

static void gfire_server_browser_connect_cb(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(p_browser->builder, "servers_tree_view")));
	GtkTreeModel *model;
	GtkTreeIter iter;

	// Get selected server
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gfire_game_server *server = NULL;
		gtk_tree_model_get(model, &iter, 4, &server, -1);

		if(!server)
			return;

		// Put server in struct
		gfire_game_data game;

		game.id = p_browser->query_game;
		game.ip.value = server->ip;
		game.port = server->port;

		// Finally join server
		gfire_join_game(&game);
	}
	else
		purple_debug_error("gfire", "Server Browser: Couldn't get selected server to join.\n");
}

static void gfire_server_browser_close(gfire_server_browser *p_browser, GtkWidget *p_sender)
{
	if(p_browser->query)
	{
		gfire_server_query_free(p_browser->query);
		p_browser->query = NULL;
	}

	if(p_browser->builder)
	{
		// Free all cached servers
		gfire_server_browser_clear(p_browser);

		// Destroy window
		GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "server_browser_window"));
		gtk_widget_destroy(server_browser_window);

		// Free builder
		g_object_unref(G_OBJECT(p_browser->builder));
		p_browser->builder = NULL;
	}
}

static void gfire_server_browser_game_changed_cb(GtkComboBox *p_cbox, gpointer p_data)
{
	gfire_server_browser *browser = (gfire_server_browser*)p_data;

	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(browser->builder, "refresh_button"));
	GtkWidget *addfav_button = GTK_WIDGET(gtk_builder_get_object(browser->builder, "add_favorite_button"));
	gtk_widget_set_sensitive(refresh_button, gtk_combo_box_get_active(p_cbox) >= 0);
	gtk_widget_set_sensitive(addfav_button, gtk_combo_box_get_active(p_cbox) >= 0);

	if(gtk_combo_box_get_active(p_cbox) >= 0)
		gfire_server_browser_request_serverlist_cb(browser, NULL);
}

static void gfire_server_browser_selection_changed_cb(GtkTreeSelection *p_selection, gpointer p_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gfire_server_browser *browser = (gfire_server_browser*)p_data;

	GtkWidget *details_button = GTK_WIDGET(gtk_builder_get_object(browser->builder, "details_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(browser->builder, "connect_button"));
	GtkWidget *remfav_button = GTK_WIDGET(gtk_builder_get_object(browser->builder, "remove_favorite_button"));

	if(gtk_tree_selection_get_selected(p_selection, &model, &iter))
	{
		gfire_game_server *server = NULL;
		gtk_tree_model_get(model, &iter, 4, &server, -1);
		if(server)
		{
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			GtkTreePath *path_fav = gtk_tree_model_get_path(model, &browser->fav_iter);

			gtk_widget_set_sensitive(details_button, TRUE);
			gtk_widget_set_sensitive(connect_button, TRUE);

			if(gtk_tree_path_is_ancestor(path_fav, path))
				gtk_widget_set_sensitive(remfav_button, TRUE);
			else
				gtk_widget_set_sensitive(remfav_button, FALSE);

			gtk_tree_path_free(path);
			gtk_tree_path_free(path_fav);
			return;
		}
	}

	// By default: turn everything off
	gtk_widget_set_sensitive(details_button, FALSE);
	gtk_widget_set_sensitive(connect_button, FALSE);
	gtk_widget_set_sensitive(remfav_button, FALSE);
}

static gint gfire_server_browser_sort_name_column(GtkTreeModel *p_model, GtkTreeIter *p_a, GtkTreeIter *p_b,
												  gpointer p_data)
{
	gfire_game_server *server_a = NULL, *server_b = NULL;
	gtk_tree_model_get(p_model, p_a, 4, &server_a, -1);
	gtk_tree_model_get(p_model, p_b, 4, &server_b, -1);

	// Keep the sorting of the root elements constant
	if(!server_a || !server_b)
	{
		if(!server_a && !server_b)
			return 0;
		else
			return server_a ? -1 : 1;
	}

	// Sort the servers by name, but IP only servers last
	if(!server_a->data || !server_a->data->name || !server_b->data || !server_b->data->name)
	{
		if((!server_a->data || !server_a->data->name) && (!server_b->data || !server_b->data->name))
			return 0;
		else
			return (server_a->data && server_a->data->name) ? -1 : 1;
	}

	return g_utf8_collate(server_a->data->name, server_b->data->name);
}

static gint gfire_server_browser_sort_ping_column(GtkTreeModel *p_model, GtkTreeIter *p_a, GtkTreeIter *p_b,
												  gpointer p_data)
{
	gfire_game_server *server_a = NULL, *server_b = NULL;
	gtk_tree_model_get(p_model, p_a, 4, &server_a, -1);
	gtk_tree_model_get(p_model, p_b, 4, &server_b, -1);

	// Keep the sorting of the root elements constant
	if(!server_a || !server_a->data || !server_b || !server_b->data)
	{
		if((!server_a || !server_a->data) && (!server_b || !server_b->data))
			return 0;
		else
			return (server_a && server_a->data) ? -1 : 1;
	}

	// Sort the servers by ping
	if(server_a->data->ping > server_b->data->ping)
		return -1;
	else if(server_a->data->ping < server_b->data->ping)
		return 1;
	else
		return 0;
}

static gint gfire_server_browser_sort_player_column(GtkTreeModel *p_model, GtkTreeIter *p_a, GtkTreeIter *p_b,
												  gpointer p_data)
{
	gfire_game_server *server_a = NULL, *server_b = NULL;
	gtk_tree_model_get(p_model, p_a, 4, &server_a, -1);
	gtk_tree_model_get(p_model, p_b, 4, &server_b, -1);

	// Keep the sorting of the root elements constant
	if(!server_a || !server_a->data || !server_b || !server_b->data)
	{
		if((!server_a || !server_a->data) && (!server_b || !server_b->data))
			return 0;
		else
			return (server_a && server_a->data) ? -1 : 1;
	}

	// Sort the servers by players
	if(server_a->data->players > server_b->data->players)
		return -1;
	else if(server_a->data->players < server_b->data->players)
		return 1;
	else
		return 0;
}

static gint gfire_server_browser_sort_map_column(GtkTreeModel *p_model, GtkTreeIter *p_a, GtkTreeIter *p_b,
												  gpointer p_data)
{
	gfire_game_server *server_a = NULL, *server_b = NULL;
	gtk_tree_model_get(p_model, p_a, 4, &server_a, -1);
	gtk_tree_model_get(p_model, p_b, 4, &server_b, -1);

	// Keep the sorting of the root elements constant
	if(!server_a || !server_b)
	{
		if(!server_a && !server_b)
			return 0;
		else
			return server_a ? -1 : 1;
	}

	// Sort the servers by mapname
	if(!server_a->data || !server_a->data->map || !server_b->data || !server_b->data->map)
	{
		if((!server_a->data || !server_a->data->map) && (!server_b->data || !server_b->data->map))
			return 0;
		else
			return (server_a->data && server_a->data->map) ? -1 : 1;
	}

	return g_utf8_collate(server_a->data->map, server_b->data->map);
}

static gboolean gfire_server_browser_add_configured_games_cb(const gfire_game_configuration *p_gconf,
															 GtkComboBox *p_game_combo)
{
	// Check if the game has a protocol defined
	const gchar *protocol = gfire_game_server_query_type(p_gconf->game_id);
	if(protocol)
	{
		const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
		if(game)
		{
			GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(p_game_combo));
			GtkTreeIter iter;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, game->name, 1, game->id, -1);
		}
	}

	return FALSE;
}

void gfire_server_browser_show(gfire_server_browser *p_browser)
{
	if(!p_browser || p_browser->builder)
		return;

	if(!(p_browser->builder = gtk_builder_new()))
	{
		purple_debug_error("gfire", "Couldn't build server browser interface.\n");
		return;
	}

#ifdef ENABLE_NLS
	gtk_builder_set_translation_domain(p_browser->builder, GETTEXT_PACKAGE);
#endif // ENABLE_NLS

	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(p_browser->builder, builder_file, NULL);
	g_free(builder_file);

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "server_browser_window"));
	GtkWidget *tree_view = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "servers_tree_view"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "refresh_button"));
	GtkWidget *details_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "details_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "connect_button"));
	GtkWidget *add_fav_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "add_favorite_button"));
	GtkWidget *remove_fav_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "remove_favorite_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(p_browser->builder, "quit_button"));
	GtkComboBox *game_combo = GTK_COMBO_BOX(gtk_builder_get_object(p_browser->builder, "game_combo"));

	// Connect signals
	g_signal_connect_swapped(server_browser_window, "destroy", G_CALLBACK(gfire_server_browser_close), p_browser);
	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gfire_server_browser_close), p_browser);

	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_server_browser_request_serverlist_cb), p_browser);
	g_signal_connect_swapped(details_button, "clicked", G_CALLBACK(gfire_server_browser_show_details_cb), p_browser);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), p_browser);
	g_signal_connect_swapped(add_fav_button, "clicked", G_CALLBACK(gfire_server_browser_add_fav_server_cb), p_browser);
	g_signal_connect_swapped(remove_fav_button, "clicked", G_CALLBACK(gfire_server_browser_remove_fav_server_cb), p_browser);

	// Connect too when user double clicks on a server
	g_signal_connect_swapped(tree_view, "row-activated", G_CALLBACK(gfire_server_browser_connect_cb), p_browser);

	// Add configured games
	gfire_game_config_foreach(G_CALLBACK(gfire_server_browser_add_configured_games_cb), game_combo);
	g_signal_connect(game_combo, "changed", G_CALLBACK(gfire_server_browser_game_changed_cb), p_browser);

	// Initialize the tree view
	p_browser->tree_store = GTK_TREE_STORE(gtk_builder_get_object(p_browser->builder, "servers_list_tree_store"));
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(p_browser->tree_store), 0, gfire_server_browser_sort_name_column, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(p_browser->tree_store), 1, gfire_server_browser_sort_ping_column, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(p_browser->tree_store), 2, gfire_server_browser_sort_player_column, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(p_browser->tree_store), 3, gfire_server_browser_sort_map_column, NULL, NULL);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(p_browser->builder, "servers_tree_view")));
	g_signal_connect(selection, "changed", G_CALLBACK(gfire_server_browser_selection_changed_cb), p_browser);

	// Show window
	gtk_widget_show_all(server_browser_window);
}

static gboolean gfire_server_browser_delete_query(gpointer p_data)
{
	gfire_server_query_free((gfire_server_query*)p_data);
	return FALSE;
}

static void gfire_server_browser_single_queried(gfire_game_server *p_server, gpointer p_server_data, gpointer p_data)
{
	if(p_server->data)
	{
		gchar *content = gfire_game_server_details(p_server);
		if(!content)
			content = g_strdup(_("No detailed information available!"));

		gchar *title = g_strdup_printf(_("Server Details for %u.%u.%u.%u:%u"),
									   (p_server->ip & 0xff000000) >> 24,
									   (p_server->ip & 0xff0000) >> 16,
									   (p_server->ip & 0xff00) >> 8,
									   p_server->ip & 0xff, p_server->port);
		purple_notify_formatted(NULL, _("Server Details"), title, NULL, content, NULL, NULL);
		g_free(content);
		g_free(title);
	}
	else
		purple_notify_error(NULL, _("Server Query: Error"), _("Could not query the server"),
							_("Unfortunately Gfire wasn't able to query the server. Maybe another try shows a better result..."));

	// Deferred delete of the query manager
	g_idle_add(gfire_server_browser_delete_query, p_data);
}

gboolean gfire_server_browser_show_single(guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	if(!p_gameid || !p_ip)
		return FALSE;

	gfire_server_query *query = gfire_server_query_create();
	if(!gfire_server_query_start(query, gfire_game_server_query_type(p_gameid), TRUE,
								 gfire_server_browser_single_queried, query))
	{
		gfire_server_query_free(query);
		return FALSE;
	}

	gfire_server_query_add_server(query, p_ip, p_port, NULL);

	return TRUE;
}
