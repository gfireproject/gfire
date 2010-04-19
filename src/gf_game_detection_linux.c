/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2010  Oliver Ney <oliver@dryder.de>
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dirent.h>
#include <sys/stat.h>

#include "gf_game_detection.h"

static const gchar *get_winepath(const gchar *p_wine_prefix, const gchar *p_command)
{
	static gchar cmd_out[PATH_MAX];

	gchar *cmd = NULL;
	if(!p_wine_prefix)
		cmd = g_strdup_printf("winepath -u '%s'", p_command);
	else
		cmd = g_strdup_printf("WINEPREFIX='%s' winepath -u '%s'", p_wine_prefix, p_command);

#ifdef DEBUG
	purple_debug_misc("gfire", "get_winepath: Executing \"%s\"\n", cmd);
#endif // DEBUG

	FILE *winepath = popen(cmd, "r");
	g_free(cmd);

	if(!winepath)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_winepath: popen() failed\n");
#endif // DEBUG
		return NULL;
	}

	if(!fgets(cmd_out, PATH_MAX, winepath))
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_winepath: fgets() failed\n");
#endif // DEBUG
		pclose(winepath);
		return NULL;
	}


	pclose(winepath);

#ifdef DEBUG
	purple_debug_misc("gfire", "get_winepath: Result \"%s\"\n", cmd_out);
#endif // DEBUG

	// Remove trailing spaces and return
	return g_strstrip(cmd_out);
}

static const gchar *get_proc_exe(const gchar *p_proc_path)
{
	static gchar exe[PATH_MAX];

	gchar *proc_exe = g_strdup_printf("%s/exe", p_proc_path);
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_exe: Resolving symlink \"%s\"\n", proc_exe);
#endif // DEBUG

	int len = readlink(proc_exe, exe, PATH_MAX - 1);
	if(len == -1)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_proc_exe: readlink() failed\n");
#endif // DEBUG
		g_free(proc_exe);
		return NULL;
	}

	exe[len] = 0;
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_exe: \"%s\" -> \"%s\"\n", proc_exe, exe);
#endif // DEBUG
	g_free(proc_exe);

	return exe;
}

static void get_proc_cmdline(gchar **p_command, gchar **p_args, const gchar *p_proc_path)
{
	// Get process cmdline
	gchar *proc_cmdline = g_strdup_printf("%s/cmdline", p_proc_path);
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_cmdline: Reading command line from \"%s\"\n", proc_cmdline);
#endif // DEBUG

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
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_cmdline: Command \"%s\"; Args: \"%s\"\n", *p_command, *p_args);
#endif // DEBUG
}

static GHashTable *get_environ(const gchar *p_proc_path)
{
	// Get process environment
	gchar *proc_environ = g_strdup_printf("%s/environ", p_proc_path);
#ifdef DEBUG
	purple_debug_misc("gfire", "get_environ: Reading environment from \"%s\"\n", proc_environ);
#endif // DEBUG

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

#ifdef DEBUG
	purple_debug_misc("gfire", "get_environ: saved %u values\n", g_hash_table_size(ret));
#endif // DEBUG
	return ret;
}

static const gchar *get_proc_cwd(GHashTable *p_environ, const gchar *p_proc_path)
{
	static gchar cwd[PATH_MAX];

	/* DISABLED FOR DEBUGGING PURPOSES
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_cwd: Checking for CWD from environment\n");
#endif // DEBUG

	const gchar *env_cwd = g_hash_table_lookup(p_environ, "PWD");
	if(env_cwd)
	{
#ifdef DEBUG
		purple_debug_misc("gfire", "get_proc_cwd: Found CWD in env: \"%s\"\n", env_cwd);
#endif // DEBUG
		strncpy(cwd, env_cwd, PATH_MAX);
		return cwd;
	}*/

	gchar *proc_cwd = g_strdup_printf("%s/cwd", p_proc_path);
#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_cwd: No match, resolving symlink \"%s\"\n", proc_cwd);
#endif // DEBUG

	if(readlink(proc_cwd, cwd, PATH_MAX) == -1)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_proc_cwd: readlink() failed\n");
#endif // DEBUG
		g_free(proc_cwd);
		return NULL;
	}

