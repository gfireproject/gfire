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

#include "gf_groups_proto.h"
#include "gf_protocol.h"
#include "gf_groups.h"

guint16 gfire_group_proto_create_create_group(const gchar *p_name)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '1A' Group name
	offset = gfire_proto_write_attr_bs(0x1A, 0x01, p_name, strlen(p_name), offset);

	gfire_proto_write_header(offset, 0x1A, 1, 0);
	return offset;
}

guint16 gfire_group_proto_create_remove_group(guint32 p_groupid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '19' Group ID
	p_groupid = GUINT32_TO_LE(p_groupid);
	offset = gfire_proto_write_attr_bs(0x19, 0x02, &p_groupid, sizeof(p_groupid), offset);

	gfire_proto_write_header(offset, 0x1B, 1, 0);
	return offset;
}

guint16 gfire_group_proto_create_rename_group(guint32 p_groupid, const gchar *p_name)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '19' Group ID
	p_groupid = GUINT32_TO_LE(p_groupid);
	offset = gfire_proto_write_attr_bs(0x19, 0x02, &p_groupid, sizeof(p_groupid), offset);

	// '1A' New group name
	offset = gfire_proto_write_attr_bs(0x1A, 0x01, p_name, strlen(p_name), offset);

	gfire_proto_write_header(offset, 0x1C, 2, 0);
	return offset;
}

guint16 gfire_group_proto_create_add_buddy_to_group(guint32 p_groupid, guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '01' User ID
	p_userid = GUINT32_TO_LE(p_userid);
	offset = gfire_proto_write_attr_bs(0x01, 0x02, &p_userid, sizeof(p_userid), offset);

	// '19' Group ID
	p_groupid = GUINT32_TO_LE(p_groupid);
	offset = gfire_proto_write_attr_bs(0x19, 0x02, &p_groupid, sizeof(p_groupid), offset);

	gfire_proto_write_header(offset, 0x1D, 2, 0);
	return offset;
}

guint16 gfire_group_proto_create_remove_buddy_from_group(guint32 p_groupid, guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// '01' User ID
	p_userid = GUINT32_TO_LE(p_userid);
	offset = gfire_proto_write_attr_bs(0x01, 0x02, &p_userid, sizeof(p_userid), offset);

	// '19' Group ID
	p_groupid = GUINT32_TO_LE(p_groupid);
	offset = gfire_proto_write_attr_bs(0x19, 0x02, &p_groupid, sizeof(p_groupid), offset);

	gfire_proto_write_header(offset, 0x1E, 2, 0);
	return offset;
}

void gfire_group_proto_groups(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	GList *groupids = NULL;
	GList *groupnames = NULL;

	guint32 offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &groupids, 0x19, offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !groupids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &groupnames, 0x1A, offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !groupnames)
	{
		gfire_list_clear(groupids);
		return;
	}

	GList *id = groupids;
	GList *name = groupnames;

	while(id)
	{
		gfire_group *group = gfire_group_create(p_gfire, (gchar*)name->data, *((guint32*)id->data));
		gfire_add_group(p_gfire, group);

		id = g_list_next(id);
		name = g_list_next(name);
	}

	gfire_list_clear(groupids);
	gfire_list_clear(groupnames);
}

void gfire_group_proto_buddies_in_groups(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	GList *userids = NULL;
	GList *groupids = NULL;

	guint32 offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &userids, 0x01, offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !userids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &groupids, 0x19, offset);
	// Parsing error or empty list -> skip further parsing
	if(offset == -1 || !groupids)
	{
		gfire_list_clear(userids);
		return;
	}

	GList *cur_user = userids;
	GList *cur_group = groupids;

	while(cur_user)
	{
		gfire_group *group = gfire_find_group(p_gfire, cur_group->data, GFFG_GID);
		if(group)
		{
			gfire_group_add_buddy(group, *((guint32*)cur_user->data), FALSE);
		}

		cur_user = g_list_next(cur_user);
		cur_group = g_list_next(cur_group);
	}

	gfire_list_clear(userids);
	gfire_list_clear(groupids);
}

void gfire_group_proto_group_added(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 groupid = 0;
	gchar *groupname = NULL;

	guint32 offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &groupid, 0x19, offset);
	// Parsing error -> skip further parsing
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &groupname, 0x1A, offset);
	// Parsing error -> skip further parsing
	if(offset == -1 || !groupname)
		return;

	gfire_group *group = gfire_find_group(p_gfire, groupname, GFFG_NAME);
	if(group)
		gfire_group_added(group, groupid);

	g_free(groupname);
}
