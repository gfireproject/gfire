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

#include <core/oGLuiLog.h>

#include <cstdio>

oGLuiLog::oGLuiLog(const std::string &p_product)
{
	m_product = p_product;

	m_colors[Error] = Red;
	m_colors[Warning] = Yellow;
	m_colors[Info] = Green;
}

oGLuiLog::~oGLuiLog()
{
}

void oGLuiLog::writeLine(MsgType p_type, const string &p_str)
{
	string typestr;

	switch(p_type)
	{
		case Error:
			typestr = "ERROR";
			break;
		case Warning:
			typestr = "WARNING";
			break;
		case Info:
			typestr = "INFO";
			break;
	}

	static char esc_char = 0x1B;
	printf("%c[%dm[%s - %s]%c[0m %s\n", esc_char, m_colors[p_type] + 30, m_product.c_str(), typestr.c_str(),  esc_char, p_str.c_str());
}
