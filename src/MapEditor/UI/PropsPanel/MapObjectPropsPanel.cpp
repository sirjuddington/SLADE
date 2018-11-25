
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    MapObjectPropsPanel.cpp
// Description: MapObjectPropsPanel class, a wx panel containing a property
//              grid for viewing/editing map object properties
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
#include "Graphics/Icons.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "MapObjectPropsPanel.h"
#include "MOPGProperty.h"
#include "UI/WxUtils.h"
#include "UI/Controls/SIconButton.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Bool, mobj_props_show_all, false, CVAR_SAVE)
CVAR(Bool, mobj_props_auto_apply, false, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// MapObjectPropsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MapObjectPropsPanel::MapObjectPropsPanel
//
// MapObjectPropsPanel class constructor
// ----------------------------------------------------------------------------
MapObjectPropsPanel::MapObjectPropsPanel(wxWindow* parent, bool no_apply) :
	PropsPanelBase{ parent },
	no_apply_{ no_apply }
{
	// Setup sizer
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add item label
	cb_show_all_ = new wxCheckBox(this, -1, "Show All");
	cb_show_all_->SetValue(mobj_props_show_all);
	sizer->Add(cb_show_all_, 0, wxEXPAND|wxALL, UI::pad());

	// Add tabs
	stc_sections_ = STabCtrl::createControl(this);
	sizer->Add(stc_sections_, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	const wxColour& inactiveTextColour =
		wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

	// Add main property grid
	pg_properties_ = new wxPropertyGrid(
		stc_sections_,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER
	);
	pg_properties_->SetCaptionTextColour(inactiveTextColour);
	pg_properties_->SetCellDisabledTextColour(inactiveTextColour);
	stc_sections_->AddPage(pg_properties_, "Properties");

	// Create side property grids
	pg_props_side1_ = new wxPropertyGrid(
		stc_sections_,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER
	);
	pg_props_side1_->SetCaptionTextColour(inactiveTextColour);
	pg_props_side1_->SetCellDisabledTextColour(inactiveTextColour);
	pg_props_side2_ = new wxPropertyGrid(
		stc_sections_,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER
	);
	pg_props_side2_->SetCaptionTextColour(inactiveTextColour);
	pg_props_side2_->SetCellDisabledTextColour(inactiveTextColour);

	// Add buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Add button
	btn_add_ = new SIconButton(this, "plus", "Add Property");
	hbox->Add(btn_add_, 0, wxEXPAND|wxRIGHT, UI::pad());
	hbox->AddStretchSpacer(1);

	// Reset button
	btn_reset_ = new SIconButton(this, "close", "Discard Changes");
	hbox->Add(btn_reset_, 0, wxEXPAND|wxRIGHT, UI::pad());

	// Apply button
	btn_apply_ = new SIconButton(this, "tick", "Apply Changes");
	hbox->Add(btn_apply_, 0, wxEXPAND);

	wxPGCell cell;
	cell.SetText("<multiple values>");
	cell.SetFgCol(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
	pg_properties_->GetGrid()->SetUnspecifiedValueAppearance(cell);
	pg_props_side1_->GetGrid()->SetUnspecifiedValueAppearance(cell);
	pg_props_side2_->GetGrid()->SetUnspecifiedValueAppearance(cell);

	// Bind events
	btn_apply_->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnApply, this);
	btn_reset_->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnReset, this);
	cb_show_all_->Bind(wxEVT_CHECKBOX, &MapObjectPropsPanel::onShowAllToggled, this);
	btn_add_->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnAdd, this);
	pg_properties_->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);
	pg_props_side1_->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);
	pg_props_side2_->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);

	// Hide side property grids
	pg_props_side1_->Show(false);
	pg_props_side2_->Show(false);

	// Hide apply button if needed
	if (no_apply || mobj_props_auto_apply)
	{
		btn_apply_->Show(false);
		btn_reset_->Show(false);
	}

	Layout();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::showAll
