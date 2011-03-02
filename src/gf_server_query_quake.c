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

typedef struct _gfire_sq_quake_player
{
	gint score, ping;
	gchar *name;
} gfire_sq_quake_player;

typedef struct _gfire_sq_quake_data
{
	GData *info;
	GSList *players;
} gfire_sq_quake_data;

static const struct { gchar ch; const gchar *hex; } q3codes[] =
{
	{ '0', "000000" }, { '1', "DA0120" }, { '2', "00B906" }, { '3', "E8FF19" },
	{ '4', "170BDB" }, { '5', "23C2C6" }, { '6', "E201DB" }, { '7', "FFFFFF" },
	{ '8', "CA7C27" }, { '9', "757575" }, { 'a', "EB9F53" }, { 'b', "106F59" },
	{ 'c', "5A134F" }, { 'd', "035AFF" }, { 'e', "681EA7" }, { 'f', "5097C1" },
	{ 'g', "BEDAC4" }, { 'h', "024D2C" }, { 'i', "7D081B" }, { 'j', "90243E" },
	{ 'k', "743313" }, { 'l', "A7905E" }, { 'm', "555C26" }, { 'n', "AEAC97" },
	{ 'o', "C0BF7F" }, { 'p', "000000" }, { 'q', "DA0120" }, { 'r', "00B906" },
	{ 's', "E8FF19" }, { 't', "170BDB" }, { 'u', "23C2C6" }, { 'v', "E201DB" },
	{ 'w', "FFFFFF" }, { 'x', "CA7C27" }, { 'y', "757575" }, { 'z', "CC8034" },
	{ '/', "DBDF70" }, { '*', "BBBBBB" }, { '-', "747228" }, { '+', "993400" },
	{ '?', "670504" }, { '@', "623307" }, { 0, NULL }
};

// Prototypes
static void gfire_sq_quake_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_quake_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_quake_details(gfire_game_server *p_server);
static void gfire_sq_quake_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_quake_driver =
{
	gfire_sq_quake_query,
	gfire_sq_quake_parse,
	gfire_sq_quake_details,
	gfire_sq_quake_free_server,
	3
};

static void gfire_sq_quake_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	if(p_server->data)
		return;

	static const unsigned char query[] =
	{ 0xff, 0xff, 0xff, 0xff, 'g', 'e', 't', 's', 't', 'a', 't', 'u', 's', 0x0a };

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, 14, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gchar *gfire_sq_quake_read_info_chunk(const unsigned char *p_data, guint p_len, guint *p_offset)
{
	if(*(p_data + *p_offset) != '\\')
		return NULL;
	(*p_offset)++;

	const unsigned char *start = p_data + *p_offset;
	while(*p_offset < p_len && *(p_data + *p_offset) != '\\' && *(p_data + *p_offset) != '\n')
		(*p_offset)++;

	return g_strndup((gchar*)start, (p_data + *p_offset) - start);
}

static gboolean gfire_sq_quake_parse_info(gfire_sq_quake_data *p_qdata, const unsigned char *p_data, guint p_len, guint *p_offset)
{
	gchar *key = NULL;
	while((key = gfire_sq_quake_read_info_chunk(p_data, p_len, p_offset)))
	{
		gchar *value = gfire_sq_quake_read_info_chunk(p_data, p_len, p_offset);
		if(!value)
			return FALSE;

		g_datalist_set_data_full(&p_qdata->info, key, value, g_free);
		g_free(key);
	}

	if(*(p_data + *p_offset) == '\n')
		return TRUE;
	else
		return FALSE;
}

static gchar *gfire_sq_quake_read_player_chunk(const unsigned char *p_data, guint p_len, guint *p_offset)
{
	if(*(p_data + *p_offset) != '\n')
		return NULL;
	(*p_offset)++;

	const unsigned char *start = p_data + *p_offset;
	while(*p_offset < p_len && *(p_data + *p_offset) != '\n')
		(*p_offset)++;

	return g_strndup((gchar*)start, (p_data + *p_offset) - start);
}

static gchar **gfire_sq_quake_split_player_chunk(const gchar *p_chunk)
{
	gchar **ret = g_malloc0(sizeof(gchar*));

	guint values = 0;
	guint pos = 0;
	gchar delim;

	GString *value = NULL;
	while(p_chunk[pos] != 0)
	{
		value = g_string_new(NULL);

		if(p_chunk[pos] == '"')
		{
			pos++;
			delim = '"';
		}
		else
			delim = ' ';

		while(p_chunk[pos] != 0 && p_chunk[pos] != delim)
		{
			g_string_append_c(value, p_chunk[pos]);
			pos++;
		}

		values++;
		ret = g_realloc(ret, (values + 1) * sizeof(gchar*));
		ret[values - 1] = g_string_free(value, FALSE);
		ret[values] = NULL;

		if(p_chunk[pos] == 0)
		{
			if(delim == '"')
			{
				g_strfreev(ret);
				return NULL;
			}
			else
				return ret;
		}

		pos++;
	}

	return ret;
}

