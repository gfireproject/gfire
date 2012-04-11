/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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

// Enable GNU extensions on glibc
#ifndef _GNU_SOURCE
#	define _GNU_SOURCE 1
#endif // !_GNU_SOURCE

#include <dirent.h>
#include <sys/stat.h>

#include "gf_game_detection.h"

static gchar *get_winepath(const gchar *p_wine_path, const gchar *p_cwd, GHashTable *p_environ, const gchar *p_command)
{
	// Build argument vector
	static gchar *argv[] = { NULL, "winepath.exe", "-u", NULL, NULL };

	gchar *wine_cmd = g_strdup_printf("%s/wine", p_wine_path);
	argv[0] = wine_cmd;
    argv[3] = (gchar*)p_command;

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_winepath: argv: \"%s %s %s '%s'\"\n", argv[0], argv[1], argv[2], argv[3]);
#endif // DEBUG_VERBOSE

	// Build environment vector
	gchar **env = (gchar**)g_malloc(sizeof(gchar*) * (g_hash_table_size(p_environ) + 1));

	GHashTableIter iter;
	g_hash_table_iter_init(&iter, p_environ);

	gchar *key, *value;
	gint pos = 0;
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
	{
		if(g_strcmp0(key, "WINESERVERSOCKET") != 0)
		{
			env[pos] = g_strdup_printf("%s=%s", key, value);
			pos++;
		}
	}
	env[pos] = NULL;

#ifdef DEBUG_VERBOSE
	gchar **cur = env;
	while(*cur != NULL)
	{
		purple_debug_misc("gfire", "get_winepath: env: %s\n", *cur);
		cur++;
	}
#endif // DEBUG_VERBOSE

	gchar *result = NULL;

#ifdef DEBUG_VERBOSE
	gchar *error = NULL;
	if(!g_spawn_sync(p_cwd, argv, env, 0, NULL, NULL, &result, &error, NULL, NULL))
#else
	if(!g_spawn_sync(p_cwd, argv, env, G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, &result, NULL, NULL, NULL))
#endif // DEBUG_VERBOSE
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_winepath: g_spawn_sync() failed\n");
#endif // DEBUG
		g_strfreev(env);
		g_free(wine_cmd);
		return NULL;
	}

	g_strfreev(env);
	g_free(wine_cmd);

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_winepath: Error: \"%s\"\n", error ? error : "NULL");
	purple_debug_misc("gfire", "get_winepath: Result: \"%s\"\n", result ? result : "NULL");
	g_free(error);
#endif // DEBUG_VERBOSE

	// Remove trailing spaces and return
	if(!result)
		return NULL;
	else
		return g_strstrip(result);
}

static const gchar *get_proc_exe(const gchar *p_proc_path)
{
    static gchar exe[PATH_MAX + 1];

	gchar *proc_exe = g_strdup_printf("%s/exe", p_proc_path);
#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_exe: Resolving symlink \"%s\"\n", proc_exe);
#endif // DEBUG_VERBOSE

    ssize_t len = readlink(proc_exe, exe, PATH_MAX);
	if(len == -1)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_proc_exe: readlink() failed\n");
#endif // DEBUG
		g_free(proc_exe);
		return NULL;
	}
	exe[len] = 0;

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_exe: \"%s\" -> \"%s\"\n", proc_exe, exe);
#endif // DEBUG_VERBOSE
	g_free(proc_exe);

	return exe;
}

static void get_proc_cmdline(gchar **p_command, gchar **p_args, const gchar *p_proc_path)
{
	// Get process cmdline
	gchar *proc_cmdline = g_strdup_printf("%s/cmdline", p_proc_path);
#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_cmdline: Reading command line from \"%s\"\n", proc_cmdline);
#endif // DEBUG_VERBOSE

	FILE *fcmdline = fopen(proc_cmdline, "r");
	g_free(proc_cmdline);

	if(!fcmdline)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_proc_cmdline: fopen() failed\n");
#endif // DEBUG
		return;
	}

	gchar *line = NULL;
	size_t line_len = 0;
	gboolean first = TRUE;
	GString *arg_str = g_string_new("");

	while(getdelim(&line, &line_len, 0, fcmdline) != -1)
	{
		if(!first)
			g_string_append_printf(arg_str, "%s ", line);
		else
		{
			first = FALSE;
			*p_command = g_strdup(line);
		}
	}
	g_free(line);

	fclose(fcmdline);

	*p_args = g_strstrip(g_string_free(arg_str, FALSE));
#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_cmdline: Command \"%s\"; Args: \"%s\"\n", *p_command, *p_args);
#endif // DEBUG_VERBOSE
}

