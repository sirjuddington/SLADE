
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BaseResourceArchivesPanel.cpp
 * Description: Panel containing controls to select from and modify
 *              saved paths to base resource archives
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
#include "BaseResourceArchivesPanel.h"
#include "ArchiveManager.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, base_resource)
EXTERN_CVAR(String, dir_last)


/*******************************************************************
 * BASERESOURCEARCHIVESPANEL CLASS FUNCTIONS
 *******************************************************************/

/* BaseResourceArchivesPanel::BaseResourceArchivesPanel
 * BaseResourceArchivesPanel class constructor
 *******************************************************************/
BaseResourceArchivesPanel::BaseResourceArchivesPanel(wxWindow* parent)
	: wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(hbox);

	// Init paths list
	//int sel_index = -1;
	list_base_archive_paths = new wxListBox(this, -1);
	for (size_t a = 0; a < theArchiveManager->numBaseResourcePaths(); a++)
	{
		list_base_archive_paths->Append(theArchiveManager->getBaseResourcePath(a));
	}

	// Select the currently open base archive if any
	if (base_resource >= 0)
		list_base_archive_paths->Select(base_resource);

	// Add paths list
	hbox->Add(list_base_archive_paths, 1, wxEXPAND|wxALL, 4);

	// Setup buttons
	btn_add = new wxButton(this, -1, "Add Archive");
	btn_remove = new wxButton(this, -1, "Remove Archive");

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(btn_add, 0, wxEXPAND|wxBOTTOM, 4);
	vbox->Add(btn_remove, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(vbox, 0, wxEXPAND|wxALL, 4);

	// Bind events
	btn_add->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &BaseResourceArchivesPanel::onBtnAdd, this);
	btn_remove->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &BaseResourceArchivesPanel::onBtnRemove, this);

	// Init layout
	Layout();
}

/* BaseResourceArchivesPanel::~BaseResourceArchivesPanel
 * BaseResourceArchivesPanel class destructor
 *******************************************************************/
BaseResourceArchivesPanel::~BaseResourceArchivesPanel()
{
}

/* BaseResourceArchivesPanel::getSelectedPath
 * Returns the currently selected base resource path
 *******************************************************************/
int BaseResourceArchivesPanel::getSelectedPath()
{
	return list_base_archive_paths->GetSelection();
}


/*******************************************************************
 * BASERESOURCEARCHIVESPANEL CLASS EVENTS
 *******************************************************************/

/* BaseResourceArchivesPanel::onBtnAdd
 * Called when the 'Add Archive' button is clicked
 *******************************************************************/
void BaseResourceArchivesPanel::onBtnAdd(wxCommandEvent& e)
{
	// Create extensions string
	string extensions = theArchiveManager->getArchiveExtensionsString();

	// Open a file browser dialog that allows multiple selection
	wxFileDialog dialog_open(this, "Choose file(s) to open", dir_last, wxEmptyString, extensions, wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get an array of selected filenames
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Add each to the paths list
		for (size_t a = 0; a < files.size(); a++)
		{
			if (theArchiveManager->addBaseResourcePath(files[a]))
				list_base_archive_paths->Append(files[a]);
		}

		// Save 'dir_last'
		dir_last = dialog_open.GetDirectory();
	}
}

/* BaseResourceArchivesPanel::onBtnRemove
 * Called when the 'Remove Archive' button is clicked
 *******************************************************************/
void BaseResourceArchivesPanel::onBtnRemove(wxCommandEvent& e)
{
	// Get the selected item index and remove it
	int index = list_base_archive_paths->GetSelection();
	list_base_archive_paths->Delete(index);

	// Also remove it from the archive manager
	theArchiveManager->removeBaseResourcePath(index);
}
