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

#include "gf_protocol.h"
#include "gf_network.h"
#include "gf_buddies.h"
#include "gf_buddies_proto.h"

static GList *gfire_fof_data = NULL;

guint16 gfire_buddy_proto_create_advanced_info_request(guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '01'
	offset = gfire_proto_write_attr_bs(0x01, 0x02, &p_userid, sizeof(p_userid), offset);

	gfire_proto_write_header(offset, 0x25, 1, 0);
	return offset;
}

guint16 gfire_buddy_proto_create_typing_notification(const guint8 *p_sid, guint32 p_imindex, gboolean p_typing)
{
	if(!p_sid)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "sid"
	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);

	// "peermsg"
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 3, offset);

	// "peermsg"->"msgtype"
	guint32 msgtype = GUINT32_TO_LE(3);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);

	// "peermsg"->"imindex"
	p_imindex = GUINT32_TO_LE(p_imindex);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);

	// "peermsg"->"typing"
	guint32 typing = GUINT32_TO_LE(p_typing ? 1 : 0);
	offset = gfire_proto_write_attr_ss("typing", 0x02, &typing, sizeof(typing), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);
	return offset;
}

guint16 gfire_buddy_proto_create_send_im(const guint8 *p_sid, guint32 p_imindex, const gchar *p_msg)
{
	if(!p_sid || !p_msg)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32	msgtype = GUINT32_TO_LE(0);
	p_imindex = GUINT32_TO_LE(p_imindex);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 3, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);
	offset = gfire_proto_write_attr_ss("im", 0x01, p_msg, strlen(p_msg), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	return offset;
}

guint16 gfire_buddy_proto_create_fof_request(GList *p_sids)
{
	if(!p_sids)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_write_attr_list_ss("sid", p_sids, 0x03, XFIRE_SID_LEN, offset);

	gfire_proto_write_header(offset, 0x05, 1, 0);

	return offset;
}

guint16 gfire_buddy_proto_create_ack(const guint8 *p_sid, guint32 p_imindex)
{
	if(!p_sid)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32	msgtype = GUINT32_TO_LE(1);
	p_imindex = GUINT32_TO_LE(p_imindex);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 2, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	return offset;
}

guint16 gfire_buddy_proto_create_p2p(const guint8 *p_sid, guint32 p_ip, guint16 p_port, guint32 p_local_ip, guint16 p_local_port, guint32 p_nat_type, const gchar *p_salt)
{
	if(!p_sid || !p_salt)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32	msgtype = GUINT32_TO_LE(2);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 7, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);

	p_ip = GUINT32_TO_LE(p_ip);
	offset = gfire_proto_write_attr_ss("ip", 0x02, &p_ip, sizeof(p_ip), offset);

	guint32 port = GUINT32_TO_LE(p_port);
	offset = gfire_proto_write_attr_ss("port", 0x02, &port, sizeof(port), offset);

	p_local_ip = GUINT32_TO_LE(p_local_ip);
	offset = gfire_proto_write_attr_ss("localip", 0x02, &p_local_ip, sizeof(p_local_ip), offset);

	guint32 local_port = GUINT32_TO_LE(p_local_port);
	offset = gfire_proto_write_attr_ss("localport", 0x02, &local_port, sizeof(local_port), offset);

	p_nat_type = GUINT32_TO_LE(p_nat_type);
	offset = gfire_proto_write_attr_ss("status", 0x02, &p_nat_type, sizeof(p_nat_type), offset);

	offset = gfire_proto_write_attr_ss("salt", 0x01, p_salt, strlen(p_salt), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	return offset;
}

void gfire_buddy_proto_on_off(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset = 0;
	GList *userids = NULL;
	GList *sids = NULL;
	GList *u,*s;
	gfire_buddy *gf_buddy = NULL;

	if (p_packet_len < 16) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_on_off: buddies online status received but way too short?!? %d bytes\n",
			p_packet_len);
		return;
	}

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &userids, 0x01, offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !userids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &sids, 0x03, offset);
	if(offset == -1 || !sids)
	{
		gfire_list_clear(userids);
		return;
	}

	u = userids; s = sids;

	for(; u; u = g_list_next(u))
	{
		gf_buddy = gfire_find_buddy(p_gfire, u->data, GFFB_USERID);
		if (!gf_buddy)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_on_off: invalid user ID from Xfire\n");

			g_free(u->data);
			g_free(s->data);

			s = g_list_next(s);
			continue;
		}

		gfire_buddy_set_session_id(gf_buddy, (guint8*)s->data);

		purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s is now %s\n", gfire_buddy_get_name(gf_buddy),
					gfire_buddy_is_online(gf_buddy) ? "online" : "offline");

		g_free(u->data);
		g_free(s->data);

		s = g_list_next(s);
	}

	g_list_free(userids);
	g_list_free(sids);
}

