/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _GF_GAMES_H
#define _GF_GAMES_H

typedef struct _gfire_linfo gfire_linfo;
typedef struct _gfire_xqf_linfo gfire_xqf_linfo;



struct _gfire_linfo
{
	int 	gameid;
	gchar	*name;
	gchar	*xqfname;
	gchar	*xqfmods;
	gchar	*c_bin;
	gchar	*c_wdir;
	gchar	*c_gmod;
	gchar	*c_connect;
	gchar	*c_options;
	gchar	*c_cmd;
};

struct _gfire_xqf_linfo
{
	gchar	*gtype;			/* qstat/xqf game type ie: xqfname in gfire_launch.xml */
	gchar	*sname;			/* server name */
	gchar	*ip;			/* ip address of server */
	int		port;			/* port to connect to game */
	gchar	*mod;			/* game mod ie: xqf modlist in gfire_launch.xml */
};


void gfire_xml_download_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const char *buf, gsize len, const gchar *error_message);
gboolean gfire_parse_games_file(PurpleConnection *gc, const char *filename);
char *gfire_game_name(PurpleConnection *gc, int game);
gboolean gfire_parse_launchinfo_file(PurpleConnection *gc, const char *filename);
gboolean gfire_game_playable(PurpleConnection *gc, int game);
int gfire_get_buddy_game(PurpleConnection *gc, PurpleBuddy *b);
int gfire_get_buddy_port(PurpleConnection *gc, PurpleBuddy *b);
const gchar *gfire_get_buddy_ip(PurpleConnection *gc, PurpleBuddy *b);
gfire_linfo *gfire_linfo_new();
void gfire_linfo_free(gfire_linfo *l);
gfire_linfo *gfire_linfo_get(PurpleConnection *gc, int game);
gchar *gfire_linfo_get_cmd(gfire_linfo *l, const guint8 *ip, int prt, const gchar *mod);
gfire_xqf_linfo *gfire_xqf_linfo_new();
void gfire_xqf_linfo_free(gfire_xqf_linfo *l);
int gfire_xqf_search(PurpleConnection *gc, gfire_xqf_linfo *xqfs);
gfire_xqf_linfo *gfire_linfo_parse_xqf(gchar *fname);
gchar *gfire_ipstr_to_bin(const gchar *ip);


#endif /* _GF_GAMES_H */
