
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ListenerAnnouncer.cpp
 * Description: Listener/Announcer system classes. Mainly used for
 *              communication between underlying classes (archives, etc)
 *              and UI elements, without them needing to know about
 *              eachother.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "ListenerAnnouncer.h"


/*******************************************************************
 * LISTENER CLASS FUNCTIONS
 *******************************************************************/

/* Listener::Listener
 * Listener class constructor
 *******************************************************************/
Listener::Listener() {
	deaf = false;
}

/* Listener::~Listener
 * Listener class destructor
 *******************************************************************/
Listener::~Listener() {
	for (size_t a = 0; a < announcers.size(); a++)
		announcers[a]->removeListener(this);
}

/* Listener::listenTo
 * Subscribes this listener to an announcer
 *******************************************************************/
void Listener::listenTo(Announcer* a) {
	a->addListener(this);
	announcers.push_back(a);
}

/* Listener::stopListening
 * 'Unsubscribes' this listener from an announcer
 *******************************************************************/
void Listener::stopListening(Announcer* a) {
	for (size_t i = 0; i < announcers.size(); i++) {
		if (announcers[i] == a) {
			announcers.erase(announcers.begin() + i);
			return;
		}
	}
}

/* Listener::onAnnouncement
 * Called when an announcer that this listener is listening to
 * announces an event. Does nothing by default, is to be overridden
 * by whatever class inherits from Listener
 *******************************************************************/
void Listener::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) {
}


/*******************************************************************
 * ANNOUNCER CLASS FUNCTIONS
 *******************************************************************/

/* Announcer::Announcer
 * Announcer class constructor
 *******************************************************************/
Announcer::Announcer() {
	muted = false;
}

/* Announcer::~Announcer
 * Announcer class destructor
 *******************************************************************/
Announcer::~Announcer() {
	for (size_t a = 0; a < listeners.size(); a++)
		listeners[a]->stopListening(this);
}

/* Announcer::addListener
 * Adds a listener to the list
 *******************************************************************/
void Announcer::addListener(Listener* l) {
	listeners.push_back(l);
}

/* Announcer::removeListener
 * Removes a listener from the list
 *******************************************************************/
void Announcer::removeListener(Listener* l) {
	for (size_t a = 0; a < listeners.size(); a++) {
		if (listeners[a] == l) {
			listeners.erase(listeners.begin() + a);
			return;
		}
	}
}

/* Announcer::announce
 * 'Announces' an event to all listeners currently in the listeners
 * list, ie all Listeners that are 'listening' to this announcer.
 *******************************************************************/
void Announcer::announce(string event_name, MemChunk& event_data) {
	if (isMuted())
		return;

	for (size_t a = 0; a < listeners.size(); a++) {
		if (!listeners[a]->isDeaf())
			listeners[a]->onAnnouncement(this, event_name, event_data);
	}
}

/* Announcer::announce
 * 'Announces' an event to all listeners currently in the listeners
 * list, ie all Listeners that are 'listening' to this announcer.
 * For announcements that don't require any extra data
 *******************************************************************/
void Announcer::announce(string event_name) {
	MemChunk mc;
	announce(event_name, mc);
}
