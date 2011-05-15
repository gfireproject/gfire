EXTRA_DIST = config.rpath  config.rpath  COPYING \
			 ChangeLog \
			 INSTALL \
			 NEWS \
			 README \
			 VERSION \
			 REV \
			 gfire.spec

SUBDIRS = data pixmaps src

if INSTALL_NLS
SUBDIRS += po
endif

AM_CFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"

ACLOCAL_AMFLAGS = -I m4
