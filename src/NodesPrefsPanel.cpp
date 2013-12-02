
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    NodesPrefsPanel.cpp
 * Description: Panel containing nodebuilder preference controls
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
#include "NodesPrefsPanel.h"
#include "NodeBuilders.h"
#include "SFileDialog.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, nodebuilder_id)
EXTERN_CVAR(String, nodebuilder_options)


/*******************************************************************
 * NODESPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* NodesPrefsPanel::NodesPrefsPanel
 * NodesPrefsPanel class constructor
 *******************************************************************/
NodesPrefsPanel::NodesPrefsPanel(wxWindow* parent, bool useframe) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxSizer* sizer;
	if (useframe)
	{
		wxStaticBox* frame = new wxStaticBox(this, -1, "Node Builder Preferences");
		sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);
	}
	else
		sizer = psizer;

	// Nodebuilder list
	wxArrayString builders;
	unsigned sel = 0;
	for (unsigned a = 0; a < NodeBuilders::nNodeBuilders(); a++)
	{
		builders.Add(NodeBuilders::getBuilder(a).name);
		if (nodebuilder_id == NodeBuilders::getBuilder(a).id)
			sel = a;
	}
	choice_nodebuilder = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, builders);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Node Builder:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(choice_nodebuilder, 1, wxEXPAND);

	// Nodebuilder path text
	text_path = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Path:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(text_path, 1, wxEXPAND|wxRIGHT, 4);

	// Browse nodebuilder path button
	btn_browse_path = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_path, 0, wxEXPAND);

	// Nodebuilder options
	sizer->Add(new wxStaticText(this, -1, "Options:"), 0, wxLEFT|wxRIGHT, 4);
	clb_options = new wxCheckListBox(this, -1, wxDefaultPosition, wxDefaultSize);
	sizer->Add(clb_options, 1, wxEXPAND|wxALL, 4);

	// Bind events
	choice_nodebuilder->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &NodesPrefsPanel::onChoiceBuilderChanged, this);
	btn_browse_path->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &NodesPrefsPanel::onBtnBrowse, this);

	// Init
	choice_nodebuilder->Select(sel);
	populateOptions(nodebuilder_options);
}

/* NodesPrefsPanel::~NodesPrefsPanel
 * NodesPrefsPanel class destructor
 *******************************************************************/
NodesPrefsPanel::~NodesPrefsPanel()
{
}

/* NodesPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void NodesPrefsPanel::init()
{
	unsigned sel = 0;
	for (unsigned a = 0; a < NodeBuilders::nNodeBuilders(); a++)
	{
		if (nodebuilder_id == NodeBuilders::getBuilder(a).id)
		{
			sel = a;
			break;
		}
	}
	choice_nodebuilder->Select(sel);
	populateOptions(nodebuilder_options);
}

/* NodesPrefsPanel::populateOptions
 * Populates the options CheckListBox with options for the currently
 * selected node builder
 *******************************************************************/
void NodesPrefsPanel::populateOptions(string options)
{
	// Get current builder
	NodeBuilders::builder_t& builder = NodeBuilders::getBuilder(choice_nodebuilder->GetSelection());

	// Set builder path
	text_path->SetValue(builder.path);

	// Clear current options
	clb_options->Clear();

	// Add builder options
	for (unsigned a = 0; a < builder.option_desc.size(); a++)
	{
		clb_options->Append(builder.option_desc[a]);
		if (!options.IsEmpty() && options.Contains(S_FMT(" %s ", CHR(builder.options[a]))))
			clb_options->Check(a);
	}
}

/* NodesPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void NodesPrefsPanel::applyPreferences()
{
	// Set nodebuilder
	NodeBuilders::builder_t& builder = NodeBuilders::getBuilder(choice_nodebuilder->GetSelection());
	nodebuilder_id = builder.id;

	// Set options string
	string opt = " ";
	for (unsigned a = 0; a < clb_options->GetCount(); a++)
	{
		if (clb_options->IsChecked(a))
		{
			opt += builder.options[a];
			opt += " ";
		}
	}
	nodebuilder_options = opt;
}


/*******************************************************************
 * NODESPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* NodesPrefsPanel::onChoiceBuilderChanged
 * Called when the node builder dropdown is changed
 *******************************************************************/
void NodesPrefsPanel::onChoiceBuilderChanged(wxCommandEvent& e)
{
	populateOptions("");
}

/* NodesPrefsPanel::onBtnBrowse
 * Called when the browse path button is clicked
 *******************************************************************/
void NodesPrefsPanel::onBtnBrowse(wxCommandEvent& e)
{
	NodeBuilders::builder_t& builder = NodeBuilders::getBuilder(choice_nodebuilder->GetSelection());

	// Setup extension
#ifdef __WXMSW__
	string ext = S_FMT("%s.exe|%s.exe|All Files (*.*)|*.*", CHR(builder.exe), CHR(builder.exe));
#else
	string ext = S_FMT("%s|%s|All Files (*.*)|*.*", CHR(builder.exe), CHR(builder.exe));
#endif

	// Browse for exe
	SFileDialog::fd_info_t info;
	if (!SFileDialog::openFile(info, "Browse for Nodebuilder Executable", ext, this))
		return;

	// Set builder path
	builder.path = info.filenames[0];
	text_path->SetValue(info.filenames[0]);
}
