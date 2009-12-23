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

#ifndef _OGLUILOG_H
#define _OGLUILOG_H

#include <string>
#include <map>

using std::string;
using std::map;

class oGLuiLog
{
	public:
		enum MsgType
		{
			Error,
			Warning,
			Info
		};

		enum TextColor
		{
			Black = 0,
			Red,
			Green,
			Yellow,
			Blue,
			Magenta,
			Cyan,
			White,
			Default
		};

		oGLuiLog(const std::string &p_product);
		~oGLuiLog();

		void writeLine(MsgType p_type, const string &p_str);

	private:
		map<MsgType, TextColor> m_colors;
		std::string m_product;
};

#endif // _OGLUILOG_H
