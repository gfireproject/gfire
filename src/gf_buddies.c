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

#include "gf_network.h"
#include "gf_buddies_proto.h"
#include "gf_buddies.h"

static im_sent *gfire_im_sent_create(guint32 p_imindex, const gchar *p_msg)
{
	im_sent *ret = g_malloc0(sizeof(im_sent));
	if(!ret)
		return NULL;

	ret->imindex = p_imindex;
	if(p_msg)
		ret->msg = g_strdup(p_msg);
	else
		ret->msg = g_strdup("");

	GTimeVal gtv;
	g_get_current_time(&gtv);
	ret->time = gtv.tv_sec;

	return ret;
}

static void gfire_im_sent_free(im_sent *p_data)
{
	if(!p_data)
		return;

	if(p_data->msg) g_free(p_data->msg);
	g_free(p_data);
}

static gfire_buddy_clan_data *gfire_buddy_clan_data_create(gfire_clan *p_clan, const gchar *p_alias, gboolean p_default)
{
	if(!p_clan)
		return NULL;

	gfire_buddy_clan_data *ret = g_malloc0(sizeof(gfire_buddy_clan_data));
	if(!ret)
		goto error;

	ret->clan = p_clan;
	ret->is_default = p_default;

	if(p_alias)
	{
		ret->clan_alias = g_strdup(p_alias);
		if(!ret->clan_alias)
		{
			g_free(ret);
			goto error;
		}
	}

	return ret;

error:
	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_clan_data_create: Out of memory!\n");
	return NULL;
}

static void gfire_buddy_clan_data_free(gfire_buddy_clan_data *p_data)
{
	if(!p_data)
		return;

	if(p_data->clan_alias) g_free(p_data->clan_alias);

	g_free(p_data);
}

gfire_buddy *gfire_buddy_create(guint32 p_userid, const gchar *p_name, const gchar *p_alias, gfire_buddy_type p_type)
{
	if(!p_name)
		return NULL;

	gfire_buddy *ret = g_malloc0(sizeof(gfire_buddy));
	if(!ret)
		goto error;

	ret->sid = g_malloc0(XFIRE_SID_LEN);
	if(!ret->sid)
	{
		gfire_buddy_free(ret);
		goto error;
	}

	ret->userid = p_userid;
	ret->type = p_type;

	ret->name = g_strdup(p_name);
	if(!ret->name)
	{
		gfire_buddy_free(ret);
		goto error;
	}

	ret->lost_ims_timer = g_timeout_add(XFIRE_SEND_ACK_TIMEOUT * 1000, (GSourceFunc)gfire_buddy_check_pending_ims_cb, ret);
	ret->status = PURPLE_STATUS_AVAILABLE;

	gfire_buddy_set_alias(ret, p_alias);

	return ret;

error:
	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_buddy_create: Out of memory!\n");
	return NULL;
}

void gfire_buddy_free(gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return;

	g_source_remove(p_buddy->lost_ims_timer);

	if(p_buddy->alias) g_free(p_buddy->alias);
	if(p_buddy->status_msg) g_free(p_buddy->status_msg);
	if(p_buddy->name) g_free(p_buddy->name);
	if(p_buddy->sid) g_free(p_buddy->sid);

	GList *cur_clan_data = p_buddy->clan_data;
	for(; cur_clan_data; cur_clan_data = g_list_next(cur_clan_data))
		gfire_buddy_clan_data_free((gfire_buddy_clan_data*)cur_clan_data->data);

	GList *cur_pim_data = p_buddy->pending_ims;
	for(; cur_pim_data; cur_pim_data = g_list_next(cur_pim_data))
		gfire_im_sent_free((im_sent*)cur_pim_data->data);

	if(p_buddy->common_buddies)
		gfire_list_clear(p_buddy->common_buddies);

	g_list_free(p_buddy->clan_data);
	g_list_free(p_buddy->pending_ims);

	g_free(p_buddy);
}

gboolean gfire_buddy_is_friend(const gfire_buddy *p_buddy)
{
	return (p_buddy && (p_buddy->type == GFBT_FRIEND));
}

gboolean gfire_buddy_is_clan_member(const gfire_buddy *p_buddy)
{
	return (p_buddy && (p_buddy->type == GFBT_CLAN));
}

gboolean gfire_buddy_is_friend_of_friend(const gfire_buddy *p_buddy)
{
	return (p_buddy && (p_buddy->type == GFBT_FRIEND_OF_FRIEND));
}

