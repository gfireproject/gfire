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

#ifndef _OGLUICONFIG_H
#define _OGLUICONFIG_H

#include <map>
#include <string>

class oGLuiConfig
{
	public:
		oGLuiConfig(const std::string &p_filename);
		oGLuiConfig(const oGLuiConfig &p_config);
		oGLuiConfig();

		bool load(const std::string &p_filename);
		bool save(const std::string &p_filename) const;

		void setPair(const std::string &p_key, const std::string &p_value);
		std::string getValue(const std::string &p_key) const;
		bool hasKey(const std::string &p_key) const;
		void clear();

	private:
		std::map<std::string, std::string> m_pairs;
};

#endif // _OGLUICONFIG_H
