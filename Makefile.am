EXTRA_DIST = COPYING \
			 ChangeLog \
			 INSTALL \
			 NEWS \
			 README \
			 VERSION \
			 REV \
			 gfire_config_win.h.in \
			 Makefile.mingw \
			 po/Makefile.mingw \
			 doc/README \
			 nsis/gfire-installer.nsi \
			 nsis/gfx/gfire.ico \
			 nsis/gfx/gfire-header.bmp \
			 nsis/gfx/gfire-intro.bmp

SUBDIRS = data pixmaps src

if INSTALL_NLS
SUBDIRS += po
endif

AM_CFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"

ACLOCAL_AMFLAGS = -I m4
