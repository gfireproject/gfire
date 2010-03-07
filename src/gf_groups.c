/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
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

#include "gf_groups.h"
#include "gf_network.h"
#include "gf_groups_proto.h"

gfire_group *gfire_group_create(gfire_data *p_owner, const gchar *p_name, guint32 p_groupid)
{
	if(!p_owner || !p_name)
		return NULL;

	gfire_group *ret = g_malloc0(sizeof(gfire_group));

	ret->owner = p_owner;
	ret->groupid = p_groupid;
	ret->group = purple_find_group(p_name);

	// Create the PurpleGroup if we couldn't find it
	if(!ret->group)
	{
		ret->group = purple_group_new(p_name);
		purple_blist_add_group(ret->group, NULL);
	}

	// Tell Xfire to create the group
	if(!ret->groupid)
	{
		guint16 len = gfire_group_proto_create_create_group(p_name);
		if(len > 0) gfire_send(gfire_get_connection(ret->owner), len);
	}

	return ret;
}

void gfire_group_free(gfire_group *p_group, gboolean p_remove)
{
	if(!p_group)
		return;

	// Tell Xfire to remove the group
	if(p_remove || p_group->groupid)
	{
		guint16 len = gfire_group_proto_create_remove_group(p_group->groupid);
		if(len > 0) gfire_send(gfire_get_connection(p_group->owner), len);
	}

	gfire_list_clear(p_group->buddies);
	g_free(p_group);
}

void gfire_group_rename(gfire_group *p_group, const gchar *p_name)
{
	if(!p_group || !p_name)
		return;

	// PurpleGroup has already been renamed when we're called, so just tell Xfire
	if(p_group->groupid)
	{
		guint16 len = gfire_group_proto_create_rename_group(p_group->groupid, p_name);
		if(len > 0) gfire_send(gfire_get_connection(p_group->owner), len);
	}
}

void gfire_group_added(gfire_group *p_group, guint32 p_groupid)
{
	if(!p_group)
		return;

	p_group->groupid = p_groupid;

	// Add all yet added buddies
	GList *cur = p_group->buddies;
	while(cur)
	{
		guint16 len = gfire_group_proto_create_add_buddy_to_group(p_group->groupid, *((guint32*)cur->data));
		if(len > 0) gfire_send(gfire_get_connection(p_group->owner), len);

		cur = g_list_next(cur);
	}
}

PurpleGroup *gfire_group_get_group(gfire_group *p_group)
{
	return (p_group ? p_group->group : NULL);
}

void gfire_group_add_buddy(gfire_group *p_group, guint32 p_buddyid, gboolean p_byuser)
{
	if(!p_group)
		return;

	guint32 *id = g_malloc(sizeof(p_buddyid));
	*id = p_buddyid;
	p_group->buddies = g_list_append(p_group->buddies, id);

	if(p_group->groupid && p_byuser)
	{
		guint16 len = gfire_group_proto_create_add_buddy_to_group(p_group->groupid, p_buddyid);
		if(len > 0) gfire_send(gfire_get_connection(p_group->owner), len);
	}

	purple_debug_info("gfire", "Added buddy to group: Group=%s / UserID=%u\n",
					  purple_group_get_name(p_group->group),
					  p_buddyid);
}

void gfire_group_remove_buddy(gfire_group *p_group, guint32 p_buddyid)
{
	if(!p_group)
		return;

	GList *cur = p_group->buddies;
	while(cur)
	{
		if(*((guint32*)cur->data) == p_buddyid)
		{
			if(p_group->groupid)
			{
				guint16 len = gfire_group_proto_create_remove_buddy_from_group(p_group->groupid, *((guint32*)cur->data));
				if(len > 0) gfire_send(gfire_get_connection(p_group->owner), len);
			}

			g_free(cur->data);
			p_group->buddies = g_list_delete_link(p_group->buddies, cur);
			return;
		}

		cur = g_list_next(cur);
	}
}

gboolean gfire_group_has_buddy(const gfire_group *p_group, guint32 p_buddyid)
{
	if(!p_group)
		return FALSE;

	GList *cur = p_group->buddies;
	while(cur)
	{
		if(*((guint32*)cur->data) == p_buddyid)
		{
			return TRUE;
		}

		cur = g_list_next(cur);
	}

	return FALSE;
}

gboolean gfire_group_is_by_purple_group(const gfire_group *p_group, const PurpleGroup *p_prpl_group)
{
	return (p_group && p_prpl_group && (p_group->group == p_prpl_group));
}

gboolean gfire_group_is_by_name(const gfire_group *p_group, const gchar *p_name)
{
	return (p_group && p_name && !strcmp(purple_group_get_name(p_group->group), p_name));
}

gboolean gfire_group_is_by_id(const gfire_group *p_group, guint32 p_groupid)
{
	return (p_group && (p_group->groupid == p_groupid));
}
