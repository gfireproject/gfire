#
# Most of this configure script was ripped from gaim-netsoul,
# thank you guys!!!
#

# Get Gfire version
m4_define(gfire_version, regexp(m4_include(VERSION), [\([0-9a-zA-Z.-]+\)], [\1]))
m4_define(gfire_version_patch, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\3]))
m4_define(gfire_version_minor, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\2]))
m4_define(gfire_version_major, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\1]))
m4_define(gfire_version_suffix, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)-?\([a-zA-Z0-9]+\)], [\4]))
m4_define(gfire_revision, regexp(m4_include(REV), [\([0-9a-zA-Z.-]+\)], [\1]))

AC_PREREQ(2.50)
AC_INIT([Gfire], [gfire_version], [support@gfireproject.org])
AC_CONFIG_SRCDIR([src/gfire.c])


AM_INIT_AUTOMAKE([gfire], [gfire_version], "")
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PATH_PROG(sedpath, sed)
AC_CONFIG_MACRO_DIR([m4])

#
# Define Gfire header
#
AH_TOP([
#ifndef _GFIRE_CONFIG_H
#define _GFIRE_CONFIG_H
])
AH_BOTTOM([#endif])

AC_DEFINE_UNQUOTED([GFIRE_VERSION_SUFFIX], ["gfire_version_suffix"], [Gfire version suffix])
AC_DEFINE([GFIRE_VERSION_PATCH], [gfire_version_patch], [Gfire patch version])
AC_DEFINE([GFIRE_VERSION_MINOR], [gfire_version_minor], [Gfire minor version])
AC_DEFINE([GFIRE_VERSION_MAJOR], [gfire_version_major], [Gfire major version])
AC_DEFINE_UNQUOTED([GFIRE_VERSION_STRING], ["gfire_version"], [Gfire version string])
AC_DEFINE_UNQUOTED([GFIRE_REVISION], ["gfire_revision"], [Gfire revision version])

#
# Check C compiler
#
CFLAGS_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$CFLAGS_save"

#
# Setup libtool
#
AC_DISABLE_STATIC
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [Compile with debugging support.]), enable_debug="$enableval", enable_debug=no)

if test x$enable_debug != xno; then
	DEBUG_CFLAGS="$DEBUG_CFLAGS -g"
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
	if test x$enable_debug = xverbose; then
		AC_DEFINE(DEBUG_VERBOSE, 1, [Define if verbose debugging is enabled.])
	fi
fi

AC_SUBST(DEBUG_CFLAGS)
 
if test "x$GCC"="xyes"; then
	CFLAGS="$CFLAGS -Wall -D_FILE_OFFSET_BITS=64"
fi
AC_SUBST(CFLAGS)

