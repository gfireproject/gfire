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

#ifndef _GF_GROUPS_H
#define _GF_GROUPS_H

typedef struct _gfire_group gfire_group;

#include "gf_base.h"
#include "gf_buddies.h"
#include "gfire.h"

struct _gfire_group
{
	PurpleGroup *group;
	guint32 groupid;
	GList *buddies;
	gfire_data *owner;
};

// Creation and freeing
gfire_group *gfire_group_create(gfire_data *p_owner, const gchar *p_name, guint32 p_groupid);
void gfire_group_free(gfire_group *p_group, gboolean p_remove);

// Group handling
void gfire_group_rename(gfire_group *p_group, const gchar *p_name);
void gfire_group_added(gfire_group *p_group, guint32 p_groupid);
PurpleGroup *gfire_group_get_group(gfire_group *p_group);

// Buddy handling
void gfire_group_add_buddy(gfire_group *p_group, guint32 p_buddyid, gboolean p_byuser);
void gfire_group_remove_buddy(gfire_group *p_group, guint32 p_buddyid);
gboolean gfire_group_has_buddy(const gfire_group *p_group, guint32 p_buddyid);

// Identification
gboolean gfire_group_is_by_purple_group(const gfire_group *p_group, const PurpleGroup *p_prpl_group);
gboolean gfire_group_is_by_name(const gfire_group *p_group, const gchar *p_name);
gboolean gfire_group_is_by_id(const gfire_group *p_group, guint32 p_groupid);

#endif // _GF_GROUPS_H
