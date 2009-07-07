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

#include "gf_server_browser_proto.h"

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);

	offset = gfire_proto_write_attr_bs(0x21, 0x02, &p_gameid, sizeof(p_gameid), offset);

	gfire_proto_write_header(offset, 0x16, 1, 0);

	return offset;
}

void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;
	GList *i, *p = NULL;

	if(!p_gfire->server_browser)
		return;

	if (p_packet_len < 16) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_server_browser_proto_serverlist: Packet 131 received, but too short (%d bytes).\n", p_packet_len);
		return;
	}

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &gameid, 0x21, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ips, 0x22, offset);
	if(offset == -1 || !ips)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ports, 0x23, offset);
	if(offset == -1 || !ports)
	{
		//mem cleanup code
		if (ips) g_list_free(ips);
		return;
	}

	i = ips; p = ports;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(serverlist): got the server list for %u\n", gameid);

	GtkBuilder *builder = p_gfire->server_browser;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "servers_list_store"));
	gtk_list_store_clear(list_store);

	GtkTreeIter iter;

	for(; i; i = g_list_next(i))
	{
		gfire_game_data ip_data;

		ip_data.ip.value = *((guint32*)i->data);
		ip_data.port = *((guint32*)p->data);

		gchar *addr = gfire_game_data_addr_str(&ip_data);

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, addr, 1, addr, 2, N_("N/A"), 3, N_("N/A"), -1);

		g_free(addr);

		g_free(i->data);
		g_free(p->data);

		p = g_list_next(p);
	}
	// Create a thread to update the server list
	g_thread_create((GThreadFunc)gfire_server_browser_update_server_list_thread, list_store, FALSE, NULL);
}
