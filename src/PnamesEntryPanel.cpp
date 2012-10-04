
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008 Simon Judd
 *
 * Email:       veilofsorrow@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PnamesEntryPanel.cpp
 * Description: PnamesEntryPanel class. The UI for editing PNAMES
 *              and similar lumps.
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
#include "WxStuff.h"
#include "PnamesEntryPanel.h"
#include "Archive.h"
#include "ArchiveManager.h"
#include "Misc.h"
#include <wx/listctrl.h>


/*******************************************************************
 * ANIMATEDENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* PnamesEntryPanel::PnamesEntryPanel
 * PnamesEntryPanel class constructor
 *******************************************************************/
PnamesEntryPanel::PnamesEntryPanel(wxWindow* parent, uint8_t mode)
: EntryPanel(parent, "pnames") {
	type = mode;

	// Setup panel sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer_main->Add(sizer, 1, wxEXPAND, 0);

	// Add entry list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Names");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	list_entries = new ListView(this, -1);
	list_entries->showIcons(false);
	framesizer->Add(list_entries, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	Layout();
}

/* PnamesEntryPanel::~PnamesEntryPanel
 * PnamesEntryPanel class destructor
 *******************************************************************/
PnamesEntryPanel::~PnamesEntryPanel() {
	// Delete data
	names.clear();
	list_entries->ClearAll();
}

/* PnamesEntryPanel::loadEntry
 * Loads an entry into the PNAMES entry panel
 *******************************************************************/
bool PnamesEntryPanel::loadEntry(ArchiveEntry* entry) {
	// Do nothing if entry is already open
	if (this->entry == entry)
		return true;

	// Empty previous list
	names.clear();

	// Read PNAMES entry
	uint32_t n_pnames = 0;
	entry->seek(0, SEEK_SET);
	if (!entry->read(&n_pnames, 4)) {
		wxLogMessage("Error: PNAMES entry is corrupt");
		return false;
	}

	// Read pnames content
	for (uint32_t a = 0; a < n_pnames; a++) {
		char pname[9] = "";
		pname[8] = 0;

		// Try to read pname
		if (!entry->read(&pname, 8)) {
			wxLogMessage("Error: PNAMES entry is corrupt");
			return false;
		}

		// Add pname
		names.push_back(wxString(pname).Upper());
	}

	// Update variables
	this->entry = entry;
	setModified(false);

	// Refresh controls
	populateEntryList();
	Layout();

	return true;
}

/* PnamesEntryPanel::saveEntry
 * Saves any changes made to the entry
 *******************************************************************/
bool PnamesEntryPanel::saveEntry() {
	return false;
}

/* PnamesEntryPanel::populateEntryList
 * Clears and adds all entries to the entry list
 *******************************************************************/
void PnamesEntryPanel::populateEntryList() {
	// Clear current list
	list_entries->ClearAll();

	// Add columns
	list_entries->InsertColumn(0, "Index");
	list_entries->InsertColumn(1, "Name");

	// Add each graphic to the list
	list_entries->enableSizeUpdate(false);
	for (uint32_t a = 0; a < names.size(); a++) {
		string cols[] = { S_FMT("%d", a), S_FMT("%s", names[a]) };
		list_entries->addItem(a, wxArrayString(2, cols));
	}

	// Update list width
	list_entries->enableSizeUpdate(true);
	list_entries->updateSize();
}
