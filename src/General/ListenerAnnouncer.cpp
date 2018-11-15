
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ListenerAnnouncer.cpp
// Description: Listener/Announcer system classes. Mainly used for communication
//              between underlying classes (archives, etc) and UI elements,
//              without them needing to know about eachother.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "General/ListenerAnnouncer.h"


// -----------------------------------------------------------------------------
//
// Listener Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Listener class constructor
// -----------------------------------------------------------------------------
Listener::Listener()
{
	deaf_ = false;
}

// -----------------------------------------------------------------------------
// Listener class destructor
// -----------------------------------------------------------------------------
Listener::~Listener()
{
	for (size_t a = 0; a < announcers_.size(); a++)
		announcers_[a]->removeListener(this);
}

// -----------------------------------------------------------------------------
// Subscribes this listener to an announcer
// -----------------------------------------------------------------------------
void Listener::listenTo(Announcer* a)
{
	a->addListener(this);
	announcers_.push_back(a);
}

// -----------------------------------------------------------------------------
// 'Unsubscribes' this listener from an announcer
// -----------------------------------------------------------------------------
void Listener::stopListening(Announcer* a)
{
	for (size_t i = 0; i < announcers_.size(); i++)
	{
		if (announcers_[i] == a)
		{
			announcers_.erase(announcers_.begin() + i);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Called when an announcer that this listener is listening to announces an
// event. Does nothing by default, is to be overridden by whatever class
// inherits from Listener
// -----------------------------------------------------------------------------
void Listener::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) {}


// -----------------------------------------------------------------------------
//
// Announcer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Announcer class constructor
// -----------------------------------------------------------------------------
Announcer::Announcer()
{
	muted_ = false;
}

// -----------------------------------------------------------------------------
// Announcer class destructor
// -----------------------------------------------------------------------------
Announcer::~Announcer()
{
	for (size_t a = 0; a < listeners_.size(); a++)
		listeners_[a]->stopListening(this);
}

// -----------------------------------------------------------------------------
// Adds a listener to the list
// -----------------------------------------------------------------------------
void Announcer::addListener(Listener* l)
{
	listeners_.push_back(l);
}

// -----------------------------------------------------------------------------
// Removes a listener from the list
// -----------------------------------------------------------------------------
void Announcer::removeListener(Listener* l)
{
	for (size_t a = 0; a < listeners_.size(); a++)
	{
		if (listeners_[a] == l)
		{
			listeners_.erase(listeners_.begin() + a);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// 'Announces' an event to all listeners currently in the listeners list,
// ie all Listeners that are 'listening' to this announcer.
// -----------------------------------------------------------------------------
void Announcer::announce(string event_name, MemChunk& event_data)
{
	if (isMuted())
		return;

	for (size_t a = 0; a < listeners_.size(); a++)
	{
		if (!listeners_[a]->isDeaf())
			listeners_[a]->onAnnouncement(this, event_name, event_data);
	}
}

// -----------------------------------------------------------------------------
// 'Announces' an event to all listeners currently in the listeners list,
// ie all Listeners that are 'listening' to this announcer.
// For announcements that don't require any extra data
// -----------------------------------------------------------------------------
void Announcer::announce(string event_name)
{
	MemChunk mc;
	announce(event_name, mc);
}
