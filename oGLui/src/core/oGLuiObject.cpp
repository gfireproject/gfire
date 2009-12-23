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

#include <core/oGLuiObject.h>

#include <core/oGLuiApplication.h>

oGLuiObject::oGLuiObject(oGLuiObject *p_parent)
{
	m_parent = 0;
	setParent(p_parent);

	m_being_deleted = false;
}

oGLuiObject::~oGLuiObject()
{
	m_being_deleted = true;

	if(oGLuiApp)
		oGLuiApp->removeEventsFor(this);

	if(m_parent && !m_parent->m_being_deleted)
		m_parent->removeChild(this);

	// Remove all connections
	list<oGLuiConnection>::iterator coniter;
	for(coniter = m_connections.begin(); coniter != m_connections.end(); coniter++)
	{
		disconnect(coniter->sender, coniter->signal, coniter->receiver, coniter->slot);
		coniter--;
	}

	// Free all children
	list<oGLuiObject*>::iterator iter;
	for(iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		delete (*iter);
	}
}

void oGLuiObject::setParent(oGLuiObject *p_parent)
{
	if(m_parent == p_parent)
		return;

	if(m_parent)
		m_parent->removeChild(this);

	if(p_parent)
		p_parent->addChild(this);

	m_parent = p_parent;
}

oGLuiObject *oGLuiObject::parent() const
{
	return m_parent;
}

bool oGLuiObject::hasChildren() const
{
	return !m_children.empty();
}

bool oGLuiObject::isWidget() const
{
	return m_is_widget;
}

const list<oGLuiObject*> &oGLuiObject::children() const
{
	return m_children;
}

void oGLuiObject::registerSignal(const string &p_name)
{
	if(p_name.empty())
	{
		// TODO: Add some warning
		return;
	}
	else if(hasSignal(p_name))
	{
		// TODO: Add some warning
		return;
	}

	m_signals.push_back(p_name);
}

void oGLuiObject::unregisterSignal(const string &p_name)
{
	list<std::string>::iterator iter;
	for(iter = m_signals.begin(); iter != m_signals.end(); iter++)
	{
		if(*iter == p_name)
		{

			list<oGLuiConnection>::iterator coniter;
			for(coniter = m_connections.begin(); coniter != m_connections.end(); coniter++)
			{
				if(coniter->sender == this && coniter->signal == p_name)
				{
					disconnect(coniter->sender, coniter->signal, coniter->receiver, coniter->slot);
					coniter--;
				}
			}

			m_signals.erase(iter);

			return;
		}
	}

	// TODO: Add some warning
}

void oGLuiObject::registerSlot(const string &p_name, oGLuiSlot p_slot)
{
	if(p_name.empty() || !p_slot)
	{
		// TODO: Add some warning
		return;
	}
	else if(hasSlot(p_name))
	{
		// TODO: Add some warning
		return;
	}

	oGLuiRegisteredSlot slot;
	slot.name = p_name;
	slot.slot = p_slot;

	m_slots.push_back(slot);
}

void oGLuiObject::unregisterSlot(const string &p_name)
{
	list<oGLuiRegisteredSlot>::iterator iter;
	for(iter = m_slots.begin(); iter != m_slots.end(); iter++)
	{
		if(iter->name == p_name)
		{
			list<oGLuiConnection>::iterator coniter;
			for(coniter = m_connections.begin(); coniter != m_connections.end(); coniter++)
			{
				if(coniter->receiver == this && coniter->slot == p_name)
				{
					disconnect(coniter->sender, coniter->signal, coniter->receiver, coniter->slot);
					coniter--;
				}
			}

			m_slots.erase(iter);

			return;
		}
	}

	// TODO: Add some warning
}

bool oGLuiObject::hasSignal(const string &p_name) const
{
	list<std::string>::const_iterator iter;
	for(iter = m_signals.begin(); iter != m_signals.end(); iter++)
	{
		if(*iter == p_name)
			return true;
	}

	return false;
}

bool oGLuiObject::hasSlot(const string &p_name) const
{
	list<oGLuiRegisteredSlot>::const_iterator iter;
	for(iter = m_slots.begin(); iter != m_slots.end(); iter++)
	{
		if(iter->name == p_name)
			return true;
	}

	return false;
}

