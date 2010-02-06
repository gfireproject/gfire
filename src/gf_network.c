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
#include "gfire_proto.h"
#include "gf_buddies_proto.h"
#include "gf_chat_proto.h"
#include "gf_friend_search_proto.h"
#include "gf_server_browser_proto.h"
#include "gf_groups_proto.h"
#include "gf_games.h"

static guint8 *gfire_buffout = NULL;
static guint32 gfire_buffout_refcount = 0;

void gfire_network_init()
{
	gfire_buffout_refcount++;
	if(!gfire_buffout) gfire_buffout = g_malloc0(GFIRE_BUFFOUT_SIZE);
}

void gfire_network_cleanup()
{
	if(gfire_buffout_refcount == 0)
		return;

	gfire_buffout_refcount--;

	if(gfire_buffout_refcount == 0)
	{
		if(gfire_buffout) g_free(gfire_buffout);
		gfire_buffout = NULL;
	}
}

void gfire_network_buffout_write(const void *p_data, guint16 p_len, guint16 p_offset)
{
	if(!p_data || p_len == 0 || (p_len + p_offset) > GFIRE_BUFFOUT_SIZE)
		return;

	if(!gfire_buffout)
		gfire_network_init();

	memcpy(gfire_buffout + p_offset, p_data, p_len);
}

void gfire_network_buffout_copy(void *p_buffer, guint16 p_len)
{
	if(!p_buffer || !p_len)
		return;

	if(!gfire_buffout)
		gfire_network_init();

	memcpy(p_buffer, gfire_buffout, (p_len < GFIRE_BUFFOUT_SIZE) ? p_len : GFIRE_BUFFOUT_SIZE);
}

