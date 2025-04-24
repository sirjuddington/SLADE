
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "LinePropsPanel.h"
#include "Game/Configuration.h"
#include "Game/UDMFProperty.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/ActionSpecialPanel.h"
#include "MapEditor/UI/ArgsPanel.h"
#include "MapObjectPropsPanel.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/SLADEMap.h"
#include "SidePropsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// LinePropsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// LinePropsPanel class constructor
// -----------------------------------------------------------------------------
LinePropsPanel::LinePropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND);

	// General tab
	stc_tabs_->AddPage(setupGeneralTab(), wxS("General"));

	// Special tab
	stc_tabs_->AddPage(setupSpecialTab(), wxS("Special"));

	// Args tab
	if (mapeditor::editContext().mapDesc().format != MapFormat::Doom)
	{
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_args_), wxS("Args"));
		panel_special_->setArgsPanel(panel_args_);
	}

	// Textures tab
	stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, setupTexturesTab()), wxS("Textures"));

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
	stc_tabs_->AddPage(mopp_all_props_, wxS("Other Properties"));

	// Bind events
	cb_override_special_->Bind(
		wxEVT_CHECKBOX, [&](wxCommandEvent& e) { panel_special_->Enable(cb_override_special_->IsChecked()); });
}

// -----------------------------------------------------------------------------
// LinePropsPanel class destructor
// -----------------------------------------------------------------------------
LinePropsPanel::~LinePropsPanel()
{
	mopp_all_props_->clearGrid();
}

