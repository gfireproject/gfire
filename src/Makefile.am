EXTRA_DIST = \
	gfire.h \
	debug.h \
	gf_network.h \
	gf_packet.h \
	gf_games.h \
	gf_chat.h

plugindir = $(libdir)/purple-2
libxfire_la_LDFLAGS = -module -avoid-version

if PLUGINS
plugin_LTLIBRARIES = libxfire.la

libxfire_la_SOURCES = \
	gfire.c \
	gf_network.c \
	gf_packet.c \
	gf_games.c \
	gf_chat.c

libxfire_la_LIBADD = \
	$(GLIB_LIBS) $(SSL_LIBS)
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(libdir)/purple-2/\" \
	-DDATADIR=\"$(datadir)\" \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(SSL_CFLAGS) \
	$(DEBUG_CFLAGS)

AM_CFLAGS = -DPURPLE_PLUGINS
