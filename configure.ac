#
# Most of this configure script was ripped from gaim-netsoul,
# thank you guys!!!
#

# Get Gfire version
m4_define(gfire_version, m4_include(VERSION))
m4_define(gfire_version_patch, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\3]))
m4_define(gfire_version_minor, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\2]))
m4_define(gfire_version_major, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)], [\1]))
m4_define(gfire_version_suffix, regexp(gfire_version, [\([0-9]+\).\([0-9]+\).\([0-9]+\)\(-[a-zA-Z0-9]+\)], [\4]))

AC_PREREQ(2.50)
AC_INIT([Gfire], [gfire_version], [gfireteam@gmail.com])
AC_CONFIG_SRCDIR([src/gfire.c])


AM_INIT_AUTOMAKE([gfire], [gfire_version], "")

AC_PATH_PROG(sedpath, sed)
AC_CONFIG_MACRO_DIR([m4])

#
# Internationalisation support
#
AC_PROG_INTLTOOL
GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define the gettext package to be used])
ALL_LINGUAS="cz de es fr sk sv"
AM_GLIB_GNU_GETTEXT

#
# Define Gfire header
#
AH_TOP([
#ifndef _GFIRE_CONFIG_H
#define _GFIRE_CONFIG_H

/* Compareable Gfire version */
#define GFIRE_VERSION ((GFIRE_VERSION_MAJOR << 16) | (GFIRE_VERSION_MINOR << 8) | GFIRE_VERSION_PATCH)
])
AH_BOTTOM([#endif])

AC_DEFINE([GFIRE_VERSION_PATCH], [gfire_version_patch], [Gfire patch version])
AC_DEFINE([GFIRE_VERSION_MINOR], [gfire_version_minor], [Gfire minor version])
AC_DEFINE([GFIRE_VERSION_MAJOR], [gfire_version_major], [Gfire major version])
if test "gfire_version_suffix" = ""; then
	AC_DEFINE([GFIRE_VERSION_HAS_SUFFIX], [0], [Gfire version suffix available])
else
	AC_DEFINE([GFIRE_VERSION_HAS_SUFFIX], [1], [Gfire version suffix available])
fi
AC_DEFINE_UNQUOTED([GFIRE_VERSION_STRING], ["gfire_version"], [Gfire version string])

#
# Check C compiler
#
CFLAGS_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$CFLAGS_save"

AC_ARG_ENABLE(debug, [  --enable-debug          Compile with debugging support.],, enable_debug=no)

if test $enable_debug = "yes"; then
	DEBUG_CFLAGS="$DEBUG_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
fi

AC_SUBST(DEBUG_CFLAGS)
 
if test "x$GCC"="xyes"; then
	CFLAGS="$CFLAGS -Wall -g3"
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
AC_ARG_ENABLE(gtk, [  --disable-gtk           Don't compile with GTK+ support. (no server browser, no game manager)],, enable_gtk=yes)
AC_ARG_ENABLE(libnotify, [  --enable-libnotify      Compile with libnotify support. (depends on GTK flag)],, enable_libnotify=no)

if test $enable_gtk = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0, AC_DEFINE(HAVE_GTK, 1, [Define if we've support for GTK+]), AC_MSG_ERROR([
*** GTK+ 2.0 is required to build Gfire; please make sure you have
*** the GLib development headers installed. The latest version of GLib is
*** always available at http://www.gtk.org]))
	AC_SUBST(GTK_CFLAGS)
	AC_SUBST(GTK_LIBS)

	if test $enable_libnotify = "yes"; then
		PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.3.2, AC_DEFINE(HAVE_LIBNOTIFY, 1, [Define if we've support for libnotify]), AC_MSG_ERROR([
*** You have requested to build Gfire with libnotify support. You do not have it;
*** please make sure you have the libnotify development headers installed.
*** The latest version of libnotify is always available at http://galago-project.org]))
		AC_SUBST(LIBNOTIFY_CFLAGS)
		AC_SUBST(LIBNOTIFY_LIBS)
	fi
fi


AM_PATH_GLIB_2_0(2.16.0,, AC_MSG_ERROR([
*** GLib 2.0 is required to build Gfire; please make sure you have
*** the GLib development headers installed. The latest version of GLib is
*** always available at http://www.gtk.org]), gthread)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

#
# Check if plugins are enabled
#
want_plugins=yes
AM_CONDITIONAL(PLUGINS, test "$want_plugins"="yes")

#
# Setup libtool
#
AC_DISABLE_STATIC
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

#
# Finish up
#
AM_CONFIG_HEADER(gfire_config.h)

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
pixmaps/Makefile
])

AC_OUTPUT

echo;
echo "Configuration complete."
echo;

purplelibdir=$(pkg-config purple --variable=libdir)
purplepfxdir=$(pkg-config purple --variable=prefix)

echo "Debugging enabled.............: $enable_debug"
echo "Libpurple package prefix......: $purplepfxdir"
echo "Libpurple package libdir......: $purplelibdir"
echo "Install prefix................: $prefix"

gfld=`eval echo $libdir`
gfld=`eval echo $gfld`
echo "Install libdir................: $gfld"
echo -n "Pidgin libdir detected........: "
if test -d "$gfld/pidgin"; then
	echo "yes"
else
	echo "no!"
	echo;
	echo "We did not detect a Pidgin directory off of the \"install libdir\": $gfld/pidgin"
	echo;
	echo "This is not a fatal error, as we cannot detect all cases. For most"
	echo "users though, we should be able to detect this directory. The compile"
	echo "will not fail, however the install may put files in the wrong place."
	echo;
	echo "The Pidgin package libdir and prefix is where Pidgin believes itself to"
	echo "be installed. The Install prefix and libdir should match them."
	echo "use ./configure --prefix=$pidginpfxdir to fix this,"
	echo "and if necessary use: ./configure --prefix=$pidginpfxdir --libdir=$pidginlibdir"
fi

echo;
echo "Type make to compile."
