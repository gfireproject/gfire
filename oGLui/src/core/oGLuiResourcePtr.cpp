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

#include <core/oGLuiResourcePtr.h>

oGLuiResourcePtr::oGLuiResourcePtr()
{
	m_resource = 0;
}

oGLuiResourcePtr::oGLuiResourcePtr(const oGLuiResourcePtr &p_ptr)
{
	m_resource = p_ptr.m_resource;
	if(m_resource)
		m_resource->ref();
}

oGLuiResourcePtr::oGLuiResourcePtr(oGLuiResource *p_resource)
{
	m_resource = p_resource;
	if(m_resource)
		m_resource->ref();
}

oGLuiResourcePtr::~oGLuiResourcePtr()
{
	if(m_resource)
		m_resource->unref();
}

bool oGLuiResourcePtr::operator==(const oGLuiResourcePtr &p_ptr) const
{
	return (m_resource == p_ptr.m_resource);
}

bool oGLuiResourcePtr::operator!=(const oGLuiResourcePtr &p_ptr) const
{
	return (m_resource != p_ptr.m_resource);
}

oGLuiResourcePtr &oGLuiResourcePtr::operator=(const oGLuiResourcePtr &p_ptr)
{
	if(m_resource == p_ptr.m_resource)
		return *this;

	if(m_resource)
		m_resource->unref();

	m_resource = p_ptr.m_resource;
	if(m_resource)
		m_resource->ref();

	return *this;
}

oGLuiResourcePtr &oGLuiResourcePtr::operator=(oGLuiResource *p_resource)
{
	if(m_resource == p_resource)
		return *this;

	if(m_resource)
		m_resource->unref();

	m_resource = p_resource;
	if(m_resource)
		m_resource->ref();

	return *this;
}

oGLuiResourcePtr::operator oGLuiResource*()
{
	return m_resource;
}

oGLuiResourcePtr::operator bool()
{
	return (m_resource != 0);
}
