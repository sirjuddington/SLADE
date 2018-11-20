
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSIEntryPanel.cpp
// Description: ANSIEntryPanel class. Views ANSI entry data content
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
#include "ANSIEntryPanel.h"
#include "UI/Canvas/ANSICanvas.h"


// -----------------------------------------------------------------------------
//
// ANSIEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ANSIEntryPanel class constructor
// -----------------------------------------------------------------------------
ANSIEntryPanel::ANSIEntryPanel(wxWindow* parent) : EntryPanel(parent, "ansi")
{
	// Get the VGA font
	ansi_chardata_.assign(DATASIZE, 0);
	ansi_canvas_ = new ANSICanvas(this, -1);
	sizer_main_->Add(ansi_canvas_->toPanel(this), 1, wxEXPAND, 0);

	// Hide toolbar (no reason for it on this panel, yet)
	toolbar_->Show(false);

	Layout();
}

// -----------------------------------------------------------------------------
// Loads an entry to the panel
// -----------------------------------------------------------------------------
bool ANSIEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Check entry exists
	if (!entry)
		return false;

	if (entry->getSize() == DATASIZE)
	{
		ansi_chardata_.assign(entry->getData(), entry->getData() + DATASIZE);
		ansi_canvas_->loadData(ansi_chardata_.data());
		for (size_t i = 0; i < DATASIZE / 2; i++)
			ansi_canvas_->drawCharacter(i);
		Layout();
		Refresh();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Saves changes to the entry
// -----------------------------------------------------------------------------
bool ANSIEntryPanel::saveEntry()
{
	entry_->importMem(ansi_chardata_.data(), DATASIZE);
	return true;
}
