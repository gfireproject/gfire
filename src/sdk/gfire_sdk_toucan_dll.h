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

#ifndef _GFIRE_SDK_TOUCAN_DLL_H
#define _GFIRE_SDK_TOUCAN_DLL_H

#include "gfire_ipc_client.h"
#include <windows.h>

#define DLL_EXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

LRESULT DLL_EXPORT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);

int DLL_EXPORT ToucanSendGameClientDataA_V1(int p_numkeys, const char **p_keys, const char **p_values);
int DLL_EXPORT ToucanSendGameClientDataW_V1(int p_numkeys, const wchar_t **p_keys, const wchar_t **p_values);
int DLL_EXPORT ToucanSendGameClientDataUTF8_V1(int p_numkeys, const char **p_keys, const char **p_values);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _GFIRE_SDK_TOUCAN_DLL_H
