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

#include "gf_friend_search.h"

static GtkBuilder *gtk_builder = NULL;
static GtkWidget *search_dialog = NULL;

static void gfire_friend_search_search_cb(PurpleConnection *gc, GtkWidget *p_sender)
{
	gfire_data *gfire = (gfire_data*)gc->proto_data;
	if(!gc || !gfire || !gtk_builder)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_friend_search_search_cb: GC not set and/or couldn't access gfire data or invalid GtkBuilder\n");
		return;
	}

	GtkWidget *search_entry = GTK_WIDGET(gtk_builder_get_object(gtk_builder, "search_entry"));

	const gchar *search_str = gtk_entry_get_text(GTK_ENTRY(search_entry));

	if(strlen(search_str) == 0)
		return;

	int len = gfire_create_friend_search(gc, search_str);
	if(len > 0)
		gfire_send(gc, gfire->buff_out, len);

	gtk_widget_set_sensitive(p_sender, FALSE);

	return;
}

static void gfire_friend_search_selchange_cb(GtkTreeSelection *p_selection, void *p_unused)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;

	GtkWidget *add_button = GTK_WIDGET(gtk_builder_get_object(gtk_builder, "add_button"));
	if(!add_button)
		return;

	if(gtk_tree_selection_get_selected(p_selection, &model, &iter))
		gtk_widget_set_sensitive(add_button, TRUE);
	else
		gtk_widget_set_sensitive(add_button, FALSE);
}

static void gfire_friend_search_add_cb(PurpleConnection *gc, GtkWidget *p_sender)
{
	GtkTreeSelection *selection = NULL;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;

	GtkWidget *results_treeview = GTK_WIDGET(gtk_builder_get_object(gtk_builder, "results_treeview"));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(results_treeview));

	if(!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gchar *username = NULL;
	gtk_tree_model_get(model, &iter, 0, &username, -1);

	gtk_widget_destroy(search_dialog);

	purple_blist_request_add_buddy(purple_connection_get_account(gc), username, GFIRE_DEFAULT_GROUP_NAME, "");
	g_free(username);
}

void gfire_show_friend_search_cb(PurplePluginAction *p_action)
{
	if(search_dialog)
		return;

	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	gfire_data *gfire = NULL;

	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_show_friend_search_cb: GC not set and/or couldn't access gfire data.\n");
		return;
	}

	gtk_builder = gtk_builder_new();

	GError *builder_error = NULL;
	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "friend_search.glade", NULL);
	if(gtk_builder_add_from_file(gtk_builder, builder_file, &builder_error) == 0)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_show_friend_search_cb: Loading of \"%s\" failed: %s\n", builder_file, builder_error->message);
		g_error_free(builder_error);
		if(builder_file)
			g_free(builder_file);

		return;
	}

	if(builder_file)
		g_free(builder_file);

	search_dialog =					GTK_WIDGET(gtk_builder_get_object(gtk_builder, "friend_search_dialog"));
	GtkWidget *search_button =		GTK_WIDGET(gtk_builder_get_object(gtk_builder, "search_button"));
	GtkWidget *add_button =			GTK_WIDGET(gtk_builder_get_object(gtk_builder, "add_button"));
	GtkWidget *cancel_button =		GTK_WIDGET(gtk_builder_get_object(gtk_builder, "cancel_button"));
	GtkWidget *results_treeview =	GTK_WIDGET(gtk_builder_get_object(gtk_builder, "results_treeview"));

	g_signal_connect(search_dialog, "destroy", G_CALLBACK(gtk_widget_destroyed), &search_dialog);
	g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), search_dialog);
	g_signal_connect_swapped(search_button, "clicked", G_CALLBACK(gfire_friend_search_search_cb), gc);
	g_signal_connect_swapped(add_button, "clicked", G_CALLBACK(gfire_friend_search_add_cb), gc);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(results_treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(selection, "changed", G_CALLBACK(gfire_friend_search_selchange_cb), NULL);

	gtk_widget_show_all(search_dialog);
}

void gfire_friend_search_results(PurpleConnection *gc, GList *p_usernames, GList *p_firstnames, GList *p_lastnames)
{
	if(!gtk_builder)
		return;

	GtkTreeIter iter;

	GtkListStore *result_store = GTK_LIST_STORE(gtk_builder_get_object(gtk_builder, "search_result_list_store"));
	if(!result_store)
		return;

	gtk_list_store_clear(result_store);

	GtkWidget *users_label = GTK_WIDGET(gtk_builder_get_object(gtk_builder, "users_label"));
	gchar *label_txt = g_strdup_printf(N_("Found %u users"), g_list_length(p_usernames));
	gtk_label_set_text(GTK_LABEL(users_label), label_txt);
	g_free(label_txt);

	GList *cur_username = p_usernames;
	GList *cur_firstname = p_firstnames;
	GList *cur_lastname = p_lastnames;

	while(cur_username)
	{
		gtk_list_store_append(result_store, &iter);
		gtk_list_store_set(result_store, &iter, 0, cur_username->data, 1, cur_firstname->data, 2, cur_lastname->data, -1);

		g_free(cur_username->data);
		g_free(cur_firstname->data);
		g_free(cur_lastname->data);

		cur_username = g_list_next(cur_username);
		cur_firstname = g_list_next(cur_firstname);
		cur_lastname = g_list_next(cur_lastname);
	}

	g_list_free(p_usernames);
	g_list_free(p_firstnames);
	g_list_free(p_lastnames);

	GtkWidget *search_button = GTK_WIDGET(gtk_builder_get_object(gtk_builder, "search_button"));
	gtk_widget_set_sensitive(search_button, TRUE);
}
