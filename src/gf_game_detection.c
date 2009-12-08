/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
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

process_info *gfire_process_info_new(const gchar *p_name, const gchar *p_args, const guint32 p_id)
{
	if (!p_name)
		return NULL;

	process_info *ret = g_malloc0(sizeof(process_info));
	ret->name = g_strdup(p_name);

	if(p_args)
		ret->args = g_strdup(p_args);

	if (p_id)
		ret->pid = p_id;

	return ret;
}

static void gfire_process_info_free(process_info *p_info)
{
	if(!p_info)
		return;

	if(p_info->name) g_free(p_info->name);
	if(p_info->args) g_free(p_info->args);

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

gboolean gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_name, const gchar *p_required_args, const gchar *p_invalid_args)
{
	if(!p_list || !p_list->processes || !p_name)
		return FALSE;

	GList *cur = p_list->processes;
	while(cur)
	{
		process_info *info = cur->data;
		if(!info)
			continue;

		if(g_strcmp0(info->name, p_name) == 0)
		{
			// First check invalid arguments
			gboolean process_invalid_args = FALSE;

			if(p_invalid_args && strlen(p_invalid_args) > 0)
			{
				gchar **p_invalid_args_parts = g_strsplit(p_invalid_args, ";", -1);
				if (p_invalid_args_parts)
				{
					int i;
					for(i = 0; i < g_strv_length(p_invalid_args_parts); i++)
					{
						if (g_strrstr(info->args, p_invalid_args_parts[i]) != NULL)
						{
							process_invalid_args = TRUE;
							break;
						}
					}

					g_strfreev(p_invalid_args_parts);
				}
			}

			// Then check required args
			gboolean process_required_args = TRUE;

			if (process_invalid_args != TRUE)
			{
				if(p_required_args && strlen(p_required_args) > 0)
				{
					gchar **p_required_args_parts = g_strsplit(p_required_args, ";", -1);
					if (p_required_args_parts)
					{
						int i;
						for(i = 0; i < g_strv_length(p_required_args_parts); i++)
						{
							if (g_strrstr(info->args, p_required_args_parts[i]) == NULL)
							{
								process_required_args = FALSE;
								break;
							}
						}

						g_strfreev(p_required_args_parts);
					}
				}
			}

			// Finally return as found if valid
			if (process_invalid_args == FALSE && process_required_args == TRUE)
				return TRUE;
		}

		cur = g_list_next(cur);
	}

	return FALSE;
}