void gfire_buddy_proto_game_status(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	gfire_buddy *gf_buddy = NULL;
	GList *sids = NULL;
	GList *gameids = NULL;
	GList *gameips = NULL;
	GList *gameports = NULL;
	GList *s, *g, *ip, *gp;

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &sids, "sid", offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !sids)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &gameids, "gameid", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &gameips, "gip", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		gfire_list_clear(gameids);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &gameports, "gport", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		gfire_list_clear(gameids);
		gfire_list_clear(gameips);
		return;
	}

	g = gameids; s = sids; ip = gameips; gp = gameports;

	GList *fof_sids = NULL;
	for(; s; s = g_list_next(s))
	{
		gf_buddy = gfire_find_buddy(p_gfire, s->data, GFFB_SID);
		// Needs to be an FoF buddy
		if(!gf_buddy)
		{
			if(gfire_wants_fofs(p_gfire))
			{
				fof_sids = g_list_append(fof_sids, s->data);
				gfire_fof_data = g_list_append(gfire_fof_data, gfire_fof_game_data_create(s->data, *(guint32*)g->data, *(guint32*)ip->data, *(guint32*)gp->data & 0xFFFF));
			}

			g_free(g->data);
			g_free(gp->data);
			g_free(ip->data);

			g = g_list_next(g);
			ip = g_list_next(ip);
			gp = g_list_next(gp);

			continue;
		}

		gfire_buddy_set_game_status(gf_buddy, *(guint32*)g->data, *(guint32*)gp->data & 0xFFFF, *(guint32*)ip->data);

		// Remove FoF as soon as he stops playing
		if(gfire_buddy_is_friend_of_friend(gf_buddy) && !gfire_buddy_is_playing(gf_buddy))
			gfire_remove_buddy(p_gfire, gf_buddy, FALSE, TRUE);

		g_free(s->data);
		g_free(g->data);
		g_free(gp->data);
		g_free(ip->data);

		g = g_list_next(g);
		ip = g_list_next(ip);
		gp = g_list_next(gp);
	}

	g_list_free(gameids);
	g_list_free(gameports);
	g_list_free(sids);
	g_list_free(gameips);

	if(g_list_length(fof_sids) > 0)
	{
		purple_debug_misc("gfire", "requesting FoF network info for %u users\n", g_list_length(fof_sids));
		guint16 len = gfire_buddy_proto_create_fof_request(fof_sids);
		if(len > 0) gfire_send(gfire_get_connection(p_gfire), len);
	}

	gfire_list_clear(fof_sids);
}

void gfire_buddy_proto_voip_status(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	gfire_buddy *gf_buddy = NULL;
	GList *sids = NULL;
	GList *voipids = NULL;
	GList *voipips = NULL;
	GList *voipports = NULL;
	GList *s, *v, *ip, *vp;

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &sids, "sid", offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !sids)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &voipids, "vid", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &voipips, "vip", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		gfire_list_clear(voipids);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &voipports, "vport", offset);
	// Parsing error -> skip further parsing
	if (offset == -1)
	{
		gfire_list_clear(sids);
		gfire_list_clear(voipids);
		gfire_list_clear(voipips);
		return;
	}

	v = voipids; s = sids; ip = voipips; vp = voipports;

	for(; s; s = g_list_next(s))
	{
		gf_buddy = gfire_find_buddy(p_gfire, s->data, GFFB_SID);
		if(!gf_buddy)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_voip_status: unkown session ID from Xfire\n");

			g_free(s->data);
			g_free(v->data);
			g_free(vp->data);
			g_free(ip->data);

			v = g_list_next(v);
			ip = g_list_next(ip);
			vp = g_list_next(vp);

			continue;
		}

		gfire_buddy_set_voip_status(gf_buddy, *(guint32*)v->data, *(guint32*)vp->data & 0xFFFF, *(guint32*)ip->data);

		g_free(s->data);
		g_free(v->data);
		g_free(vp->data);
		g_free(ip->data);

		v = g_list_next(v);
		ip = g_list_next(ip);
		vp = g_list_next(vp);
	}

	g_list_free(voipids);
	g_list_free(voipports);
	g_list_free(sids);
	g_list_free(voipips);
}

