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

#ifndef _OGLUIOBJECT_H
#define _OGLUIOBJECT_H

#include <string>
#include <list>

using std::string;
using std::list;

class oGLuiEvent;
class oGLuiObject;

typedef void (*oGLuiSlot)(oGLuiObject*, void*);

#define OGLUI_OBJECT(klass) \
	protected: \
		inline virtual void *metaCast(const string &p_type) \
		{ \
			if(p_type.empty()) return 0; \
			if(p_type == #klass) return static_cast<klass*>(this); \
			else return oGLuiObject::metaCast(p_type); \
		}

#define OGLUI_OBJECT1(klass, sup1) \
	protected: \
		inline virtual void *metaCast(const string &p_type) \
		{ \
			if(p_type.empty()) return 0; \
			if(p_type == #klass) return static_cast<klass*>(this); \
			void *ret = sup1::metaCast(p_type); \
			if(!ret) \
				return oGLuiObject::metaCast(p_type); \
			else return ret; \
		}

#define OGLUI_OBJECT2(klass, sup1, sup2) \
	protected: \
		inline virtual void *metaCast(const string &p_type) \
		{ \
			if(p_type.empty()) return 0; \
			if(p_type == #klass) return static_cast<klass*>(this); \
			void *ret = sup1::metaCast(p_type); \
			if(!ret) \
				ret = sup2::metaCast(p_type); \
			else return ret; \
			if(!ret) \
				return oGLuiObject::metaCast(p_type); \
		}

class oGLuiObject
{
	public:
		oGLuiObject(oGLuiObject *p_parent = 0);
		virtual ~oGLuiObject();

		virtual void setParent(oGLuiObject *p_parent);
		oGLuiObject *parent() const;

		bool hasChildren() const;
		bool isWidget() const;

		const list<oGLuiObject*> &children() const;

		bool hasSignal(const string &p_name) const;
		bool hasSlot(const string &p_name) const;
		bool hasConnection(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot) const;

		inline bool inherits(const string &p_type) { return (metaCast(p_type) != 0); }

	private:
		struct oGLuiConnection
		{
			oGLuiObject *sender;
			string signal;
			oGLuiObject *receiver;
			string slot;
		};

		struct oGLuiRegisteredSlot
		{
			string name;
			oGLuiSlot slot;
		};

		list<oGLuiRegisteredSlot> m_slots;
		list<std::string> m_signals;
		list<oGLuiConnection> m_connections;

		oGLuiObject *m_parent;

		list<oGLuiObject*> m_children;

		bool m_being_deleted;
		bool m_is_widget;

		virtual bool eventHandler(oGLuiEvent *p_event);

		void callSlot(const string &p_name, void *p_param);

	protected:
		void registerSignal(const string &p_name);
		void unregisterSignal(const string &p_name);
		void registerSlot(const string &p_name, oGLuiSlot p_slot);
		void unregisterSlot(const string &p_name);

		void emit(const string &p_signal, void *p_param) const;

		static bool connect(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot);
		static bool disconnect(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot);

		void addChild(oGLuiObject *p_child);
		bool isChild(const oGLuiObject *p_child);
		void removeChild(const oGLuiObject *p_child);

		virtual void *metaCast(const string &p_type);

		friend class oGLuiApplication;
		friend class oGLuiWidget;
};

#define OGLUI_SIGNAL(x) registerSignal(x)
#define OGLUI_SLOT(x) registerSlot(#x, x)

#define slots

#endif // _OGLUIGUIOBJECT_H
