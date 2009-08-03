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

#include "gf_util.h"

/**
 * Removes color codes from a string,
 * used in the server browser to display the server names properly.
**/
gchar *gfire_escape_color_codes(const gchar *p_string)
{
	const gchar color_codes[] = {
		'0', 'P', '9', 'Y', 'Z', '7', 'W',
		'J', '1', 'Q', 'I',
		'H', '2', 'R', 'G',
		'M', '3', 'S', 'O', 'N',
		'F', 'D', '4', 'T',
		'B', '5', 'U',
		'C', 'E', '6', 'V',
		'K', 'L', '8', 'Y', 'A',
		'?', '+', '@', '-', '/',
		'&', 'X'
	};

	if(!p_string)
		return NULL;

	gchar *escaped = g_strdup(p_string);

	if(escaped)
	{
		int i;
		gchar code[3];
		gchar *tmp = NULL;
		for (i = 0; i < sizeof(color_codes); i++)
		{
			g_snprintf(code, 3, "^%c", color_codes[i]);
			tmp = purple_strcasereplace(escaped, code, "");
			g_free(escaped);
			escaped = tmp;
		}
	}

	return escaped;
}

gchar *gfire_escape_html(const gchar *p_html)
{
	if(p_html != NULL)
	{
		gchar *tmp = NULL;
		gchar *tmp2 = NULL;

		tmp = purple_strreplace(p_html, "&", "&amp;");

		tmp2 = purple_strreplace(tmp, "<", "&lt;");
		if(tmp) g_free(tmp);

		tmp = purple_strreplace(tmp2, ">", "&gt;");
		if(tmp2) g_free(tmp2);

		tmp2 = purple_strreplace(tmp, "\"", "&quot;");
		if(tmp) g_free(tmp);

		tmp = purple_strreplace(tmp2, "\n", "<br />");
		if(tmp2) g_free(tmp2);

		return tmp;
	}
	else
		return NULL;
}

void gfire_list_clear(GList *p_list)
{
	if(!p_list)
		return;

	GList *cur = g_list_first(p_list);
	for(; cur; cur = g_list_next(cur))
		if(cur->data) g_free(cur->data);

	g_list_free(p_list);
}

gchar *gfire_strip_character_range(gchar *p_string, gchar p_start, gchar p_end)
{
	if(!p_string)
		return NULL;

	int i;
	int len = strlen(p_string);

	for(i = 0; i < len; i++)
	{
		if((p_string[i] >= p_start) && (p_string[i] <= p_end))
		{
			memcpy(&p_string[i], &p_string[i + 1], len - i);
			i--;
			len--;
		}
	}

	return p_string;
}
