#!/bin/sh

echo "Generating configuration files for oGLui, please wait..."
echo;

echo "Running libtoolize, please ignore non-fatal messages."
echo n | libtoolize --copy --force || exit;

aclocal -I m4 || exit;
autoheader || exit;
automake --add-missing --copy || exit;
autoconf || exit;
automake || exit;

intltoolize --copy --force --automake || exit;

echo;
echo "Done."