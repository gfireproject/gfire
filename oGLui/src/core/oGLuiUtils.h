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

#ifndef _OGLUIUTILS_H
#define _OGLUIUTILS_H

#include <string>

namespace oGLui
{
	std::string toHexString(unsigned int p_int);
	unsigned int fromHexString(const std::string &p_string);

	std::string intToStr(int p_int);
	std::string uIntToStr(unsigned int p_uint);
	int strToInt(const std::string &p_string);

	template<class T>
			const T &max(const T &p_a, const T &p_b)
	{
		return (p_a > p_b) ? p_a : p_b;
	}

	template<class T>
			const T &min(const T &p_a, const T &p_b)
	{
		return (p_a < p_b) ? p_a : p_b;
	}
}

#endif // _OGLUIUTILS_H
