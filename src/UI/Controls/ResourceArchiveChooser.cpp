
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ResourceArchiveChooser.cpp
// Description: A custom panel with controls to open/select resource archives
//              and change the base resource
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
#include "ResourceArchiveChooser.h"
#include "Archive/ArchiveManager.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"


// -----------------------------------------------------------------------------
//
// ResourceArchiveChooser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ResourceArchiveChooser class constructor
// -----------------------------------------------------------------------------
ResourceArchiveChooser::ResourceArchiveChooser(wxWindow* parent, Archive* archive) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Resource archive list
	list_resources_ = new wxCheckListBox(this, -1);
	sizer->Add(list_resources_, 1, wxEXPAND | wxBOTTOM, UI::pad());
	list_resources_->SetInitialSize(WxUtils::scaledSize(350, 100));

	// Populate resource archive list
	int index = 0;
	for (int a = 0; a < App::archiveManager().numArchives(); a++)
	{
		Archive* arch = App::archiveManager().getArchive(a);
		if (arch != archive)
		{
			list_resources_->Append(arch->filename(false));
			archives_.push_back(arch);
			if (App::archiveManager().archiveIsResource(arch))
				list_resources_->Check(index);
			index++;
		}
	}

	// 'Open Resource' button
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxRIGHT, UI::pad());
	btn_open_resource_ = new wxButton(this, -1, "Open Archive");
	hbox->Add(btn_open_resource_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// 'Open Recent' button
	btn_recent_ = new wxButton(this, -1, "Open Recent");
	hbox->Add(btn_recent_, 0, wxEXPAND, 0);

	// Bind events
	btn_open_resource_->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnOpenResource, this);
	btn_recent_->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnRecent, this);
	list_resources_->Bind(wxEVT_CHECKLISTBOX, &ResourceArchiveChooser::onResourceChecked, this);

	Layout();
}

// -----------------------------------------------------------------------------
// Returns a list of archives that have been selected as resources
// -----------------------------------------------------------------------------
vector<Archive*> ResourceArchiveChooser::getSelectedResourceArchives()
{
	wxArrayInt       checked;
	vector<Archive*> list;
	list_resources_->GetCheckedItems(checked);
	for (unsigned a = 0; a < checked.size(); a++)
		list.push_back(archives_[checked[a]]);
	return list;
}

// -----------------------------------------------------------------------------
// Returns a string of all selected resource archive filenames
// -----------------------------------------------------------------------------
string ResourceArchiveChooser::getSelectedResourceList()
{
	vector<Archive*> selected = getSelectedResourceArchives();
	string           ret;
	for (unsigned a = 0; a < selected.size(); a++)
		ret += S_FMT("\"%s\" ", selected[a]->filename());
	return ret;
}


// -----------------------------------------------------------------------------
//
// ResourceArchiveChooser Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Open Archive' button is clicked
// -----------------------------------------------------------------------------
void ResourceArchiveChooser::onBtnOpenResource(wxCommandEvent& e)
{
	SFileDialog::FDInfo info;
	if (SFileDialog::openFile(info, "Open Resource Archive", App::archiveManager().getArchiveExtensionsString(), this))
	{
		UI::showSplash("Opening Resource Archive", true);
		Archive* na = App::archiveManager().openArchive(info.filenames[0], true, true);
		UI::hideSplash();
		if (na)
		{
			list_resources_->Append(na->filename(false));
			list_resources_->Check(list_resources_->GetCount() - 1);
			archives_.push_back(na);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Open Recent' button is clicked
// -----------------------------------------------------------------------------
void ResourceArchiveChooser::onBtnRecent(wxCommandEvent& e)
{
	// Build list of recent wad filename strings
	wxArrayString recent;
	for (unsigned a = 0; a < App::archiveManager().numRecentFiles(); a++)
		recent.Add(App::archiveManager().recentFile(a));

	// Show dialog
	wxSingleChoiceDialog dlg(this, "Select a recent Archive to open", "Open Recent", recent);
	if (dlg.ShowModal() == wxID_OK)
	{
		auto na = App::archiveManager().openArchive(App::archiveManager().recentFile(dlg.GetSelection()), true, true);
		if (na)
		{
			list_resources_->Append(na->filename(false));
			list_resources_->Check(list_resources_->GetCount() - 1);
			archives_.push_back(na);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when an item in the resources list is (un)checked
// -----------------------------------------------------------------------------
void ResourceArchiveChooser::onResourceChecked(wxCommandEvent& e)
{
	App::archiveManager().setArchiveResource(archives_[e.GetInt()], list_resources_->IsChecked(e.GetInt()));
}
