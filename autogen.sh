#!/bin/sh
echo "Generating configuration files for Gfire, please wait..."
echo;

echo "Running libtoolize, please ignore non-fatal messages."
echo n | libtoolize --copy --force || exit;

aclocal || exit;
autoheader || exit;
automake --add-missing --copy || exit;
autoconf || exit;
automake || exit;

echo;
echo "Done."
