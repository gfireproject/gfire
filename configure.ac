
# Most of this configure script was ripped from gaim-netsoul
# Thank you guys!!
#

AC_INIT(src/gfire.c)
AC_PREREQ([2.50])

GFIRE_VERSION="`cat ./VERSION`"
AM_INIT_AUTOMAKE([Gfire], $GFIRE_VERSION, 'no-define')
AC_DEFINE_UNQUOTED(GFIRE_VERSION, "$GFIRE_VERSION", [Gfire Version])

AC_PATH_PROG(sedpath, sed)
#
# Our header
#

AH_TOP([ /* our header */
#ifndef _GFIRE_CONFIG_H
#define _GFIRE_CONFIG_H
])
AH_BOTTOM([
#endif /* _GFIRE_CONFIG_H */
])

#
# Look for the C compiler
#
CFLAGS_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$CFLAGS_save"

AC_ARG_ENABLE(debug,   [  --enable-debug          compile with debugging support],,enable_debug=no)

if test "x$enable_debug" = "xyes" ; then
	DEBUG_CFLAGS="$DEBUG_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
fi

AC_SUBST(DEBUG_CFLAGS)

if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -g3"
fi
AC_SUBST(CFLAGS)

#
# Check for Pidgin
#

PKG_CHECK_MODULES(PIDGIN, pidgin,
[
   AC_DEFINE(HAVE_PIDGIN, 1, [Define if we've found pidgin.])
])

AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LIBS)

#
# check for gtk and other dependencies
#

AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR([
*** GLib 2.0 is required to build gfire; please make sure you have
*** the GLib development headers installed. The latest version of GLib is
*** always available at http://www.gtk.org/.]))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

#
# Check if plugins are enabled
#
want_plugins=yes
AM_CONDITIONAL(PLUGINS, test "$want_plugins" = "yes")

AM_GLIB_GNU_GETTEXT

#
# Setup libtool
#
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

#
# Finish up
#
AM_CONFIG_HEADER(pre_config.h)
AC_OUTPUT([Makefile
		pixmaps/Makefile
		src/Makefile
		  ])

echo;
echo Configuration complete
echo;

#grab where Pidgin libdir is
pidginlibdir=$(pkg-config pidgin --variable=libdir)
pidginpfxdir=$(pkg-config pidgin --variable=prefix)

echo Debugging enabled.............: $enable_debug
echo Pidgin package prefix.........: $pidginpfxdir
echo Pidgin package libdir.........: $pidginlibdir
echo Install prefix................: $prefix
gfld=`eval echo $libdir`
gfld=`eval echo $gfld`
echo Install libdir................: $gfld
echo -n "Pidgin lib dir detected.......: "
if test -d "$gfld/pidgin" ; then
	echo Yes
else
	echo NO!
	echo;
	echo We did not detect a Pidgin directory off of the \"Install libdir\" dir:
	echo "   $gfld/pidgin"
	echo This is not a fatal error, as we cannot detect all cases.  For most
	echo users though we should be able to detect this directory.  The compile
	echo will not fail, however the install may put files in the wrong place.
	echo;
	echo The Pidgin package libdir and prefix is where Pidgin believes itself to
	echo be installed.  The Install prefix and libdir should match them.
	echo use ./configure --prefix=$pidginpfxdir to fix this
	echo and if necessary use:
	echo ./configure --prefix=$pidginpfxdir --libdir=$pidginlibdir
fi
echo;
echo;
echo Type make to compile
echo;
echo Make sure you copy data/gfire_games.xml to $HOME/.purple/
echo;
