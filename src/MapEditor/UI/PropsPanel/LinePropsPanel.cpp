
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LinePropsPanel.cpp
// Description: UI for editing line properties - has tabs for flags,
//              special, args, sides and other properties
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "Game/Configuration.h"
#include "LinePropsPanel.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapObjectPropsPanel.h"
#include "SidePropsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// LinePropsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// LinePropsPanel::LinePropsPanel
//
// LinePropsPanel class constructor
// ----------------------------------------------------------------------------
LinePropsPanel::LinePropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND);

	// General tab
	stc_tabs_->AddPage(setupGeneralTab(), "General");

	// Special tab
	stc_tabs_->AddPage(setupSpecialTab(), "Special");

	// Args tab
	if (MapEditor::editContext().mapDesc().format != MAP_DOOM)
	{
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(WxUtils::createPadPanel(stc_tabs_, panel_args_), "Args");
		panel_special_->setArgsPanel(panel_args_);
	}

	// Front side tab
	panel_side1_ = new SidePropsPanel(this);
	stc_tabs_->AddPage(WxUtils::createPadPanel(stc_tabs_, panel_side1_), "Front Side");

	// Back side tab
	panel_side2_ = new SidePropsPanel(this);
	stc_tabs_->AddPage(WxUtils::createPadPanel(stc_tabs_, panel_side2_), "Back Side");

	// All properties tab
	mopp_all_props_ = new MapObjectPropsPanel(stc_tabs_, true);
	mopp_all_props_->hideFlags(true);
	mopp_all_props_->hideTriggers(true);
	mopp_all_props_->hideProperty("special");
	mopp_all_props_->hideProperty("arg0");
	mopp_all_props_->hideProperty("arg1");
	mopp_all_props_->hideProperty("arg2");
	mopp_all_props_->hideProperty("arg3");
	mopp_all_props_->hideProperty("arg4");
	mopp_all_props_->hideProperty("texturetop");
	mopp_all_props_->hideProperty("texturemiddle");
	mopp_all_props_->hideProperty("texturebottom");
	mopp_all_props_->hideProperty("offsetx");
	mopp_all_props_->hideProperty("offsety");
	mopp_all_props_->hideProperty("id");
	stc_tabs_->AddPage(mopp_all_props_, "Other Properties");

	// Bind events
	cb_override_special_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e)
	{
		panel_special_->Enable(cb_override_special_->IsChecked());
	});
}

// ----------------------------------------------------------------------------
// LinePropsPanel::~LinePropsPanel
//
// LinePropsPanel class destructor
// ----------------------------------------------------------------------------
LinePropsPanel::~LinePropsPanel()
{
	mopp_all_props_->clearGrid();
}