// -----------------------------------------------------------------------------
// Loads values from all lines/sides in [lines]
// -----------------------------------------------------------------------------
void LinePropsPanel::openObjects(vector<MapObject*>& lines)
{
	if (lines.empty())
		return;

	auto map_format = mapeditor::editContext().mapDesc().format;

	// Load flags
	if (map_format == MapFormat::UDMF)
	{
		bool val = false;
		for (auto& flag : flags_)
		{
			if (MapObject::multiBoolProperty(lines, flag.udmf, val))
				flag.check_box->SetValue(val);
			else
				flag.check_box->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}
	else
	{
		for (auto& flag : flags_)
		{
			// Set initial flag checked value
			flag.check_box->SetValue(game::configuration().lineFlagSet(flag.index, dynamic_cast<MapLine*>(lines[0])));

			// Go through subsequent lines
			for (unsigned b = 1; b < lines.size(); b++)
			{
				// Check for mismatch
				if (flag.check_box->GetValue()
					!= game::configuration().lineFlagSet(flag.index, dynamic_cast<MapLine*>(lines[b])))
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
	if (map_format == MapFormat::Doom)
	{
		int tag = -1;
		if (MapObject::multiIntProperty(lines, "arg0", tag))
			text_tag_->setNumber(tag);
	}

	// Line ID
	if (map_format == MapFormat::UDMF)
	{
		int id = -1;
		if (MapObject::multiIntProperty(lines, "id", id))
			text_id_->setNumber(id);
	}

	// First side
	vector<MapSide*> sides;
	for (auto& line : lines)
	{
		if (auto s = dynamic_cast<MapLine*>(line)->s1())
			sides.push_back(s);
	}
	if (sides.empty())
		panel_side1_->Enable(false);
	else
		panel_side1_->openSides(sides);

	// Second side
	sides.clear();
	for (auto& line : lines)
	{
		if (auto s = dynamic_cast<MapLine*>(line)->s2())
			sides.push_back(s);
	}
	if (sides.empty())
		panel_side2_->Enable(false);
	else
		panel_side2_->openSides(sides);

	// Load all properties
	mopp_all_props_->openObjects(lines);

	// Update internal objects list
	objects_.clear();
	for (auto line : lines)
		objects_.push_back(line);

	// Update layout
	Layout();
	Refresh();
}

// -----------------------------------------------------------------------------
// Applies values to [lines]
// -----------------------------------------------------------------------------
void LinePropsPanel::applyChanges()
{
	auto map_format = mapeditor::editContext().mapDesc().format;

	// Apply general properties
	for (auto& object : objects_)
	{
		// Flags
		if (map_format == MapFormat::UDMF)
		{
			// UDMF
			for (auto& flag : flags_)
				if (flag.check_box->Get3StateValue() != wxCHK_UNDETERMINED)
					object->setBoolProperty(flag.udmf, flag.check_box->GetValue());
		}
		else
		{
			// Other
			for (auto& flag : flags_)
				if (flag.check_box->Get3StateValue() != wxCHK_UNDETERMINED)
					game::configuration().setLineFlag(
						flag.index, dynamic_cast<MapLine*>(object), flag.check_box->GetValue());
		}

		// Sector tag
		if (map_format == MapFormat::Doom && !text_tag_->IsEmpty())
			object->setIntProperty("arg0", text_tag_->number(object->intProperty("arg0")));

		// Line ID
		if (map_format == MapFormat::UDMF && !text_id_->IsEmpty())
			object->setIntProperty("id", text_id_->number(object->intProperty("id")));
	}

	// Apply special
	panel_special_->applyTo(objects_, !cb_override_special_->IsShown() || cb_override_special_->GetValue());

	// Apply first side
	vector<MapSide*> sides;
	for (auto& object : objects_)
	{
		if (auto s = dynamic_cast<MapLine*>(object)->s1())
			sides.push_back(s);
	}
	if (!sides.empty())
		panel_side1_->applyTo(sides);

	// Apply second side
	sides.clear();
	for (auto& object : objects_)
	{
		if (auto s = dynamic_cast<MapLine*>(object)->s2())
			sides.push_back(s);
	}
	if (!sides.empty())
		panel_side2_->applyTo(sides);

	// Apply other properties
	mopp_all_props_->applyChanges();
}

// -----------------------------------------------------------------------------
// Creates and sets up the 'General' properties tab panel
// -----------------------------------------------------------------------------
wxPanel* LinePropsPanel::setupGeneralTab()
{
	auto lh = ui::LayoutHelper(this);

	auto panel_flags = new wxPanel(stc_tabs_, -1);
	auto map_format  = mapeditor::editContext().mapDesc().format;

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel_flags->SetSizer(sizer);

	// Flags
	auto sizer_flags = new wxStaticBoxSizer(wxVERTICAL, panel_flags, wxS("Flags"));
	sizer->Add(sizer_flags, lh.sfWithBorder().Expand());

	// Init flags
	auto gb_sizer_flags = new wxGridBagSizer(lh.pad() / 2, lh.pad());
	sizer_flags->Add(gb_sizer_flags, lh.sfWithBorder(1).Expand());
	unsigned row = 0;
	unsigned col = 0;

	// Get all UDMF properties
	auto& props = game::configuration().allUDMFProperties(MapObject::Type::Line);

	// UDMF flags
	if (map_format == MapFormat::UDMF)
	{
		// Get all udmf flag properties
		vector<game::UDMFProperty> flags_udmf;
		for (auto& i : props)
			if (i.second.isFlag())
				flags_udmf.push_back(i.second);

		// Add flag checkboxes
		unsigned flag_mid = flags_udmf.size() / 3;
		if (flags_udmf.size() % 3 == 0)
			flag_mid--;
		for (unsigned a = 0; a < flags_udmf.size(); a++)
		{
			auto cb_flag = new wxCheckBox(
				panel_flags,
				-1,
				wxString::FromUTF8(flags_udmf[a].name()),
				wxDefaultPosition,
				wxDefaultSize,
				wxCHK_3STATE);
			gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			flags_.push_back({ cb_flag, static_cast<int>(a), flags_udmf[a].propName() });

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
		unsigned flag_mid = game::configuration().nLineFlags() / 3;
		if (game::configuration().nLineFlags() % 3 == 0)
			flag_mid--;
		for (unsigned a = 0; a < game::configuration().nLineFlags(); a++)
		{
			if (game::configuration().lineFlag(a).activation)
				continue;

			auto cb_flag = new wxCheckBox(
				panel_flags,
				-1,
				wxString::FromUTF8(game::configuration().lineFlag(a).name),
				wxDefaultPosition,
				wxDefaultSize,
				wxCHK_3STATE);
			gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			flags_.push_back({ cb_flag, static_cast<int>(a), "" });

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
	if (map_format == MapFormat::Doom)
	{
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

		hbox->Add(new wxStaticText(panel_flags, -1, wxS("Sector Tag:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
		hbox->Add(text_tag_ = new NumberTextCtrl(panel_flags), lh.sfWithBorder(1, wxRIGHT).CenterVertical());
		btn_new_tag_ = new wxButton(panel_flags, -1, wxS("New Tag"));
		hbox->Add(btn_new_tag_, wxSizerFlags().Expand());

		// Bind event
		btn_new_tag_->Bind(
			wxEVT_COMMAND_BUTTON_CLICKED,
			[&](wxCommandEvent& e) { text_tag_->setNumber(mapeditor::editContext().map().sectors().firstFreeId()); });
	}

	// Id
	if (map_format == MapFormat::UDMF)
	{
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

		hbox->Add(new wxStaticText(panel_flags, -1, wxS("Line ID:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
		hbox->Add(text_id_ = new NumberTextCtrl(panel_flags), lh.sfWithBorder(1, wxRIGHT).CenterVertical());
		hbox->Add(btn_new_id_ = new wxButton(panel_flags, -1, wxS("New ID")), wxSizerFlags().Expand());

		// Bind event
		btn_new_id_->Bind(
			wxEVT_COMMAND_BUTTON_CLICKED,
			[&](wxCommandEvent& e)
			{
				auto& map = mapeditor::editContext().map();
				text_id_->setNumber(map.lines().firstFreeId(map.currentFormat()));
			});
	}

	return panel_flags;
}

// -----------------------------------------------------------------------------
// Creates and sets up the 'Special' properties tab
// -----------------------------------------------------------------------------
wxPanel* LinePropsPanel::setupSpecialTab()
{
	auto panel = new wxPanel(stc_tabs_, -1);
	auto lh    = ui::LayoutHelper(panel);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Action special panel
	panel_special_ = new ActionSpecialPanel(panel);
	sizer->Add(panel_special_, lh.sfWithBorder(1).Expand());

	// 'Override Special' checkbox
	cb_override_special_ = new wxCheckBox(panel, -1, wxS("Override Action Special"));
	cb_override_special_->SetToolTip(
		wxS("Differing action specials detected, tick this to set the action special for all selected lines"));
	sizer->Add(cb_override_special_, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates and sets up the 'Textures' tab
// -----------------------------------------------------------------------------
wxPanel* LinePropsPanel::setupTexturesTab()
{
	auto panel = new wxPanel(stc_tabs_, -1);
	auto lh    = ui::LayoutHelper(panel);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Front side
	auto sbs_front = new wxStaticBoxSizer(wxVERTICAL, panel, wxS("Front Side"));
	sbs_front->AddSpacer(lh.padSmall());
	sbs_front->Add(panel_side1_ = new SidePropsPanel(panel), wxSizerFlags(1).Expand());
	sizer->Add(sbs_front, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Back side
	auto sbs_back = new wxStaticBoxSizer(wxVERTICAL, panel, wxS("Back Side"));
	sbs_back->AddSpacer(lh.padSmall());
	sbs_back->Add(panel_side2_ = new SidePropsPanel(panel), wxSizerFlags(1).Expand());
	sizer->Add(sbs_back, lh.sfWithBorder(0, wxBOTTOM).Expand());

	return panel;
}