void gfire_buddy_proto_status_msg(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	gfire_buddy *gf_buddy = NULL;
	GList *sids = NULL;
	GList *msgs = NULL;
	GList *s, *m;

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &sids, "sid", offset);
	if(offset == -1)
	{
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &msgs, "msg", offset);
	if(offset == -1)
	{
		gfire_list_clear(sids);
		return;
	}

	m = msgs; s = sids;

	for(; s; s = g_list_next(s))
	{
		gf_buddy = gfire_find_buddy(p_gfire, s->data, GFFB_SID);
		if(!gf_buddy)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_status_msg: unkown session ID from Xfire\n");
			g_free(s->data);
			g_free(m->data);

			m = g_list_next(m);
			continue;
		}

		gfire_buddy_set_status(gf_buddy, (gchar*)m->data);

		purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s's status set to \"%s\"\n",
					 gfire_buddy_get_name(gf_buddy), (gchar*)m->data);

		g_free(s->data);
		g_free(m->data);

		m = g_list_next(m);
	}

	g_list_free(msgs);
	g_list_free(sids);
}

void gfire_buddy_proto_alias_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	guint32 uid = 0;
	gchar *nick = NULL;
	gfire_buddy *gf_buddy = NULL;

	offset = XFIRE_HEADER_LEN;

	// grab the userid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &uid, 0x01, offset);
	if(offset == -1)
		return;

	// grab the new nick
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &nick, 0x0D, offset);
	if(offset == -1 || !nick)
		return;

	gf_buddy = gfire_find_buddy(p_gfire, (gpointer) &uid, GFFB_USERID);
	if (!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_alias_change: unknown user ID from Xfire\n");
		g_free(nick);
		return;
	}

	purple_debug(PURPLE_DEBUG_INFO, "gfire", "User %s changed nick from \"%s\" to \"%s\"\n",
			gfire_buddy_get_name(gf_buddy), gfire_buddy_get_alias(gf_buddy), NN(nick));

	gfire_buddy_set_alias(gf_buddy, nick);

	g_free(nick);
}

void gfire_buddy_proto_changed_avatar(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	guint32 uid;
	guint32 avatarType = 0x0;
	guint32 avatarNum = 0x0;
	gfire_buddy *gf_buddy = NULL;

	offset = XFIRE_HEADER_LEN;

	// grab the userid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &uid, 0x01, offset);
	if(offset == -1)
		return;

	// grab the avatarType
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &avatarType, 0x34, offset);
	if(offset == -1)
		return;

	// grab the avatarNum
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &avatarNum, 0x1F, offset);
	if(offset == -1)
		return;

	gf_buddy = gfire_find_buddy(p_gfire, (gpointer) &uid, GFFB_USERID);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_changed_avatar: unknown user ID from Xfire\n");
		return;
	}

	if(gfire_buddy_is_friend(gf_buddy) || gfire_buddy_is_clan_member(gf_buddy))
		gfire_buddy_download_avatar(gf_buddy, avatarType, avatarNum);
}

