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

#ifndef _GF_GAME_DETECTION_H

#include "gf_base.h"

typedef struct _process_info
{
	gchar *name;
	gchar *args;
	guint32 pid;
} process_info;

typedef struct _gfire_process_list
{
	GList *processes;
} gfire_process_list;

// OS independent (gf_game_detection.c)
gfire_process_list *gfire_process_list_new();
void gfire_process_list_free(gfire_process_list *p_list);
void gfire_process_list_clear(gfire_process_list *p_list);
gboolean gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_name, const gchar *p_required_args, const gchar *p_invalid_args);

// For internal use only
process_info *gfire_process_info_new(const gchar *p_name, const gchar *p_args, const guint32 p_id);

// OS dependent (gf_game_detection_X.c)
void gfire_process_list_update(gfire_process_list *p_list);

#endif // _GF_GAME_DETECTION_H
