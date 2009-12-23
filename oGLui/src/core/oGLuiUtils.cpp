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

#include <core/oGLuiUtils.h>

#include <cmath>
#include <sstream>

namespace oGLui
{
	static const char hex_map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	std::string toHexString(unsigned int p_int)
	{
		std::string ret;

		unsigned int old_result = p_int;
		unsigned int result = p_int / 16;
		do
		{
			ret = hex_map[old_result % 16] + ret;
			old_result = result;
			result = result / 16;
		} while (old_result != 0);

		if(ret.length() % 2)
			ret = '0' + ret;

		return ret;
	}

	unsigned int fromHexString(const std::string &p_string)
	{
		if(p_string.empty())
			return 0;

		// Invalid hex string
		if(p_string.find_first_not_of("0123456789ABCDEF") != std::string::npos)
			return 0;

		unsigned int ret = 0;
		unsigned int exp = 0;
		for(int pos = p_string.length() - 1; pos >= 0; pos--)
		{
			for(unsigned int hex_val = 0; hex_val < 16; hex_val++)
			{
				if(p_string.at(pos) == hex_map[hex_val])
				{
					ret += hex_val * pow(16, exp);
					break;
				}
			}

			exp++;
		}

		return ret;
	}

	std::string intToStr(int p_int)
	{
		std::stringstream ss;
		ss << p_int;
		return ss.str();
	}

	std::string uIntToStr(unsigned int p_uint)
	{
		std::stringstream ss;
		ss << p_uint;
		return ss.str();
	}

	int strToInt(const std::string &p_string)
	{
		int ret;
		std::stringstream ss(p_string);
		ss >> ret;
		return ret;
	}
}