void gfire_buddy_proto_clans(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	guint32 uid;
	GList *clanids = NULL;
	GList *clanShortNames = NULL;
	GList *clanLongNames = NULL;
	GList *clanNicks = NULL;
	GList *ci, *csn, *cln, *cn;
	gfire_buddy *gf_buddy = NULL;
	gfire_clan *gf_clan = NULL;

	offset = XFIRE_HEADER_LEN;

	// grab the userid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &uid, 0x01, offset);
	if(offset == -1)
		return;

	// grab the clanids
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanids, 0x6C, offset);
	if(offset == -1 || !clanids)
		return;

	// grab the clanShortNames
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanShortNames, 0x72, offset);
	if(offset == -1 || !clanShortNames)
	{
		gfire_list_clear(clanids);
		return;
	}

	// grab the clanLongNames
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanLongNames, 0x81, offset);
	if(offset == -1 || !clanLongNames)
	{
		gfire_list_clear(clanids);
		gfire_list_clear(clanShortNames);
		return;
	}

	// grab the clanNicks
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanNicks, 0x6D, offset);
	if(offset == -1 || !clanNicks)
	{
		gfire_list_clear(clanids);
		gfire_list_clear(clanShortNames);
		gfire_list_clear(clanLongNames);
		return;
	}

	gf_buddy = gfire_find_buddy(p_gfire, (gpointer) &uid, GFFB_USERID);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_clans: unknown user ID from Xfire\n");
		gfire_list_clear(clanids);
		gfire_list_clear(clanShortNames);
		gfire_list_clear(clanLongNames);
		gfire_list_clear(clanNicks);
		return;
	}

	ci = clanids;
	csn = clanShortNames;
	cln = clanLongNames;
	cn = clanNicks;

	while(ci)
	{
		gf_clan = gfire_find_clan(p_gfire, *(guint32*)ci->data);
		if(!gf_clan)
		{
			gf_clan = gfire_clan_create(*(guint32*)ci->data, (gchar*)cln->data, (gchar*)csn->data, FALSE);
			if(gf_clan)
				gfire_add_clan(p_gfire, gf_clan);
		}

		g_free(ci->data);
		g_free(csn->data);
		g_free(cln->data);

		ci = g_list_next(ci);
		csn = g_list_next(csn);
		cln = g_list_next(cln);

		if(!gf_clan)
		{
			g_free(cn->data);
			cn = g_list_next(cn);

			continue;
		}

		gfire_buddy_add_to_clan(gf_buddy, gf_clan, (gchar*)cn->data, FALSE);
		g_free(cn->data);
		cn = g_list_next(cn);
	}

	g_list_free(clanids);
	g_list_free(clanShortNames);
	g_list_free(clanLongNames);
	g_list_free(clanNicks);
}

void gfire_buddy_proto_im(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint8 *sid, peermsg;
	guint32 msgtype,imindex = 0;
	gchar *im = NULL;
	gfire_buddy *gf_buddy = NULL;
	guint32 typing = 0;

	if(p_packet_len < 16)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 133 recv'd but way too short?!? %d bytes\n",
			p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	// get session id ("sid")
	offset = gfire_proto_read_attr_sid_ss(p_gfire->buff_in, &sid, "sid", offset);
	if(offset == -1 || !sid)
		return;

	gf_buddy = gfire_find_buddy(p_gfire, (gpointer)sid, GFFB_SID);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_im: Unknown session ID for IM packet.\n");
		g_free(sid);
		return;
	}

	g_free(sid);

	// get peermsg parent attribute ("peermsg")
	offset = gfire_proto_read_attr_children_count_ss(p_gfire->buff_in, &peermsg, "peermsg", offset);
	if(offset == -1)
		return;

	// Get message type ("msgtype")
	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &msgtype, "msgtype", offset);
	if(offset == -1)
		return;

	switch(msgtype)
	{
		// Instant message
		case 0:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			// the IM itself ("im")
			offset = gfire_proto_read_attr_string_ss(p_gfire->buff_in, &im, "im", offset);
			if(offset == -1 || !im)
				return;

			gfire_buddy_got_im(gf_buddy, imindex, im, FALSE);
		break;
		// ACK packet
		case 1:
			// got an ack packet from a previous IM sent
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM ack packet received.\n");

			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			gfire_buddy_got_im_ack(gf_buddy, imindex);
		break;
		// P2P Info
		case 2:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "Got P2P info.\n");

			guint32 ip, localip;
			guint32 port, localport;
			guint32 status;
			gchar *salt = NULL;

			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &ip, "ip", offset);
			if(offset == -1)
				return;

			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &port, "port", offset);
			if(offset == -1)
				return;

			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &localip, "localip", offset);
			if(offset == -1)
				return;

			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &localport, "localport", offset);
			if(offset == -1)
				return;

			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &status, "status", offset);
			if(offset == -1)
				return;

			offset = gfire_proto_read_attr_string_ss(p_gfire->buff_in, &salt, "salt", offset);
			if(offset == -1)
				return;

			gfire_buddy_got_p2p_data(gf_buddy, GUINT32_FROM_LE(ip), (guint16)GUINT32_FROM_LE(port), GUINT32_FROM_LE(localip), (guint16)GUINT32_FROM_LE(localport), status, salt);

			g_free(salt);
		break;
		// Typing notification
		case 3:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			// typing ("typing")
			offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &typing, "typing", offset);
			if(offset == -1)
				return;

			gfire_buddy_got_typing(gf_buddy, typing == 1);
		break;
		// Unknown
		default:
			purple_debug(PURPLE_DEBUG_INFO, "gfire", "unknown IM msgtype %u.\n", msgtype);
	}
}

