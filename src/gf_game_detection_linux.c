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

	if (!p_list)
		return;

	gfire_process_list_clear(p_list);

	FILE *ps = popen("ps --width 1024 -Ao comm:15,pid:8,cmd --no-heading", "r");
	if (!ps)
		return;

	gchar *buffer = g_malloc0(BUF_SIZE);
	while(!feof(ps))
	{
		fgets(buffer, BUF_SIZE, ps);
		if (strlen(buffer) < 24)
			continue;

		// Extract short process name and command line
		gchar *process_name_short, *command_line;

		process_name_short = g_strndup(buffer, 15);
		if (!process_name_short)
			continue;

		process_name_short = g_strchomp(process_name_short);

		command_line = g_strrstr_len(buffer + 24, -1, process_name_short);
		if (!command_line)
		{
			g_free(process_name_short);
			continue;
		}

		// Extract process id
		guint32 pid;

		gchar *process_id = g_strndup(buffer + 16, 8);
		if (!process_id)
		{
			g_free(process_name_short);
			continue;
		}

		process_id = g_strstrip(process_id);
		pid = (guint32 )atoi(process_id);

		g_free(process_id);

		// Get full process name if needed
		gchar *process_name, *spacing_pos;

		spacing_pos = strchr(command_line + strlen(process_name_short), ' ');
		if (!spacing_pos)
			process_name = g_strdup(command_line);
		else
			process_name = g_strndup(command_line, spacing_pos - command_line);

		g_free(process_name_short);
		process_name = g_strstrip(process_name);

		// Get process arguments and add process to list
		gchar *arguments = g_strstrip(command_line + strlen(process_name));
		process_info *info = NULL;

		info = gfire_process_info_new(process_name, (arguments[0] != 0) ? arguments : NULL, pid);
		p_list->processes = g_list_append(p_list->processes, info);

		g_free(process_name);
	}

	g_free(buffer);
	pclose(ps);
}
