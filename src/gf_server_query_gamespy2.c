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

typedef struct _gfire_sq_gamespy2_player
{
	gchar *name;
	gint score;
	gint ping;
	gint deaths;
	gint kills;
	gint team;
} gfire_sq_gamespy2_player;

typedef struct _gfire_sq_gamespy2_data
{
	GData *info;
	GSList *players;
} gfire_sq_gamespy2_data;

// Prototypes
static void gfire_sq_gamespy2_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_gamespy2_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_gamespy2_details(gfire_game_server *p_server);
static void gfire_sq_gamespy2_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_gamespy2_driver =
{
	gfire_sq_gamespy2_query,
	gfire_sq_gamespy2_parse,
	gfire_sq_gamespy2_details,
	gfire_sq_gamespy2_free_server,
	3
};

static void gfire_sq_gamespy2_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static guint8 query[] = { 0xfe, 0xfd, 0x00, 'G', 'F', 'S', 'Q', 0xff, 0x00, 0x00 };
	if(p_full)
	{
		query[8] = 0xff;
		query[9] = 0xff;
	}
	else
	{
		query[8] = 0x00;
		query[9] = 0x00;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, 10, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static guint gfire_sq_gamespy2_parseInfo(gfire_sq_gamespy2_data *p_gsdata, const unsigned char *p_data, guint p_len)
{
	guint pos = 0;

	g_datalist_init(&p_gsdata->info);

	while(pos < p_len)
	{
		int keylen = strlen((const char*)p_data + pos);
		if(!keylen)
			break;

		const char *key = (const char*)p_data + pos;

		pos += keylen + 1;
		const char *value = (const char*)p_data + pos;

		pos += strlen((const char*)p_data + pos) + 1;

		g_datalist_set_data_full(&p_gsdata->info, key, g_strdup(value), g_free);
	}

	return pos + 2;
}

static guint gfire_sq_gamespy2_parsePlayers(gfire_sq_gamespy2_data *p_gsdata, const unsigned char *p_data, guint p_len)
{
	guint pos = 1;
	guchar playercount = *p_data;

	// Read keys
	GPtrArray *keys = g_ptr_array_new();
	while(pos < p_len)
	{
		int keylen = strlen((const char*)p_data + pos);
		if(!keylen)
			break;

		g_ptr_array_add(keys, (unsigned char*)p_data + pos);

		pos += keylen + 1;
	}
	pos++;

	// Read players
	gfire_sq_gamespy2_player *player = NULL;
	int cur = 0;
	for(; (pos < p_len) && (cur < playercount); cur++)
	{
		player = g_new0(gfire_sq_gamespy2_player, 1);

		int key = 0;
		while(pos < p_len)
		{
			const char *value = (const char*)p_data + pos;
			pos += strlen(value) + 1;

			if(g_strcmp0(g_ptr_array_index(keys, key), "player_") == 0)
				player->name = g_strdup(value);
			else if(g_strcmp0(g_ptr_array_index(keys, key), "score_") == 0)
				sscanf(value, "%d", &player->score);
			else if(g_strcmp0(g_ptr_array_index(keys, key), "deaths_") == 0)
				sscanf(value, "%d", &player->deaths);
			else if(g_strcmp0(g_ptr_array_index(keys, key), "ping_") == 0)
				sscanf(value, "%d", &player->ping);
			else if(g_strcmp0(g_ptr_array_index(keys, key), "team_") == 0)
				sscanf(value, "%d", &player->team);
			else if(g_strcmp0(g_ptr_array_index(keys, key), "kills_") == 0)
				sscanf(value, "%d", &player->kills);

			key = (key + 1) % keys->len;
			if(key == 0)
				break;
		}

		p_gsdata->players = g_slist_append(p_gsdata->players, player);
	}

	g_ptr_array_free(keys, TRUE);

	return pos + 2;
}

static gboolean gfire_sq_gamespy2_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	// Check header
	if(p_data[0] != 0 || memcmp(p_data + 1, "GFSQ", 4) != 0)
		return FALSE;

	p_server->data = g_new0(gfire_game_server_data, 1);
	p_server->data->driver = &gf_sq_gamespy2_driver;
	p_server->data->ping = p_ping;

	gfire_sq_gamespy2_data *gsdata = p_server->data->proto_data = g_new0(gfire_sq_gamespy2_data, 1);

	// Parse info
	guint offset = 5;
	offset += gfire_sq_gamespy2_parseInfo(gsdata, p_data + offset, p_len - offset);

	if(g_datalist_get_data(&gsdata->info, "hostname"))
		p_server->data->name = g_strdup(g_datalist_get_data(&gsdata->info, "hostname"));
	if(g_datalist_get_data(&gsdata->info, "mapname"))
		p_server->data->map = g_strdup(g_datalist_get_data(&gsdata->info, "mapname"));
	if(g_datalist_get_data(&gsdata->info, "numplayers"))
		sscanf(g_datalist_get_data(&gsdata->info, "numplayers"), "%u", &p_server->data->players);
	if(g_datalist_get_data(&gsdata->info, "maxplayers"))
		sscanf(g_datalist_get_data(&gsdata->info, "maxplayers"), "%u", &p_server->data->max_players);

	if(p_full)
	{
		// Parse players
		offset += gfire_sq_gamespy2_parsePlayers(gsdata, p_data + offset, p_len - offset);
	}

	return FALSE;
}

