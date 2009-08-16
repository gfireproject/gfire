EXTRA_DIST = \
	gfire.h \
	gfire_proto.h \
	debug.h \
	gf_network.h \
	gf_games.h \
	gf_chat.h \
	gf_chat_proto.h \
	gf_friend_search.h \
	gf_friend_search_proto.h \
	gf_query.h \
	gf_protocol.h \
	gf_purple.h \
	gf_menus.h \
	gf_util.h \
	gf_server_detection.h \
	gf_server_detection_linux.h \
	gf_server_detection_win.h \
	gf_server_detection_win.c \
	gf_game_detection.h \
	gf_game_detection_linux.h \
	gf_game_detection_win.h \
	gf_game_detection_win.c \
	gf_server_browser.h \
	gf_server_browser_proto.h \
	gf_buddies.h \
	gf_buddies_proto.h

INCLUDES = \
	-I$(top_srcdir) \
    -DLOCALEDIR=\""$(localedir)"\"

plugindir = $(libdir)/purple-2
libxfire_la_LDFLAGS = -module -avoid-version

if PLUGINS
plugin_LTLIBRARIES = libxfire.la

libxfire_la_SOURCES = \
	gf_buddies.c \
	gf_buddies_proto.c \
	gf_chat.c \
	gf_chat_proto.c \
	gf_friend_search.c \
	gf_friend_search_proto.c \
	gf_games.c \
	gf_network.c \
	gf_protocol.c \
	gf_purple.c \
	gf_query.c \
	gfire.c \
	gfire_proto.c \
	gf_server_browser.c \
	gf_server_browser_proto.c \
	gf_menus.c \
	gf_util.c \
	gf_server_detection_linux.c \
	gf_game_detection.c \
	gf_game_detection_linux.c

libxfire_la_LIBADD = \
	$(GLIB_LIBS) $(GTK_LIBS) $(SSL_LIBS)
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(libdir)/purple-2/\" \
	-DDATADIR=\"$(datadir)\" \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(SSL_CFLAGS) \
	$(DEBUG_CFLAGS)

AM_CFLAGS = -DPURPLE_PLUGINS
