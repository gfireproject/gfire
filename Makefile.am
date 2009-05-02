EXTRA_DIST = config.rpath  config.rpath  COPYING \
			 ChangeLog \
			 INSTALL \
			 NEWS \
			 README \
			 VERSION \
			 gfire.spec \
			 gfire_config.h

SUBDIRS = data pixmaps po src

AM_CFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"
BUILT_SOURCES = gfire_config.h
$(OBJECTS): $(BUILT_SOURCES)

gfire_config.h: pre_config.h
	$(sedpath) 's/#define PACKAGE/#define SNPP_PACKAGE/g' pre_config.h > $@

ACLOCAL_AMFLAGS = -I m4
