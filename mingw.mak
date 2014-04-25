# The name of your windows compiler
WIN32_COMPILER = i686-w64-mingw32-gcc
WIN32_WINDRES = i686-w64-mingw32-windres
WIN32_STRIP = i686-w64-mingw32-strip

# The directory containing the GTK and GLib files
WIN32_DEV_DIR = ${GFIRE_DIR}/../pidgin_mingw/win32-dev
WIN32_GTK_DIR = ${WIN32_DEV_DIR}/gtk_2_0

# The directory containing pidgin
WIN32_PIDGIN_DIR = ${GFIRE_DIR}/../pidgin_mingw/pidgin-2.10.8

# Install dir (the files are copied there for creation of NSIS installer and .zip-file)
WIN32_INSTALL_DIR = ${GFIRE_DIR}/win32-install
WIN32_INSTALL_DIR_DEBUG = ${GFIRE_DIR}/win32-install-dbgsym

# Your msgfmt (part of GNU gettext) program
GMSGFMT = /usr/bin/gmsgfmt

# Your makensis program
MAKENSIS = /usr/bin/makensis

# Your C and LD flags
#USER_CFLAGS = -DDEBUG=1
USER_LDFLAGS =
