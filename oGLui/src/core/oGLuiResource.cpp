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

#include <core/oGLuiResource.h>
#include <core/oGLuiResourceManager.h>

oGLuiResource::oGLuiResource(oGLuiResource::ResourceType p_type, unsigned int p_crc32)
{
	m_ref_count = 0;
	m_type = p_type;

	m_crc32 = p_crc32;
}

oGLuiResource::~oGLuiResource()
{
	oGLuiResMan.removeResource(this);
}

void oGLuiResource::ref()
{
	m_ref_count++;
}

void oGLuiResource::unref()
{
	m_ref_count--;

	if(m_ref_count <= 0)
		delete this;
}

oGLuiResource::ResourceType oGLuiResource::type() const
{
	return m_type;
}

unsigned int oGLuiResource::crc32() const
{
	return m_crc32;
}

oGLuiPixmapResource::oGLuiPixmapResource(oGLuiPixmap *p_pixmap, unsigned int p_crc32) :
		oGLuiResource(oGLuiResource::Pixmap, p_crc32)
{
	m_pixmap = p_pixmap;
}

oGLuiPixmapResource::~oGLuiPixmapResource()
{
	if(m_pixmap)
		delete m_pixmap;
}

oGLuiPixmapResource::operator const oGLuiPixmap*() const
{
	return m_pixmap;
}

const oGLuiPixmap *oGLuiPixmapResource::pixmap() const
{
	return m_pixmap;
}
