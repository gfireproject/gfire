AC_PREREQ(2.50)

m4_define([oGLui_version_major], [0])
m4_define([oGLui_version_minor], [1])
m4_define([oGLui_version], [oGLui_version_major.oGLui_version_minor])

AC_INIT([oGLui], [oGLui_version], [oliver@dryder.de])
AC_CONFIG_SRCDIR([src/core/oGLuiApplication.cpp])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([oGLui], [oGLui_version], [])

AC_CONFIG_MACRO_DIR([m4])

# Libtool .so file version
OGLUI_LT_VERSION="oGLui_version_major:oGLui_version_minor:0"
AC_SUBST(OGLUI_LT_VERSION)

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [Compile with debugging support.]),, enable_debug=no)
if test $enable_debug = "yes"; then
	AC_DEFINE(DEBUG, [1], [Debugging enabled])
fi

AC_PROG_CC
AC_PROG_CXX

AC_PROG_INSTALL

AM_PROG_LIBTOOL
LIBTOOL="${LIBTOOL} --silent"

PKG_CHECK_MODULES(PANGO_CAIRO, pangocairo, , AC_MSG_ERROR([Cairo and Pango are required to build oGLui. Please make sure you have both installed!]))
AC_SUBST(PANGO_CAIRO_CFLAGS)
AC_SUBST(PANGO_CAIRO_LIBS)

AX_CHECK_GL
if test x$no_gl = xyes; then
	AC_MSG_ERROR([OpenGL is required for building and using oGLui! Please make sure you have it installed!])
fi
AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

AX_CHECK_GLU
if test x$no_glu = xyes; then
	AC_MSG_ERROR([The OpenGL Utilities are required for building and using oGLui! Please make sure you have it installed!])
fi
AC_SUBST(GLU_CFLAGS)
AC_SUBST(GLU_LIBS)

AC_CONFIG_FILES([
Makefile
oGLui.pc
src/Makefile
test/Makefile
po/Makefile.in])

AC_OUTPUT
