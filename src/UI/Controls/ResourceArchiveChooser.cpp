
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor/MainEditor.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ResourceArchiveChooser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ResourceArchiveChooser class constructor
// -----------------------------------------------------------------------------
ResourceArchiveChooser::ResourceArchiveChooser(wxWindow* parent, const Archive* archive) : wxPanel(parent, -1)
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Resource archive list
	list_resources_ = new wxCheckListBox(this, -1);
	sizer->Add(list_resources_, lh.sfWithBorder(1, wxBOTTOM).Expand());
	list_resources_->SetInitialSize(lh.size(350, 100));

	// Populate resource archive list
	int index = 0;
	for (int a = 0; a < app::archiveManager().numArchives(); a++)
	{
		auto arch = app::archiveManager().getArchive(a).get();
		if (arch != archive)
		{
			list_resources_->Append(arch->filename(false));
			archives_.push_back(arch);
			if (app::archiveManager().archiveIsResource(arch))
				list_resources_->Check(index);
			index++;
		}
	}

	// 'Open Resource' button
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxRIGHT).Expand());
	btn_open_resource_ = new wxButton(this, -1, "Open Archive");
	hbox->Add(btn_open_resource_, lh.sfWithBorder(0, wxRIGHT).Expand());

	// 'Open Recent' button
	btn_recent_ = new wxButton(this, -1, "Open Recent");
	hbox->Add(btn_recent_, wxSizerFlags().Expand());

	// Bind events
	btn_open_resource_->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnOpenResource, this);
	btn_recent_->Bind(wxEVT_BUTTON, &ResourceArchiveChooser::onBtnRecent, this);
	list_resources_->Bind(wxEVT_CHECKLISTBOX, &ResourceArchiveChooser::onResourceChecked, this);

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Returns a list of archives that have been selected as resources
// -----------------------------------------------------------------------------
vector<Archive*> ResourceArchiveChooser::selectedResourceArchives() const
{
	wxArrayInt       checked;
	vector<Archive*> list;
	list_resources_->GetCheckedItems(checked);
	for (int a : checked)
		list.push_back(archives_[a]);
	return list;
}

// -----------------------------------------------------------------------------
// Returns a string of all selected resource archive filenames
// -----------------------------------------------------------------------------
wxString ResourceArchiveChooser::selectedResourceList() const
{
	vector<Archive*> selected = selectedResourceArchives();
	wxString         ret;
	for (auto a : selected)
		ret += wxString::Format("\"%s\" ", a->filename());
	return ret;
}


// -----------------------------------------------------------------------------
//
// ResourceArchiveChooser Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Open Archive' button is clicked
// -----------------------------------------------------------------------------
void ResourceArchiveChooser::onBtnOpenResource(wxCommandEvent& e)
{
	filedialog::FDInfo info;
	if (filedialog::openFile(info, "Open Resource Archive", app::archiveManager().getArchiveExtensionsString(), this))
	{
		ui::showSplash("Opening Resource Archive", true, maineditor::windowWx());
		auto na = app::archiveManager().openArchive(info.filenames[0], true, true);
		ui::hideSplash();
		if (na)
		{
			list_resources_->Append(na->filename(false));
			list_resources_->Check(list_resources_->GetCount() - 1);
			archives_.push_back(na.get());
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
	for (unsigned a = 0; a < app::archiveManager().numRecentFiles(); a++)
		recent.Add(app::archiveManager().recentFile(a));

	// Show dialog
	wxSingleChoiceDialog dlg(this, "Select a recent Archive to open", "Open Recent", recent);
	if (dlg.ShowModal() == wxID_OK)
	{
		auto na = app::archiveManager().openArchive(app::archiveManager().recentFile(dlg.GetSelection()), true, true);
		if (na)
		{
			list_resources_->Append(na->filename(false));
			list_resources_->Check(list_resources_->GetCount() - 1);
			archives_.push_back(na.get());
		}
	}
}

// -----------------------------------------------------------------------------
// Called when an item in the resources list is (un)checked
// -----------------------------------------------------------------------------
void ResourceArchiveChooser::onResourceChecked(wxCommandEvent& e)
{
	app::archiveManager().setArchiveResource(archives_[e.GetInt()], list_resources_->IsChecked(e.GetInt()));
}
