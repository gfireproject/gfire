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

#include "gfire_proto.h"

guint16 gfire_proto_create_auth(const gchar *p_name, const gchar *p_pw_hash)
{
	if(!p_name || !p_pw_hash)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	/*
	 * packet_length 00 type(01) 00 numberOfAtts
	 * attribute_length 'name'  usernameLength_length usernameLength 00 username
	 * attribute_length 'password'  passwdLength_length passwdLength 00 cryptedPassword
	 */

	// "name"
	offset = gfire_proto_write_attr_ss("name", 0x01, p_name, strlen(p_name), offset);

	// "password"
	offset = gfire_proto_write_attr_ss("password", 0x01, p_pw_hash, strlen(p_pw_hash), offset);

	// "flags"
	guint32 flags = 0;
	offset = gfire_proto_write_attr_ss("flags", 0x02, &flags, sizeof(flags), offset);

	gfire_proto_write_header(offset, 0x01, 3, 0);
	return offset;
}

guint16 gfire_proto_create_collective_statistics(const gchar *p_lang, const gchar *p_skin, const gchar *p_theme, const gchar *p_partner)
{
	if(!p_lang || !p_skin || !p_theme || !p_partner)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "lang"
	offset = gfire_proto_write_attr_ss("lang", 0x01, p_lang, strlen(p_lang), offset);

	// "skin"
	offset = gfire_proto_write_attr_ss("skin", 0x01, p_skin, strlen(p_skin), offset);

	// "theme"
	offset = gfire_proto_write_attr_ss("theme", 0x01, p_theme, strlen(p_theme), offset);

	// "partner"
	offset = gfire_proto_write_attr_ss("partner", 0x01, p_partner, strlen(p_partner), offset);

	gfire_proto_write_header(offset, 0x10, 4, 0);
	return offset;
}

guint16 gfire_proto_create_client_version(guint32 p_version)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "version"
	p_version = GUINT32_TO_LE(p_version);
	offset = gfire_proto_write_attr_ss("version", 0x02, &p_version, sizeof(p_version), offset);

	gfire_proto_write_header(offset, 0x03, 1, 0);
	return offset;
}

guint16 gfire_proto_create_status_text(const gchar *p_status)
{
	if(!p_status)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// '2E'
	offset = gfire_proto_write_attr_bs(0x2E, 0x01, p_status, strlen(p_status), offset);

	gfire_proto_write_header(offset, 0x20, 1, 0);
	return offset;
}

/*send keep alive packet to the server*/
guint16 gfire_proto_create_keep_alive()
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "value"
	guint32 value = 0;
	offset = gfire_proto_write_attr_ss("value", 0x02, &value, sizeof(value), offset);

	// "stats"
	offset = gfire_proto_write_attr_list_ss("stats", NULL, 0x02, 4, offset);

	gfire_proto_write_header(offset, 0x0D, 2, 0);

	return offset;
}

guint16 gfire_proto_create_invitation(const gchar *p_name, const gchar *p_msg)
{
	if(!p_name || !p_msg)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "name"
	offset = gfire_proto_write_attr_ss("name", 0x01, p_name, strlen(p_name), offset);

	// "msg"
	offset = gfire_proto_write_attr_ss("msg", 0x01, p_msg, strlen(p_msg), offset);

	gfire_proto_write_header(offset, 0x06, 2, 0);
	return offset;
}

guint16 gfire_proto_create_invitation_reject(const gchar *p_name)
{
	if(!p_name)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "name"
	offset = gfire_proto_write_attr_ss("nick", 0x01, p_name, strlen(p_name), offset);

	gfire_proto_write_header(offset, 0x08, 1, 0);
	return offset;
}

guint16 gfire_proto_create_invitation_accept(const gchar *p_name)
{
	if(!p_name)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "name"
	offset = gfire_proto_write_attr_ss("nick", 0x01, p_name, strlen(p_name), offset);

	gfire_proto_write_header(offset, 0x07, 1, 0);
	return offset;
}