gboolean gfire_buddy_is_by_userid(const gfire_buddy *p_buddy, guint32 p_userid)
{
	if(!p_buddy)
		return FALSE;

	return (p_userid == p_buddy->userid);
}

gboolean gfire_buddy_is_by_sid(const gfire_buddy *p_buddy, const guint8 *p_sid)
{
	if(!p_buddy || !p_sid)
		return FALSE;

	return (memcmp(p_buddy->sid, p_sid, XFIRE_SID_LEN) == 0);
}

void gfire_buddy_send(gfire_buddy *p_buddy, const gchar *p_msg)
{
	if(!p_buddy || !p_msg)
		return;

	p_buddy->im++;

	p_buddy->pending_ims = g_list_append(p_buddy->pending_ims, gfire_im_sent_create(p_buddy->im, p_msg));

	/* in 2.0 the gtkimhtml stuff started escaping special chars: '&' is now "&amp";
	   XFire native clients don't handle it. */
	p_msg = purple_unescape_html(p_msg);
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Sending IM to %s: %s\n", gfire_buddy_get_name(p_buddy), NN(p_msg));
	guint16 packet_len = gfire_buddy_proto_create_send_im(p_buddy->sid, p_buddy->im, p_msg);
	if(packet_len > 0) gfire_send(p_buddy->gc, packet_len);
}

void gfire_buddy_got_im(gfire_buddy *p_buddy, guint32 p_imindex, const gchar *p_msg)
{
	if(!p_buddy || !p_msg || !p_buddy->gc)
		return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Received IM from %s: %s\n", gfire_buddy_get_name(p_buddy), p_msg);

	// Send ACK
	guint16 len = gfire_buddy_proto_create_ack(p_buddy->sid, p_imindex);
	if(len > 0) gfire_send(p_buddy->gc, len);

	// Show IM
	PurpleAccount *account = purple_connection_get_account(p_buddy->gc);
	//	Only if buddy is not blocked
	if(purple_privacy_check(account, gfire_buddy_get_name(p_buddy)))
	{
		gchar *escaped = gfire_escape_html(p_msg);
		serv_got_im(p_buddy->gc, gfire_buddy_get_name(p_buddy), escaped, 0, time(NULL));
		g_free(escaped);
	}
}

void gfire_buddy_send_typing(gfire_buddy *p_buddy, gboolean p_typing)
{
	if(!p_buddy)
		return;

	p_buddy->im++;
	guint16 packet_len = gfire_buddy_proto_create_typing_notification(p_buddy->sid, p_buddy->im, p_typing);
	if(packet_len > 0) gfire_send(p_buddy->gc, packet_len);
}

void gfire_buddy_got_typing(const gfire_buddy *p_buddy, gboolean p_typing)
{
	if(!p_buddy)
		return;

	purple_debug_info("gfire", "%s %s.\n", gfire_buddy_get_name(p_buddy), p_typing ? "is now typing" : "stopped typing");

	serv_got_typing(p_buddy->gc, gfire_buddy_get_name(p_buddy), XFIRE_SEND_TYPING_TIMEOUT, p_typing ? PURPLE_TYPING : PURPLE_NOT_TYPING);
}

void gfire_buddy_got_im_ack(gfire_buddy *p_buddy, guint32 p_imindex)
{
	if(!p_buddy)
		return;

	GList *cur = p_buddy->pending_ims;
	while(cur)
	{
		im_sent *ims = cur->data;
		if(!ims)
		{
			cur = g_list_next(cur);
			continue;
		}

		if(ims->imindex == p_imindex)
		{
			gfire_im_sent_free(ims);
			p_buddy->pending_ims = g_list_delete_link(p_buddy->pending_ims, cur);

			return;
		}

		cur = g_list_next(cur);
	}
}

gboolean gfire_buddy_check_pending_ims_cb(gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	GList *cur = p_buddy->pending_ims;
	while(cur)
	{
		im_sent *ims = cur->data;
		if(!ims)
		{
			cur = g_list_next(cur);
			continue;
		}

		if(gtv.tv_sec - ims->time > XFIRE_SEND_ACK_TIMEOUT)
		{
			gchar *warn = g_strdup_printf("%s may have not received this message:\n%s", gfire_buddy_get_alias(p_buddy), ims->msg);
			gchar *escaped = gfire_escape_html(warn);
			g_free(warn);
			purple_conv_present_error(gfire_buddy_get_name(p_buddy), purple_buddy_get_account(p_buddy->prpl_buddy), escaped);
			g_free(escaped);

			gfire_im_sent_free(ims);
			p_buddy->pending_ims = g_list_delete_link(p_buddy->pending_ims, cur);
			cur = p_buddy->pending_ims;
		}

		cur = g_list_next(cur);
	}

	return TRUE;
}