static gboolean gfire_sq_quake_parse_players(gfire_sq_quake_data *p_qdata, const unsigned char *p_data, guint p_len, guint *p_offset)
{
	gchar *chunk = NULL;
	while(*p_offset < p_len && (chunk = gfire_sq_quake_read_player_chunk(p_data, p_len, p_offset)))
	{
		gchar **player_chunks = gfire_sq_quake_split_player_chunk(chunk);
		g_free(chunk);

		if(!player_chunks)
		{
			while(p_qdata->players)
			{
				gfire_sq_quake_player *player = (gfire_sq_quake_player*)p_qdata->players->data;
				g_free(player->name);
				g_free(player);
				p_qdata->players = g_slist_delete_link(p_qdata->players, p_qdata->players);
			}
			return FALSE;
		}

		if(g_strv_length(player_chunks) >= 3)
		{
			gfire_sq_quake_player *player = g_malloc0(sizeof(gfire_sq_quake_player));
			p_qdata->players = g_slist_append(p_qdata->players, player);

			player->score = g_ascii_strtoll(player_chunks[0], NULL, 10);
			player->ping = g_ascii_strtoll(player_chunks[1], NULL, 10);
			player->name = g_strdup(player_chunks[2]);
		}

		g_strfreev(player_chunks);
	}

	return TRUE;
}

static gboolean gfire_sq_quake_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	guint offset = 19;

	static const unsigned char respMagic[] =
	{ 0xff, 0xff, 0xff, 0xff, 's', 't', 'a', 't', 'u', 's', 'R', 'e', 's', 'p', 'o', 'n', 's', 'e', 0x0a };
	if(memcmp(p_data, respMagic, 19) != 0)
		return FALSE;

	gfire_sq_quake_data *data = g_malloc0(sizeof(gfire_sq_quake_data));

	// Parse general server infos
	g_datalist_init(&data->info);
	if(!gfire_sq_quake_parse_info(data, p_data, p_len, &offset))
	{
		g_datalist_clear(&data->info);
		g_free(data);
		return FALSE;
	}

	// Parse players
	if(!gfire_sq_quake_parse_players(data, p_data, p_len, &offset))
	{
		g_datalist_clear(&data->info);
		g_free(data);
		return FALSE;
	}

	// Parsing done, store default values
	p_server->data = g_malloc0(sizeof(gfire_game_server_data));

	p_server->data->driver = &gf_sq_quake_driver;
	p_server->data->proto_data = data;

	p_server->data->name = gfire_remove_quake3_color_codes(g_datalist_get_data(&data->info, "sv_hostname"));
	p_server->data->map = g_strdup(g_datalist_get_data(&data->info, "mapname"));
	const gchar *max_players = g_datalist_get_data(&data->info, "sv_maxclients");
	p_server->data->max_players = max_players ? g_ascii_strtoll(max_players, NULL, 10) : 0;
	p_server->data->players = g_slist_length(data->players);
	p_server->data->ping = p_ping;

	// No further queries
	return FALSE;
}

static gchar *gfire_sq_quake_color_codes_to_html(const gchar *p_str)
{
	if(!p_str)
		return NULL;

	gchar **chunks = g_strsplit(p_str, "^", -1);

	GString *str = g_string_new(NULL);
	int i = 0;
	for(; i < g_strv_length(chunks); i++)
	{
		if(i > 0)
		{
			int j = 0;
			while(q3codes[j].ch)
			{
				if(g_ascii_tolower(chunks[i][0]) == q3codes[j].ch)
				{
					g_string_append_printf(str, "<font color=\"#%s\">%s</font>", q3codes[j].hex, chunks[i] + 1);
					break;
				}
				j++;
			}
			if(!q3codes[j].ch)
				g_string_append(g_string_append_c(str, '^'), chunks[i]);
		}
		else
			g_string_append(str, chunks[i]);
	}

	g_strfreev(chunks);
	return g_string_free(str, FALSE);
}

static void gfire_sq_quake_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	GString *str = (GString*)p_udata;
	gchar *value = gfire_sq_quake_color_codes_to_html((gchar*)p_data);

	g_string_append_printf(str, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), value);

	g_free(value);
}