//
// Returns true if 'Show All' is ticked
// ----------------------------------------------------------------------------
bool MapObjectPropsPanel::showAll()
{
	return cb_show_all_->IsChecked();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addBoolProperty
//
// Adds a boolean property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addBoolProperty(
	wxPGProperty* group,
	string label,
	string propname,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGBoolProperty* prop = new MOPGBoolProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addIntProperty
//
// Adds an integer property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addIntProperty(
	wxPGProperty* group,
	string label,
	string propname,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGIntProperty* prop = new MOPGIntProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addFloatProperty
//
// Adds a float property cell to the grid under [group] for the object property
// [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addFloatProperty(
	wxPGProperty* group,
	string label,
	string propname,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGFloatProperty* prop = new MOPGFloatProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addStringProperty
//
// Adds a string property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addStringProperty(
	wxPGProperty* group,
	string label,
	string propname,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGStringProperty* prop = new MOPGStringProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addLineFlagProperty
//
// Adds a line flag property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addLineFlagProperty(
	wxPGProperty* group,
	string label,
	string propname,
	int index,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGLineFlagProperty* prop = new MOPGLineFlagProperty(label, propname, index);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addThingFlagProperty
//
// Adds a thing flag property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addThingFlagProperty(
	wxPGProperty* group,
	string label,
	string propname,
	int index,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGThingFlagProperty* prop = new MOPGThingFlagProperty(label, propname, index);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addTextureProperty
//
// Adds a texture property cell to the grid under [group] for the object
// property [propname]
// ----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addTextureProperty(
	wxPGProperty* group, 
	string label,
	string propname,
	int textype,
	bool readonly,
	wxPropertyGrid* grid,
	UDMFProperty* udmf_prop)
{
	// Create property
	MOPGTextureProperty* prop = new MOPGTextureProperty(textype, label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties_.push_back(prop);
	if (!grid)	pg_properties_->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::setBoolProperty
//
// Sets the boolean property cell [prop]'s value to [value]
// ----------------------------------------------------------------------------
bool MapObjectPropsPanel::setBoolProperty(wxPGProperty* prop, bool value, bool force_set)
{
	// Set if forcing
	if (prop && force_set)
	{
		prop->SetValue(value);
		return false;
	}

	// Ignore if already unspecified
	if (!prop || prop->IsValueUnspecified())
		return true;

	// Set to unspecified if values mismatch
	if (prop->GetValue().GetBool() != value)
	{
		prop->SetValueToUnspecified();
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::addUDMFProperty
//
// Adds the UDMF property [prop] to the grid, under [basegroup]. Will add the
// correct property cell type for the UDMF property
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::addUDMFProperty(UDMFProperty& prop, int objtype, string basegroup, wxPropertyGrid* grid)
{
	// Set grid to add to (main one if grid is NULL)
	if (!grid)
		grid = pg_properties_;

	// Determine group name
	string groupname;
	if (!basegroup.IsEmpty())
		groupname = basegroup + ".";
	groupname += prop.group();

	// Get group to add
	wxPGProperty* group = grid->GetProperty(groupname);
	if (!group)
		group = grid->Append(new wxPropertyCategory(prop.group(), groupname));

	// Determine property name
	string propname;
	if (!basegroup.IsEmpty())
		propname = basegroup + ".";
	propname += prop.propName();

	// Add property depending on type
	if (prop.type() == UDMFProperty::Type::Boolean)
		addBoolProperty(group, prop.name(), propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Int)
		addIntProperty(group, prop.name(), propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Float)
		addFloatProperty(group, prop.name(), propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::String)
		addStringProperty(group, prop.name(), propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Colour)
	{
		MOPGColourProperty* prop_col = new MOPGColourProperty(prop.name(), propname);
		prop_col->setParent(this);
		prop_col->setUDMFProp(&prop);
		properties_.push_back(prop_col);
		grid->AppendIn(group, prop_col);
	}
	else if (prop.type() == UDMFProperty::Type::ActionSpecial)
	{
		MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty(prop.name(), propname);
		prop_as->setParent(this);
		prop_as->setUDMFProp(&prop);
		properties_.push_back(prop_as);
		grid->AppendIn(group, prop_as);
	}
	else if (prop.type() == UDMFProperty::Type::SectorSpecial)
	{
		MOPGSectorSpecialProperty* prop_ss = new MOPGSectorSpecialProperty(prop.name(), propname);
		prop_ss->setParent(this);
		prop_ss->setUDMFProp(&prop);
		properties_.push_back(prop_ss);
		grid->AppendIn(group, prop_ss);
	}
	else if (prop.type() == UDMFProperty::Type::ThingType)
	{
		MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty(prop.name(), propname);
		prop_tt->setParent(this);
		prop_tt->setUDMFProp(&prop);
		properties_.push_back(prop_tt);
		grid->AppendIn(group, prop_tt);
	}
	else if (prop.type() == UDMFProperty::Type::Angle)
	{
		MOPGAngleProperty* prop_angle = new MOPGAngleProperty(prop.name(), propname);
		prop_angle->setParent(this);
		prop_angle->setUDMFProp(&prop);
		properties_.push_back(prop_angle);
		grid->AppendIn(group, prop_angle);
	}
	else if (prop.type() == UDMFProperty::Type::TextureWall)
		addTextureProperty(group, prop.name(), propname, 0, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::TextureFlat)
		addTextureProperty(group, prop.name(), propname, 1, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ID)
	{
		int tagtype;
		if (objtype == MOBJ_LINE)
			tagtype = MOPGTagProperty::TT_LINEID;
		else if (objtype == MOBJ_THING)
			tagtype = MOPGTagProperty::TT_THINGID;
		else
			tagtype = MOPGTagProperty::TT_SECTORTAG;

		MOPGTagProperty* prop_id = new MOPGTagProperty(tagtype, prop.name(), propname);
		prop_id->setParent(this);
		prop_id->setUDMFProp(&prop);
		properties_.push_back(prop_id);
		grid->AppendIn(group, prop_id);
	}
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::setupType
//
// Adds all relevant properties to the grid for [objtype]
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::setupType(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type_ == objtype && !udmf_)
		return;

	// Get map format
	int map_format = MapEditor::editContext().mapDesc().format;

	// Clear property grid
	clearGrid();
	btn_add_->Show(false);

	// Hide buttons if not needed
	if (no_apply_ || mobj_props_auto_apply)
	{
		btn_apply_->Show(false);
		btn_reset_->Show(false);
	}
	else if (!no_apply_)
	{
		btn_apply_->Show();
		btn_reset_->Show();
	}

	// Vertex properties
	if (objtype == MOBJ_VERTEX)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Vertex");

		// Add 'basic' group
		wxPGProperty* g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add X and Y position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		last_type_ = MOBJ_VERTEX;
	}

	// Line properties
	else if (objtype == MOBJ_LINE)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Line");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add side indices
		addIntProperty(g_basic, "Front Side", "sidefront");
		addIntProperty(g_basic, "Back Side", "sideback");

		// Add 'Special' group
		if (!propHidden("special"))
		{
			wxPGProperty* g_special = pg_properties_->Append(new wxPropertyCategory("Special"));

			// Add special
			MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties_.push_back(prop_as);
			pg_properties_->AppendIn(g_special, prop_as);

			// Add args (hexen)
			if (map_format == MAP_HEXEN)
			{
				for (unsigned a = 0; a < 5; a++)
				{
					// Add arg property
					MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_special, S_FMT("Arg%u", a+1), S_FMT("arg%u", a));

					// Link to action special
					args_[a] = prop;
				}
			}
			else   // Sector tag otherwise
			{
				//addIntProperty(g_special, "Sector Tag", "arg0");
				MOPGTagProperty* prop_tag = new MOPGTagProperty(MOPGTagProperty::TT_SECTORTAG, "Sector Tag", "arg0");
				prop_tag->setParent(this);
				properties_.push_back(prop_tag);
				pg_properties_->AppendIn(g_special, prop_tag);
			}

			// Add SPAC
			if (map_format == MAP_HEXEN)
			{
				MOPGSPACTriggerProperty* prop_spac = new MOPGSPACTriggerProperty("Trigger", "spac");
				prop_spac->setParent(this);
				properties_.push_back(prop_spac);
				pg_properties_->AppendIn(g_special, prop_spac);
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			wxPGProperty* g_flags = pg_properties_->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (unsigned a = 0; a < Game::configuration().nLineFlags(); a++)
				addLineFlagProperty(g_flags, Game::configuration().lineFlag(a).name, S_FMT("flag%u", a), a);
		}

		// --- Sides ---
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, "Front Side");
		stc_sections_->AddPage(pg_props_side2_, "Back Side");

		// 'General' group 1
		wxPGProperty* subgroup = pg_props_side1_->Append(new wxPropertyCategory("General", "side1.general"));
		addIntProperty(subgroup, "Sector", "side1.sector");

		// 'Textures' group 1
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side1_->Append(new wxPropertyCategory("Textures", "side1.textures"));
			addTextureProperty(subgroup, "Upper Texture", "side1.texturetop", 0);
			addTextureProperty(subgroup, "Middle Texture", "side1.texturemiddle", 0);
			addTextureProperty(subgroup, "Lower Texture", "side1.texturebottom", 0);
		}

		// 'Offsets' group 1
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side1_->Append(new wxPropertyCategory("Offsets", "side1.offsets"));
			addIntProperty(subgroup, "X Offset", "side1.offsetx");
			addIntProperty(subgroup, "Y Offset", "side1.offsety");
		}

		// 'General' group 2
		subgroup = pg_props_side2_->Append(new wxPropertyCategory("General", "side2.general"));
		addIntProperty(subgroup, "Sector", "side2.sector");

		// 'Textures' group 2
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side2_->Append(new wxPropertyCategory("Textures", "side2.textures"));
			addTextureProperty(subgroup, "Upper Texture", "side2.texturetop", 0);
			addTextureProperty(subgroup, "Middle Texture", "side2.texturemiddle", 0);
			addTextureProperty(subgroup, "Lower Texture", "side2.texturebottom", 0);
		}

		// 'Offsets' group 2
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side2_->Append(new wxPropertyCategory("Offsets", "side2.offsets"));
			addIntProperty(subgroup, "X Offset", "side2.offsetx");
			addIntProperty(subgroup, "Y Offset", "side2.offsety");
		}
	}

	// Sector properties
	else if (objtype == MOBJ_SECTOR)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Sector");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add heights
		if (!propHidden("heightfloor")) addIntProperty(g_basic, "Floor Height", "heightfloor");
		if (!propHidden("heightceiling")) addIntProperty(g_basic, "Ceiling Height", "heightceiling");

		// Add tag
		if (!propHidden("id"))
		{
			MOPGTagProperty* prop_tag = new MOPGTagProperty(MOPGTagProperty::TT_SECTORTAG, "Tag/ID", "id");
			prop_tag->setParent(this);
			properties_.push_back(prop_tag);
			pg_properties_->AppendIn(g_basic, prop_tag);
		}

		// Add 'Lighting' group
		if (!propHidden("lightlevel"))
		{
			wxPGProperty* g_light = pg_properties_->Append(new wxPropertyCategory("Lighting"));

			// Add light level
			addIntProperty(g_light, "Light Level", "lightlevel");
		}

		// Add 'Textures' group
		if (!propHidden("texturefloor"))
		{
			wxPGProperty* g_textures = pg_properties_->Append(new wxPropertyCategory("Textures"));

			// Add textures
			addTextureProperty(g_textures, "Floor Texture", "texturefloor", 1);
			addTextureProperty(g_textures, "Ceiling Texture", "textureceiling", 1);
		}

		// Add 'Special' group
		if (!propHidden("special"))
		{
			wxPGProperty* g_special = pg_properties_->Append(new wxPropertyCategory("Special"));

			// Add special
			MOPGSectorSpecialProperty* prop_special = new MOPGSectorSpecialProperty("Special", "special");
			prop_special->setParent(this);
			properties_.push_back(prop_special);
			pg_properties_->AppendIn(g_special, prop_special);
		}
	}

	// Thing properties
	else if (objtype == MOBJ_THING)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Thing");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		// Add z height
		if (map_format != MAP_DOOM && !propHidden("height"))
			addIntProperty(g_basic, "Z Height", "height");

		// Add angle
		if (!propHidden("angle"))
		{
			MOPGAngleProperty* prop_angle = new MOPGAngleProperty("Angle", "angle");
			prop_angle->setParent(this);
			properties_.push_back(prop_angle);
			pg_properties_->AppendIn(g_basic, prop_angle);
		}

		// Add type
		if (!propHidden("type"))
		{
			MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty("Type", "type");
			prop_tt->setParent(this);
			properties_.push_back(prop_tt);
			pg_properties_->AppendIn(g_basic, prop_tt);
		}

		// Add id
		if (map_format != MAP_DOOM && !propHidden("id"))
		{
			MOPGTagProperty* prop_id = new MOPGTagProperty(MOPGTagProperty::TT_THINGID, "ID", "id");
			prop_id->setParent(this);
			properties_.push_back(prop_id);
			pg_properties_->AppendIn(g_basic, prop_id);
		}

		if (map_format == MAP_HEXEN && !propHidden("special"))
		{
			// Add 'Scripting Special' group
			wxPGProperty* g_special = pg_properties_->Append(new wxPropertyCategory("Scripting Special"));

			// Add special
			MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties_.push_back(prop_as);
			pg_properties_->AppendIn(g_special, prop_as);

			// Add 'Args' group
			wxPGProperty* g_args = pg_properties_->Append(new wxPropertyCategory("Args"));
			for (unsigned a = 0; a < 5; a++)
			{
				MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_args, S_FMT("Arg%u", a+1), S_FMT("arg%u", a));
				args_[a] = prop;
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			wxPGProperty* g_flags = pg_properties_->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (int a = 0; a < Game::configuration().nThingFlags(); a++)
				addThingFlagProperty(g_flags, Game::configuration().thingFlag(a), S_FMT("flag%u", a), a);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	last_type_ = objtype;
	udmf_ = false;
	
	Layout();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::setupTypeUDMF
//
// Adds all relevant UDMF properties to the grid for [objtype]
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::setupTypeUDMF(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type_ == objtype && udmf_)
		return;

	// Clear property grids
	clearGrid();

	// Hide buttons if not needed
	if (no_apply_ || mobj_props_auto_apply)
	{
		btn_apply_->Show(false);
		btn_reset_->Show(false);
	}
	else if (!no_apply_)
	{
		btn_apply_->Show();
		btn_reset_->Show();
	}

	// Set main tab title
	if (objtype == MOBJ_VERTEX)
		stc_sections_->SetPageText(0, "Vertex");
	else if (objtype == MOBJ_LINE)
		stc_sections_->SetPageText(0, "Line");
	else if (objtype == MOBJ_SECTOR)
		stc_sections_->SetPageText(0, "Sector");
	else if (objtype == MOBJ_THING)
		stc_sections_->SetPageText(0, "Thing");

	// Go through all possible properties for this type
	auto& props = Game::configuration().allUDMFProperties(objtype);
	for (auto& i : props)
	{
		// Skip if hidden
		if ((hide_flags_ && i.second.isFlag()) ||
			(hide_triggers_ && i.second.isTrigger()))
		{
			hide_props_.push_back(i.second.propName());
			continue;
		}
		if (VECTOR_EXISTS(hide_props_, i.second.propName()))
			continue;

		addUDMFProperty(i.second, objtype);
	}

	// Add side properties if line type
	if (objtype == MOBJ_LINE)
	{
		// Add side tabs
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, "Front Side");
		stc_sections_->AddPage(pg_props_side2_, "Back Side");

		// Get side properties
		auto& sprops = Game::configuration().allUDMFProperties(MOBJ_SIDE);

		// Front side
		for (auto& i : sprops)
		{
			// Skip if hidden
			if ((hide_flags_ && i.second.isFlag()) ||
				(hide_triggers_ && i.second.isTrigger()))
			{
				hide_props_.push_back(i.second.propName());
				continue;
			}
			if (VECTOR_EXISTS(hide_props_, i.second.propName()))
				continue;

			addUDMFProperty(i.second, objtype, "side1", pg_props_side1_);
		}

		// Back side
		for (auto& i : sprops)
		{
			// Skip if hidden
			if ((hide_flags_ && i.second.isFlag()) ||
				(hide_triggers_ && i.second.isTrigger()))
			{
				hide_props_.push_back(i.second.propName());
				continue;
			}
			if (VECTOR_EXISTS(hide_props_, i.second.propName()))
				continue;

			addUDMFProperty(i.second, objtype, "side2", pg_props_side2_);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Remember arg properties for passing to type/special properties (or set
	// to NULL if args don't exist)
	for (unsigned arg = 0; arg < 5; arg++)
		args_[arg] = pg_properties_->GetProperty(S_FMT("arg%u", arg));

	last_type_ = objtype;
	udmf_ = true;

	Layout();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::openObject
//
// Populates the grid with properties for [object]
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::openObject(MapObject* object)
{
	// Do open multiple objects
	vector<MapObject*> list;
	if (object) list.push_back(object);
	openObjects(list);
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::openObject
//
// Populates the grid with properties for all MapObjects in [objects]
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::openObjects(vector<MapObject*>& objects)
{
	// Check any objects were given
	if (objects.size() == 0 || objects[0] == nullptr)
	{
		this->objects_.clear();
		pg_properties_->DisableProperty(pg_properties_->GetGrid()->GetRoot());
		pg_properties_->SetPropertyValueUnspecified(pg_properties_->GetGrid()->GetRoot());
		pg_properties_->Refresh();
		pg_props_side1_->DisableProperty(pg_props_side1_->GetGrid()->GetRoot());
		pg_props_side1_->SetPropertyValueUnspecified(pg_props_side1_->GetGrid()->GetRoot());
		pg_props_side1_->Refresh();
		pg_props_side2_->DisableProperty(pg_props_side2_->GetGrid()->GetRoot());
		pg_props_side2_->SetPropertyValueUnspecified(pg_props_side2_->GetGrid()->GetRoot());
		pg_props_side2_->Refresh();

		return;
	}
	else
		pg_properties_->EnableProperty(pg_properties_->GetGrid()->GetRoot());

	// Setup property grid for the object type
	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
		setupTypeUDMF(objects[0]->getObjType());
	else
		setupType(objects[0]->getObjType());

	// Find any custom properties (UDMF only)
	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
	{
		for (unsigned a = 0; a < objects.size(); a++)
		{
			// Go through object properties
			vector<MobjPropertyList::prop_t> objprops = objects[a]->props().allProperties();
			for (unsigned b = 0; b < objprops.size(); b++)
			{
				// Ignore unset properties
				if (!objprops[b].value.hasValue())
					continue;

				// Ignore side property
				if (objprops[b].name.StartsWith("side1.") || objprops[b].name.StartsWith("side2."))
					continue;

				// Check if hidden
				if (VECTOR_EXISTS(hide_props_, objprops[b].name))
					continue;

				// Check if property is already on the list
				bool exists = false;
				for (unsigned c = 0; c < properties_.size(); c++)
				{
					if (properties_[c]->getPropName() == objprops[b].name)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
				{
					// Create custom group if needed
					if (!group_custom_)
						group_custom_ = pg_properties_->Append(new wxPropertyCategory("Custom"));

					//LOG_MESSAGE(2, "Add custom property \"%s\"", objprops[b].name);

					// Add property
					switch (objprops[b].value.getType())
					{
					case PROP_BOOL:
						addBoolProperty(group_custom_, objprops[b].name, objprops[b].name); break;
					case PROP_INT:
						addIntProperty(group_custom_, objprops[b].name, objprops[b].name); break;
					case PROP_FLOAT:
						addFloatProperty(group_custom_, objprops[b].name, objprops[b].name); break;
					default:
						addStringProperty(group_custom_, objprops[b].name, objprops[b].name); break;
					}
				}
			}
		}
	}

	// Generic properties
	for (unsigned a = 0; a < properties_.size(); a++)
		properties_[a]->openObjects(objects);

	// Handle line sides
	if (objects[0]->getObjType() == MOBJ_LINE)
	{
		// Enable/disable side properties
		wxPGProperty* prop = pg_properties_->GetProperty("sidefront");
		if (prop && (prop->IsValueUnspecified() || prop->GetValue().GetInteger() >= 0))
			pg_props_side1_->EnableProperty(pg_props_side1_->GetGrid()->GetRoot());
		else
		{
			pg_props_side1_->DisableProperty(pg_props_side1_->GetGrid()->GetRoot());
			pg_props_side1_->SetPropertyValueUnspecified(pg_props_side1_->GetGrid()->GetRoot());
		}
		prop = pg_properties_->GetProperty("sideback");
		if (prop && (prop->IsValueUnspecified() || prop->GetValue().GetInteger() >= 0))
			pg_props_side2_->EnableProperty(pg_props_side2_->GetGrid()->GetRoot());
		else
		{
			pg_props_side2_->DisableProperty(pg_props_side2_->GetGrid()->GetRoot());
			pg_props_side2_->SetPropertyValueUnspecified(pg_props_side2_->GetGrid()->GetRoot());
		}
	}

	// Update internal objects list
	if (&objects != &this->objects_)
	{
		this->objects_.clear();
		for (unsigned a = 0; a < objects.size(); a++)
			this->objects_.push_back(objects[a]);
	}

	// Possibly update the argument names and visibility
	updateArgs(nullptr);

	pg_properties_->Refresh();
	pg_props_side1_->Refresh();
	pg_props_side2_->Refresh();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::updateArgs
//
// Updates the names and visibility of the "arg" properties
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::updateArgs(MOPGIntWithArgsProperty* source)
{
	MOPGProperty* prop;
	MOPGIntWithArgsProperty* prop_with_args;
	MOPGIntWithArgsProperty* owner_prop = source;  // sane default

	// First determine which property owns the args.  Use the last one that has
	// a specified and non-zero value.  ThingType always wins, though, because
	// ThingTypes with args ignore their specials.
	for (unsigned a = 0; a < properties_.size(); a++)
	{
		prop = properties_[a];

		if (prop->getType() == MOPGProperty::TYPE_TTYPE
			|| prop->getType() == MOPGProperty::TYPE_ASPECIAL)
		{
			prop_with_args = (MOPGIntWithArgsProperty*)prop;

			if (!prop_with_args->IsValueUnspecified()
				&& prop_with_args->GetValue().GetInteger() != 0
				&& prop_with_args->hasArgs())
			{
				owner_prop = prop_with_args;

				if (prop->getType() == MOPGProperty::TYPE_TTYPE)
					break;
			}
		}
	}

	if (owner_prop)
		owner_prop->updateArgs(args_);
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::applyChanges
//
// Applies any property changes to the opened object(s)
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::applyChanges()
{
	// Go through all current properties and apply the current value
	for (unsigned a = 0; a < properties_.size(); a++)
		properties_[a]->applyValue();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::clearGrid
//
// Clears all property grid rows and tabs
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::clearGrid()
{
	// Clear property grids
	for (unsigned a = 0; a < 5; a++)
		args_[a] = nullptr;
	pg_properties_->Clear();
	pg_props_side1_->Clear();
	pg_props_side2_->Clear();
	group_custom_ = nullptr;
	properties_.clear();
	btn_add_->Show();

	// Remove side1/2 tabs if they exist
	// Calling RemovePage() changes the focus for no good reason; this is a
	// workaround.  See http://trac.wxwidgets.org/ticket/11333
	stc_sections_->Freeze();
	stc_sections_->Hide();
	while (stc_sections_->GetPageCount() > 1)
		stc_sections_->RemovePage(1);
	stc_sections_->Show();
	stc_sections_->Thaw();
	pg_props_side1_->Show(false);
	pg_props_side2_->Show(false);

	// Reset last type so the grid is rebuilt next time objects are opened
	last_type_ = -1;
}


// ----------------------------------------------------------------------------
//
// MapObjectPropsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MapObjectPropsPanel::onBtnApply
//
// Called when the apply button is clicked
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnApply(wxCommandEvent& e)
{
	string type;
	if (last_type_ == MOBJ_VERTEX)
		type = "Vertex";
	else if (last_type_ == MOBJ_LINE)
		type = "Line";
	else if (last_type_ == MOBJ_SECTOR)
		type = "Sector";
	else if (last_type_ == MOBJ_THING)
		type = "Thing";

	// Apply changes
	MapEditor::editContext().beginUndoRecordLocked(S_FMT("Modify %s Properties", CHR(type)), true, false, false);
	applyChanges();
	MapEditor::editContext().endUndoRecord();

	// Refresh map view
	MapEditor::window()->forceRefresh(true);
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::onBtnReset
//
// Called when the reset button is clicked
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnReset(wxCommandEvent& e)
{
	// Go through all current properties and reset the value
	for (unsigned a = 0; a < properties_.size(); a++)
		properties_[a]->resetValue();
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::onShowAllToggled
//
// Called when the 'show all' checkbox is toggled
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::onShowAllToggled(wxCommandEvent& e)
{
	mobj_props_show_all = cb_show_all_->GetValue();

	// Refresh each property's visibility
	for (unsigned a = 0; a < properties_.size(); a++)
		properties_[a]->updateVisibility();

	updateArgs(nullptr);
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::onBtnAdd
//
// Called when the add property button is clicked
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnAdd(wxCommandEvent& e)
{
	wxDialog dlg(this, -1, "Add UDMF Property");

	// Setup dialog sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(msizer);
	wxGridBagSizer* sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	msizer->Add(sizer, 1, wxEXPAND|wxALL, UI::padLarge());

	// Name
	wxTextCtrl* text_name = new wxTextCtrl(&dlg, -1, "");
	sizer->Add(new wxStaticText(&dlg, -1, "Name:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_name, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Type
	string types[] ={ "Boolean", "String", "Integer", "Float", "Angle", "Texture (Wall)", "Texture (Flat)", "Colour" };
	wxChoice* choice_type = new wxChoice(&dlg, -1, wxDefaultPosition, wxDefaultSize, 7, types);
	choice_type->SetSelection(0);
	sizer->Add(new wxStaticText(&dlg, -1, "Type:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_type, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Buttons
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), { 2, 0 }, { 1, 2 }, wxEXPAND);

	// Show dialog
	dlg.Layout();
	dlg.Fit();
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		// Create custom group if needed
		if (!group_custom_)
			group_custom_ = pg_properties_->Append(new wxPropertyCategory("Custom"));

		// Get name entered
		string propname = text_name->GetValue().Lower();
		if (propname == "" || propname.Contains(" "))	// TODO: Proper regex check
		{
			wxMessageBox("Invalid property name", "Error");
			return;
		}

		// Check if existing
		for (unsigned a = 0; a < properties_.size(); a++)
		{
			if (properties_[a]->getPropName() == propname)
			{
				wxMessageBox(S_FMT("Property \"%s\" already exists", propname), "Error");
				return;
			}
		}

		// Add property
		if (choice_type->GetSelection() == 0)
			addBoolProperty(group_custom_, propname, propname);
		else if (choice_type->GetSelection() == 1)
			addStringProperty(group_custom_, propname, propname);
		else if (choice_type->GetSelection() == 2)
			addIntProperty(group_custom_, propname, propname);
		else if (choice_type->GetSelection() == 3)
			addFloatProperty(group_custom_, propname, propname);
		else if (choice_type->GetSelection() == 4)
		{
			MOPGAngleProperty* prop_angle = new MOPGAngleProperty(propname, propname);
			prop_angle->setParent(this);
			properties_.push_back(prop_angle);
			pg_properties_->AppendIn(group_custom_, prop_angle);
		}
		else if (choice_type->GetSelection() == 5)
			addTextureProperty(group_custom_, propname, propname, 0);
		else if (choice_type->GetSelection() == 6)
			addTextureProperty(group_custom_, propname, propname, 1);
		else if (choice_type->GetSelection() == 7)
		{
			MOPGColourProperty* prop_col = new MOPGColourProperty(propname, propname);
			prop_col->setParent(this);
			properties_.push_back(prop_col);
			pg_properties_->AppendIn(group_custom_, prop_col);
		}
	}
}

// ----------------------------------------------------------------------------
// MapObjectPropsPanel::onPropertyChanged
//
// Called when a property value is changed
// ----------------------------------------------------------------------------
void MapObjectPropsPanel::onPropertyChanged(wxPropertyGridEvent& e)
{
	// Ignore if no auto apply
	if (no_apply_ || !mobj_props_auto_apply)
	{
		e.Skip();
		return;
	}

	// Find property
	string name = e.GetPropertyName();
	for (unsigned a = 0; a < properties_.size(); a++)
	{
		if (properties_[a]->getPropName() == name)
		{
			// Found, apply value
			string type;
			if (last_type_ == MOBJ_VERTEX)
				type = "Vertex";
			else if (last_type_ == MOBJ_LINE)
				type = "Line";
			else if (last_type_ == MOBJ_SECTOR)
				type = "Sector";
			else if (last_type_ == MOBJ_THING)
				type = "Thing";
			
			MapEditor::editContext().beginUndoRecordLocked(
				S_FMT("Modify %s Properties", CHR(type)),
				true,
				false,
				false
			);
			properties_[a]->applyValue();
			MapEditor::editContext().endUndoRecord();
			return;
		}
	}
}
