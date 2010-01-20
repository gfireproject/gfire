// gfire_config_win.h. Copied from gfire_config.h for Windows builds
#ifndef _GFIRE_CONFIG_H
#define _GFIRE_CONFIG_H

/* Compareable Gfire version */
#define GFIRE_VERSION ((GFIRE_VERSION_MAJOR << 16) | (GFIRE_VERSION_MINOR << 8) | GFIRE_VERSION_PATCH)

/* always defined to indicate that i18n is enabled */
#define ENABLE_NLS 1

/* Define the gettext package to be used */
#define GETTEXT_PACKAGE "gfire"

/* Gfire version suffix available */
#define GFIRE_VERSION_HAS_SUFFIX 1

/* Gfire major version */
#define GFIRE_VERSION_MAJOR 0

/* Gfire minor version */
#define GFIRE_VERSION_MINOR 9

/* Gfire patch version */
#define GFIRE_VERSION_PATCH 0

/* Gfire version string */
#define GFIRE_VERSION_STRING "0.9.0-svn"

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
#define HAVE_BIND_TEXTDOMAIN_CODESET 1

/* Define to 1 if you have the `dcgettext' function. */
#define HAVE_DCGETTEXT 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if we've support for GTK+ */
#define HAVE_GTK 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if we've found pidgin. */
#define HAVE_PIDGIN 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "gfireteam@gmail.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "Gfire"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Gfire 0.9.0-svn"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gfire"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.9.0-svn"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

#endif