#ifdef DEBUG
	purple_debug_misc("gfire", "get_proc_cwd: \"%s\" -> \"%s\"\n", proc_cwd, cwd);
#endif // DEBUG
	g_free(proc_cwd);
	return cwd;
}

void gfire_process_list_update(gfire_process_list *p_list)
{
	if(!p_list)
		return;

#ifdef DEBUG
	purple_debug_info("gfire", "gfire_process_list_update: TRACE\n");
#endif // DEBUG

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
#ifdef DEBUG
		purple_debug_info("gfire", "gfire_process_list_update: checking: \"%s\"\n", proc_dirent->d_name);
#endif // DEBUG

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
#ifdef DEBUG
			purple_debug_error("gfire", "gfire_process_list_update: \"%s\" is no valid process directory\n",
							   proc_dirent->d_name);
#endif // DEBUG
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
#ifdef DEBUG
			purple_debug_error("gfire", "gfire_process_list_update: \"%s\" is not owned by current user\n",
							   proc_dirent->d_name);
#endif // DEBUG
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
#ifdef DEBUG
			purple_debug_misc("gfire", "gfire_process_list_update: WINE game! Starting WINE based detection...\n");
#endif // DEBUG

			// Get Wine prefix for winepath
			GHashTable *environ = get_environ(proc_path);
			const gchar *prefix = NULL;
			if(environ)
				prefix = g_hash_table_lookup(environ, "WINEPREFIX");

#ifdef DEBUG
			if(prefix)
				purple_debug_misc("gfire", "gfire_process_list_update: WINEPREFIX=\"%s\"\n", prefix);
			else
				purple_debug_misc("gfire", "gfire_process_list_update: no WINEPREFIX set\n");
#endif // DEBUG

			// Get process name using winepath
			const gchar *real_path = get_winepath(prefix, process_cmd);

			// Some error occured
			if(!real_path)
			{
				g_hash_table_destroy(environ);
				g_free(process_cmd);
				g_free(process_args);
				g_free(proc_path);
				continue;
			}

#ifdef DEBUG
			purple_debug_misc("gfire", "gfire_process_list_update: Canonicalizing path: \"%s\"\n", real_path);
#endif // DEBUG

			// Get the physical path
			gchar *phys_path = canonicalize_file_name(real_path);

			// We might have only the executables name, try with adding the CWD
			if(!phys_path)
			{
#ifdef DEBUG
				purple_debug_misc("gfire", "gfire_process_list_update: No such file :'( Trying to prepend the CWD...\n");
#endif // DEBUG
				const gchar *cwd = get_proc_cwd(environ, proc_path);
				// Okay, we really can't do anything about it
				if(!cwd)
				{
					g_hash_table_destroy(environ);
					g_free(process_cmd);
					g_free(process_args);
					g_free(proc_path);
					continue;
				}

				gchar *full_cmd = g_strdup_printf("%s/%s", cwd, process_cmd);
				g_free(process_cmd);

#ifdef DEBUG
				purple_debug_misc("gfire", "gfire_process_list_update: Trying the following path: \"%s\"\n", full_cmd);
#endif // DEBUG

				real_path = get_winepath(prefix, full_cmd);
				g_free(full_cmd);
				g_hash_table_destroy(environ);

				// Again some error :'(
				if(!real_path)
				{
					g_free(process_args);
					g_free(proc_path);
					continue;
				}

#ifdef DEBUG
			purple_debug_misc("gfire", "gfire_process_list_update: Canonicalizing path: \"%s\"\n", real_path);
#endif // DEBUG

				// Try again
				phys_path = canonicalize_file_name(real_path);

				// Okay...we lost
				if(!phys_path)
				{
#ifdef DEBUG
					purple_debug_error("gfire", "gfire_process_list_update: No such file, we give up...\n");
#endif // DEBUG
					g_free(process_args);
					g_free(proc_path);
					continue;
				}
			}
			else
			{
				g_hash_table_destroy(environ);
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
#ifdef DEBUG
		purple_debug_info("gfire", "gfire_process_list_update: added process: pid=%u exe=\"%s\" args=\"%s\"\n",
						  process_id, process_real_exe, process_args);
#endif // DEBUG
		p_list->processes = g_list_append(p_list->processes, info);

		g_free(process_real_exe);
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
