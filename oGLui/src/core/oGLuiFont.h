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

#ifndef _OGLUIFONT_H
#define _OGLUIFONT_H

#include <pango/pangocairo.h>
#include <string>

class oGLuiFont
{
	public:
		oGLuiFont(const std::string &p_family = "", unsigned int p_size = 10);
		oGLuiFont(const oGLuiFont &p_font);
		~oGLuiFont();

		void setFamily(const std::string &p_family);
		const std::string &family() const;

		void setSize(unsigned int p_size);
		unsigned int size() const;

		void setBold(bool p_bold);
		bool bold() const;

		void setItalic(bool p_italic);
		bool italic() const;

		void setUnderlined(bool p_underlined);
		bool underlined() const;

		void setAllCapital(bool p_capital);
		bool allCapital() const;

		static std::string defaultFamily() { return "Verdana"; }

		oGLuiFont &operator=(const oGLuiFont &p_font);

	private:
		PangoFontDescription *m_font_desc;

		std::string m_family;
		unsigned int m_size;

		bool m_bold;
		bool m_italic;
		bool m_underlined;
		bool m_allcapital;

		friend class oGLuiFontRenderer;
};

#endif // _OGLUIFONT_H