guint16 gfire_proto_create_delete_buddy(guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "userid"
	p_userid = GUINT32_TO_LE(p_userid);
	offset = gfire_proto_write_attr_ss("userid", 0x02, &p_userid, sizeof(p_userid), offset);

	gfire_proto_write_header(offset, 0x09, 1, 0);
	return offset;
}

/*
* Sends a nickname change to the server
*/
guint16 gfire_proto_create_change_alias(const gchar *p_alias)
{
	if(!p_alias)
		return 0;

	guint32 offset = XFIRE_HEADER_LEN;

	// "nick"
	offset = gfire_proto_write_attr_ss("nick", 0x01, p_alias, strlen(p_alias), offset);

	gfire_proto_write_header(offset, 0x0E, 1, 0);
	return offset;
}

/*
* Sends the packet when we join a game or leave it (gameid 00 00)
*/
guint16 gfire_proto_create_join_game(const gfire_game_data *p_game)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "gameid"
	guint32 gameid = GUINT32_TO_LE(p_game->id);
	offset = gfire_proto_write_attr_ss("gameid", 0x02, &gameid, sizeof(gameid), offset);

	// "gip"
	guint32 gameip = GUINT32_TO_LE(p_game->ip.value);
	offset = gfire_proto_write_attr_ss("gip", 0x02, &gameip, sizeof(gameip), offset);

	// "gport"
	guint32 gameport = GUINT32_TO_LE(p_game->port);
	offset = gfire_proto_write_attr_ss("gport", 0x02, &gameport, sizeof(gameport), offset);

	gfire_proto_write_header(offset, 0x04, 3, 0);
	return offset;
}

/*
* Sends the packet when we join a voip server or leave it (voipid 00 00)
*/
guint16 gfire_proto_create_join_voip(const gfire_game_data *p_voip)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "vid"
	guint32 voipid = GUINT32_TO_LE(p_voip->id);
	offset = gfire_proto_write_attr_ss("vid", 0x02, &voipid, sizeof(voipid), offset);

	// "vip"
	guint32 voipip = GUINT32_TO_LE(p_voip->ip.value);
	offset = gfire_proto_write_attr_ss("vip", 0x02, &voipip, sizeof(voipip), offset);

	// "vport"
	guint32 voipport = GUINT32_TO_LE(p_voip->port);
	offset = gfire_proto_write_attr_ss("vport", 0x02, &voipport, sizeof(voipport), offset);

	gfire_proto_write_header(offset, 0x0F, 3, 0);
	return offset;
}

// reads buddy list from server and populates purple blist
void gfire_proto_buddy_list(gfire_data *p_gfire, guint16 p_packet_len)
{
	guint32 offset;
	gfire_buddy *gf_buddy = NULL;
	GList *friends = NULL;
	GList *nicks = NULL;
	GList *userids = NULL;
	GList *f, *n, *u;

	if (p_packet_len < 16)
	{
		purple_debug_error("gfire", "buddy list received, but too short. (%d bytes)\n", p_packet_len);
		return;
	}

	offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &friends, "friends", offset);
	// Parsing error or empty list -> skip further handling
	if(offset == -1 || !friends)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &nicks, "nick", offset);
	// Parsing error -> free other lists and skip further handling
	if(offset == -1)
	{
		g_list_free(friends);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &userids, "userid", offset);
	// Parsing error -> free other lists and skip further handling
	if(offset == -1)
	{
		g_list_free(friends);
		g_list_free(nicks);
		return;
	}

	f = friends;
	n = nicks;
	u = userids;

	for(; f; f = g_list_next(f))
	{
		gf_buddy = gfire_find_buddy(p_gfire, f->data, GFFB_NAME);
		if(!gf_buddy)
		{
			gf_buddy = gfire_buddy_create(*(guint32*)u->data, (gchar*)f->data, (gchar*)n->data, GFBT_FRIEND);
			if(gf_buddy)
				gfire_add_buddy(p_gfire, gf_buddy, NULL);
		}
		else if(!gfire_buddy_is_friend(gf_buddy))
		{
			gfire_buddy_make_friend(gf_buddy, NULL);
			gfire_buddy_set_alias(gf_buddy, (gchar*)n->data);
		}

		g_free(f->data);
		g_free(u->data);
		g_free(n->data);

		u = g_list_next(u);
		n = g_list_next(n);
	}

	g_list_free(friends);
	g_list_free(nicks);
	g_list_free(userids);
}