void gfire_buddy_proto_fof_list(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	gfire_buddy *gf_buddy = NULL;
	GList *fofsid = NULL;
	GList *fofs = NULL;
	GList *names = NULL;
	GList *nicks = NULL;
	GList *common = NULL;
	GList *f, *na, *n, *s, *c;

	offset = XFIRE_HEADER_LEN;

	if(!gfire_wants_fofs(p_gfire))
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &fofsid, "fnsid", offset);
	if(offset == -1 || !fofsid)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &fofs, "userid", offset);
	// Parsing error or empty list -> skip further handling
	if(offset == -1 || !fofs)
	{
		gfire_list_clear(fofsid);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &names, "name", offset);
	// Parsing error -> free other lists and skip further handling
	if(offset == -1 || !names)
	{
		gfire_list_clear(fofsid);
		gfire_list_clear(fofs);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &nicks, "nick", offset);
	// Parsing error -> free other lists and skip further handling
	if(offset == -1 || !nicks)
	{
		gfire_list_clear(fofsid);
		gfire_list_clear(fofs);
		gfire_list_clear(names);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &common, "friends", offset);
	// Parsing error -> free other lists and skip further handling
	if(offset == -1 || !common)
	{
		gfire_list_clear(fofsid);
		gfire_list_clear(fofs);
		gfire_list_clear(names);
		gfire_list_clear(nicks);
		return;
	}

	s = fofsid;
	f = fofs;
	na = names;
	n = nicks;
	c = common;

	for(; s; s = g_list_next(s))
	{
		// Invalid FoF
		if(*(guint32*)f->data == 0)
		{
			g_free(s->data);
			g_free(f->data);
			g_free(na->data);
			g_free(n->data);
			gfire_list_clear(c->data);

			f = g_list_next(f);
			na = g_list_next(na);
			n = g_list_next(n);
			c = g_list_next(c);

			continue;
		}

		if(!gfire_is_self(p_gfire, *(guint32*)f->data))
		{
			gf_buddy = gfire_find_buddy(p_gfire, na->data, GFFB_NAME);
			if(!gf_buddy)
			{
				gf_buddy = gfire_buddy_create(*(guint32*)f->data, (gchar*)na->data, (gchar*)n->data, GFBT_FRIEND_OF_FRIEND);
				if(gf_buddy)
				{
					gfire_add_buddy(p_gfire, gf_buddy, NULL);
					gfire_buddy_set_session_id(gf_buddy, (guint8*)s->data);

					GList *common_names = NULL;
					GList *cur = c->data;
					for(; cur; cur = g_list_next(cur))
					{
						gfire_buddy *common_buddy = gfire_find_buddy(p_gfire, cur->data, GFFB_USERID);
						if(common_buddy)
							common_names = g_list_append(common_names, g_strdup(gfire_buddy_get_name(common_buddy)));

						g_free(cur->data);
					}

					gfire_buddy_set_common_buddies(gf_buddy, common_names);

					cur = gfire_fof_data;
					for(; cur; cur = g_list_next(cur))
					{
						if(memcmp(((fof_game_data*)cur->data)->sid, s->data, XFIRE_SID_LEN) == 0)
						{
							gfire_buddy_set_game_status(gf_buddy, ((fof_game_data*)cur->data)->game.id, ((fof_game_data*)cur->data)->game.port, ((fof_game_data*)cur->data)->game.ip.value);

							if(((fof_game_data*)cur->data)->gcd)
							{
								gfire_buddy_set_game_client_data(gf_buddy, ((fof_game_data*)cur->data)->gcd);
								((fof_game_data*)cur->data)->gcd = NULL;
							}

							gfire_fof_game_data_free((fof_game_data*)cur->data);
							gfire_fof_data = g_list_delete_link(gfire_fof_data, cur);

							break;
						}
					}
				}
				else
				{
					GList *cur = gfire_fof_data;
					for(; cur; cur = g_list_next(cur))
					{
						if(memcmp(((fof_game_data*)cur->data)->sid, s->data, XFIRE_SID_LEN) == 0)
						{
							gfire_fof_game_data_free((fof_game_data*)cur->data);
							gfire_fof_data = g_list_delete_link(gfire_fof_data, cur);

							break;
						}
					}
				}
			}
			else
			{
				GList *cur = gfire_fof_data;
				for(; cur; cur = g_list_next(cur))
				{
					if(memcmp(((fof_game_data*)cur->data)->sid, s->data, XFIRE_SID_LEN) == 0)
					{
						gfire_fof_game_data_free((fof_game_data*)cur->data);
						gfire_fof_data = g_list_delete_link(gfire_fof_data, cur);

						break;
					}
				}
			}
		}
		else
		{
			GList *cur = gfire_fof_data;
			for(; cur; cur = g_list_next(cur))
			{
				if(memcmp(((fof_game_data*)cur->data)->sid, s->data, XFIRE_SID_LEN) == 0)
				{
					gfire_fof_game_data_free((fof_game_data*)cur->data);
					gfire_fof_data = g_list_delete_link(gfire_fof_data, cur);

					break;
				}
			}
		}

		g_free(s->data);
		g_free(f->data);
		g_free(na->data);
		g_free(n->data);
		g_list_free(c->data);

		f = g_list_next(f);
		na = g_list_next(na);
		n = g_list_next(n);
		c = g_list_next(c);
	}

	g_list_free(fofsid);
	g_list_free(fofs);
	g_list_free(nicks);
	g_list_free(names);
	g_list_free(common);
}

