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

#include "gfire_sdk_toucan_dll.h"
#include <stdio.h>

static gfire_ipc_client *gfire_ipc = NULL;

BOOL APIENTRY DllMain(HMODULE p_module, DWORD p_reason_for_call, LPVOID p_reserved)
{
	if(p_reason_for_call == DLL_PROCESS_ATTACH)
	{
	}
	else if(p_reason_for_call == DLL_PROCESS_DETACH)
	{
		if(gfire_ipc)
		{
			gfire_ipc_client_cleanup(gfire_ipc);
			gfire_ipc = NULL;

			printf("Gfire IPC closed\n");
		}
	}
	return TRUE;
}

int initIPC()
{
	if(!gfire_ipc)
	{
		gfire_ipc = gfire_ipc_client_create();
		if(gfire_ipc_client_init(gfire_ipc) != 0)
		{
			printf("Gfire IPC init failed!\n");
			gfire_ipc_client_cleanup(gfire_ipc);
			gfire_ipc = NULL;

			return -1;
		}

		printf("Gfire IPC started\n");
		return 0;
	}

	return -1;
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static char success = 0;

	if(!success)
	{
		printf("Injection was successful!\n");
		success = 1;
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

/*
 * Layout of a SDK IPC Packet (ID 1):
 *
 * 4 bytes - the number of key-value pairs
 * for each pair:
 *   2 bytes - the length of key
 *   * bytes - non-zero terminated key
 *   2 bytes - the length of value
 *   * bytes - non-zero terminated value
 *
 * The strings are expected to be UTF-8
 *
 */
static int sendIPCSDKData(int p_numkeys, const char **p_keys, const char **p_values)
{
	if(!p_numkeys || !p_keys || !p_values || (!gfire_ipc && initIPC() != 0))
		return -1;

	char buffer[GFIRE_IPC_BUFFER_LEN];
	int offset = 0;
	memcpy(buffer, &p_numkeys, 4);
	offset += 4;

	int i;
	for(i = 0; i < p_numkeys; i++)
	{
		unsigned short keylen = strlen(p_keys[i]);
		unsigned short valuelen = strlen(p_values[i]);

		memcpy(buffer + offset, &keylen, 2);
		offset += 2;

		memcpy(buffer + offset, p_keys[i], keylen);
		offset += keylen;

		memcpy(buffer + offset, &valuelen, 2);
		offset += 2;

		memcpy(buffer + offset, p_values[i], valuelen);
		offset += valuelen;
	}

	if(gfire_ipc_client_send(gfire_ipc, GFIRE_IPC_ID_SDK, buffer, offset) < 0)
		return -1;

	return p_numkeys;
}

int ToucanSendGameClientDataA_V1(int p_numkeys, const char **p_keys, const char **p_values)
{
	if(!p_numkeys || (!gfire_ipc && initIPC() != 0))
		return 0;

	// ASCII is part of UTF-8 no coverting required
	if(sendIPCSDKData(p_numkeys, p_keys, p_values) != p_numkeys)
		return 0;

	return p_numkeys;
}

int ToucanSendGameClientDataW_V1(int p_numkeys, const wchar_t **p_keys, const wchar_t **p_values)
{
	if(!p_numkeys || (!gfire_ipc && initIPC() != 0))
		return 0;

	// We have to convert this from UTF-16 to UTF-8
	char *keys[p_numkeys];
	char *values[p_numkeys];

	int i;
	for(i = 0; i < p_numkeys; i++)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, p_keys[i], -1, NULL, 0, NULL, NULL);
		keys[i] = malloc(size);
		WideCharToMultiByte(CP_UTF8, 0, p_keys[i], -1, keys[i], size, NULL, NULL);

		size = WideCharToMultiByte(CP_UTF8, 0, p_values[i], -1, NULL, 0, NULL, NULL);
		values[i] = malloc(size);
		WideCharToMultiByte(CP_UTF8, 0, p_values[i], -1, values[i], size, NULL, NULL);
	}

	if(sendIPCSDKData(p_numkeys, (const char**)keys, (const char**)values) != p_numkeys)
	{
		for(i = 0; i < p_numkeys; i++)
		{
			free(keys[i]);
			free(values[i]);
		}

		return 0;
	}

	for(i = 0; i < p_numkeys; i++)
	{
		free(keys[i]);
		free(values[i]);
	}

	return p_numkeys;
}

int ToucanSendGameClientDataUTF8_V1(int p_numkeys, const char **p_keys, const char **p_values)
{
	if(!p_numkeys || (!gfire_ipc && initIPC() != 0))
		return 0;

	// UTF-8 as expected
	if(sendIPCSDKData(p_numkeys, p_keys, p_values) != p_numkeys)
		return 0;

	return p_numkeys;
}
