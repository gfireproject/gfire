/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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

#define GFSQ_GAMESPY_STAGE_INFO		1
#define GFSQ_GAMESPY_STAGE_RULES	2
#define GFSQ_GAMESPY_STAGE_PLAYERS	3

typedef struct _gfire_sq_gamespy_player
{
	gchar *name;
	gint frags;
	gint ping;
	gint16 team;
	gchar *mesh;
	gchar *skin;
	gchar *face;
	gboolean ngsecret;
} gfire_sq_gamespy_player;

typedef struct _gfire_sq_gamespy_data
{
	// Query handling
	guint8 query_stage;
	guint query_id;
	guint cur_fragments;
	guint fragments;

	// Extended data
	GData *info;
	GData *rules;
	GData *player_data;
	GSList *players;
} gfire_sq_gamespy_data;

// Prototypes
static void gfire_sq_gamespy_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_gamespy_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_gamespy_details(gfire_game_server *p_server);
static void gfire_sq_gamespy_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_gamespy_driver =
{
	gfire_sq_gamespy_query,
	gfire_sq_gamespy_parse,
	gfire_sq_gamespy_details,
	gfire_sq_gamespy_free_server,
	3
};

static void gfire_sq_gamespy_data_free(gfire_sq_gamespy_data *p_data)
{
	g_datalist_clear(&p_data->info);
	g_datalist_clear(&p_data->rules);
	g_datalist_clear(&p_data->player_data);

	while(p_data->players)
	{
		gfire_sq_gamespy_player *player = (gfire_sq_gamespy_player*)p_data->players->data;
		g_free(player->name);
		g_free(player->mesh);
		g_free(player->skin);
		g_free(player->face);
		g_free(player);
		p_data->players = g_slist_delete_link(p_data->players, p_data->players);
	}
	g_free(p_data);
}

static void gfire_sq_gamespy_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static const gchar info_query[] = "\\info\\";
	static const gchar rules_query[] = "\\rules\\";
	static const gchar players_query[] = "\\players\\";
	const gchar *query = NULL;

	if(!p_server->data)
		query = info_query;
	else
	{
		gfire_sq_gamespy_data *data = (gfire_sq_gamespy_data*)p_server->data->proto_data;
		if(data->query_stage == GFSQ_GAMESPY_STAGE_RULES && !data->query_id)
			query = rules_query;
		else if(data->query_stage == GFSQ_GAMESPY_STAGE_PLAYERS && !data->query_id)
			query = players_query;
		else
			return;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, strlen(query), 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gboolean gfire_sq_gamespy_parse_chunk(gfire_sq_gamespy_data *p_sq_data, GData **p_datalist,
											 const unsigned char *p_data, guint p_len, gboolean *p_done)
{
	if(*p_data != '\\')
		return FALSE;

	// Make sure the data is zero-terminated
	gchar *strdata = g_strndup((const gchar*)p_data, p_len);

	// Parse all key/value pairs
	gboolean final = FALSE;
	gchar **chunks = g_strsplit(strdata + 1, "\\", -1);
	g_free(strdata);
	gchar **cur = chunks;
	while(*cur)
	{
		gchar *key = *cur++;
		if(g_strcmp0(key, "final") == 0)
			final = TRUE;
		else if(!strlen(key))
			break;

		gchar *value = *cur++;
		if(!value)
			break;

		g_datalist_set_data_full(p_datalist, key, g_strdup(gfire_strip_invalid_utf8(value)), g_free);
	}
	g_strfreev(chunks);

	// Get query id and fragment number
	gchar *queryid = g_datalist_get_data(p_datalist, "queryid");
	if(!queryid)
		return FALSE;

	guint id, fragment;
	if(sscanf(queryid, "%u.%u", &id, &fragment) != 2)
		return FALSE;

	if(p_sq_data->query_id && p_sq_data->query_id != id)
		return FALSE;
	else
		p_sq_data->query_id = id;

	p_sq_data->cur_fragments++;
	if(final)
		p_sq_data->fragments = fragment;

	if(p_sq_data->cur_fragments == p_sq_data->fragments)
		*p_done = TRUE;

	return TRUE;
}

static gboolean gfire_sq_gamespy_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	if(!p_server->data)
	{
		gfire_sq_gamespy_data *data = g_new0(gfire_sq_gamespy_data, 1);
		data->query_stage = GFSQ_GAMESPY_STAGE_INFO;
		g_datalist_init(&data->info);
		g_datalist_init(&data->rules);
		g_datalist_init(&data->player_data);

		p_server->data = g_new0(gfire_game_server_data, 1);
		p_server->data->driver = &gf_sq_gamespy_driver;
		p_server->data->proto_data = data;
		p_server->data->ping = p_ping;
	}

	gfire_sq_gamespy_data *data = (gfire_sq_gamespy_data*)p_server->data->proto_data;

	gboolean done = FALSE;
	if(data->query_stage == GFSQ_GAMESPY_STAGE_INFO)
	{
		if(!gfire_sq_gamespy_parse_chunk(data, &data->info, p_data, p_len, &done))
		{
			gfire_sq_gamespy_data_free(data);
			g_free(p_server->data);
			p_server->data = NULL;
			return FALSE;
		}

		if(done)
		{
			p_server->data->name = gfire_strip_character_range(g_strdup(g_datalist_get_data(&data->info, "hostname")), 0x01, 0x1f);
			p_server->data->map = g_strdup(g_datalist_get_data(&data->info, "mapname"));
			p_server->data->ping = p_ping;
			if(g_datalist_get_data(&data->info, "numplayers"))
				sscanf(g_datalist_get_data(&data->info, "numplayers"), "%u", &p_server->data->players);
			if(g_datalist_get_data(&data->info, "maxplayers"))
				sscanf(g_datalist_get_data(&data->info, "maxplayers"), "%u", &p_server->data->max_players);

			// Go on with requesting if desired
			if(p_full)
			{
				data->query_stage = GFSQ_GAMESPY_STAGE_RULES;
				data->query_id = 0;
				data->cur_fragments = 0;
				data->fragments = 0;
				return TRUE;
			}
		}
		else
			return TRUE;
	}
	else if(data->query_stage == GFSQ_GAMESPY_STAGE_RULES)
	{
		if(!gfire_sq_gamespy_parse_chunk(data, &data->rules, p_data, p_len, &done))
		{
			gfire_sq_gamespy_data_free(data);
			g_free(p_server->data->name);
			g_free(p_server->data->map);
			g_free(p_server->data);
			p_server->data = NULL;
			return FALSE;
		}

		if(done)
		{
			g_datalist_set_data_full(&data->rules, "queryid", NULL, NULL);
			g_datalist_set_data_full(&data->rules, "final", NULL, NULL);

			data->query_stage = GFSQ_GAMESPY_STAGE_PLAYERS;
			data->query_id = 0;
			data->cur_fragments = 0;
			data->fragments = 0;
			return TRUE;
		}
		else
			return TRUE;
	}
	else if(data->query_stage == GFSQ_GAMESPY_STAGE_PLAYERS)
	{
		if(!gfire_sq_gamespy_parse_chunk(data, &data->player_data, p_data, p_len, &done))
		{
			gfire_sq_gamespy_data_free(data);
			g_free(p_server->data->name);
			g_free(p_server->data->map);
			g_free(p_server->data);
			p_server->data = NULL;
			return FALSE;
		}

		if(!done)
			return TRUE;
		else
		{
			int i;
			for(i = 0; i < p_server->data->players; i++)
			{
				gfire_sq_gamespy_player *player = g_new0(gfire_sq_gamespy_player, 1);

				gchar tmp[30];
				const gchar *value;
				snprintf(tmp, 30, "player_%d", i);
				player->name = gfire_strip_character_range(g_strdup(g_datalist_get_data(&data->player_data, tmp) ?
											g_datalist_get_data(&data->player_data, tmp) : _("N/A")), 0x01, 0x1f);

				snprintf(tmp, 30, "frags_%d", i);
				value = g_datalist_get_data(&data->player_data, tmp);
				if(value)
					sscanf(value, "%d", &player->frags);

				snprintf(tmp, 30, "ping_%d", i);
				value = g_datalist_get_data(&data->player_data, tmp);
				if(value)
					sscanf(value, "%d", &player->ping);

				snprintf(tmp, 30, "team_%d", i);
				value = g_datalist_get_data(&data->player_data, tmp);
				if(value)
					sscanf(value, "%hd", &player->team);

				snprintf(tmp, 30, "mesh_%d", i);
				player->mesh = g_strdup(g_datalist_get_data(&data->player_data, tmp) ?
											g_datalist_get_data(&data->player_data, tmp) : _("N/A"));

				snprintf(tmp, 30, "skin_%d", i);
				player->skin = g_strdup(g_datalist_get_data(&data->player_data, tmp) ?
											g_datalist_get_data(&data->player_data, tmp) : _("N/A"));

				snprintf(tmp, 30, "face_%d", i);
				player->face = g_strdup(g_datalist_get_data(&data->player_data, tmp) ?
											g_datalist_get_data(&data->player_data, tmp) : _("N/A"));

				snprintf(tmp, 30, "ngsecret_%d", i);
				value = g_datalist_get_data(&data->player_data, tmp);
				if(value && g_strcmp0(value, "true") == 0)
					player->ngsecret = TRUE;

				data->players = g_slist_append(data->players, player);
			}
		}
	}

	return FALSE;
}

