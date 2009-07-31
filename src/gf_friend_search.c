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

#include "gf_friend_search.h"

static void gfire_friend_search_search_cb(PurpleConnection *p_gc, gchar *p_search_str)
{
	if(!p_gc || !p_search_str)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_friend_search_search_cb: Invalid GC or invalid search string\n");
		return;
	}

	if(strlen(p_search_str) == 0)
		return;

	guint16 len = gfire_friend_search_proto_create_request(p_search_str);
	if(len > 0)
		gfire_send(p_gc, len);

	return;
}

static void gfire_friend_search_add_cb(PurpleConnection *p_gc, GList *p_row, gpointer p_user_data)
{
	if(!p_gc || !p_row)
		return;

	purple_blist_request_add_buddy(purple_connection_get_account(p_gc), (gchar*)g_list_first(p_row)->data, GFIRE_DEFAULT_GROUP_NAME, "");
}

void gfire_show_friend_search_cb(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	gfire_data *gfire = NULL;

	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_show_friend_search_cb: GC not set and/or couldn't access gfire data.\n");
		return;
	}

	purple_request_input(gc, _("Xfire Friend Search"), _("Please enter a Xfire username, name or e-Mail address here:"),
						 _("For example: gill123, Gill Bates or gill@bates.net"), "", FALSE, FALSE,
						 NULL, _("Search"), G_CALLBACK(gfire_friend_search_search_cb), _("Cancel"), NULL, purple_connection_get_account(gc),
						 NULL, NULL, gc);
}

void gfire_friend_search_results(gfire_data *p_gfire, GList *p_usernames, GList *p_firstnames, GList *p_lastnames)
{
	PurpleNotifySearchResults *search_result = purple_notify_searchresults_new();
	if(!search_result)
	{
		gfire_list_clear(p_usernames);
		gfire_list_clear(p_firstnames);
		gfire_list_clear(p_lastnames);
		return;
	}

	purple_notify_searchresults_column_add(search_result, purple_notify_searchresults_column_new(_("Username")));
	purple_notify_searchresults_column_add(search_result, purple_notify_searchresults_column_new(_("First Name")));
	purple_notify_searchresults_column_add(search_result, purple_notify_searchresults_column_new(_("Last Name")));
	purple_notify_searchresults_button_add(search_result, PURPLE_NOTIFY_BUTTON_INVITE, gfire_friend_search_add_cb);

	GList *cur_username = p_usernames;
	GList *cur_firstname = p_firstnames;
	GList *cur_lastname = p_lastnames;

	while(cur_username)
	{
		GList *row = g_list_append(NULL, cur_username->data);
		row = g_list_append(row, cur_firstname->data);
		row = g_list_append(row, cur_lastname->data);
		purple_notify_searchresults_row_add(search_result, row);

		cur_username = g_list_next(cur_username);
		cur_firstname = g_list_next(cur_firstname);
		cur_lastname = g_list_next(cur_lastname);
	}

	g_list_free(p_usernames);
	g_list_free(p_firstnames);
	g_list_free(p_lastnames);

	purple_notify_searchresults(gfire_get_connection(p_gfire), _("Xfire Friend Search"), _("Search results"), "", search_result, NULL, NULL);
}
