#!/bin/sh
echo "Generating configuration files for Gfire, please wait..."
echo;

# Write SVN revision to REV
if builtin type -p svnversion &> /dev/null; then
	svnversion | sed 's/\([0-9]\+:\)\?\([0-9]\+\)P\?M\?S\?/\2/' > REV
elif [ ! -e REV ]; then
	echo "0" > REV
fi

echo "Running libtoolize, please ignore non-fatal messages."
echo n | libtoolize --copy --force || exit;

aclocal || exit;
autoheader || exit;
automake --add-missing --copy || exit;
autoconf || exit;
automake || exit;

intltoolize --copy --force --automake || exit;

echo;
echo "Done."