static gchar *gfire_sq_gamespy_fixed_len_string(const gchar *p_str, guint p_len)
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

static void gfire_sq_gamespy_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	gchar *escaped = gfire_escape_html((gchar*)p_data);
	g_string_append_printf((GString*)p_udata, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), escaped);
	g_free(escaped);
}

static gchar *gfire_sq_gamespy_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);

	gfire_sq_gamespy_data *data = (gfire_sq_gamespy_data*)p_server->data->proto_data;

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
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"), (g_strcmp0("True", g_datalist_get_data(&data->rules, "password")) == 0) ||
						   (g_strcmp0("1", g_datalist_get_data(&data->rules, "password")) == 0)
						   ? _("Yes") : _("No"));
	// Game Type
	g_string_append_printf(str, _("<b>Game Type:</b> %s<br>"), g_datalist_get_data(&data->info, "gametype") ?
							   (gchar*)g_datalist_get_data(&data->info, "gametype") : _("N/A"));
	// Version
	g_string_append_printf(str, _("<b>Version:</b> %s"), g_datalist_get_data(&data->info, "gamever") ?
							   (gchar*)g_datalist_get_data(&data->info, "gamever") : _("N/A"));

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br><font face=\"monospace\"><b>Name             Frags      Ping</b><br>"));
	GSList *cur = data->players;
	while(cur)
	{
		gfire_sq_gamespy_player *player = (gfire_sq_gamespy_player*)cur->data;

		gchar *unescaped = gfire_sq_gamespy_fixed_len_string(player->name, 16);
		gchar *name = gfire_escape_html(unescaped);
		g_free(unescaped);

		g_string_append_printf(str, "%s %-10d %d<br>", name, player->frags, player->ping);

		g_free(name);

		cur = g_slist_next(cur);
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Rules:</font></b><br>"));
	g_datalist_foreach(&data->rules, gfire_sq_gamespy_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_gamespy_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
		gfire_sq_gamespy_data_free((gfire_sq_gamespy_data*)p_server->data->proto_data);
}