static GHashTable *get_environ(const gchar *p_proc_path)
{
	// Get process environment
	gchar *proc_environ = g_strdup_printf("%s/environ", p_proc_path);
#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_environ: Reading environment from \"%s\"\n", proc_environ);
#endif // DEBUG_VERBOSE

	FILE *fenviron = fopen(proc_environ, "r");
	g_free(proc_environ);

	if(!fenviron)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_environ: fopen() failed\n");
#endif // DEBUG
		return NULL;
	}

	GHashTable *ret = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	gchar *line = NULL;
	size_t line_len = 0;

	while(getdelim(&line, &line_len, 0, fenviron) != -1)
	{
		const gchar *equal = strchr(line, '=');
		if(!equal)
			continue;

		g_hash_table_insert(ret, g_strndup(line, equal - line), g_strdup(equal + 1));
	}
	fclose(fenviron);
	g_free(line);

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_environ: saved %u values\n", g_hash_table_size(ret));
#endif // DEBUG_VERBOSE
	return ret;
}

static const gchar *get_proc_cwd(GHashTable *p_environ, const gchar *p_proc_path)
{
    static gchar cwd[PATH_MAX + 1];

	gchar *proc_cwd = g_strdup_printf("%s/cwd", p_proc_path);
#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_cwd: No match, resolving symlink \"%s\"\n", proc_cwd);
#endif // DEBUG_VERBOSE

    ssize_t len = readlink(proc_cwd, cwd, PATH_MAX);
    if(len == -1)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_proc_cwd: readlink() failed\n");
#endif // DEBUG
		g_free(proc_cwd);
		return NULL;
	}
    cwd[len] = 0;

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "get_proc_cwd: \"%s\" -> \"%s\"\n", proc_cwd, cwd);
#endif // DEBUG_VERBOSE
	g_free(proc_cwd);
	return cwd;
}