// ----------------------------------------------------------------------------
// LinePropsPanel::setupGeneralTab
//
// Creates and sets up the 'General' properties tab panel
// ----------------------------------------------------------------------------
wxPanel* LinePropsPanel::setupGeneralTab()
{
	wxPanel* panel_flags = new wxPanel(stc_tabs_, -1);
	int map_format = MapEditor::editContext().mapDesc().format;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel_flags->SetSizer(sizer);

	// Flags
	wxStaticBoxSizer* sizer_flags = new wxStaticBoxSizer(wxVERTICAL, panel_flags, "Flags");
	sizer->Add(sizer_flags, 0, wxEXPAND|wxALL, UI::pad());

	// Init flags
	wxGridBagSizer* gb_sizer_flags = new wxGridBagSizer(UI::pad() / 2, UI::pad());
	sizer_flags->Add(gb_sizer_flags, 1, wxEXPAND|wxALL, UI::pad());
	unsigned row = 0;
	unsigned col = 0;

	// Get all UDMF properties
	auto& props = Game::configuration().allUDMFProperties(MOBJ_LINE);

	// UDMF flags
	if (map_format == MAP_UDMF)
	{
		// Get all udmf flag properties
		vector<UDMFProperty> flags_udmf;
		for (auto& i : props)
			if (i.second.isFlag())
				flags_udmf.push_back(i.second);

		// Add flag checkboxes
		unsigned flag_mid = flags_udmf.size() / 3;
		if (flags_udmf.size() % 3 == 0) flag_mid--;
		for (unsigned a = 0; a < flags_udmf.size(); a++)
		{
			auto cb_flag = new wxCheckBox(
				panel_flags,
				-1,
				flags_udmf[a].name(),
				wxDefaultPosition,
				wxDefaultSize,
				wxCHK_3STATE
			);
			gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			flags_.push_back({ cb_flag, (int)a, flags_udmf[a].propName() });

			if (row > flag_mid)
			{
				row = 0;
				col++;
			}
		}
	}

	// Non-UDMF flags
	else
	{
		// Add flag checkboxes
		unsigned flag_mid = Game::configuration().nLineFlags() / 3;
		if (Game::configuration().nLineFlags() % 3 == 0) flag_mid--;
		for (unsigned a = 0; a < Game::configuration().nLineFlags(); a++)
		{
			if (Game::configuration().lineFlag(a).activation)
				continue;

			wxCheckBox* cb_flag = new wxCheckBox(
				panel_flags,
				-1,
				Game::configuration().lineFlag(a).name,
				wxDefaultPosition,
				wxDefaultSize,
				wxCHK_3STATE
			);
			gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			flags_.push_back({ cb_flag, (int)a, wxEmptyString });

			if (row > flag_mid)
			{
				row = 0;
				col++;
			}
		}
	}

	gb_sizer_flags->AddGrowableCol(0, 1);
	gb_sizer_flags->AddGrowableCol(1, 1);
	gb_sizer_flags->AddGrowableCol(2, 1);

	// Sector tag
	if (map_format == MAP_DOOM)
	{
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

		hbox->Add(new wxStaticText(panel_flags, -1, "Sector Tag:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, UI::pad());
		hbox->Add(text_tag_ = new NumberTextCtrl(panel_flags), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, UI::pad());
		btn_new_tag_ = new wxButton(panel_flags, -1, "New Tag");
		hbox->Add(btn_new_tag_, 0, wxEXPAND);

		// Bind event
		btn_new_tag_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [&](wxCommandEvent& e)
		{
			text_tag_->setNumber(MapEditor::editContext().map().findUnusedSectorTag());
		});
	}

	// Id
	if (map_format == MAP_UDMF)
	{
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

		hbox->Add(new wxStaticText(panel_flags, -1, "Line ID:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, UI::pad());
		hbox->Add(text_id_ = new NumberTextCtrl(panel_flags), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, UI::pad());
		hbox->Add(btn_new_id_ = new wxButton(panel_flags, -1, "New ID"), 0, wxEXPAND);

		// Bind event
		btn_new_id_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [&](wxCommandEvent& e)
		{
			text_id_->setNumber(MapEditor::editContext().map().findUnusedLineId());
		});
	}

	return panel_flags;
}

// ----------------------------------------------------------------------------
// LinePropsPanel::setupSpecialTab
//
// Creates and sets up the 'Special' properties tab
// ----------------------------------------------------------------------------
wxPanel* LinePropsPanel::setupSpecialTab()
{
	wxPanel* panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Action special panel
	panel_special_ = new ActionSpecialPanel(panel);
	sizer->Add(panel_special_, 1, wxEXPAND|wxALL, UI::pad());

	// 'Override Special' checkbox
	cb_override_special_ = new wxCheckBox(panel, -1, "Override Action Special");
	cb_override_special_->SetToolTip(
		"Differing action specials detected, tick this to set the action special for all selected lines"
	);
	sizer->Add(cb_override_special_, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	return panel;
}

// ----------------------------------------------------------------------------
// LinePropsPanel::openLines
//
// Loads values from all lines/sides in [lines]
// ----------------------------------------------------------------------------
void LinePropsPanel::openObjects(vector<MapObject*>& lines)
{
	if (lines.empty())
		return;

	int map_format = MapEditor::editContext().mapDesc().format;

	// Load flags
	if (map_format == MAP_UDMF)
	{
		bool val = false;
		for (unsigned a = 0; a < flags_.size(); a++)
		{
			if (MapObject::multiBoolProperty(lines, flags_[a].udmf, val))
				flags_[a].check_box->SetValue(val);
			else
				flags_[a].check_box->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}
	else
	{
		for (auto& flag : flags_)
		{
			// Set initial flag checked value
			flag.check_box->SetValue(Game::configuration().lineFlagSet(flag.index, (MapLine*)lines[0]));

			// Go through subsequent lines
			for (unsigned b = 1; b < lines.size(); b++)
			{
				// Check for mismatch			
				if (flag.check_box->GetValue() !=
					Game::configuration().lineFlagSet(flag.index, (MapLine*)lines[b]))
				{
					// Set undefined
					flag.check_box->Set3StateValue(wxCHK_UNDETERMINED);
					break;
				}
			}
		}
	}

	// Load special/trigger(s)/args
	panel_special_->openLines(lines);

	// Check action special
	int special = lines[0]->intProperty("special");
	for (unsigned a = 1; a < lines.size(); a++)
	{
		if (lines[a]->intProperty("special") != special)
		{
			special = -1;
			break;
		}
	}
	if (special >= 0)
	{
		// Enable special tab
		cb_override_special_->Enable(false);
		cb_override_special_->Show(false);
	}
	else
	{
		cb_override_special_->Enable(true);
		cb_override_special_->SetValue(false);
		cb_override_special_->Enable(true);
	}

	// Sector tag
	if (map_format == MAP_DOOM)
	{
		int tag = -1;
		if (MapObject::multiIntProperty(lines, "arg0", tag))
			text_tag_->setNumber(tag);
	}

	// Line ID
	if (map_format == MAP_UDMF)
	{
		int id = -1;
		if (MapObject::multiIntProperty(lines, "id", id))
			text_id_->setNumber(id);
	}

	// First side
	vector<MapSide*> sides;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (MapSide* s = ((MapLine*)lines[a])->s1())
			sides.push_back(s);
	}
	if (sides.empty())
		panel_side1_->Enable(false);
	else
		panel_side1_->openSides(sides);

	// Second side
	sides.clear();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (MapSide* s = ((MapLine*)lines[a])->s2())
			sides.push_back(s);
	}
	if (sides.empty())
		panel_side2_->Enable(false);
	else
		panel_side2_->openSides(sides);

	// Load all properties
	mopp_all_props_->openObjects(lines);

	// Update internal objects list
	this->objects_.clear();
	for (unsigned a = 0; a < lines.size(); a++)
		this->objects_.push_back(lines[a]);

	// Update layout
	Layout();
	Refresh();
}

// ----------------------------------------------------------------------------
// LinePropsPanel::applyChanges
//
// Applies values to [lines]
// ----------------------------------------------------------------------------
void LinePropsPanel::applyChanges()
{
	int map_format = MapEditor::editContext().mapDesc().format;

	// Apply general properties
	for (unsigned l = 0; l < objects_.size(); l++)
	{
		// Flags
		if (map_format == MAP_UDMF)
		{
			// UDMF
			for (auto& flag : flags_)
				if (flag.check_box->Get3StateValue() != wxCHK_UNDETERMINED)
					objects_[l]->setBoolProperty(flag.udmf, flag.check_box->GetValue());
		}
		else
		{
			// Other
			for (auto& flag : flags_)
				if (flag.check_box->Get3StateValue() != wxCHK_UNDETERMINED)
					Game::configuration().setLineFlag(
						flag.index, (MapLine*)objects_[l],
						flag.check_box->GetValue()
					);
		}

		// Sector tag
		if (map_format == MAP_DOOM && !text_tag_->IsEmpty())
			objects_[l]->setIntProperty("arg0", text_tag_->getNumber(objects_[l]->intProperty("arg0")));

		// Line ID
		if (map_format == MAP_UDMF && !text_id_->IsEmpty())
			objects_[l]->setIntProperty("id", text_id_->getNumber(objects_[l]->intProperty("id")));
	}

	// Apply special
	panel_special_->applyTo(objects_, !cb_override_special_->IsShown() || cb_override_special_->GetValue());

	// Apply first side
	vector<MapSide*> sides;
	for (unsigned a = 0; a < objects_.size(); a++)
	{
		if (MapSide* s = ((MapLine*)objects_[a])->s1())
			sides.push_back(s);
	}
	if (!sides.empty())
		panel_side1_->applyTo(sides);

	// Apply second side
	sides.clear();
	for (unsigned a = 0; a < objects_.size(); a++)
	{
		if (MapSide* s = ((MapLine*)objects_[a])->s2())
			sides.push_back(s);
	}
	if (!sides.empty())
		panel_side2_->applyTo(sides);

	// Apply other properties
	mopp_all_props_->applyChanges();
}
