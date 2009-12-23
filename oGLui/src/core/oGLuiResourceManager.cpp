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

#include <core/oGLuiResourceManager.h>
#include <core/oGLuiCRC32.h>

oGLuiResourceManager oGLuiResMan;

oGLuiResourcePtr oGLuiResourceManager::loadPixmapResource(const string &p_filename)
{
	if(p_filename.empty())
		return oGLuiResourcePtr();

	unsigned int crc32 = oGLuiCRC32(p_filename.c_str(), p_filename.length());

	oGLuiResource *res = findResource(crc32);
	if(res)
		return oGLuiResourcePtr(res);

	oGLuiPixmap *pixmap = new oGLuiPixmap(1,1);
	if(!pixmap->fromPNGFile(p_filename))
	{
		delete pixmap;
		return oGLuiResourcePtr();
	}

	res = new oGLuiPixmapResource(pixmap, crc32);

	m_resources.push_back(res);

	return oGLuiResourcePtr(res);
}

oGLuiResource *oGLuiResourceManager::findResource(unsigned int p_crc32)
{
	list<oGLuiResource*>::iterator iter;
	for(iter = m_resources.begin(); iter != m_resources.end(); iter++)
	{
		if((*iter)->crc32() == p_crc32)
		{
			return *iter;
		}
	}

	return 0;
}

void oGLuiResourceManager::removeResource(oGLuiResource *p_resource)
{
	list<oGLuiResource*>::iterator iter;
	for(iter = m_resources.begin(); iter != m_resources.end(); iter++)
	{
		if(*iter == p_resource)
		{
			m_resources.erase(iter);
			return;
		}
	}
}