static gfire_buddy_clan_data *gfire_buddy_get_default_clan_data(gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	GList *cur = p_buddy->clan_data;
	for(; cur; cur = g_list_next(cur))
	{
		if(((gfire_buddy_clan_data*)cur->data)->is_default)
			return (gfire_buddy_clan_data*)cur->data;
	}

	return NULL;
}

guint32 gfire_buddy_get_default_clan(gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return 0;

	gfire_buddy_clan_data *data = gfire_buddy_get_default_clan_data(p_buddy);
	if(data)
		return data->clan->id;
	else
		return 0;
}

void gfire_buddy_prpl_add(gfire_buddy *p_buddy, PurpleGroup *p_group)
{
	if(!p_buddy || !p_buddy->gc || p_buddy->prpl_buddy)
		return;

	PurpleBuddy *prpl_buddy = purple_find_buddy(purple_connection_get_account(p_buddy->gc), gfire_buddy_get_name(p_buddy));
	if(!prpl_buddy)
	{
		prpl_buddy = purple_buddy_new(purple_connection_get_account(p_buddy->gc), gfire_buddy_get_name(p_buddy), gfire_buddy_get_alias(p_buddy));
		if(!prpl_buddy)
		{
			purple_debug_error("gfire", "gfire_buddy_prpl_add: Creation of PurpleBuddy failed\n");
			return;
		}

		if(gfire_buddy_is_friend(p_buddy))
		{
			if(!p_group)
			{
				p_group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
				if(!p_group)
				{
					p_group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
					purple_blist_add_group(p_group, NULL);
				}
			}
		}
		else if(gfire_buddy_is_clan_member(p_buddy))
		{
			if(!p_buddy->clan_data)
				return;

			p_group = gfire_clan_get_prpl_group(gfire_buddy_get_default_clan_data(p_buddy)->clan);
			if(!p_group)
				return;
		}
		else if(gfire_buddy_is_friend_of_friend(p_buddy))
		{
			p_group = purple_find_group(GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME);
			if(!p_group)
			{
				p_group = purple_group_new(GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME);
				purple_blist_add_group(p_group, NULL);
				purple_blist_node_set_bool((PurpleBlistNode*)p_group, "collapsed", TRUE);
			}
		}

		purple_blist_add_buddy(prpl_buddy, NULL, p_group, NULL);

		if(gfire_buddy_is_friend_of_friend(p_buddy))
			purple_blist_node_set_flags((PurpleBlistNode*)prpl_buddy, PURPLE_BLIST_NODE_FLAG_NO_SAVE);
		else if(gfire_buddy_is_clan_member(p_buddy))
			purple_blist_node_set_bool((PurpleBlistNode*)prpl_buddy, "clanmember", TRUE);
	}
	else
	{
		p_buddy->avatarnumber = purple_blist_node_get_int((PurpleBlistNode*)prpl_buddy, "avatarnumber");
		p_buddy->avatartype = purple_blist_node_get_int((PurpleBlistNode*)prpl_buddy, "avatartype");
	}

	p_buddy->prpl_buddy = prpl_buddy;

	serv_got_alias(p_buddy->gc, gfire_buddy_get_name(p_buddy), gfire_buddy_get_alias(p_buddy));
}

void gfire_buddy_prpl_remove(gfire_buddy *p_buddy)
{
	if(!p_buddy || !p_buddy->prpl_buddy)
		return;

	purple_blist_remove_buddy(p_buddy->prpl_buddy);
	p_buddy->prpl_buddy = NULL;
}

static void gfire_buddy_update_status(gfire_buddy *p_buddy)
{
	if(!p_buddy || !p_buddy->prpl_buddy)
		return;

	if(gfire_buddy_is_online(p_buddy))
		purple_prpl_got_user_status(purple_buddy_get_account(p_buddy->prpl_buddy), gfire_buddy_get_name(p_buddy), purple_primitive_get_id_from_type(p_buddy->status), "message", gfire_buddy_get_status_text(p_buddy), NULL);
	else
		purple_prpl_got_user_status(purple_buddy_get_account(p_buddy->prpl_buddy), gfire_buddy_get_name(p_buddy), "offline", NULL);
}