bool oGLuiObject::hasConnection(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot) const
{
	list<oGLuiConnection>::const_iterator coniter;
	for(coniter = m_connections.begin(); coniter != m_connections.end(); coniter = m_connections.begin())
	{
		if(coniter->sender == p_sender && coniter->signal == p_signal && coniter->receiver == p_receiver && coniter->slot == p_slot)
			return true;
	}

	return false;
}

bool oGLuiObject::connect(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot)
{
	if(!p_sender || p_signal.empty() || !p_receiver || p_slot.empty())
	{
		// TODO: Add some warning
		return false;
	}
	else if(!p_sender->hasSignal(p_signal))
	{
		// TODO: Add some warning
		return false;
	}
	else if(!p_receiver->hasSlot(p_slot))
	{
		// TODO: Add some warning
		return false;
	}
	else if(p_sender->hasConnection(p_sender, p_signal, p_receiver, p_slot))
	{
		// TODO: Add some warning
		return false;
	}

	oGLuiConnection connection;
	connection.sender = p_sender;
	connection.receiver = p_receiver;
	connection.signal = p_signal;
	connection.slot = p_slot;

	p_sender->m_connections.push_back(connection);
	p_receiver->m_connections.push_back(connection);

	return true;
}

bool oGLuiObject::disconnect(oGLuiObject *p_sender, const string &p_signal, oGLuiObject *p_receiver, const string &p_slot)
{
	if(!p_sender || p_signal.empty() || !p_receiver || p_slot.empty())
	{
		// TODO: Add some warning
		return false;
	}

	list<oGLuiConnection>::iterator iter;
	for(iter = p_sender->m_connections.begin(); iter != p_sender->m_connections.end(); iter = p_sender->m_connections.begin())
	{
		if(iter->sender == p_sender && iter->signal == p_signal && iter->receiver == p_receiver && iter->slot == p_slot)
		{
			p_sender->m_connections.erase(iter);
			break;
		}
	}

	if(iter == p_sender->m_connections.end())
	{
		// TODO: Add some warning
		return false;
	}

	for(iter = p_receiver->m_connections.begin(); iter != p_receiver->m_connections.end(); iter = p_receiver->m_connections.begin())
	{
		if(iter->sender == p_sender && iter->signal == p_signal && iter->receiver == p_receiver && iter->slot == p_slot)
		{
			p_receiver->m_connections.erase(iter);
			break;
		}
	}

	// This should never happen...
	if(iter == p_receiver->m_connections.end())
	{
		// TODO: Add some warning
		return false;
	}

	return true;
}

void oGLuiObject::emit(const string &p_signal, void *p_param) const
{
	if(p_signal.empty() || !hasSignal(p_signal))
	{
		// TODO: Add some warning
		return;
	}

	list<oGLuiConnection>::const_iterator iter;
	for(iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		if(iter->sender == this && iter->signal == p_signal)
			iter->receiver->callSlot(iter->slot, p_param);
	}
}

void oGLuiObject::callSlot(const string &p_name, void *p_param)
{
	list<oGLuiRegisteredSlot>::iterator iter;
	for(iter = m_slots.begin(); iter != m_slots.end(); iter++)
	{
		if(iter->name == p_name)
		{
			iter->slot(this, p_param);
			return;
		}
	}
}

void oGLuiObject::addChild(oGLuiObject *p_child)
{
	if(!p_child)
		return;

	if(!isChild(p_child))
	{
		if(p_child->parent() && p_child->parent() != this)
			p_child->parent()->removeChild(p_child);

		m_children.push_back(p_child);
		p_child->setParent(this);
	}
}

bool oGLuiObject::isChild(const oGLuiObject *p_child)
{
	if(!p_child)
		return false;

	list<oGLuiObject*>::const_iterator iter;
	for(iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(*iter == p_child)
			return true;
	}

	return false;
}

void oGLuiObject::removeChild(const oGLuiObject *p_child)
{
	if(!p_child || m_children.size() == 0)
		return;

	list<oGLuiObject*>::iterator iter;
	for(iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(*iter == p_child)
		{
			m_children.erase(iter);
			return;
		}
	}
}

bool oGLuiObject::eventHandler(oGLuiEvent *p_event)
{
	return false;
}

void *oGLuiObject::metaCast(const string &p_type)
{
	if(p_type.empty()) return 0;
	else if(p_type == "oGLuiObject") return dynamic_cast<oGLuiObject*>(this);
	return 0;
}
