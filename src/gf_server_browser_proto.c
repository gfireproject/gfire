/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
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

#include "gf_server_browser_proto.h"
#include "gf_server_browser.h"

// Serverlist requests
guint16 gfire_server_browser_proto_create_friends_fav_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);

	gfire_proto_write_header(offset, 0x15, 1, 0);
	return offset;
}

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	offset = gfire_proto_write_attr_bs(0x21, 0x02, &p_gameid, sizeof(p_gameid), offset);

	gfire_proto_write_header(offset, 0x16, 1, 0);
	return offset;
}

// Remote fav serverlist functions (requests)
guint16 gfire_server_browser_proto_create_add_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);

	p_ip = GUINT32_TO_LE(p_ip);
	offset = gfire_proto_write_attr_ss("gip", 0x02, &p_ip, sizeof(p_ip), offset);

	p_port = GUINT32_TO_LE(p_port);
	offset = gfire_proto_write_attr_ss("gport", 0x02, &p_port, sizeof(p_port), offset);

	gfire_proto_write_header(offset, 0x13, 3, 0);
	return offset;
}

guint16 gfire_server_browser_proto_create_remove_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);

	p_ip = GUINT32_TO_LE(p_ip);
	offset = gfire_proto_write_attr_ss("gip", 0x02, &p_ip, sizeof(p_ip), offset);

	p_port = GUINT32_TO_LE(p_port);
	offset = gfire_proto_write_attr_ss("gport", 0x02, &p_port, sizeof(p_port), offset);

	gfire_proto_write_header(offset, 0x14, 3, 0);
	return offset;
}

void gfire_server_browser_proto_friends_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	// FIXME: What's the correct length in bytes?
	if(p_packet_len < 16)
	{
		purple_debug_error("gfire", "Packet 149 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;
	GList *userids = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &gameid, "gameid", offset);

	if(gameid != p_gfire->server_browser->query_game)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ips, "gip", offset);
	if(offset == -1)
	{
		if(ips) gfire_list_clear(ips);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ports, "gport", offset);
	if(offset == -1)
	{
		if(ips) gfire_list_clear(ips);
		if(ports) gfire_list_clear(ports);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &userids, "friends", offset);
	if(offset == -1)
	{
		if(ips) gfire_list_clear(ips);
		if(ports) gfire_list_clear(ports);
		if(userids) gfire_list_clear(userids);

		return;
	}

	// FIXME: Not used yet, free...
	while(userids)
	{
		gfire_list_clear(userids->data);
		userids = g_list_delete_link(userids, userids);
	}

	GList *ip = ips, *port = ports;
	for(; ip; ip = g_list_next(ip))
	{
		gfire_server_browser_add_server(p_gfire->server_browser, GFSBT_FFAVOURITE,
										*((guint32*)ip->data), *((guint16*)port->data));

		// Free data and go to next server
		g_free(ip->data);
		g_free(port->data);

		port = g_list_next(port);
	}

	g_list_free(ips);
	g_list_free(ports);
}

void gfire_server_browser_proto_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	if(p_packet_len < 42)
	{
		purple_debug_error("gfire", "Packet 148 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 max_favs;
	GList *gameids = NULL;
	GList *ips = NULL;
	GList *ports = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &max_favs, "max", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &gameids, "gameid", offset);
	if(offset == -1)
	{
		if(gameids) gfire_list_clear(gameids);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ips, "gip", offset);
	if(offset == -1)
	{
		if(gameids) gfire_list_clear(gameids);
		if(ips) gfire_list_clear(ips);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ports, "gport", offset);
	if(offset == -1)
	{
		if(gameids) gfire_list_clear(gameids);
		if(ips) gfire_list_clear(ips);
		if(ports) gfire_list_clear(ports);
		return;
	}

	// Set max favs
	gfire_server_browser_max_favs(p_gfire->server_browser, max_favs);

	// Fill in favorite server list
	GList *id = gameids, *ip = ips, *port = ports;
	for(; id; id = g_list_next(id))
	{
		gfire_server_browser_add_favourite(p_gfire->server_browser, *((guint32*)id->data),
										   *((guint32*)ip->data), *((guint16*)port->data), FALSE);

		// Free data and go to next server
		g_free(id->data);
		g_free(ip->data);
		g_free(port->data);

		ip = g_list_next(ip);
		port = g_list_next(port);
	}

	g_list_free(gameids);
	g_list_free(ips);
	g_list_free(ports);
}

void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	if(p_packet_len < 16)
	{
		purple_debug_error("gfire", "Packet 150 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &gameid, 0x21, offset);
	if(offset == -1)
		return;

	if(gameid != p_gfire->server_browser->query_game)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ips, 0x22, offset);
	if(offset == -1)
	{
		if(ips) gfire_list_clear(ips);

		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ports, 0x23, offset);
	if(offset == -1)
	{
		if(ips) gfire_list_clear(ips);
		if(ports) gfire_list_clear(ports);

		return;
	}

	GList *ip = ips, *port = ports;
	for(; ip; ip = g_list_next(ip))
	{
		gfire_server_browser_add_server(p_gfire->server_browser, GFSBT_GENERAL, *((guint32*)ip->data),
										*((guint16*)port->data));

		// Free data and go to next server
		g_free(ip->data);
		g_free(port->data);

		port = g_list_next(port);
	}

	g_list_free(ips);
	g_list_free(ports);
}
