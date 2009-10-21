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

gfire_process_list *gfire_process_list_new()
{
	gfire_process_list *ret = g_malloc0(sizeof(gfire_process_list));
	if(!ret)
		return NULL;

	return ret;
}

void gfire_process_list_free(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	gfire_process_list_clear(p_list);

	g_free(p_list);
}

process_info *gfire_process_info_new(const gchar *p_name, const gchar *p_args)
{
	if(!p_name)
		return NULL;

	process_info *ret = g_malloc0(sizeof(process_info));
	ret->name = g_strdup(p_name);

	if(p_args)
		ret->args = g_strsplit(p_args, " ", 0);

	return ret;
}

static void gfire_process_info_free(process_info *p_info)
{
	if(!p_info)
		return;

	if(p_info->name) g_free(p_info->name);
	if(p_info->args) g_strfreev(p_info->args);

	g_free(p_info);
}

void gfire_process_list_clear(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	while(p_list->processes)
	{
		gfire_process_info_free((process_info*)p_list->processes->data);
		p_list->processes = g_list_delete_link(p_list->processes, p_list->processes);
	}
}

gboolean gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_name, const gchar *p_args)
{
	if(!p_list || !p_list->processes || !p_name)
		return FALSE;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

#ifndef _WIN32
		// The ps command only shows the 15 first characters of a process, remove the extra characters
		// Dirty hack which needs to be removed!!
		if(strncmp(info->name, p_name, 15) == 0)
#else
		if(g_strcmp0(info->name, p_name) == 0)
#endif // !_WIN32
		{
			if(p_args && (strlen(p_args) > 0))
			{
				if(!info->args)
					return FALSE;

				gchar **arg_pieces = g_strsplit(p_args, " ", 0);
				if(!arg_pieces)
					return FALSE;

				int i;
				for(i = 0; i < g_strv_length(arg_pieces); i++)
				{
					purple_debug_error("gfire", "arg_pieces: %s\n", arg_pieces[i]);
					int j;
					gboolean found = FALSE;
					for(j = 1; j < g_strv_length(info->args); j++)
					{
						if(g_strcmp0(arg_pieces[i], info->args[j]) == 0)
						{
							found = TRUE;
							break;
						}
					}

					if(!found)
					{
						g_strfreev(arg_pieces);
						return FALSE;
					}
				}

				g_strfreev(arg_pieces);
				return TRUE;
			}
			else
				return TRUE;
		}

		cur = g_list_next(cur);
	}

	return FALSE;
}
