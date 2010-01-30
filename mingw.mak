# The name of your windows compiler
WIN32_COMPILER = i686-mingw32-gcc
WIN32_WINDRES = i686-mingw32-windres

# The directory containing the GTK and GLib files
WIN32_DEV_DIR = ${GFIRE_DIR}/../../pidgin_mingw/win32-dev
WIN32_GTK_DIR = ${WIN32_DEV_DIR}/gtk_2_0

# The directory containing pidgin
WIN32_PIDGIN_DIR = ${GFIRE_DIR}/../../pidgin_mingw/pidgin-2.6.1

# Install dir (the files are copied there for creation of NSIS installer and .zip-file)
WIN32_INSTALL_DIR = ${GFIRE_DIR}/win32-install

# Your msgfmt (part of GNU gettext) program
GMSGFMT = /usr/bin/gmsgfmt

# Your makensis program
MAKENSIS = /usr/bin/makensis