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

#include "gf_game_detection.h"

// Required headers

#include <windows.h>
#include <tlhelp32.h>

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

static PVOID get_peb_address(HANDLE p_process_handle)
{
	if(!NtQueryInformationProcess)
	{
		NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
		if(!NtQueryInformationProcess)
			return NULL;
	}

	PROCESS_BASIC_INFORMATION pbi;

	NtQueryInformationProcess(p_process_handle, 0, &pbi, sizeof(pbi), NULL);
	return pbi.PebBaseAddress;
}

static gchar *get_process_cmdline(gint p_pid)
{
	HANDLE process;
	PVOID peb;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING cmdline;
	WCHAR *cmdline_buff;

	if((process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, p_pid)) == 0)
	{
		// Don't handle missing permissions as an error
		if(GetLastError() != ERROR_ACCESS_DENIED)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not open process for reading:\n(%u) %s", (guint)GetLastError(), tmpError);
		}

		return NULL;
	}

	peb = get_peb_address(process);
	if(!peb)
		return NULL;

	if(!ReadProcessMemory(process, (PCHAR)peb + 0x10, &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		if(GetLastError() != ERROR_ACCESS_DENIED  && GetLastError() != ERROR_PARTIAL_COPY)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read the address of ProcessParameters:\n(%u) %s", (guint)GetLastError(), tmpError);
		}

		CloseHandle(process);
		return NULL;
	}

	if(!ReadProcessMemory(process, (PCHAR)rtlUserProcParamsAddress + 0x40, &cmdline, sizeof(cmdline), NULL))
	{
		if(GetLastError() != ERROR_ACCESS_DENIED  && GetLastError() != ERROR_PARTIAL_COPY)
		{
			gchar tmpError[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, tmpError, 1024, NULL);
			purple_debug_error("gfire", "get_process_cmdline: Could not read CommandLine:\n(%u) %s", (guint)GetLastError(), tmpError);
		}

		CloseHandle(process);
		return NULL;
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

		g_free(cmdline_buff);
		CloseHandle(process);
		return NULL;
	}

	gchar *ret = g_utf16_to_utf8((gunichar2*)cmdline_buff, cmdline.Length / 2, NULL, NULL, NULL);
	g_free(cmdline_buff);

	CloseHandle(process);

	return ret;
}

void gfire_process_list_update(gfire_process_list *p_list)
{
	if(!p_list)
		return;

	gfire_process_list_clear(p_list);

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

			gchar *executable_name = pe.szExeFile;
			if(!executable_name)
				continue;

			gchar *cmdline = get_process_cmdline(pe.th32ProcessID);
			process_info *info = NULL;
			// TODO: Get full path to executable
			if(cmdline)
			{
				info = gfire_process_info_new(executable_name, "", pe.th32ProcessID, cmdline + strlen(pe.szExeFile));
				g_free(cmdline);
			}
			else
				info = gfire_process_info_new(executable_name, "", pe.th32ProcessID, NULL);

			g_free(executable_name);

			p_list->processes = g_list_append(p_list->processes, info);
		}
	} while(Process32Next(hProcSnapShot, &pe));

	CloseHandle(hProcSnapShot);
}