void gfire_send(PurpleConnection *p_gc, guint16 p_size)
{
	if(!p_gc || p_size == 0)
		return;

	gfire_data *gfire = (gfire_data*)p_gc->proto_data;
	if(!gfire)
		return;

	GTimeVal gtv;
	int tmp = 0;

	if (gfire->fd >= 0)
	{
		tmp = send(gfire->fd, gfire_buffout, p_size, 0);
		if(tmp < 0)
		{
			if(errno != EAGAIN)
			{
				purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_send: error %d: %s\n", errno, strerror(errno));
				purple_connection_error_reason(gfire_get_connection(gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, strerror(errno));
			}
		}
		else
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(send): wrote %d Bytes\n", tmp);
	}

	memset((void *)gfire_buffout, 0x00, GFIRE_BUFFOUT_SIZE);
	g_get_current_time(&gtv);
	gfire->last_packet = gtv.tv_sec;
}

void gfire_input_cb(gpointer p_data, gint p_source, PurpleInputCondition p_condition)
{
	guint16 packet_len = 0;
	static int tmp = 0;
	guint16 pkt_id = 0;
	gfire_data *gfire = (gfire_data*)p_data;

	if(p_condition != PURPLE_INPUT_READ)
		return;

	if(gfire->bytes_read < 2)
	{
		// Read the first 2 bytes (packet len)
		tmp = recv(p_source, (void*)gfire->buff_in, 2, 0);
		// Check for errors
		if(tmp <= 0)
		{
			// Connection closed when we receive a 0 byte packet
			if(tmp == 0)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "(input): read 0 bytes, connection closed by peer\n");
				purple_connection_error_reason(gfire_get_connection(gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Connection closed by peer."));
			}
			// We couldn't read now; not necessarily an error
			else if(errno == EAGAIN)
				return;
			else
			{
				purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Reading from socket failed errno = %d err_str = %s.\n",
						errno, strerror(errno));
				purple_connection_error_reason(gfire_get_connection(gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Socket read failure."));
			}
			gfire->bytes_read = 0;
			return;
		}

		gfire->bytes_read += tmp;

		if(gfire->bytes_read < 2)
			return;
	}

	// Get packet len
	memcpy(&packet_len, gfire->buff_in, sizeof(packet_len));
	packet_len = GUINT16_FROM_LE(packet_len);

	// Read the rest of the packet
	tmp = recv(p_source, (void*)gfire->buff_in + gfire->bytes_read, packet_len - gfire->bytes_read, 0);
	// Check for errors
	if(tmp <= 0)
	{
		// Connection closed when we receive a 0 byte packet
		if(tmp == 0)
		{
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(input): read 0 bytes, connection closed by peer\n");
			purple_connection_error_reason(gfire_get_connection(gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Connection closed by peer."));
		}
		// We couldn't read now; not necessarily an error
		else if(errno == EAGAIN)
			return;
		else
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Reading from socket failed errno = %d err_str = %s.\n",
				errno, strerror(errno));
			purple_connection_error_reason(gfire_get_connection(gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Socket read failure."));
		}
		gfire->bytes_read = 0;
		return;
	}

	gfire->bytes_read += tmp;

	// We still don't have all data, wait until our next call
	if(gfire->bytes_read != packet_len)
		return;

	// We have the whole xfire packet, process it
	//		Get packet id
	memcpy(&pkt_id, gfire->buff_in + 2, sizeof(pkt_id));
	pkt_id = GUINT16_FROM_LE(pkt_id);

	GTimeVal gtv;
	g_get_current_time(&gtv);
	gfire->last_response = gtv.tv_sec;

	gfire->bytes_read = 0;
	gfire_parse_packet(gfire, packet_len, pkt_id);
}


void gfire_parse_packet(gfire_data *p_gfire, guint16 p_packet_len, guint16 p_packet_id)
{
	guint16 ob_len = 0;
	guint32 newver = 0;
	char tmp[100] = "";
	PurpleAccount *account = NULL;

	switch(p_packet_id)
	{
		case 128:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received salt packet\n");
			gfire_proto_login_salt(p_gfire, p_packet_len);
		break;

		case 129:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received: wrong passwd/username\n");
			purple_connection_error_reason(gfire_get_connection(p_gfire), PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED, _("Password or Username Incorrect."));
		break;

		case 130:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "Log in was successful\n");
			purple_connection_set_state(gfire_get_connection(p_gfire), PURPLE_CONNECTED);

			// Parse session information
			gfire_proto_session_info(p_gfire, p_packet_len);

			// Send collective statistics
			ob_len = gfire_proto_create_collective_statistics(getenv("LANG") ? getenv("LANG") : "en_GB", "Gfire", GFIRE_VERSION_STRING, "");
			if(ob_len > 0) gfire_send(gfire_get_connection(p_gfire), ob_len);

			// Update current status
			gfire_set_current_status(p_gfire);

			// Load game xml from user dir; these don't need to work unless we are connected
			gfire_game_load_config_xml();
		break;

		case 131:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got buddylist: names and nicks\n");
			gfire_proto_buddy_list(p_gfire, p_packet_len); /* buddy list from server */
		break;

		case 132:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got buddylist: user is on/offline\n");
			gfire_buddy_proto_on_off(p_gfire, p_packet_len);
		break;

		case 133:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got IM (or ack Packet)\n");
			gfire_buddy_proto_im(p_gfire, p_packet_len);
		break;

		case 134:
			/* out of date version .. */
			/* autoset NEW VERSION :) */
			memcpy(&newver, p_gfire->buff_in + 17, sizeof(newver));
			newver = GUINT32_FROM_LE(newver);
			g_sprintf(tmp, _("Protocol version mismatch, needs to be %d. Auto set to new value."), newver);
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "login ok, but version too old, needs to be = %d\n", newver);
			account = purple_connection_get_account(gfire_get_connection(p_gfire));
			purple_account_set_int(account, "version", newver);
			purple_connection_error_reason(gfire_get_connection(p_gfire), PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		break;

		case 135:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got buddylist: game that a buddy is playing\n");
			gfire_buddy_proto_game_status(p_gfire, p_packet_len);
		break;

		case 136:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got list of friends of friends\n");
			gfire_buddy_proto_fof_list(p_gfire, p_packet_len);
		break;

		case 137:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "invitation result\n");
		break;

		case 138:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got buddy invitation\n");
			gfire_proto_invitation(p_gfire, p_packet_len);
		break;

		case 139:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "Remove buddy received\n");
			gfire_proto_buddy_remove(p_gfire, p_packet_len);
		break;

		case 143:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "Received friends search result\n");
			gfire_friend_search_proto_result(p_gfire, p_packet_len);
		break;

		case 144:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received keep alive response (PONG)\n");
		break;

		case 145:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: You have signed on from another location.\n");
			purple_connection_error_reason(gfire_get_connection(p_gfire), PURPLE_CONNECTION_ERROR_NAME_IN_USE, _("You have signed on from another location."));
		break;

		case 147:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got buddylist: voip software that a buddy is using\n");
			gfire_buddy_proto_voip_status(p_gfire, p_packet_len);
		break;

		case 150:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received serverlist\n");
			// Only used in conjunction with server browser which requires GTK
#ifdef HAVE_GTK
			gfire_server_browser_proto_serverlist(p_gfire, p_packet_len);
#endif // HAVE_GTK
		break;

		case 151:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received groups\n");
			gfire_group_proto_groups(p_gfire, p_packet_len);
		break;

		case 152:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received buddies in groups\n");
			gfire_group_proto_buddies_in_groups(p_gfire, p_packet_len);
		break;

		case 153:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received group-add confirmation\n");
			gfire_group_proto_group_added(p_gfire, p_packet_len);
		break;

		case 154:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received status packet.\n");
			gfire_buddy_proto_status_msg(p_gfire, p_packet_len);
		break;

		case 155:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received group chats\n");
			gfire_chat_proto_persistent_chats(p_gfire, p_packet_len);
		break;

		case 156:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received game client data packet.\n");
			gfire_buddy_proto_game_client_data(p_gfire, p_packet_len);
		break;

		case 158:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received clan list\n");
			gfire_proto_clan_list(p_gfire, p_packet_len);
		break;

		case 159:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received clan buddy list\n");
			gfire_proto_clan_blist(p_gfire, p_packet_len);
		break;

		case 160:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received clan member left\n");
			gfire_proto_clan_leave(p_gfire, p_packet_len);
		break;

		case 161:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received buddy nick change packet\n");
			gfire_buddy_proto_alias_change(p_gfire, p_packet_len);
		break;

		case 162:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received clan buddy nick change packet\n");
			gfire_buddy_proto_clan_alias_change(p_gfire, p_packet_len);
		break;

		case 169:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received system broadcast\n");
			gfire_proto_system_broadcast(p_gfire, p_packet_len);
		break;

		case 174:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received avatar info\n");
			gfire_buddy_proto_changed_avatar(p_gfire, p_packet_len);
		break;

		case 176:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received clan member info\n");
			gfire_buddy_proto_clans(p_gfire, p_packet_len);
		break;

		case 183:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received external game info\n");
			gfire_proto_external_game(p_gfire, p_packet_len);
		break;

		case 191:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received contest infos\n");
		break;

		case 350:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room topic change\n");
			gfire_chat_proto_topic_change(p_gfire, p_packet_len);
		break;

		case 351:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room join info\n");
			gfire_chat_proto_join_info(p_gfire, p_packet_len);
		break;

		case 353:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room, user join message\n");
			gfire_chat_proto_user_join(p_gfire, p_packet_len);
		break;

		case 354:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room, user leave message\n");
			gfire_chat_proto_user_leave(p_gfire, p_packet_len);
		break;

		case 355:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "got chat room message\n");
			gfire_chat_proto_msg(p_gfire, p_packet_len);
		break;

		case 356:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room invite\n");
			gfire_chat_proto_invite(p_gfire, p_packet_len);
		break;

		case 357:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room buddy permission changed\n");
			gfire_chat_proto_buddy_permission_change(p_gfire, p_packet_len);
		break;

		case 358:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "persistent chat room infos received\n");
			gfire_chat_proto_persistent_chat_infos(p_gfire, p_packet_len);
		break;

		case 359:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room buddy kicked notification received\n");
			gfire_chat_proto_buddy_kicked(p_gfire, p_packet_len);
		break;

		case 368:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room info (buddy list)\n");
			gfire_chat_proto_room_info(p_gfire, p_packet_len);
		break;

		case 370:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room default permission change received\n");
			gfire_chat_proto_default_permission_change(p_gfire, p_packet_len);
		break;

		case 374:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room motd changed\n");
			gfire_chat_proto_motd_change(p_gfire, p_packet_len);
		break;

		case 385:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room password change received\n");
			gfire_chat_proto_password_change(p_gfire, p_packet_len);
		break;

		case 386:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room accessibility change received\n");
			gfire_chat_proto_accessibility_change(p_gfire, p_packet_len);
		break;

		case 387:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received chat room reject confirmation\n");
		break;

		case 388:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room silenced change received\n");
			gfire_chat_proto_silenced_change(p_gfire, p_packet_len);
		break;

		case 389:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "chat room show join/leave messages changed received\n");
			gfire_chat_proto_show_join_leave_change(p_gfire, p_packet_len);
		break;

		default:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "received: length=%u command=%u\n", p_packet_len, p_packet_id);
	}
}