void gfire_proto_clan_leave(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 clanid = 0;
	guint32 userid = 0;
	gfire_buddy *clan_member = NULL;

	guint32 offset = XFIRE_HEADER_LEN;

	if(p_packet_len < 17)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_proto_read_clan_leave: received, but too short. (%d bytes)\n", p_packet_len);
		return;
	}

	// '6C' clanid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &clanid, 0x6C, offset);
	if(offset == -1)
		return;

	// '01' userid
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);
	if(offset == -1)
		return;

	if(gfire_is_self(p_gfire, userid))
		gfire_leave_clan(p_gfire, clanid);
	else
	{
		clan_member = gfire_find_buddy(p_gfire, &userid, GFFB_USERID);
		if(!clan_member)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_proto_clan_leave: invalid user ID from Xfire\n");
			return;
		}

		gfire_remove_buddy_from_clan(p_gfire, clan_member, clanid);
	}
}

void gfire_proto_login_salt(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	gchar *salt = NULL;
	offset = gfire_proto_read_attr_string_ss(p_gfire->buff_in, &salt, "salt", offset);
	if(offset == -1 || !salt)
	{
		purple_connection_error(gfire_get_connection(p_gfire), N_("Received invalid login salt!"));
		return;
	}

	purple_debug_info("gfire", "salt: %s\n", salt);

	gfire_authenticate(p_gfire, salt);
	g_free(salt);
}

/* reads session information from intial login grabs our info */
void gfire_proto_session_info(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 userid = 0;
	guint8 *sid = NULL;
	gchar *alias = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &userid, "userid", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_sid_ss(p_gfire->buff_in, &sid, "sid", offset);
	if(offset == -1 || !sid)
		return;

	offset = gfire_proto_read_attr_string_ss(p_gfire->buff_in, &alias, "nick", offset);
	if(offset == -1 || !alias)
	{
		g_free(sid);
		return;
	}

	gfire_set_userid(p_gfire, userid);
	gfire_set_sid(p_gfire, sid);
	gfire_set_alias(p_gfire, alias);

	g_free(sid);
	g_free(alias);
}

void gfire_proto_invitation(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	GList *names = NULL;
	GList *aliases = NULL;
	GList *msgs = NULL;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &names, "name", offset);
	if(offset == -1 || !names)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &aliases, "nick", offset);
	if(offset == -1 || !aliases)
	{
		g_list_free(names);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &msgs, "msg", offset);
	if(offset == -1 || !msgs)
	{
		g_list_free(names);
		g_list_free(aliases);
		return;
	}

	GList *n = names; GList *a = aliases; GList *m = msgs;
	for(; n; n = g_list_next(n))
	{
		gfire_got_invitation(p_gfire, (gchar*)n->data, (gchar*)a->data, (gchar*)m->data);

		g_free(n->data);
		g_free(a->data);
		g_free(m->data);

		a = g_list_next(a);
		m = g_list_next(m);
	}

	g_list_free(names);
	g_list_free(aliases);
	g_list_free(msgs);
}