void gfire_buddy_set_session_id(gfire_buddy *p_buddy, const guint8 *p_sessionid)
{
	if(!p_buddy || !p_sessionid)
		return;

	memcpy(p_buddy->sid, p_sessionid, XFIRE_SID_LEN);

	// Reset Game states
	if(!gfire_buddy_is_online(p_buddy))
	{
		// Remove offline FoF
		if(gfire_buddy_is_friend_of_friend(p_buddy))
		{
			gfire_remove_buddy((gfire_data*)purple_account_get_connection(purple_buddy_get_account(p_buddy->prpl_buddy))->proto_data, p_buddy, FALSE, TRUE);
			return;
		}

		memset(&p_buddy->game_data, 0, sizeof(gfire_game_data));
		memset(&p_buddy->voip_data, 0, sizeof(gfire_game_data));

		// Reset status
		if(p_buddy->status_msg) g_free(p_buddy->status_msg);
		p_buddy->status_msg = NULL;
	}
	else
	{
		p_buddy->status = PURPLE_STATUS_AVAILABLE;

		purple_debug_misc("gfire", "requesting advanced info for %s\n", gfire_buddy_get_name(p_buddy));
		// Request advanced infoview
		guint16 len = gfire_buddy_proto_create_advanced_info_request(p_buddy->userid);
		if(len > 0) gfire_send(p_buddy->gc, len);
	}

	gfire_buddy_update_status(p_buddy);
}

void gfire_buddy_set_game_status(gfire_buddy *p_buddy, guint32 p_gameid, guint32 p_port, guint32 p_ip)
{
	if(!p_buddy)
		return;

	p_buddy->game_data.id = p_gameid;
	p_buddy->game_data.port = p_port;
	p_buddy->game_data.ip.value = p_ip;

	gfire_buddy_update_status(p_buddy);

	purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s is playing %d on %d.%d.%d.%d:%d\n",
					 gfire_buddy_get_name(p_buddy), p_buddy->game_data.id, p_buddy->game_data.ip.octets[0],
					 p_buddy->game_data.ip.octets[1], p_buddy->game_data.ip.octets[2],
					 p_buddy->game_data.ip.octets[3], p_buddy->game_data.port);
}

void gfire_buddy_set_voip_status(gfire_buddy *p_buddy, guint32 p_voipid, guint32 p_port, guint32 p_ip)
{
	if(!p_buddy)
		return;

	p_buddy->voip_data.id = p_voipid;
	p_buddy->voip_data.port = p_port;
	p_buddy->voip_data.ip.value = p_ip;

	gfire_buddy_update_status(p_buddy);

	purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s is playing %d on %d.%d.%d.%d:%d\n",
					 gfire_buddy_get_name(p_buddy), p_buddy->voip_data.id, p_buddy->voip_data.ip.octets[0],
					 p_buddy->voip_data.ip.octets[1], p_buddy->voip_data.ip.octets[2],
					 p_buddy->voip_data.ip.octets[3], p_buddy->voip_data.port);
}

gboolean gfire_buddy_is_playing(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	return (p_buddy->game_data.id > 0);
}

gboolean gfire_buddy_is_talking(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	return (p_buddy->voip_data.id > 0);
}

const gfire_game_data *gfire_buddy_get_game_data(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	return &p_buddy->game_data;
}

const gfire_game_data *gfire_buddy_get_voip_data(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	return &p_buddy->voip_data;
}

gboolean gfire_buddy_is_available(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	return p_buddy->status == PURPLE_STATUS_AVAILABLE;
}

gboolean gfire_buddy_is_away(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	return p_buddy->status == PURPLE_STATUS_AWAY;
}

gboolean gfire_buddy_is_busy(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	return p_buddy->status == PURPLE_STATUS_UNAVAILABLE;
}

gboolean gfire_buddy_is_online(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return FALSE;

	// Create offline ID
	guint8 offline_id[XFIRE_SID_LEN];
	memset(offline_id, 0, XFIRE_SID_LEN);

	return (memcmp(p_buddy->sid, offline_id, XFIRE_SID_LEN) != 0);
}

