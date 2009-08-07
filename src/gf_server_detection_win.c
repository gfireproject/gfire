#ifdef _WIN32

#include "gf_server_detection_win.h"

static _NtQueryInformationProcess NtQueryInformationProcess = NULL;

static PVOID get_peb_address(HANDLE p_process_handle)
{
	if(!NtQueryInformationProcess)
		NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

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
		purple_debug_error("gfire", "get_process_cmdline: Could not open process for reading");
		CloseHandle(process);
		return NULL;
	}

	peb = get_peb_address(process);

	if(!ReadProcessMemory(process, (PCHAR)peb + 0x10, &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		purple_debug_error("gfire", "get_process_cmdline: Could not read the address of ProcessParameters!\n");
		CloseHandle(process);
		return NULL;
	}

	if(!ReadProcessMemory(process, (PCHAR)rtlUserProcParamsAddress + 0x40, &cmdline, sizeof(cmdline), NULL))
	{
		purple_debug_error("gfire", "get_process_cmdline: Could not read CommandLine!\n");
		CloseHandle(process);
		return NULL;
	}

	cmdline_buff = (WCHAR*)g_malloc0(cmdline.Length);
	if(!ReadProcessMemory(process, cmdline.Buffer, cmdline_buff, cmdline.Length, NULL))
	{
		purple_debug_error("gfire", "get_process_cmdline: Could not read the command line string!\n");
		g_free(cmdline_buff);
		CloseHandle(process);
		return NULL;
	}

	gchar *ret = g_utf16_to_utf8((gunichar2*)cmdline_buff, cmdline.Length / 2, NULL, NULL, NULL);
	g_free(cmdline_buff);

	CloseHandle(process);

	return ret;
}

/**
 * Checks if an argument has been used on a running process.
 * This function was made to find game mods.
 *
 * @param process_id: the process id
 * @param process_argument: the argument to check
 *
 * @return: TRUE if the process has been launched with the given argument, FALSE if not or if an error occured.
 *
**/
static gboolean check_process_argument(gint p_pid, const gchar *process_argument)
{
	if(!process_argument || strlen(process_argument) == 0)
		return TRUE;

	gchar *cmdline = get_process_cmdline(p_pid);
	if(!cmdline)
	{
		purple_debug_error("gfire", "Could not get command line of process %d\n", p_pid);
		return FALSE;
	}

	gchar **arg_pieces = g_strsplit(process_argument, " ", 0);
	if(!arg_pieces)
	{
		g_free(cmdline);
		return FALSE;
	}

	gchar **cmd_pieces = g_strsplit(cmdline, " ", 0);
	g_free(cmdline);
	if(!arg_pieces || g_strv_length(cmd_pieces) <= 1)
	{
		g_strfreev(cmd_pieces);
		g_strfreev(arg_pieces);
		return FALSE;
	}

	int i;
	for(i = 0; i < g_strv_length(arg_pieces); i++)
	{
		int j;
		gboolean found = FALSE;
		for(j = 1; j < g_strv_length(cmd_pieces); j++)
		{
			if(g_strcmp0(arg_pieces[i], cmd_pieces[j]) == 0)
			{
				found = TRUE;
				break;
			}
		}

		if(!found)
		{
			g_strfreev(arg_pieces);
			g_strfreev(cmd_pieces);
			return FALSE;
		}
	}

	g_strfreev(arg_pieces);
	g_strfreev(cmd_pieces);

	return TRUE;
}

gboolean check_process(const gchar *process, const gchar *process_argument)
{
	if(!process || strlen(process) == 0)
		return FALSE;

	PROCESSENTRY32 pe;
	memset(&pe, 0, sizeof(pe));

	pe.dwSize = sizeof(pe);

	HANDLE hProcSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hProcSnapShot == NULL)
		return FALSE;

	if(!Process32First(hProcSnapShot, &pe))
	{
		CloseHandle(hProcSnapShot);
		return FALSE;
	}

	do
	{
		gchar *executable_name = g_path_get_basename(pe.szExeFile);
		if(!executable_name)
			continue;

		if(g_strcmp0(executable_name, process) == 0)
		{
			g_free(executable_name);
			CloseHandle(hProcSnapShot);

			if(process_argument && strlen(process_argument) > 0)
				return check_process_argument(pe.th32ProcessID, process_argument);

			return TRUE;
		}
		g_free(executable_name);
	} while(Process32Next(hProcSnapShot, &pe));

	CloseHandle(hProcSnapShot);

	return FALSE;
}

void gfire_detect_teamspeak_server(guint8 **voip_ip, guint32 *voip_port)
{
}

void gfire_server_detection_detect(gfire_data *p_gfire)
{
}

#endif // _WIN32
