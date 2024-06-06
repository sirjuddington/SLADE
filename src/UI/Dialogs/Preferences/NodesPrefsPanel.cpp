
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NodesPrefsPanel.cpp
// Description: Panel containing nodebuilder preference controls
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
#include "NodesPrefsPanel.h"
#include "MapEditor/NodeBuilders.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, nodebuilder_id)
EXTERN_CVAR(String, nodebuilder_options)


// -----------------------------------------------------------------------------
//
// NodesPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// NodesPrefsPanel class constructor
// -----------------------------------------------------------------------------
NodesPrefsPanel::NodesPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto sizer = new wxGridBagSizer(ui::pad(this), ui::pad(this));
	SetSizer(sizer);

	// Nodebuilder list
	wxArrayString builders;
	unsigned      sel = 0;
	for (unsigned a = 0; a < nodebuilders::nNodeBuilders(); a++)
	{
		builders.Add(nodebuilders::builder(a).name);
		if (nodebuilder_id == nodebuilders::builder(a).id)
			sel = a;
	}
	choice_nodebuilder_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, builders);
	sizer->Add(new wxStaticText(this, -1, "Node Builder:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_nodebuilder_, { 0, 1 }, { 1, 2 }, wxEXPAND);

	// Nodebuilder path text
	text_path_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	sizer->Add(new wxStaticText(this, -1, "Path:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_path_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Browse nodebuilder path button
	btn_browse_path_ = new wxButton(this, -1, "Browse");
	sizer->Add(btn_browse_path_, { 1, 2 }, { 1, 1 }, wxEXPAND);

	// Nodebuilder options
	clb_options_ = new wxCheckListBox(this, -1, wxDefaultPosition, wxDefaultSize);
	sizer->Add(wxutil::createLabelVBox(this, "Options:", clb_options_), { 2, 0 }, { 1, 3 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);
	sizer->AddGrowableRow(2, 1);

	// Bind events
	choice_nodebuilder_->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) { populateOptions(""); });
	btn_browse_path_->Bind(wxEVT_BUTTON, &NodesPrefsPanel::onBtnBrowse, this);

	// Init
	choice_nodebuilder_->Select(sel);
	populateOptions(nodebuilder_options);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void NodesPrefsPanel::init()
{
	unsigned sel = 0;
	for (unsigned a = 0; a < nodebuilders::nNodeBuilders(); a++)
	{
		if (nodebuilder_id == nodebuilders::builder(a).id)
		{
			sel = a;
			break;
		}
	}
	choice_nodebuilder_->Select(sel);
	populateOptions(nodebuilder_options);
}

// -----------------------------------------------------------------------------
// Populates the options CheckListBox with options for the currently selected
// node builder
// -----------------------------------------------------------------------------
void NodesPrefsPanel::populateOptions(const wxString& options) const
{
	// Get current builder
	auto& builder = nodebuilders::builder(choice_nodebuilder_->GetSelection());
	btn_browse_path_->Enable(builder.id != "none");

	// Set builder path
	text_path_->SetValue(builder.path);

	// Clear current options
	clb_options_->Clear();

	// Add builder options
	for (unsigned a = 0; a < builder.option_desc.size(); a++)
	{
		clb_options_->Append(builder.option_desc[a]);
		if (!options.IsEmpty() && options.Contains(wxString::Format(" %s ", builder.options[a])))
			clb_options_->Check(a);
	}
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void NodesPrefsPanel::applyPreferences()
{
	// Set nodebuilder
	auto& builder  = nodebuilders::builder(choice_nodebuilder_->GetSelection());
	nodebuilder_id = builder.id;

	// Set options string
	string opt = " ";
	for (unsigned a = 0; a < clb_options_->GetCount(); a++)
	{
		if (clb_options_->IsChecked(a))
		{
			opt += builder.options[a];
			opt += " ";
		}
	}
	nodebuilder_options = opt;
}


// -----------------------------------------------------------------------------
//
// NodesPrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the browse path button is clicked
// -----------------------------------------------------------------------------
void NodesPrefsPanel::onBtnBrowse(wxCommandEvent& e)
{
	auto& builder = nodebuilders::builder(choice_nodebuilder_->GetSelection());

	// Setup extension
#ifdef __WXMSW__
	auto ext = fmt::format("{}.exe|{}.exe|All Files (*.*)|*.*", builder.exe, builder.exe);
#else
	auto ext = fmt::format("{}|{}|All Files (*.*)|*.*", builder.exe, builder.exe);
#endif

	// Browse for exe
	filedialog::FDInfo info;
	if (!filedialog::openFile(info, "Browse for Nodebuilder Executable", ext, this))
		return;

	// Set builder path
	builder.path = info.filenames[0];
	text_path_->SetValue(info.filenames[0]);
}
