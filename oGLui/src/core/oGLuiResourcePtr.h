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

#ifndef _OGLUIRESOURCEPTR_H
#define _OGLUIRESOURCEPTR_H

#include <core/oGLuiResource.h>

class oGLuiResourcePtr
{
	public:
		oGLuiResourcePtr();
		oGLuiResourcePtr(const oGLuiResourcePtr &p_ptr);
		oGLuiResourcePtr(oGLuiResource *p_resource);
		~oGLuiResourcePtr();

		bool operator==(const oGLuiResourcePtr &p_ptr) const;
		bool operator!=(const oGLuiResourcePtr &p_ptr) const;

		oGLuiResourcePtr &operator=(const oGLuiResourcePtr &p_ptr);
		oGLuiResourcePtr &operator=(oGLuiResource *p_resource);

		operator oGLuiResource*();
		operator bool();

	private:
		oGLuiResource *m_resource;
};

#endif // _OGLUIRESOURCEPTR_H
