/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BinaryControlLump.cpp
 * Description: Various classes used to represent data objects from
 *				Boom's SWITCHES lumps.
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
#include "Misc.h"
#include "SwitchesList.h"
#include "ListView.h"

/*******************************************************************
 * SWITCHESENTRY CLASS FUNCTIONS
 *******************************************************************/

/* SwitchesEntry::SwitchesEntry
 * SwitchesEntry class constructor
 *******************************************************************/
SwitchesEntry::SwitchesEntry(switches_t entry) {
	// Init variables
	this->off = wxString::From8BitData(entry.off, 8);
	this->on = wxString::From8BitData(entry.on, 8);
	this->type = entry.type;
	this->status = LV_STATUS_NORMAL;
}

/* SwitchesEntry::~SwitchesEntry
 * CTexture class destructor
 *******************************************************************/
SwitchesEntry::~SwitchesEntry() {
}

/*******************************************************************
 * SWITCHESLIST CLASS FUNCTIONS
 *******************************************************************/

/* SwitchesList::SwitchesList
 * SwitchesList class constructor
 *******************************************************************/
SwitchesList::SwitchesList() {
}

/* SwitchesList::~SwitchesList
 * SwitchesList class destructor
 *******************************************************************/
SwitchesList::~SwitchesList() {
	entries.clear();
}

/* SwitchesList::getEntry
 * Returns the SwitchesEntry at [index], or NULL if [index] is out of
 * range
 *******************************************************************/
SwitchesEntry* SwitchesList::getEntry(size_t index) {
	// Check index range
	if (index < 0 || index > nEntries())
		return NULL;

	return entries[index];
}

/* SwitchesList::getEntry
 * Returns a SwitchesEntry matching [name], or NULL if no match
 * found; looks for name at both the on and the off frames
 *******************************************************************/
SwitchesEntry* SwitchesList::getEntry(string name) {
	for (size_t a = 0; a < nEntries(); a++) {
		if (entries[a]->getOn().CmpNoCase(name) == 0 ||
			entries[a]->getOff().CmpNoCase(name) == 0)
			return entries[a];
	}

	// No match found
	return NULL;
}

/* SwitchesList::clear
 * Clears all entries
 *******************************************************************/
void SwitchesList::clear() {
	for (size_t a = 0; a < entries.size(); a++)
		delete entries[a];
	entries.clear();

}

/* SwitchesList::readSWITCHESData
 * Reads in a Boom-format SWITCHES entry. Returns true on success,
 * false otherwise
 *******************************************************************/
bool SwitchesList::readSWITCHESData(ArchiveEntry* switches) {
	// Check entries were actually given
	if (!switches || switches->getSize() == 0)
		return false;

	uint8_t * data = new uint8_t[switches->getSize()];
	memcpy(data, switches->getData(), switches->getSize());
	uint8_t * cursor = data;
	uint8_t * eodata = cursor + switches->getSize();
	switches_t * type;

	while (cursor < eodata && *cursor != SWCH_STOP) {
		// reads an entry
		if (cursor + sizeof(switches_t) > eodata) {
			wxLogMessage("Error: SWITCHES entry is corrupt");
			delete[] data;
			return false;
		}
		type = (switches_t *) cursor;
		SwitchesEntry * entry = new SwitchesEntry(*type);
		cursor += sizeof(switches_t);

		// Add texture to list
		entries.push_back(entry);
	}
	delete[] data;
	return true;
}

/* SwitchesList::addEntry
 * Adds an entry at the given position
 *******************************************************************/
bool SwitchesList::addEntry(SwitchesEntry* entry, size_t pos) {
	if (entry == NULL)
		return false;
	if (pos >= nEntries()) {
		entries.push_back(entry);
	} else {
		vector<SwitchesEntry*>::iterator it = entries.begin() + pos;
		entries.insert(it, entry);
	}
	return true;
}

/* SwitchesList::removeEntry
 * Removes the entry at the given position
 *******************************************************************/
bool SwitchesList::removeEntry(size_t pos) {
	if (pos >= nEntries()) {
		entries.pop_back();
	} else {
		vector<SwitchesEntry*>::iterator it = entries.begin() + pos;
		entries.erase(it);
	}
	return true;
}

/* SwitchesList::swapEntries
 * Swaps the entries at the given positions
 *******************************************************************/
bool SwitchesList::swapEntries(size_t pos1, size_t pos2) {
	if (pos1 >= nEntries()) {
		pos1 = nEntries() - 1;
	}
	if (pos2 >= nEntries()) {
		pos2 = nEntries() - 1;
	}
	if (pos1 == pos2) {
		return false;
	}
	SwitchesEntry* swap = entries[pos1];
	entries[pos1] = entries[pos2];
	entries[pos2] = swap;
	return true;
}

/* SwitchesList::convertSwitches
 * Converts SWITCHES data in [entry] to ANIMDEFS format, written to
 * [animdata]
 *******************************************************************/
bool SwitchesList::convertSwitches(ArchiveEntry* entry, MemChunk* animdata) {
	const uint8_t * cursor = entry->getData(true);
	const uint8_t * eodata = cursor + entry->getSize();
	const switches_t * switches;
	string conversion;

	while (cursor < eodata && *cursor != SWCH_STOP) {
		// reads an entry
		if (cursor + sizeof(switches_t) > eodata) {
			wxLogMessage("Error: SWITCHES entry is corrupt");
			return false;
		}
		switches = (switches_t *) cursor;
		cursor += sizeof(switches_t);

		// Create animation string
		conversion = S_FMT("Switch\tDoom %d\t\t%-8s\tOn Pic\t%-8s\tTics 0\n",
			switches->type, switches->off, switches->on);

		// Write string to animdata
		animdata->reSize(animdata->getSize() + conversion.length(), true);
		animdata->write(conversion.To8BitData().data(), conversion.length());
	}
	return true;
}