gboolean gfire_buddy_is_clan_member_of(const gfire_buddy *p_buddy, guint32 p_clanid)
{
	if(!p_buddy)
		return FALSE;

	GList *cur_data = p_buddy->clan_data;
	while(cur_data)
	{
		if(gfire_clan_is(((gfire_buddy_clan_data*)cur_data->data)->clan, p_clanid))
			return TRUE;

		cur_data = g_list_next(cur_data);
	}

	return FALSE;
}

void gfire_buddy_set_alias(gfire_buddy *p_buddy, const gchar *p_alias)
{
	if(!p_buddy || !p_alias)
		return;

	if(p_buddy->alias) g_free(p_buddy->alias);

	if(strlen(p_alias) == 0)
		p_buddy->alias = NULL;
	else
		p_buddy->alias = g_strdup(p_alias);

	if(p_buddy->prpl_buddy &&
	   !(gfire_buddy_is_clan_member(p_buddy) &&
	   p_buddy->clan_data &&
	   ((gfire_buddy_clan_data*)p_buddy->clan_data->data)->clan_alias))
		serv_got_alias(purple_account_get_connection(p_buddy->prpl_buddy->account), p_buddy->name, p_buddy->alias);
}

const gchar *gfire_buddy_get_alias(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	if(gfire_buddy_is_clan_member(p_buddy) &&
	   p_buddy->clan_data &&
	   ((gfire_buddy_clan_data*)p_buddy->clan_data->data)->clan_alias)
		return ((gfire_buddy_clan_data*)p_buddy->clan_data->data)->clan_alias;

	if(p_buddy->alias)
		return p_buddy->alias;
	else
		return p_buddy->name;
}

const gchar *gfire_buddy_get_name(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	return p_buddy->name;
}

void gfire_buddy_set_status(gfire_buddy *p_buddy, const gchar *p_status_msg)
{
	if(!p_buddy)
		return;

	if(p_buddy->status_msg) g_free(p_buddy->status_msg);
	if(p_status_msg)
	{
		p_buddy->status_msg = g_strdup(p_status_msg);
		if((strncmp(p_status_msg, "(AFK) ", 6) == 0) || (strncmp(p_status_msg, "(ABS) ", 6) == 0))
		{
			p_buddy->status = PURPLE_STATUS_AWAY;
			p_buddy->status_msg = g_strdup(p_status_msg + 6);
		}
		else if(strncmp(p_status_msg, "(Busy) ", 7) == 0)
		{
			p_buddy->status = PURPLE_STATUS_UNAVAILABLE;
			p_buddy->status_msg = g_strdup(p_status_msg + 7);
		}
		else
		{
			p_buddy->status = PURPLE_STATUS_AVAILABLE;
			p_buddy->status_msg = g_strdup(p_status_msg);
		}

		g_strchomp(g_strchug(p_buddy->status_msg));
		if(strlen(p_buddy->status_msg) == 0)
		{
			g_free(p_buddy->status_msg);
			p_buddy->status_msg = NULL;
		}
	}
	else
	{
		p_buddy->status = PURPLE_STATUS_AVAILABLE;
		p_buddy->status_msg = NULL;
	}

	gfire_buddy_update_status(p_buddy);
}

gchar *gfire_buddy_get_status_text(const gfire_buddy *p_buddy)
{
	if(!p_buddy)
		return NULL;

	if(gfire_buddy_is_playing(p_buddy))
	{
		gchar *tmp = purple_unescape_html(gfire_game_name(p_buddy->game_data.id));
		gchar *ret = g_strdup_printf(N_("Playing %s"), tmp);
		g_free(tmp);
		return ret;
	}
	else if(p_buddy->status_msg)
		return g_strdup(p_buddy->status_msg);
	else
		return NULL;
}

const gchar *gfire_buddy_get_status_name(const gfire_buddy *p_buddy)
{
	return purple_primitive_get_name_from_type(p_buddy->status);
}

void gfire_buddy_set_avatar(gfire_buddy *p_buddy, guchar *p_data, guint32 p_len)
{
	if(!p_buddy || !p_data)
		return;
	else if(!p_len || !p_buddy->prpl_buddy)
	{
		g_free(p_data);
		return;
	}

	PurpleBuddyIcon *icon = purple_buddy_get_icon(p_buddy->prpl_buddy);
	if(!icon)
		purple_buddy_icon_new(p_buddy->prpl_buddy->account, p_buddy->name, p_data, p_len, NULL);
	else
		purple_buddy_icon_set_data(icon, p_data, p_len, NULL);
}