static gchar *gfire_sq_quake_fixed_len_string(const gchar *p_str, guint p_len)
{
	if(!p_str)
		return NULL;

	// Get the length without Quake 3 color codes
	guint len = 0;
	guint pos = 0;
	while(p_str[pos] != 0)
	{
		if(p_str[pos] == '^')
		{
			guint i = 0;
			while(q3codes[i].ch)
			{
				if(q3codes[i].ch == g_ascii_tolower(p_str[pos + 1]))
					break;
				i++;
			}
			if(q3codes[i].ch)
			{
				pos += 2;
				continue;
			}
		}
		pos++;
		len++;
	}

	// Pad the string with spaces if necessary
	if(len < p_len)
	{
		gchar *str = g_strnfill(p_len + strlen(p_str) - len, ' ');
		memcpy(str, p_str, strlen(p_str));
		return str;
	}
	// Copy max p_len (uncolored) bytes
	else if(len > p_len)
	{
		GString *str = g_string_sized_new(p_len);

		pos = 0;
		guint copied = 0;
		while(p_str[pos] != 0 && copied < (p_len - 3))
		{
			if(p_str[pos] == '^')
			{
				guint i = 0;
				while(q3codes[i].ch)
				{
					if(q3codes[i].ch == g_ascii_tolower(p_str[pos + 1]))
						break;
					i++;
				}
				if(q3codes[i].ch)
				{
					pos += 2;
					continue;
				}
			}

			g_string_append_c(str, p_str[pos]);
			copied++;
			pos++;
		}
		g_string_append(str, "...");

		return g_string_free(str, FALSE);
	}
	else
		return g_strdup(p_str);
}

static gchar *gfire_sq_quake_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);

	gfire_sq_quake_data *data = (gfire_sq_quake_data*)p_server->data->proto_data;

	// General server infos
	g_string_append(str, _("<b><font size=\"5\">General Server Details:</font></b><br>"));
	//	Server Name
	if(g_datalist_get_data(&data->info, "sv_hostname"))
	{
		gchar *escaped = gfire_escape_html((gchar*)g_datalist_get_data(&data->info, "sv_hostname"));
		gchar *name = gfire_sq_quake_color_codes_to_html(escaped);
		g_free(escaped);
		g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), name);
		g_free(name);
	}
	//	Players
	g_string_append_printf(str, _("<b>Players:</b> %u/%u<br>"), p_server->data->players, p_server->data->max_players);
	//	Map
	if(g_datalist_get_data(&data->info, "mapname"))
	{
		gchar *escaped = gfire_escape_html(g_datalist_get_data(&data->info, "mapname"));
		g_string_append_printf(str, _("<b>Map:</b> %s<br>"), escaped);
		g_free(escaped);
	}
	//	Password secured
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"),
						   g_datalist_get_data(&data->info, "g_needpass") ?
						   ((((gchar*)g_datalist_get_data(&data->info, "g_needpass"))[0] != '0') ? _("Yes") : _("No")) :
						   _("No"));
	// Game name
	if(g_datalist_get_data(&data->info, "gamename"))
		g_string_append_printf(str, _("<b>Game Name:</b> %s"), (gchar*)g_datalist_get_data(&data->info, "gamename"));

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br><font face=\"monospace\"><b>Name             Score      Ping</b><br>"));
	GSList *cur = data->players;
	while(cur)
	{
		gfire_sq_quake_player *player = (gfire_sq_quake_player*)cur->data;

		gchar *name = gfire_sq_quake_fixed_len_string(player->name, 16);
		gchar *escaped = gfire_escape_html(name);
		g_free(name);
		gchar *name_html = gfire_sq_quake_color_codes_to_html(escaped);
		g_free(escaped);

		gchar *score = g_strdup_printf("%u", player->score);
		gchar *score_fixed = gfire_sq_quake_fixed_len_string(score, 10);
		g_free(score);

		g_string_append_printf(str, "%s %s %u<br>", name_html, score_fixed, player->ping);

		g_free(score_fixed);
		g_free(name_html);

		cur = g_slist_next(cur);
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Rules:</font></b><br>"));
	g_datalist_foreach(&data->info, gfire_sq_quake_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_quake_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
	{
		gfire_sq_quake_data *data = (gfire_sq_quake_data*)p_server->data->proto_data;
		g_datalist_clear(&data->info);

		while(data->players)
		{
			gfire_sq_quake_player *player = (gfire_sq_quake_player*)data->players->data;
			g_free(player->name);
			g_free(player);
			data->players = g_slist_delete_link(data->players, data->players);
		}
		g_free(data);
	}
}

#endif // HAVE_GTK
