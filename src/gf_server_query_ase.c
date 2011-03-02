/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
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

#include "gf_server_query.h"

#ifdef HAVE_GTK

typedef struct _gfire_sq_ase_player
{
	gchar *name;
	gchar *team;
	gchar *skin;
	gchar *score;
	gchar *ping;
	gchar *time;
} gfire_sq_ase_player;

typedef struct _gfire_sq_ase_data
{
	gchar *game_name;
	guint16 game_port;
	gchar *server_name;
	gchar *game_type;
	gchar *map;
	gchar *version;
	gboolean password;
	guint num_players;
	guint max_players;
	GData *rules;
	GSList *players;
} gfire_sq_ase_data;

// Prototypes
static void gfire_sq_ase_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_ase_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_ase_details(gfire_game_server *p_server);
static void gfire_sq_ase_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_ase_driver =
{
	gfire_sq_ase_query,
	gfire_sq_ase_parse,
	gfire_sq_ase_details,
	gfire_sq_ase_free_server,
	3
};

static const struct { gchar ch; const gchar *hex; } aseCodes[] =
{
	{ '0', "000000" }, { '1', "FFFFFF" }, { '2', "0000FF" },
	{ '3', "00FF00" }, { '4', "FF0000" }, { '5', "00FFFF" },
	{ '6', "FFFF00" }, { '7', "FF00FF" }, { '8', "FF9900" },
	{ '9', "808080" }, { 0, NULL }
};

static gchar *gfire_sq_ase_strip_color_codes(const gchar *p_str)
{
	GString *str = g_string_new(NULL);

	while(*p_str)
	{
		const gchar *next = strchr(p_str + 1, '$');
		if(!next)
			next = strchr(p_str + 1, 0);

		if(*p_str == '$')
		{
			int i = 0;
			while(aseCodes[i].ch)
			{
				if(*(p_str + 1) == aseCodes[i].ch)
				{
					g_string_append_len(str, p_str + 2, next - p_str - 2);
					break;
				}
				i++;
			}

			if(!aseCodes[i].ch)
				g_string_append_len(str, p_str, next - p_str);
		}
		else
			g_string_append_len(str, p_str, next - p_str);

		p_str = next;
	}

	return g_string_free(str, FALSE);
}

static gchar *gfire_sq_ase_color_codes_to_html(const gchar *p_str)
{
	GString *str = g_string_new(NULL);
	gboolean first = TRUE;

	while(*p_str)
	{
		const gchar *next = strchr(p_str + 1, '$');
		if(!next)
			next = strchr(p_str + 1, 0);

		if(*p_str == '$')
		{
			int i = 0;
			while(aseCodes[i].ch)
			{
				if(*(p_str + 1) == aseCodes[i].ch)
				{
					if(first)
					{
						g_string_append_printf(str, "<font color=\"#%s\">", aseCodes[i].hex);
						first = FALSE;
					}
					else
						g_string_append_printf(str, "</font><font color=\"#%s\">", aseCodes[i].hex);

					g_string_append_len(str, p_str + 2, next - p_str - 2);
					break;
				}
				i++;
			}

			if(!aseCodes[i].ch)
				g_string_append_len(str, p_str, next - p_str);
		}
		else
			g_string_append_len(str, p_str, next - p_str);

		p_str = next;
	}

	if(!first)
		g_string_append(str, "</font>");

	return g_string_free(str, FALSE);
}

static void gfire_sq_ase_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static const gchar query = 's';

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, &query, sizeof(query), 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gchar *parse_string(const unsigned char *p_data, guint p_len, guint *p_offset, gboolean p_retEmpty)
{
	if(p_len < (*p_offset + 1))
		return NULL;

	guint8 slen = *(p_data + (*p_offset)++);
	if((!p_retEmpty && (slen <= 1)) || p_len < (*p_offset + slen - 1))
		return NULL;

	gchar *str = g_strndup((const gchar*)p_data + *p_offset, slen - 1);
	(*p_offset) += slen - 1;
	return str;
}

static void free_ase_player(gfire_sq_ase_player *p_player)
{
	g_free(p_player->name);
	g_free(p_player->team);
	g_free(p_player->skin);
	g_free(p_player->score);
	g_free(p_player->ping);
	g_free(p_player->time);
	g_free(p_player);
}

static void free_ase_data(gfire_sq_ase_data *p_data)
{
	g_free(p_data->game_name);
	g_free(p_data->server_name);
	g_free(p_data->game_type);
	g_free(p_data->map);
	g_free(p_data->version);

	if(p_data->rules)
		g_datalist_clear(&p_data->rules);

	while(p_data->players)
	{
		free_ase_player(p_data->players->data);
		p_data->players = g_slist_delete_link(p_data->players, p_data->players);
	}

	g_free(p_data);
}

