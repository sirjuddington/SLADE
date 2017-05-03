
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ResourceArchiveChooser.cpp
 * Description: A custom control that allows to open/select resource
 *              archives and change the base resource
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
#include "ResourceArchiveChooser.h"
#include "Archive/ArchiveManager.h"
#include "Utility/SFileDialog.h"
#include "General/UI.h"


/*******************************************************************
 * RESOURCEARCHIVECHOOSER CLASS FUNCTIONS
 *******************************************************************/

/* ResourceArchiveChooser::ResourceArchiveChooser
 * ResourceArchiveChooser class constructor
 *******************************************************************/
ResourceArchiveChooser::ResourceArchiveChooser(wxWindow* parent, Archive* archive) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Resource archive list
	list_resources = new wxCheckListBox(this, -1);
	sizer->Add(list_resources, 1, wxEXPAND|wxBOTTOM, 4);
	list_resources->SetInitialSize(wxSize(350, 100));

	// Populate resource archive list
	int index = 0;
	for (int a = 0; a < theArchiveManager->numArchives(); a++)
	{
		Archive* arch = theArchiveManager->getArchive(a);
		if (arch != archive)
		{
			list_resources->Append(arch->getFilename(false));
			archives.push_back(arch);
			if (theArchiveManager->archiveIsResource(arch))
				list_resources->Check(index);
			index++;
		}
	}

	// 'Open Resource' button
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxRIGHT, 4);
	btn_open_resource = new wxButton(this, -1, "Open Archive");
	hbox->Add(btn_open_resource, 0, wxEXPAND|wxRIGHT, 4);

	// 'Open Recent' button
	btn_recent = new wxButton(this, -1, "Open Recent");
	hbox->Add(btn_recent, 0, wxEXPAND, 0);

	// Bind events
	btn_open_resource->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnOpenResource, this);
	btn_recent->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnRecent, this);
	list_resources->Bind(wxEVT_CHECKLISTBOX, &ResourceArchiveChooser::onResourceChecked, this);

	Layout();
}

/* ResourceArchiveChooser::~ResourceArchiveChooser
 * ResourceArchiveChooser class destructor
 *******************************************************************/
ResourceArchiveChooser::~ResourceArchiveChooser()
{
}

/* ResourceArchiveChooser::getSelectedResourceArchives
 * Returns a list of archives that have been selected as resources
 *******************************************************************/
vector<Archive*> ResourceArchiveChooser::getSelectedResourceArchives()
{
	wxArrayInt checked;
	vector<Archive*> list;
	list_resources->GetCheckedItems(checked);
	for (unsigned a = 0; a < checked.size(); a++)
		list.push_back(archives[checked[a]]);
	return list;
}

/* ResourceArchiveChooser::getSelectedResourceList
 * Returns a string of all selected resource archive filenames
 *******************************************************************/
string ResourceArchiveChooser::getSelectedResourceList()
{
	vector<Archive*> selected = getSelectedResourceArchives();
	string ret;
	for (unsigned a = 0; a < selected.size(); a++)
		ret += S_FMT("\"%s\" ", selected[a]->getFilename());
	return ret;
}


/*******************************************************************
 * RESOURCEARCHIVECHOOSER CLASS EVENTS
 *******************************************************************/

/* ResourceArchiveChooser::onBtnOpenResource
 * Called when the 'Open Archive' button is clicked
 *******************************************************************/
void ResourceArchiveChooser::onBtnOpenResource(wxCommandEvent& e)
{
	SFileDialog::fd_info_t info;
	if (SFileDialog::openFile(info, "Open Resource Archive", theArchiveManager->getArchiveExtensionsString(), this))
	{
		UI::showSplash("Opening Resource Archive", true);
		Archive* na = theArchiveManager->openArchive(info.filenames[0], true, true);
		UI::hideSplash();
		if (na)
		{
			list_resources->Append(na->getFilename(false));
			list_resources->Check(list_resources->GetCount()-1);
			archives.push_back(na);
		}
	}
}

/* ResourceArchiveChooser::onBtnRecent
 * Called when the 'Open Recent' button is clicked
 *******************************************************************/
void ResourceArchiveChooser::onBtnRecent(wxCommandEvent& e)
{
	// Build list of recent wad filename strings
	wxArrayString recent;
	for (unsigned a = 0; a < theArchiveManager->numRecentFiles(); a++)
		recent.Add(theArchiveManager->recentFile(a));

	// Show dialog
	wxSingleChoiceDialog dlg(this, "Select a recent Archive to open", "Open Recent", recent);
	if (dlg.ShowModal() == wxID_OK)
	{
		Archive* na = theArchiveManager->openArchive(theArchiveManager->recentFile(dlg.GetSelection()), true, true);
		if (na)
		{
			list_resources->Append(na->getFilename(false));
			list_resources->Check(list_resources->GetCount()-1);
			archives.push_back(na);
		}
	}
}

void ResourceArchiveChooser::onResourceChecked(wxCommandEvent& e)
{
	theArchiveManager->setArchiveResource(archives[e.GetInt()], list_resources->IsChecked(e.GetInt()));
}
