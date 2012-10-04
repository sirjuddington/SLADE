/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008 Simon Judd
 *
 * Email:       veilofsorrow@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PnamesList.cpp
 * Description: Various classes used to represent data objects from
 *				PNAMES and similar lumps.
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
#include "PnamesList.h"

/*******************************************************************
 * PNAMESENTRY CLASS FUNCTIONS
 *******************************************************************/

/* PnamesEntry::PnamesEntry
 * PnamesEntry class constructor
 *******************************************************************/
PnamesEntry::PnamesEntry(char * nam) {
	// Init variables
	this->name = wxString::From8BitData(nam, 8);
}

/* PnamesEntry::~PnamesEntry
 * PnamesEntry class destructor
 *******************************************************************/
PnamesEntry::~PnamesEntry() {
}

/*******************************************************************
 * PNAMESLIST CLASS FUNCTIONS
 *******************************************************************/

/* PnamesList::PnamesList
 * PnamesList class constructor
 *******************************************************************/
PnamesList::PnamesList() {
}

/* PnamesList::~PnamesList
 * PnamesList class destructor
 *******************************************************************/
PnamesList::~PnamesList() {
	clear();
}

/* PnamesList::getEntry
 * Returns the PnamesEntry at [index], or NULL if [index] is out of
 * range
 *******************************************************************/
PnamesEntry* PnamesList::getEntry(size_t index) {
	// Check index range
	if (index > nEntries())
		return NULL;

	return entries[index];
}

/* PnamesList::getEntry
 * Returns a PnamesEntry matching [name], or NULL if no match found
 *******************************************************************/
PnamesEntry* PnamesList::getEntry(string name) {
	for (size_t a = 0; a < nEntries(); a++) {
		if (entries[a]->getName().CmpNoCase(name) == 0)
			return entries[a];
	}

	// No match found
	return NULL;
}

/* PnamesList::clear
 * Clears all entries
 *******************************************************************/
void PnamesList::clear() {
	for (size_t a = 0; a < entries.size(); a++)
		delete entries[a];
	entries.clear();

}

/* PnamesList::readPNAMESData
 * Reads in a PNAMES entry. Returns true on success, false otherwise
 *******************************************************************/
bool PnamesList::readPNAMESData(ArchiveEntry* pnames) {
	// Check entries were actually given
	if (!pnames || pnames->getSize() == 0)
		return false;

	size_t pnamesize = 0;

	return true;
}

/* PnamesList::writePNAMESData
 * Write in a PNAMES entry. Returns true on success, false otherwise
 *******************************************************************/
bool PnamesList::writePNAMESData(ArchiveEntry* animated) {
	return false;
}