#
# Check for Pidgin
#
PKG_CHECK_MODULES(PURPLE, purple, [AC_DEFINE(HAVE_PURPLE, 1, [Define if we've found libpurple.])])

AC_SUBST(PURPLE_CFLAGS)
AC_SUBST(PURPLE_LIBS)

#
# Check for GTK+ and other dependencies
#
AC_ARG_ENABLE(gtk, AS_HELP_STRING([--disable-gtk], [Don't compile with GTK+ support. (no server browser, no game manager)]), enable_gtk="$enableval", enable_gtk=yes)
AC_ARG_ENABLE(libnotify, AS_HELP_STRING([--enable-libnotify], [Compile with libnotify support. (depends on GTK flag)]), enable_libnotify="$enableval", enable_libnotify=no)

if test x$enable_gtk = xyes; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.14.0, AC_DEFINE(HAVE_GTK, 1, [Define if we've support for GTK+]), AC_MSG_ERROR([
*** GTK+ 2.0 is required to build Gfire; please make sure you have
*** the GTK+ development headers installed. The latest version of GTK is
*** always available at http://www.gtk.org]))
	AC_SUBST(GTK_CFLAGS)
	AC_SUBST(GTK_LIBS)

	if test x$enable_libnotify = xyes; then
		PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.3.2, AC_DEFINE(HAVE_LIBNOTIFY, 1, [Define if we've support for libnotify]), AC_MSG_ERROR([
*** You have requested to build Gfire with libnotify support. You do not have it;
*** please make sure you have the libnotify development headers installed.
*** The latest version of libnotify is always available at http://galago-project.org]))
		AC_SUBST(LIBNOTIFY_CFLAGS)
		AC_SUBST(LIBNOTIFY_LIBS)
	fi
fi
AM_CONDITIONAL(GTK, test "x$enable_gtk" = "xyes")

AC_ARG_ENABLE(dbus-status, AS_HELP_STRING([--enable-dbus-status], [Enable status change for other IM clients (currently only kmess).]), enable_dbus_status="$enableval", enable_dbus_status=no)
if test $enable_dbus_status = "yes"; then
	PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1, AC_DEFINE(HAVE_DBUS_GLIB, 1, [Define if we've support for dbus-glib]), AC_MSG_ERROR([
*** Glib DBus bindings are required to build Gfire with your current configure flags;
*** please make sure you have the GLib DBus development headers installed.
*** The latest version of GLib is always available at http://www.gtk.org]))
	AC_SUBST(DBUS_GLIB_CFLAGS)
	AC_SUBST(DBUS_GLIB_LIBS)
	AC_DEFINE([USE_DBUS_STATUS_CHANGE], [1], [Use DBus status change if defined])
fi

AM_PATH_GLIB_2_0(2.18.0,, AC_MSG_ERROR([
*** GLib 2.0 is required to build Gfire; please make sure you have
*** the GLib development headers installed. The latest version of GLib is
*** always available at http://www.gtk.org]), gthread)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

#
# Internationalisation support
#
AC_ARG_ENABLE(nls, AS_HELP_STRING([--disable-nls], [disable installation of translation files]), enable_nls="$enableval", enable_nls=yes)
if test x$enable_nls = xyes; then
	AC_PROG_INTLTOOL
	GETTEXT_PACKAGE="$PACKAGE"
	AC_SUBST(GETTEXT_PACKAGE)
	AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define the gettext package to be used])

	ALL_LINGUAS="cs de ee es fr pl ro sk sv"
	AM_GLIB_GNU_GETTEXT
fi
AM_CONDITIONAL(INSTALL_NLS, test "x$enable_nls" = "xyes")

#
# Game detection wanted?
#
AC_ARG_ENABLE(game-detection, AS_HELP_STRING([--disable-game-detection], [Disable game detection. e.g. use this if you intend to build Gfire for embedded devices/servers.]), enable_game_detection="$enableval", enable_game_detection=yes)
if test x$enable_game_detection = xyes; then
	AC_DEFINE([USE_GAME_DETECTION], [1], [Support game detection])
fi
AM_CONDITIONAL(GAME_DETECTION, test "x$enable_game_detection" = "xyes")

#
# Update notifications wanted?
#
AC_ARG_ENABLE(update-notify, AS_HELP_STRING([--disable-update-notify], [Disable notifications on new available versions of Gfire. Use this if you intend to build Gfire for a package.]), enable_update_notify="$enableval", enable_update_notify=yes)
if test x$enable_update_notify = xyes; then
	AC_DEFINE([UPDATE_NOTIFY], [1], [Show notifications about newer versions])
fi

#
# Check if plugins are enabled
#
want_plugins=yes
AM_CONDITIONAL(PLUGINS, test "x$want_plugins" = "xyes")

#
# Finish up
#
AC_CONFIG_HEADER([gfire_config.h])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	data/Makefile
	pixmaps/Makefile
	po/Makefile.in
])

AC_OUTPUT

echo;
echo "Configuration complete."
echo;

echo "Debugging enabled.............: $enable_debug"
echo "GTK enabled...................: $enable_gtk"
echo "libnotify enabled.............: $enable_libnotify"
echo "DBus status change enabled....: $enable_dbus_status"
echo "Game detection enabled........: $enable_game_detection"
echo "Notify on new version.........: $enable_update_notify"
echo "NLS enabled...................: $enable_nls"

echo;

purplelibdir=$(pkg-config purple --variable=libdir)
purplepfxdir=$(pkg-config purple --variable=prefix)

echo "Libpurple package prefix......: $purplepfxdir"
echo "Libpurple package libdir......: $purplelibdir"
echo "Install prefix................: $prefix"

gfld=`eval echo $libdir`
gfld=`eval echo $gfld`
echo "Install libdir................: $gfld"
echo -n "Purple libdir detected........: "
if test -d "$gfld/purple-2"; then
	echo "yes"
else
	echo "no!"
	echo;
	echo "We did not detect a libpurple directory off of the \"install libdir\": $gfld/purple-2"
	echo;
	echo "This is not a fatal error, as we cannot detect all cases. For most"
	echo "users though, we should be able to detect this directory. The compile"
	echo "will not fail, however the install may put files in the wrong place."
	echo;
	echo "The libpurple package libdir and prefix is where libpurple believes itself to"
	echo "be installed. The Install prefix and libdir should match them."
	echo "use ./configure --prefix=$purplepfxdir to fix this,"
	echo "and if necessary use: ./configure --prefix=$purplepfxdir --libdir=$purplelibdir"
fi

echo;
echo "Type make to compile."
