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

#include "gf_game_detection.h"

#ifdef _WIN32

// Required headers
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

// Windows Kernel Functionality
typedef struct _PROCESS_BASIC_INFORMATION
{
	DWORD ExitStatus;
	PVOID PebBaseAddress;
	DWORD AffinityMask;
	DWORD BasePriority;
	DWORD UniqueProcessId;
	DWORD ParentProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#ifndef NTSTATUS
typedef long NTSTATUS;
#endif // NTSTATUS

typedef NTSTATUS (NTAPI *_NtQueryInformationProcess)(
		HANDLE ProcessHandle,
		DWORD ProcessInformationClass,
		PVOID ProcessInformation,
		DWORD ProcessInformationLength,
		PDWORD ReturnLength);

static _NtQueryInformationProcess NtQueryInformationProcess = NULL;

static gboolean acquirePrivileges()
{
	HANDLE token;
	TOKEN_PRIVILEGES tkp = {0};

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
	{
		purple_debug_error("gfire", "acquirePrivileges: couldn't open process token\n");
		return FALSE;
	}

	if(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
	{
		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0);
		CloseHandle(token);

		if(GetLastError() != ERROR_SUCCESS)
		{
			purple_debug_error("gfire", "acquirePrivileges: couldn't set debug privilege\n");
			return FALSE;
		}

		return TRUE;
	}

	purple_debug_error("gfire", "acquirePrivileges: no debug privilege available\n");

	CloseHandle(token);
	return FALSE;
}

static PVOID get_peb_address(HANDLE p_process_handle)
{
	if(!NtQueryInformationProcess)
	{
		NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
		if(!NtQueryInformationProcess)
		{
			purple_debug_error("gfire", "get_peb_address: No NtQueryInformationProcess available!\n");
			return NULL;
		}
	}

	PROCESS_BASIC_INFORMATION pbi;

	if(NtQueryInformationProcess(p_process_handle, 0, &pbi, sizeof(pbi), NULL) < 0)
	{
		purple_debug_error("gfire", "NtQueryInformationProcess: failed\n");
		return NULL;
	}
	return pbi.PebBaseAddress;
}

static gboolean get_process_cmdline(gint p_pid, gchar **p_exe, gchar **p_cmd_line)
{
#ifdef DEBUG_VERBOSE
	purple_debug_info("gfire", "trace: get_process_cmdline(%d)\n", p_pid);
#endif // DEBUG_VERBOSE

	HANDLE process;
	PVOID peb;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING cmdline;
	WCHAR *cmdline_buff;

	*p_exe = *p_cmd_line = NULL;

	if((process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, p_pid)) == 0)
	{
		// Don't handle missing permissions as an error
		if(GetLastError() != ERROR_ACCESS_DENIED)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not open process for reading:\n(%u) %s", (guint)GetLastError(), tmpError);
		}
#ifdef DEBUG
		else
			purple_debug_error("gfire", "get_process_cmdline: Could not open process for reading:\n Access denied\n");
#endif // DEBUG

		return FALSE;
	}

	peb = get_peb_address(process);
	if(!peb)
	{
#ifdef DEBUG
		purple_debug_error("gfire", "get_process_cmdline: bad peb address!\n");
#endif // DEBUG
		return FALSE;
	}

	if(!ReadProcessMemory(process, (PCHAR)peb + 0x10, &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		if(GetLastError() != ERROR_ACCESS_DENIED  && GetLastError() != ERROR_PARTIAL_COPY)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read the address of ProcessParameters:\n(%u) %s", (guint)GetLastError(), tmpError);
		}
#ifdef DEBUG
		else
			purple_debug_error("gfire", "get_process_cmdline: Could not read the address of ProcessParameters:\n Access denied/Partial Read\n");
#endif // DEBUG

		CloseHandle(process);
		return FALSE;
	}

	if(!ReadProcessMemory(process, (PCHAR)rtlUserProcParamsAddress + 0x40, &cmdline, sizeof(cmdline), NULL))
	{
		if(GetLastError() != ERROR_ACCESS_DENIED  && GetLastError() != ERROR_PARTIAL_COPY)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read CommandLine:\n(%u) %s", (guint)GetLastError(), tmpError);
		}
#ifdef DEBUG
		else
			purple_debug_error("gfire", "get_process_cmdline: Could not read CommandLine:\n Access denied/Partial Read\n");