static void gfire_buddy_avatar_download_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const char *p_buf, gsize p_len, const gchar *p_error_message)
{
	guchar *temp = NULL;

	if (p_data == NULL || p_buf == NULL || p_len == 0)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_avatar_download_cb: download of avatar failed (%s)\n", NN(p_error_message));
		return;
	}

	temp = g_malloc0(p_len);
	memcpy(temp, p_buf, p_len);

	gfire_buddy_set_avatar((gfire_buddy*)p_data, temp, p_len);
}

void gfire_buddy_download_avatar(gfire_buddy *p_buddy, guint32 p_type, guint32 p_avatarNum)
{
	if(!p_buddy || !p_buddy->prpl_buddy)
		return;

	gchar *avatar_url = NULL;
	PurpleBuddyIcon *buddy_icon = NULL;

	if(p_type == p_buddy->avatartype && p_avatarNum == p_buddy->avatarnumber)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_buddy_proto_changed_avatar: avatar did not change. skipping download.\n");
		return;
	}

	switch(p_type)
	{
		case 1:
			avatar_url = g_strdup_printf(XFIRE_GALLERY_AVATAR_URL, p_avatarNum);
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "trying to download avatar from: %s\n", NN(avatar_url));
			purple_util_fetch_url(avatar_url, TRUE, "Purple-xfire", TRUE, gfire_buddy_avatar_download_cb, (void*)p_buddy);
			g_free(avatar_url);

			purple_blist_node_set_int(&p_buddy->prpl_buddy->node, "avatartype", p_type);
			purple_blist_node_set_int(&p_buddy->prpl_buddy->node, "avatarnumber", p_avatarNum);
		break;
		case 2:
			avatar_url = g_strdup_printf(XFIRE_AVATAR_URL, p_buddy->name, p_avatarNum);
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "trying to download avatar from: %s\n", NN(avatar_url));
			purple_util_fetch_url(avatar_url, TRUE, "Purple-xfire", TRUE, gfire_buddy_avatar_download_cb, (void*)p_buddy);
			g_free(avatar_url);

			purple_blist_node_set_int(&p_buddy->prpl_buddy->node, "avatartype", p_type);
			purple_blist_node_set_int(&p_buddy->prpl_buddy->node, "avatarnumber", p_avatarNum);
		break;
		default:
			buddy_icon = purple_buddy_get_icon(p_buddy->prpl_buddy);
			if(buddy_icon != NULL)
			{
				purple_debug(PURPLE_DEBUG_INFO, "gfire", "removing %s's avatar\n", gfire_buddy_get_name(p_buddy));
				purple_buddy_icon_set_data(buddy_icon, NULL, 0, NULL);
			}
			else
				purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s has no avatar\n", gfire_buddy_get_name(p_buddy));

			purple_blist_node_remove_setting(&p_buddy->prpl_buddy->node, "avatartype");
			purple_blist_node_remove_setting(&p_buddy->prpl_buddy->node, "avatarnumber");
		break;
	}

	p_buddy->avatartype = p_type;
	p_buddy->avatarnumber = p_avatarNum;
}

void gfire_buddy_add_to_clan(gfire_buddy *p_buddy, gfire_clan *p_clan, const gchar *p_clanalias, gboolean p_default)
{
	if(!p_buddy || !p_clan)
		return;

	if(gfire_buddy_is_clan_member_of(p_buddy, p_clan->id) || (p_default && gfire_buddy_get_default_clan_data(p_buddy)))
		return;

	gfire_buddy_clan_data *clan_data = gfire_buddy_clan_data_create(p_clan, p_clanalias, p_default);

	if(clan_data)
		p_buddy->clan_data = g_list_append(p_buddy->clan_data, clan_data);
}

