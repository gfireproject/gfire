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

#ifndef _OGLUIRESOURCE_H
#define _OGLUIRESOURCE_H

#include <core/oGLuiPixmap.h>

class oGLuiResource
{
	public:
		enum ResourceType
		{
			Pixmap,
		};

		void ref();
		void unref();

		ResourceType type() const;

		unsigned int crc32() const;

	protected:
		oGLuiResource(ResourceType p_type, unsigned int p_crc32);
		virtual ~oGLuiResource();

	private:
		int m_ref_count;
		ResourceType m_type;
		unsigned int m_crc32;
};

class oGLuiPixmapResource : public oGLuiResource
{
	public:
		operator const oGLuiPixmap*() const;
		const oGLuiPixmap *pixmap() const;

	private:
		oGLuiPixmapResource(oGLuiPixmap *p_pixmap, unsigned int p_crc32);
		~oGLuiPixmapResource();

		oGLuiPixmap *m_pixmap;

	friend class oGLuiResourceManager;
};

#endif // _OGLUIRESOURCE_H
