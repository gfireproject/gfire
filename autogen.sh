#!/bin/sh
echo "Generating configuration files for Gfire, please wait..."
echo;

# Write git revision to REV
if [ -d .git -o -f .git ]; then
	git show-ref refs/heads/master | cut -d " " -f 1 > REV
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