void gfire_buddy_remove_clan(gfire_buddy *p_buddy, guint32 p_clanid, guint32 p_newdefault)
{
	if(!p_buddy)
		return;

	if(!gfire_buddy_is_clan_member_of(p_buddy, p_clanid))
		return;

	GList *cur = p_buddy->clan_data;
	for(; cur; cur = g_list_next(cur))
	{
		if(gfire_clan_is(((gfire_buddy_clan_data*)cur->data)->clan, p_clanid))
		{
			gfire_buddy_clan_data_free((gfire_buddy_clan_data*)cur->data);
			p_buddy->clan_data = g_list_delete_link(p_buddy->clan_data, cur);
			break;
		}
	}

	// Move buddy to new shared clan
	if(gfire_buddy_is_clan_member(p_buddy) && !gfire_buddy_get_default_clan_data(p_buddy))
	{
		purple_blist_remove_buddy(p_buddy->prpl_buddy);

		if(!gfire_buddy_is_clan_member_of(p_buddy, p_newdefault))
		{
			p_buddy->prpl_buddy = NULL;
			return;
		}

		cur = p_buddy->clan_data;
		for(; cur; cur = g_list_next(cur))
		{
			if(gfire_clan_is(((gfire_buddy_clan_data*)cur->data)->clan, p_clanid))
			{
				((gfire_buddy_clan_data*)cur->data)->is_default = TRUE;
				break;
			}
		}

		purple_blist_add_buddy(p_buddy->prpl_buddy, NULL, gfire_clan_get_prpl_group(((gfire_buddy_clan_data*)cur->data)->clan), NULL);
	}
}

void gfire_buddy_make_friend(gfire_buddy *p_buddy, PurpleGroup *p_group)
{
	if(!p_buddy || gfire_buddy_is_friend(p_buddy))
		return;

	if(gfire_buddy_is_clan_member(p_buddy) && p_buddy->clan_data)
		((gfire_buddy_clan_data*)p_buddy->clan_data->data)->is_default = FALSE;

	p_buddy->type = GFBT_FRIEND;

	// Move the buddy to the selected group
	if(p_buddy->prpl_buddy)
	{
		if(!p_group)
		{
			p_group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
			if(!p_group)
			{
				p_group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
				purple_blist_add_group(p_group, NULL);
			}
		}
		purple_blist_add_buddy(p_buddy->prpl_buddy, NULL, p_group, NULL);
		purple_blist_node_remove_setting((PurpleBlistNode*)p_buddy->prpl_buddy, "clanmember");
		purple_blist_node_set_flags((PurpleBlistNode*)p_buddy->prpl_buddy, 0);
	}
}

void gfire_buddy_set_common_buddies(gfire_buddy *p_buddy, GList *p_buddies)
{
	if(!p_buddy || !gfire_buddy_is_friend_of_friend(p_buddy))
		return;

	if(p_buddy->common_buddies) gfire_list_clear(p_buddy->common_buddies);
	p_buddy->common_buddies = p_buddies;
}

gchar *gfire_buddy_get_common_buddies_str(const gfire_buddy *p_buddy)
{
	if(!p_buddy || !gfire_buddy_is_friend_of_friend(p_buddy) || !p_buddy->common_buddies)
		return NULL;

	GString *str = g_string_new("");

	GList *cur = p_buddy->common_buddies;
	for(; cur; cur = g_list_next(cur))
	{
		if(cur != p_buddy->common_buddies)
			g_string_append_printf(str, ", %s", (gchar*)cur->data);
		else
			g_string_append(str, (gchar*)cur->data);
	}

	return g_string_free(str, FALSE);
}

static void gfire_clan_create_group(gfire_clan *p_clan)
{
	if(!p_clan || !p_clan->long_name || p_clan->prpl_group)
		return;

	gchar *clan_group_name = NULL;
	if(p_clan->short_name)
		clan_group_name = g_strdup_printf(GFIRE_CLAN_GROUP_FORMATTING, p_clan->long_name, p_clan->short_name);
	else
		clan_group_name = g_strdup(p_clan->long_name);

	if(!clan_group_name)
		return;

	gchar *escaped = gfire_escape_html(clan_group_name);
	p_clan->prpl_group = purple_group_new(escaped);
	g_free(escaped);
	g_free(clan_group_name);

	purple_blist_add_group(p_clan->prpl_group, NULL);
	purple_blist_node_set_int(&p_clan->prpl_group->node, "clanid", p_clan->id);
}

void gfire_clan_prpl_remove(gfire_clan *p_clan)
{
	if(!p_clan || !p_clan->prpl_group)
		return;

	purple_blist_remove_group(p_clan->prpl_group);

	p_clan->prpl_group = 0;
}

