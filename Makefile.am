EXTRA_DIST = config.rpath  config.rpath  COPYING \
			 ChangeLog \
			 INSTALL \
			 NEWS \
			 README \
			 VERSION \
			 gfire.spec

if INSTALL_NLS
PO_DIR = po
endif

SUBDIRS = data pixmaps $(PO_DIR) src

AM_CFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"

ACLOCAL_AMFLAGS = -I m4
