/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009       Oliver Ney <oliver@dryder.de>
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

// Result must be freed when no longer used, using g_free()
gchar *gfire_game_detection_winepath(const gchar *p_wine_prefix, const gchar *p_win_path)
{
	if (!p_win_path)
		return NULL;

	FILE *winepath;
	gchar *cmd, cmd_out[PATH_MAX];

	if (!p_wine_prefix)
		cmd = g_strdup_printf("winepath -u '%s'", p_win_path);
	else
		cmd = g_strdup_printf("WINEPREFIX='%s' winepath -u '%s'", p_wine_prefix, p_win_path);

	winepath = popen(cmd, "r");
	g_free(cmd);

	if (!winepath)
		return NULL;

	if (!fgets(cmd_out, PATH_MAX, winepath))
	{
		pclose(winepath);
		return NULL;
	}

	pclose(winepath);

	// Remove trailing spaces and return
	return g_strdup(g_strstrip(cmd_out));
}

void gfire_process_list_update(gfire_process_list *p_list)
{
	if (!p_list)
		return;

	gfire_process_list_clear(p_list);

	DIR *proc = opendir("/proc");
	struct dirent *proc_dirent;

	if (!proc)
		return;

	while((proc_dirent = readdir(proc)))
	{
		gchar *process_name = NULL;
		gchar *process_exe = NULL;
		gchar *process_args = NULL;
		gchar *process_wine_prefix = NULL;
		gchar *process_libs = NULL;

		// Get directory mode
		struct stat proc_dir_mode;
		gchar *proc_path = g_strdup_printf("/proc/%s", proc_dirent->d_name);

		if (stat(proc_path, &proc_dir_mode) == -1)
		{
			g_free(proc_path);
			continue;
		}

		g_free(proc_path);

		// Don't check current process if not owned
		if (geteuid() != proc_dir_mode.st_uid || !S_ISDIR(proc_dir_mode.st_mode))
			continue;

		// Get process id
		guint32 process_id;
		gchar *process_id_tmp = proc_dirent->d_name;

		while(*process_id_tmp != 0)
		{
			if (!g_ascii_isdigit(*process_id_tmp))
				break;

			process_id_tmp++;
		}

		if(*process_id_tmp != 0)
			continue;

		process_id = (guint32 )atoi(proc_dirent->d_name);

		// Get process exe
		gchar proc_exe[PATH_MAX];
		gint proc_exe_len = 0;
		gchar *proc_exe_path = g_strdup_printf("/proc/%u/exe", process_id);

		proc_exe_len = readlink(proc_exe_path, proc_exe, PATH_MAX);
		if (proc_exe_len == -1)
		{
			g_free(proc_exe_path);
			continue;
		}

		g_free(proc_exe_path);

		proc_exe[proc_exe_len] = 0;
		process_exe = g_strdup(proc_exe);

		// Get process cmdline
		FILE *proc_cmdline;
		gchar *proc_cmdline_path = g_strdup_printf("/proc/%u/cmdline", process_id);

		proc_cmdline = fopen(proc_cmdline_path, "r");
		if (!proc_cmdline)
		{
			g_free(process_exe);
			g_free(proc_cmdline_path);
			continue;
		}

		g_free(proc_cmdline_path);

		gchar *cmdline_current_arg = NULL;
		size_t proc_cmdline_size;
		gint i = 0;

		// Get process cmd & args
		gchar *proc_cmd = NULL;
		GString *current_arg_new = NULL;
		current_arg_new = g_string_new("");

		while(getdelim(&cmdline_current_arg, &proc_cmdline_size, 0, proc_cmdline) != -1)
		{
			if (i == 0)
			{
				proc_cmd = g_strdup(cmdline_current_arg);
				i = -1;
			}
			else
				g_string_append_printf(current_arg_new, "%s ", cmdline_current_arg);
		}

		g_free(cmdline_current_arg);
		fclose(proc_cmdline);

		if (!proc_cmd)
		{
			g_free(process_exe);
			g_string_free(current_arg_new, TRUE);
			continue;
		}

		process_args = g_string_free(current_arg_new, FALSE);
		// Remove tons of spaces sometimes present in cmdline
		g_strstrip(process_args);

		// Different behavious for Wine processes
		if (!g_strcmp0(process_exe, "/usr/bin/wine-preloader"))
		{
			// Get process environ to get wine prefix
			FILE *proc_environ;
			gchar *proc_environ_path = g_strdup_printf("/proc/%d/environ", process_id);

			proc_environ = fopen(proc_environ_path, "r");
			if (!proc_environ)
			{
				g_free(process_exe);
				g_free(process_args);
				g_free(proc_environ_path);
				continue;
			}

			g_free(proc_environ_path);

			gchar *environ_current_value = NULL;
			size_t proc_environ_size = 0;

			gchar *wine_prefix_test = NULL;
			while(getdelim(&environ_current_value, &proc_environ_size, 0, proc_environ) != -1)
			{
				wine_prefix_test = strstr(environ_current_value, "WINEPREFIX=");

				if (wine_prefix_test != NULL)
					process_wine_prefix = g_strdup_printf("%s", wine_prefix_test + 11);
			}

			g_free(wine_prefix_test);
			fclose(proc_environ);

			// Get process name using winepath
			process_name = gfire_game_detection_winepath(process_wine_prefix, proc_cmd);
			g_free(process_wine_prefix);

			if (!process_name)
			{
				g_free(process_exe);
				g_free(process_args);
				continue;
			}

			// Get path without symlinks
			gchar *process_name_tmp;
			process_name_tmp = canonicalize_file_name(process_name);
			g_free(process_name);

			if (!process_name_tmp)
			{
				g_free(process_exe);
				g_free(process_args);
				continue;
			}

			process_name = process_name_tmp;
		}
		else
			process_name = g_strdup(g_strchomp(proc_exe));

		if (!process_name)
		{
			g_free(process_exe);
			g_free(process_args);
			continue;
		}

		// Add process to list
		process_info *info = NULL;

		info = gfire_process_info_new(process_name, process_args, process_id);
		p_list->processes = g_list_append(p_list->processes, info);

		g_free(process_name);
		g_free(process_exe);
		g_free(process_args);
		// g_free(process_libs):
	}

	closedir(proc);
}
