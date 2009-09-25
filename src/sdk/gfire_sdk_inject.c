/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009	    Oliver Ney <oliver@dryder.de>
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

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>

#define _(str) str
#define GF_SDK_VERSION "0.1"
#define GF_SDK_DLL_NAME "xfire_toucan_gfire_" GF_SDK_VERSION ".dll\0"

void print_version()
{
	printf("gfire_sdk_inject.exe v%s Copyright (C) 2009 Gfire Team\n"
		   "This program comes with ABSOLUTELY NO WARRANTY;\n"
		   "This is free software, and you are welcome to redistribute it\n"
		   "under certain conditions; see COPYING file for details.\n", GF_SDK_VERSION);
}

void print_last_error(const char *p_msg, DWORD p_last_error)
{
	char tmpError[1024];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, p_last_error, 0, tmpError, 1024, NULL);
	if(p_msg)
		printf("Error: %s:\n\t%s)", p_msg, tmpError);
	else
		printf("Error: %s", tmpError);
}

void print_win_error(const char *p_msg)
{
	print_last_error(p_msg, GetLastError());
}

int main(int p_argc, char* p_argv[])
{
	if(p_argc < 2 || strlen(p_argv[1]) == 0)
	{
		printf(_("Usage: %s executable [arguments]\n\n"), p_argv[0]);

		print_version();
		return 0;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Start process
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	memset(&pi, 0, sizeof(pi));

	char cmdline[4096];
	memset(cmdline, 0, 4096);
	int i = 1;
	for(; i < p_argc; i++)
	{
		if(cmdline[0] != 0)
			snprintf(cmdline, 4096, "%s \"%s\"", cmdline, p_argv[i]);
		else
			snprintf(cmdline, 4096, "\"%s\"", p_argv[i]);
	}

	if(!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi) || pi.hProcess == INVALID_HANDLE_VALUE)
	{
		print_win_error(_("Could not launch specified process"));
		goto error;
	}

	char cwd[4096];
	_getcwd(cwd, 4096);
	char dllpath[4096];
	snprintf(dllpath, 4096, "%s\\%s", cwd, GF_SDK_DLL_NAME);

	printf("Injection DLL: %s\n", dllpath);

	HMODULE toucan_DLL = LoadLibrary(dllpath);
	if(!toucan_DLL)
	{
		print_win_error(_("Could not load toucan library"));
		goto error;
	}

	void *CBTProc = GetProcAddress(toucan_DLL, "CBTProc@12");
	if(!CBTProc)
	{
		print_win_error(_("Could not find CBTProc symbol in toucan library"));
		goto error;
	}

	HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, CBTProc, toucan_DLL, pi.dwThreadId);
	if(!hook)
	{
		print_win_error(_("Could not install hook"));
		goto error;
	}

	PostThreadMessage(pi.dwThreadId, WM_NULL, 0, 0);

	ResumeThread(pi.hThread);

	Sleep(15000);

	UnhookWindowsHookEx(hook);

	printf("No errors during injection!\n");

	goto cleanup;

error:
	TerminateProcess(pi.hProcess, 0);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return 1;

cleanup:

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}
