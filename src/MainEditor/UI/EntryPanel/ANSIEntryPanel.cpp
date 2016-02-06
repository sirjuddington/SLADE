
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ANSIEntryPanel.cpp
 * Description: ANSIEntryPanel class. Views ANSI entry data content
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
#include "UI/Canvas/ANSICanvas.h"
#include "Utility/CodePages.h"
#include "ANSIEntryPanel.h"
#include <wx/sizer.h>


/*******************************************************************
 * CONSTANTS
 *******************************************************************/
#define DATASIZE 4000


/*******************************************************************
 * ANSIENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ANSIEntryPanel::ANSIEntryPanel
 * ANSIEntryPanel class constructor
 *******************************************************************/
ANSIEntryPanel::ANSIEntryPanel(wxWindow* parent) : EntryPanel(parent, "ansi")
{
	// Get the VGA font
	ansi_chardata = new uint8_t[DATASIZE];
	ansi_canvas = new ANSICanvas(this, -1);
	sizer_main->Add(ansi_canvas->toPanel(this), 1, wxEXPAND, 0);

	// Hide toolbar (no reason for it on this panel, yet)
	toolbar->Show(false);

	Layout();
}

/* ANSIEntryPanel::~ANSIEntryPanel
 * ANSIEntryPanel class destructor
 *******************************************************************/
ANSIEntryPanel::~ANSIEntryPanel()
{
	delete[] ansi_chardata;
}

/* ANSIEntryPanel::loadEntry
 * Loads an entry to the panel
 *******************************************************************/
bool ANSIEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Check entry exists
	if (!entry)
		return false;

	if (entry->getSize() == DATASIZE)
	{
		memcpy(ansi_chardata, entry->getData(), DATASIZE);
		ansi_canvas->loadData(ansi_chardata);
		for (size_t i = 0; i < DATASIZE/2; i++)
			drawCharacter(i);
		Layout();
		Refresh();
		return true;
	}
	return false;
}

/* ANSIEntryPanel::saveEntry
 * Saves changes to the entry
 *******************************************************************/
bool ANSIEntryPanel::saveEntry()
{
	entry->importMem(ansi_chardata, DATASIZE);
	return true;
}

/* ANSIEntryPanel::drawCharacter
 * Draws a single character in the canvas
 *******************************************************************/
void ANSIEntryPanel::drawCharacter(size_t i)
{
	if (ansi_canvas)
		ansi_canvas->drawCharacter(i);
}
