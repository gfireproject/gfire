/*
 * Copyright (C) 2009 Oliver Ney <oliver@dryder.de>
 *
 * This file is part of the OpenGL Overlay GUI Library (oGLui).
 *
 * oGLui is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with oGLui.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _OGLUIRESOURCEMANAGER_H
#define _OGLUIRESOURCEMANAGER_H

#include <core/oGLuiResource.h>
#include <core/oGLuiResourcePtr.h>
#include <list>
#include <string>

using std::list;
using std::string;

class oGLuiResourceManager
{
	public:
		oGLuiResourcePtr loadPixmapResource(const string &p_filename);

	private:
		oGLuiResource *findResource(unsigned int p_crc32);
		void removeResource(oGLuiResource *p_resource);

		list<oGLuiResource*> m_resources;

	friend class oGLuiResource;
};

extern oGLuiResourceManager oGLuiResMan;

#endif // _OGLUIRESOURCEMANAGER_H