void gfire_buddy_proto_clan_alias_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	guint32 clanid = 0;
	guint32 uid = 0;
	gchar *nick = NULL;
	gfire_buddy *gf_buddy = NULL;

	offset = XFIRE_HEADER_LEN;

	// grab the clanid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &clanid, 0x6C, offset);
	if(offset == -1)
		return;

	// grab the userid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &uid, 0x01, offset);
	if(offset == -1)
		return;

	// grab the new nick
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &nick, 0x0D, offset);
	if(offset == -1 || !nick)
		return;

	gf_buddy = gfire_find_buddy(p_gfire, (gpointer) &uid, GFFB_USERID);
	if (!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_proto_clan_alias_change: unknown user ID from Xfire\n");
		g_free(nick);
		return;
	}

	gfire_clan *clan = gfire_find_clan(p_gfire, clanid);
	if(clan)
		purple_debug(PURPLE_DEBUG_INFO, "gfire", "User %s changed nick for clan %s (%u) to \"%s\"\n",
					 gfire_buddy_get_name(gf_buddy), clan->long_name, clanid, nick);

	gfire_buddy_set_clan_alias(gf_buddy, clanid, nick);

	g_free(nick);
}

void gfire_buddy_proto_game_client_data(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	GList *sids = NULL, *data = NULL;
	gfire_buddy *gf_buddy = NULL;

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &sids, "sid", offset);
	if(!sids || offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &data, "gcd", offset);
	if(!data || offset == -1)
	{
		gfire_list_clear(sids);
		return;
	}

	GList *sid = g_list_first(sids);
	GList *gcd = g_list_first(data);

	for(; sid; sid = g_list_next(sid))
	{
		gf_buddy = gfire_find_buddy(p_gfire, (const void*)sid->data, GFFB_SID);
		if(gf_buddy)
		{
			purple_debug_misc("gfire", "Got Game Client Data for buddy %s:\n", gfire_buddy_get_name(gf_buddy));
			GList *game_data = gfire_game_client_data_parse((gchar*)gcd->data);

			GList *current = g_list_first(game_data);
			for(; current; current = g_list_next(current))
				purple_debug_misc("gfire", "\t%s=%s\n", NN(((game_client_data*)current->data)->key), NN(((game_client_data*)current->data)->value));

			gfire_buddy_set_game_client_data(gf_buddy, game_data);
		}
		else
		{
			GList *cur = gfire_fof_data;
			for(; cur; cur = g_list_next(cur))
			{
				if(memcmp(((fof_game_data*)cur->data)->sid, sid->data, XFIRE_SID_LEN) == 0)
					break;
			}

			if(cur)
			{
				purple_debug_misc("gfire", "Got Game Client Data for requested FoF:\n");
				GList *game_data = gfire_game_client_data_parse((gchar*)gcd->data);

				GList *current = g_list_first(game_data);
				for(; current; current = g_list_next(current))
					purple_debug_misc("gfire", "\t%s=%s\n", NN(((game_client_data*)current->data)->key), NN(((game_client_data*)current->data)->value));

				((fof_game_data*)cur->data)->gcd = game_data;
			}
			else
				purple_debug_error("gfire", "got unknown SID from Xfire\n");
		}

		g_free(sid->data);
		g_free(gcd->data);

		gcd = g_list_next(gcd);
	}

	g_list_free(sids);
	g_list_free(data);
}
