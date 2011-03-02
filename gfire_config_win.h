/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2011  Oliver Ney <oliver@dryder.de>
 *
 * This file is part of Gfire.
 *
 * Gfire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.
*/

// gfire_config_win.h. Copied from gfire_config.h for Windows builds
#ifndef _GFIRE_CONFIG_H
#define _GFIRE_CONFIG_H

/* always defined to indicate that i18n is enabled */
#define ENABLE_NLS 1

/* Define the gettext package to be used */
#define GETTEXT_PACKAGE "gfire"

/* Gfire major version */
#define GFIRE_VERSION_MAJOR 1

/* Gfire minor version */
#define GFIRE_VERSION_MINOR 0

/* Gfire patch version */
#define GFIRE_VERSION_PATCH 0

/* Gfire version string */
#define GFIRE_VERSION_STRING "1.0.0-svn"

/* Gfire version suffix */
#define GFIRE_VERSION_SUFFIX "svn"

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
#define PACKAGE_BUGREPORT "support@gfireproject.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "Gfire"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Gfire 1.0.0-svn"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gfire"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.0-svn"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Show notifications about newer versions */
#define UPDATE_NOTIFY 1

#endif