static gchar *gfire_sq_gamespy2_fixed_len_string(const gchar *p_str, guint p_len)
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

static void gfire_sq_gamespy2_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	gchar *escaped = gfire_escape_html((gchar*)p_data);
	g_string_append_printf((GString*)p_udata, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), escaped);
	g_free(escaped);
}

static gchar *gfire_sq_gamespy2_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);

	gfire_sq_gamespy2_data *data = (gfire_sq_gamespy2_data*)p_server->data->proto_data;

	// General server infos
	g_string_append(str, _("<b><font size=\"5\">General Server Details:</font></b><br>"));
	//	Server Name
	gchar *escaped = gfire_escape_html(p_server->data->name);
	g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), escaped);
	g_free(escaped);
	//	Players
	g_string_append_printf(str, _("<b>Players:</b> %u/%u<br>"), p_server->data->players, p_server->data->max_players);
	//	Map
	escaped = gfire_escape_html(p_server->data->map);
	g_string_append_printf(str, _("<b>Map:</b> %s<br>"), escaped);
	g_free(escaped);
	//	Password secured
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"), (g_strcmp0("True", g_datalist_get_data(&data->info, "password")) == 0) ||
						   (g_strcmp0("1", g_datalist_get_data(&data->info, "password")) == 0)
						   ? _("Yes") : _("No"));
	// Game Type
	g_string_append_printf(str, _("<b>Game Type:</b> %s<br>"), g_datalist_get_data(&data->info, "gametype") ?
							   (gchar*)g_datalist_get_data(&data->info, "gametype") : _("N/A"));
	// Version
	g_string_append_printf(str, _("<b>Version:</b> %s"), g_datalist_get_data(&data->info, "gamever") ?
							   (gchar*)g_datalist_get_data(&data->info, "gamever") : _("N/A"));

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br><font face=\"monospace\"><b>Name             Score      Ping</b><br>"));
	GSList *cur = data->players;
	while(cur)
	{
		gfire_sq_gamespy2_player *player = (gfire_sq_gamespy2_player*)cur->data;

		gchar *unescaped = gfire_sq_gamespy2_fixed_len_string(player->name, 16);
		gchar *name = gfire_escape_html(unescaped);
		g_free(unescaped);

		g_string_append_printf(str, "%s %-10d %d<br>", name, player->score, player->ping);

		g_free(name);

		cur = g_slist_next(cur);
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Info:</font></b><br>"));
	g_datalist_foreach(&data->info, gfire_sq_gamespy2_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_gamespy2_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
	{
		gfire_sq_gamespy2_data *data = (gfire_sq_gamespy2_data*)p_server->data->proto_data;

		g_datalist_clear(&data->info);

		while(data->players)
		{
			gfire_sq_gamespy2_player *player = (gfire_sq_gamespy2_player*)data->players->data;
			g_free(player->name);
			g_free(player);

			data->players = g_slist_delete_link(data->players, data->players);
		}

		g_free(data);
	}
}

#endif // HAVE_GTK