void gfire_process_list_update(gfire_process_list *p_list)
{
	if(!p_list)
		return;

#ifdef DEBUG_VERBOSE
	purple_debug_info("gfire", "gfire_process_list_update: TRACE\n");
#endif // DEBUG_VERBOSE

	gfire_process_list_clear(p_list);

	DIR *proc = opendir("/proc");
	struct dirent *proc_dirent;

	if(!proc)
	{
		purple_debug_error("gfire", "gfire_process_list_update: opendir() failed\n");
		return;
	}

	while((proc_dirent = readdir(proc)))
	{
#ifdef DEBUG_VERBOSE
		purple_debug_info("gfire", "gfire_process_list_update: checking: \"%s\"\n", proc_dirent->d_name);
#endif // DEBUG_VERBOSE

		// Check if we got a valid process dir
		gboolean dir_valid = TRUE;
		int i = 0;
		for(; i < strlen(proc_dirent->d_name); i++)
		{
			if(!g_ascii_isdigit(proc_dirent->d_name[i]))
			{
				dir_valid = FALSE;
				break;
			}
		}
		if(!dir_valid)
		{
#ifdef DEBUG_VERBOSE
			purple_debug_error("gfire", "gfire_process_list_update: \"%s\" is no valid process directory\n",
							   proc_dirent->d_name);
#endif // DEBUG_VERBOSE
			continue;
		}

		gchar *proc_path = g_strdup_printf("/proc/%s", proc_dirent->d_name);

		// Check if it is a directory and owned by the current user
		struct stat process_stat;
		if(stat(proc_path, &process_stat) == -1)
		{
#ifdef DEBUG
			purple_debug_error("gfire", "gfire_process_list_update: stat(\"%s\") failed\n", proc_path);
#endif // DEBUG
			g_free(proc_path);
			continue;
		}

		// Don't check current process if not owned
		if(geteuid() != process_stat.st_uid || !S_ISDIR(process_stat.st_mode))
		{
#ifdef DEBUG_VERBOSE
			purple_debug_error("gfire", "gfire_process_list_update: \"%s\" is not owned by current user\n",
							   proc_dirent->d_name);
#endif // DEBUG_VERBOSE
			g_free(proc_path);
			continue;
		}

		// Get process id
		guint32 process_id;
		sscanf(proc_dirent->d_name, "%u", &process_id);

		// Get process exe
		const gchar *process_exe = get_proc_exe(proc_path);
		if(!process_exe)
		{
			g_free(proc_path);
			continue;
		}

		// Get process' command line
		gchar *process_cmd = NULL;
		gchar *process_args = NULL;
		get_proc_cmdline(&process_cmd, &process_args, proc_path);

		gchar *process_real_exe = NULL;
		// Different behaviour for Wine processes
		if(strstr(process_exe, "wine-preloader"))
		{
#ifdef DEBUG_VERBOSE
			purple_debug_misc("gfire", "gfire_process_list_update: WINE game! Starting WINE based detection...\n");
#endif // DEBUG_VERBOSE

			// Get path to wine executable
			gchar *wine_path = g_path_get_dirname(process_exe);

#ifdef DEBUG_VERBOSE
			purple_debug_misc("gfire", "gfire_process_list_update: Path to WINE executable: %s\n", wine_path);
#endif // DEBUG_VERBOSE

			// Get process environment for winepath
			GHashTable *environ = get_environ(proc_path);
			const gchar *cwd = get_proc_cwd(environ, proc_path);

#ifdef DEBUG_VERBOSE
			const gchar *prefix = NULL;
			if(environ)
				prefix = g_hash_table_lookup(environ, "WINEPREFIX");

			if(prefix)
				purple_debug_misc("gfire", "gfire_process_list_update: WINEPREFIX=\"%s\"\n", prefix);
			else
				purple_debug_misc("gfire", "gfire_process_list_update: no WINEPREFIX set\n");
#endif // DEBUG_VERBOSE

			// Get process name using winepath
			gchar *real_path = get_winepath(wine_path, cwd, environ, process_cmd);

			// Some error occured
			if(!real_path)
			{
				g_hash_table_destroy(environ);
				g_free(wine_path);
				g_free(process_cmd);
				g_free(process_args);
				g_free(proc_path);
				continue;
			}

#ifdef DEBUG_VERBOSE
			purple_debug_misc("gfire", "gfire_process_list_update: Canonicalizing path: \"%s\"\n", real_path);
#endif // DEBUG_VERBOSE

			// Get the physical path
#ifdef GF_OS_BSD
			gchar phys_path_buf[PATH_MAX];
			gchar *phys_path = realpath(real_path, phys_path_buf);
#else
			gchar *phys_path = canonicalize_file_name(real_path);
#endif // GF_OS_BSD
			g_free(real_path);

			// We might have only the executables name, try with adding the CWD
			if(!phys_path)
			{
#ifdef DEBUG_VERBOSE
				purple_debug_misc("gfire", "gfire_process_list_update: No such file :'( Trying to prepend the CWD...\n");
#endif // DEBUG_VERBOSE

				// Okay, we really can't do anything about it
				if(!cwd)
				{
					g_hash_table_destroy(environ);
					g_free(wine_path);
					g_free(process_cmd);
					g_free(process_args);
					g_free(proc_path);
					continue;
				}

				gchar *full_cmd = g_strdup_printf("%s/%s", cwd, process_cmd);
				g_free(process_cmd);

#ifdef DEBUG_VERBOSE
				purple_debug_misc("gfire", "gfire_process_list_update: Trying the following path: \"%s\"\n", full_cmd);
#endif // DEBUG_VERBOSE

				real_path = get_winepath(wine_path, cwd, environ, full_cmd);
				g_free(full_cmd);
				g_hash_table_destroy(environ);
				g_free(wine_path);

				// Again some error :'(
				if(!real_path)
				{
					g_free(process_args);
					g_free(proc_path);
					continue;
				}

#ifdef DEBUG_VERBOSE
			purple_debug_misc("gfire", "gfire_process_list_update: Canonicalizing path: \"%s\"\n", real_path);
#endif // DEBUG_VERBOSE

				// Try again
#ifdef GF_OS_BSD
				phys_path = realpath(real_path, phys_path_buf);
#else
				phys_path = canonicalize_file_name(real_path);
#endif // GF_OS_BSD
				g_free(real_path);

				// Okay...we lost
				if(!phys_path)
				{
#ifdef DEBUG_VERBOSE
					purple_debug_error("gfire", "gfire_process_list_update: No such file, we give up...\n");
#endif // DEBUG_VERBOSE
					g_free(process_args);
					g_free(proc_path);
					continue;
				}
			}
			else
			{
				g_hash_table_destroy(environ);
				g_free(wine_path);
				g_free(process_cmd);
			}

			process_real_exe = phys_path;
		}
		else
		{
			g_free(process_cmd);
			process_real_exe = g_strdup(process_exe);
		}

		// Add process to list
		process_info *info = gfire_process_info_new(process_real_exe, process_id, process_args);
#ifdef DEBUG_VERBOSE
		purple_debug_info("gfire", "gfire_process_list_update: added process: pid=%u exe=\"%s\" args=\"%s\"\n",
						  process_id, process_real_exe, process_args);
#endif // DEBUG_VERBOSE
		p_list->processes = g_list_append(p_list->processes, info);

#ifndef GF_OS_BSD
		g_free(process_real_exe);
#endif // !GF_OS_BSD
		g_free(process_args);
		g_free(proc_path);
	}

	closedir(proc);
}

/*GList *gfire_game_detection_get_process_libraries(const guint32 p_pid)
{
	GList *process_libs = NULL;

	gchar *proc_libs_path = g_strdup_printf("/proc/%u/smaps", p_pid);
	FILE *proc_libs = fopen(proc_libs_path, "r");
	g_free(proc_libs_path);

	if(proc_libs)
	{
		gchar buf[1024];
		while(!feof(proc_libs))
		{
			if(fgets(buf, 1024, proc_libs))
			{
				if(strstr(buf, ".so"))
				{
					// Getting file part only, using 20 spaces as delimiter
					gchar *libs_file_part = strstr(buf, "                    ");
					if(libs_file_part)
					{
						libs_file_part = g_path_get_basename(libs_file_part + 20);
						process_libs = g_list_append(process_libs, libs_file_part);
					}
				}
			}
		}

		fclose(proc_libs);
	}

	return process_libs;
}

void gfire_game_detection_process_libraries_clear(GList *p_list)
{
	while (p_list != NULL)
	{
		if(p_list->data)
			g_free(p_list->data);

		p_list = g_list_next(p_list);
	}
}*/