gfire_clan *gfire_clan_create(guint32 p_clanid, const gchar *p_longName, const gchar *p_shortName, gboolean p_createGroup)
{
	gfire_clan *ret = g_malloc0(sizeof(gfire_clan));
	if(!ret)
		goto error;

	ret->id = p_clanid;

	if(p_longName)
	{
		ret->long_name = g_strdup(p_longName);
		if(!ret->long_name)
		{
			gfire_clan_free(ret);
			goto error;
		}
	}

	if(p_shortName)
	{
		ret->short_name = g_strdup(p_shortName);
		if(!ret->short_name)
		{
			gfire_clan_free(ret);
			goto error;
		}
	}

	if(p_createGroup)
		gfire_clan_create_group(ret);

	return ret;

error:
	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_clan_create: Out of memory!\n");
	return NULL;
}

void gfire_clan_free(gfire_clan *p_clan)
{
	if(!p_clan)
		return;

	if(p_clan->long_name) g_free(p_clan->long_name);
	if(p_clan->short_name) g_free(p_clan->short_name);

	g_free(p_clan);
}

void gfire_clan_set_names(gfire_clan *p_clan, const gchar *p_longName, const gchar *p_shortName)
{
	if(!p_clan)
		return;

	if(p_longName)
	{
		if(p_clan->long_name) g_free(p_clan->long_name);
		p_clan->long_name = g_strdup(p_longName);
	}

	if(p_shortName)
	{
		if(p_clan->short_name) g_free(p_clan->short_name);
		p_clan->short_name = g_strdup(p_shortName);
	}

	if(p_clan->prpl_group)
	{
		gchar *clan_group_name = NULL;
		if(p_clan->short_name)
			clan_group_name = g_strdup_printf(GFIRE_CLAN_GROUP_FORMATTING, p_clan->long_name, p_clan->short_name);
		else
			clan_group_name = g_strdup(p_clan->long_name);

		if(!clan_group_name)
			return;

		gchar *escaped = gfire_escape_html(clan_group_name);
		purple_blist_rename_group(p_clan->prpl_group, escaped);
		g_free(escaped);
		g_free(clan_group_name);
	}
}

void gfire_clan_set_prpl_group(gfire_clan *p_clan, PurpleGroup *p_group)
{
	if(!p_clan)
		return;

	p_clan->prpl_group = p_group;
}

PurpleGroup *gfire_clan_get_prpl_group(gfire_clan *p_clan)
{
	if(!p_clan)
		return NULL;

	if(!p_clan->prpl_group)
		gfire_clan_create_group(p_clan);

	return p_clan->prpl_group;
}

gboolean gfire_clan_is(const gfire_clan *p_clan, guint32 p_clanid)
{
	if(!p_clan)
		return FALSE;

	return (p_clan->id == p_clanid);
}

GList *gfire_clan_get_existing()
{
	GList *ret = NULL;
	PurpleBlistNode *group_node = NULL;

	group_node = purple_blist_get_root();

	// Get first node of type group
	while(group_node && !PURPLE_BLIST_NODE_IS_GROUP(group_node))
		group_node = purple_blist_node_get_first_child(group_node);

	// Check for clan group nodes
	while(group_node)
	{
		if(purple_blist_node_get_int(group_node, "clanid"))
		{
			// Create the clan and add it to the list
			gfire_clan *clan = gfire_clan_create(purple_blist_node_get_int(group_node, "clanid"), NULL, NULL, FALSE);
			if(clan)
			{
				purple_debug_error("gfire", "pre-added clan %s\n", purple_group_get_name((PurpleGroup*)group_node));
				gfire_clan_set_prpl_group(clan, (PurpleGroup*)group_node);
				ret = g_list_append(ret, clan);
			}
		}

		group_node = purple_blist_node_get_sibling_next(group_node);
	}

	return ret;
}

fof_game_data *gfire_fof_game_data_create(const guint8 *p_sid, guint32 p_game, guint32 p_ip, guint16 p_port)
{
	if(!p_sid)
		return NULL;

	fof_game_data *ret = g_malloc0(sizeof(fof_game_data));
	if(!ret)
		goto error;

	ret->sid = g_malloc0(XFIRE_SID_LEN);
	if(!ret->sid)
		goto error;

	memcpy(ret->sid, p_sid, XFIRE_SID_LEN);

	ret->game.id = p_game;
	ret->game.ip.value = p_ip;
	ret->game.port = p_port;

	return ret;

error:
	if(ret) g_free(ret);
	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_fof_game_data_create: Out of memory!\n");
	return NULL;
}

void gfire_fof_game_data_free(fof_game_data *p_data)
{
	if(!p_data)
		return;

	if(p_data->sid) g_free(p_data->sid);
	g_free(p_data);
}