void gfire_proto_clan_list(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	GList *clanids = NULL;
	GList *clanLongNames = NULL;
	GList *clanShortNames = NULL;
	GList *clanTypes = NULL;
	gfire_clan *newClan = NULL;


	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanids, 0x6C, offset);
	if(offset == -1 || !clanids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanLongNames, 0x02, offset);
	if(offset == -1 || !clanLongNames)
	{
		g_list_free(clanids);
		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanShortNames, 0x72, offset);
	if(offset == -1 || !clanShortNames)
	{
		g_list_free(clanids);
		g_list_free(clanLongNames);
		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &clanTypes, 0x34, offset);
	if(offset == -1 || !clanTypes)
	{
		g_list_free(clanids);
		g_list_free(clanLongNames);
		g_list_free(clanShortNames);
		return;
	}

	GList *id = clanids; GList *ln = clanLongNames; GList *sn = clanShortNames; GList *t = clanTypes;
	for(; id; id = g_list_next(id))
	{
		newClan = gfire_clan_create(*(guint32*)id->data, (const gchar*)ln->data, (const gchar*)sn->data, TRUE);
		if(newClan)
			gfire_add_clan(p_gfire, newClan);

		g_free(id->data);
		g_free(ln->data);
		g_free(sn->data);
		g_free(t->data);

		ln = g_list_next(ln);
		sn = g_list_next(sn);
		t = g_list_next(t);
	}

	g_list_free(clanids);
	g_list_free(clanLongNames);
	g_list_free(clanShortNames);
	g_list_free(clanTypes);
}

void gfire_proto_clan_blist(gfire_data *p_gfire, gint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	gfire_buddy *gf_buddy = NULL;
	gfire_clan *clan = NULL;
	guint32 clanid = 0;
	GList *userids = NULL;
	GList *names = NULL;
	GList *aliases = NULL;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &clanid, 0x6C, offset);
	if(offset == -1)
		return;

	clan = gfire_find_clan(p_gfire, clanid);
	if(!clan)
	{
		purple_debug_error("gfire", "gfire_proto_clan_blist: Unknown Clan ID from Xfire!\n");
		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &userids, 0x01, offset);
	if(offset == -1 || !userids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &names, 0x02, offset);
	if(offset == -1 || !names)
	{
		g_list_free(userids);
		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &aliases, 0x0D, offset);
	if(offset == -1 || !aliases)
	{
		g_list_free(userids);
		g_list_free(names);
		return;
	}

	GList *u = userids, *n = names, *a = aliases;
	for(; u; u = g_list_next(u))
	{
		// Don't add ourself to the clan list
		if(gfire_is_self(p_gfire, *(guint32*)u->data))
		{
			g_free(u->data);
			g_free(n->data);
			g_free(a->data);

			n = g_list_next(n);
			a = g_list_next(a);
			continue;
		}

		// Add buddy if it doesn't exist yet
		gf_buddy = gfire_find_buddy(p_gfire, u->data, GFFB_USERID);
		if(!gf_buddy)
		{
			gf_buddy = gfire_buddy_create(*(guint32*)u->data, (const gchar*)n->data, NULL, GFBT_CLAN);
			if(gf_buddy)
			{
				gfire_buddy_add_to_clan(gf_buddy, clan, (const gchar*)a->data, TRUE);
				gfire_add_buddy(p_gfire, gf_buddy, NULL);
			}
		}
		else
			gfire_buddy_add_to_clan(gf_buddy, clan, (const gchar*)a->data, FALSE);

		g_free(u->data);
		g_free(n->data);
		g_free(a->data);

		n = g_list_next(n);
		a = g_list_next(a);
	}

	g_list_free(userids);
	g_list_free(names);
	g_list_free(aliases);
}

void gfire_proto_system_broadcast(gfire_data *p_gfire, gint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 unknown = 0;
	gchar *msg = NULL;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &unknown, 0x34, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &msg, 0x2E, offset);
	if(offset == -1 || !msg)
		return;

	gchar *escaped = gfire_escape_html(msg);
	purple_notify_info(gfire_get_connection(p_gfire), N_("Xfire System Broadcast"), N_("Xfire System Broadcast Message:"), escaped);
	g_free(escaped);

	g_free(msg);
}