static gboolean gfire_sq_ase_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	if(memcmp(p_data, "EYE1", 4) != 0)
		return FALSE;

	gfire_sq_ase_data *data = g_new0(gfire_sq_ase_data, 1);
	guint offset = 4;
	gchar *tempstr;

	// General server data
	data->game_name = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!data->game_name)
		goto error;

	tempstr = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!tempstr)
		goto error;
	data->game_port = atoi(tempstr);
	g_free(tempstr);

	data->server_name = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!data->server_name)
		goto error;

	data->game_type = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!data->game_type)
		goto error;

	data->map = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!data->map)
		goto error;

	data->version = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!data->version)
		goto error;

	tempstr = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!tempstr)
		goto error;
	data->password = (*tempstr == '1');
	g_free(tempstr);

	tempstr = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!tempstr)
		goto error;
	data->num_players = atoi(tempstr);
	g_free(tempstr);

	tempstr = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
	if(!tempstr)
		goto error;
	data->max_players = atoi(tempstr);
	g_free(tempstr);

	// Server rules
	g_datalist_init(&data->rules);
	while(1)
	{
		gchar *key = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
		if(!key)
			break;
		gchar *value = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, FALSE));
		if(!value)
		{
			g_free(key);
			goto error;
		}
		g_datalist_set_data_full(&data->rules, key, value, g_free);
		g_free(key);
	}

	// Players
	while(p_len > offset)
	{
		gfire_sq_ase_player *player = g_new0(gfire_sq_ase_player, 1);
		guint8 flags = *(p_data + offset++);

		if(flags & 1)
		{
			player->name = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->name)
			{
				free_ase_player(player);
				break;
			}
		}

		if(flags & 2)
		{
			player->team = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->team)
			{
				free_ase_player(player);
				break;
			}
		}

		if(flags & 4)
		{
			player->skin = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->skin)
			{
				free_ase_player(player);
				break;
			}
		}

		if(flags & 8)
		{
			player->score = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->score)
			{
				free_ase_player(player);
				break;
			}
		}

		if(flags & 16)
		{
			player->ping = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->ping)
			{
				free_ase_player(player);
				break;
			}
		}

		if(flags & 32)
		{
			player->time = gfire_strip_invalid_utf8(parse_string(p_data, p_len, &offset, TRUE));
			if(!player->time)
			{
				free_ase_player(player);
				break;
			}
		}

		data->players = g_slist_append(data->players, player);
	}

	// Hack for some servers...
	if(g_datalist_get_data(&data->rules, "numplayers"))
		data->num_players = atoi(g_datalist_get_data(&data->rules, "numplayers"));

	// Copy values into the standard container
	p_server->data = g_new0(gfire_game_server_data, 1);
	p_server->data->driver = &gf_sq_ase_driver;
	p_server->data->proto_data = data;
	p_server->data->name = gfire_sq_ase_strip_color_codes(data->server_name);
	p_server->data->map = g_strdup(data->map);
	p_server->data->players = data->num_players;
	p_server->data->max_players = data->max_players;
	p_server->data->ping = p_ping;

	// No more packets to request!
	return FALSE;

error:
	free_ase_data(data);
	return FALSE;
}

static gchar *gfire_sq_ase_fixed_len_string(const gchar *p_str, guint p_len)
{
	if(!p_str)
		return NULL;

	// Pad the string with spaces if necessary
	if(g_utf8_strlen(p_str, -1) < p_len)
	{
		gchar *str = g_strnfill(p_len + strlen(p_str) - g_utf8_strlen(p_str, -1), ' ');
		memcpy(str, p_str, strlen(p_str));
		return str;
	}
	// Copy max p_len bytes
	else if(g_utf8_strlen(p_str, -1) > p_len)
	{
		const gchar *end = g_utf8_offset_to_pointer(p_str, p_len - 3);
		gchar *str = g_strnfill(end - p_str + 3, '.');
		memcpy(str, p_str, end - p_str);
		return str;
	}
	else
		return g_strdup(p_str);
}

static void gfire_sq_ase_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	g_string_append_printf((GString*)p_udata, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), (gchar*)p_data);
}

static gchar *gfire_sq_ase_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);

	gfire_sq_ase_data *data = (gfire_sq_ase_data*)p_server->data->proto_data;

	// General server infos
	g_string_append(str, _("<b><font size=\"5\">General Server Details:</font></b><br>"));
	//	Server Name
	gchar *escaped = gfire_escape_html(data->server_name);
	gchar *name = gfire_sq_ase_color_codes_to_html(escaped);
	g_free(escaped);
	g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), name);
	g_free(name);
	//	Players
	g_string_append_printf(str, _("<b>Players:</b> %u/%u<br>"), p_server->data->players, p_server->data->max_players);
	//	Map
	escaped = gfire_escape_html(data->map);
	g_string_append_printf(str, _("<b>Map:</b> %s<br>"), escaped);
	g_free(escaped);
	//	Password secured
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"), data->password ? _("Yes") : _("No"));
	// Game Type
	g_string_append_printf(str, _("<b>Game Type:</b> %s<br>"), data->game_type);
	// Version
	g_string_append_printf(str, _("<b>Version:</b> %s"), data->version);

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br><font face=\"monospace\"><b>Name             Score      Ping</b><br>"));
	GSList *cur = data->players;
	while(cur)
	{
		gfire_sq_ase_player *player = (gfire_sq_ase_player*)cur->data;

		gchar *stripped = gfire_sq_ase_strip_color_codes(player->name ? player->name : _("N/A"));
		gchar *unescaped = gfire_sq_ase_fixed_len_string(stripped, 16);
		g_free(stripped);
		gchar *name = gfire_escape_html(unescaped);
		g_free(unescaped);

		unescaped = gfire_sq_ase_fixed_len_string(player->score ? player->score : _("N/A"), 10);
		gchar *score = gfire_escape_html(unescaped);
		g_free(unescaped);

		gchar *ping = gfire_escape_html(player->ping ? player->ping : _("N/A"));

		g_string_append_printf(str, "%s %s %s<br>", name, score, ping);

		g_free(ping);
		g_free(score);
		g_free(name);

		cur = g_slist_next(cur);
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Rules:</font></b><br>"));
	g_datalist_foreach(&data->rules, gfire_sq_ase_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_ase_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
		free_ase_data((gfire_sq_ase_data*)p_server->data->proto_data);
}

#endif // HAVE_GTK
