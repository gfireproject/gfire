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

#include "gf_game_detection.h"

void gfire_process_list_update(gfire_process_list *p_list)
{
#define BUF_SIZE 1024

	if(!p_list)
		return;

	gfire_process_list_clear(p_list);

	FILE *ps = popen("ps --width 1024 -Ao comm:30,cmd --no-heading", "r");
	if(!ps)
		return;

	gchar *buffer = g_malloc0(BUF_SIZE);
	while(!feof(ps))
	{
		fgets(buffer, BUF_SIZE, ps);

		gchar **parts = g_strsplit(buffer, "     ", 2);
		if(!parts)
			continue;

		process_info *info = NULL;
		if(g_strv_length(parts) > 1)
		{
			gchar **cmd_parts = g_strsplit(parts[1], " ", 2);
			info = gfire_process_info_new(parts[0], cmd_parts ? cmd_parts[1] : NULL);
			g_strfreev(cmd_parts);
		}
		else
			info = gfire_process_info_new(parts[0], NULL);

		g_strfreev(parts);

		p_list->processes = g_list_append(p_list->processes, info);
	}

	pclose(ps);
}