#endif // DEBUG

		CloseHandle(process);
		return FALSE;
	}

	cmdline_buff = (WCHAR*)g_malloc0(cmdline.Length);
	if(!ReadProcessMemory(process, cmdline.Buffer, cmdline_buff, cmdline.Length, NULL))
	{
		if(GetLastError() != ERROR_ACCESS_DENIED && GetLastError() != ERROR_PARTIAL_COPY)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read the command line string:\n(%u) %s", (guint)GetLastError(), tmpError);
		}
#ifdef DEBUG
		else
			purple_debug_error("gfire", "get_process_cmdline: Could not read the command line string:\n Access denied/Partial Read\n");
#endif // DEBUG

		g_free(cmdline_buff);
		CloseHandle(process);
		return FALSE;
	}

	*p_cmd_line = g_utf16_to_utf8((gunichar2*)cmdline_buff, cmdline.Length / 2, NULL, NULL, NULL);
	g_free(cmdline_buff);

#ifdef DEBUG
	if(!*p_cmd_line)
		purple_debug_error("gfire", "get_process_cmdline: g_utf16_to_utf8 failed on cmdline\n");
#endif // DEBUG

	// Get process executable
	cmdline_buff = (WCHAR*)g_malloc(2048);
	DWORD len = GetModuleFileNameExW(process, NULL, cmdline_buff, 1024);
	if(len == 0)
	{
		if(GetLastError() != ERROR_ACCESS_DENIED)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read the executable filename string:\n(%u) %s", (guint)GetLastError(), tmpError);
		}
#ifdef DEBUG
		else
			purple_debug_error("gfire", "get_process_cmdline: Could not read the executable filename string:\n Access denied\n");
#endif // DEBUG

		g_free(cmdline_buff);
		g_free(*p_cmd_line);
		*p_cmd_line = NULL;
		CloseHandle(process);
		return FALSE;
	}

	*p_exe = g_utf16_to_utf8((gunichar2*)cmdline_buff, len, NULL, NULL, NULL);
	g_free(cmdline_buff);

#ifdef DEBUG
	if(!*p_exe)
		purple_debug_error("gfire", "get_process_cmdline: g_utf16_to_utf8 failed on exe\n");
#endif // DEBUG

	CloseHandle(process);

	return (*p_exe && *p_cmd_line);
}

void gfire_process_list_update(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	gfire_process_list_clear(p_list);

	acquirePrivileges();

	PROCESSENTRY32 pe;
	memset(&pe, 0, sizeof(pe));

	pe.dwSize = sizeof(pe);

	HANDLE hProcSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(!hProcSnapShot)
		return;

	if(!Process32First(hProcSnapShot, &pe))
	{
		CloseHandle(hProcSnapShot);
		return;
	}

	do
	{
		if(pe.th32ProcessID > 0)
		{
#ifdef DEBUG
			purple_debug_info("gfire", "detection: probing %s\n", pe.szExeFile);
#endif // DEBUG

			gchar *cmdline = NULL;
			gchar *executable_file = NULL;
			if(!get_process_cmdline(pe.th32ProcessID, &executable_file, &cmdline))
				continue;

#ifdef DEBUG
			purple_debug_info("gfire", "executable file: %s\n", executable_file);
			purple_debug_info("gfire", "cmdline: %s\n", cmdline);
#endif // DEBUG

			// Extract the args from the command line
			gchar *args = strstr(g_strstrip(cmdline), pe.szExeFile);
			if(args)
			{
				args += strlen(pe.szExeFile);
				if(args[0] == 0)
					args = NULL;
				// If the first char behind the process' name is ", strip it
				else if(args[0] == '\"')
				{
					args++;
					if(args[0] == 0)
						args = NULL;
				}
			}

			if(args)
			{
				g_strstrip(args);
#ifdef DEBUG
				purple_debug_info("gfire", "args: %s\n", args);
#endif // DEBUG
			}

			// Add the process
			process_info *info = gfire_process_info_new(executable_file, pe.th32ProcessID, args);
			g_free(cmdline);
			g_free(executable_file);

			p_list->processes = g_list_append(p_list->processes, info);
		}
	} while(Process32Next(hProcSnapShot, &pe));

	CloseHandle(hProcSnapShot);
}

#endif // _WIN32
