
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapObjectPropsPanel.cpp
 * Description: MapObjectPropsPanel class, a wx panel containing a
 *              property grid for viewing/editing map object
 *              properties
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
#include "Game/Configuration.h"
#include "Graphics/Icons.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "MapObjectPropsPanel.h"
#include "MOPGProperty.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, mobj_props_show_all, false, CVAR_SAVE)
CVAR(Bool, mobj_props_auto_apply, false, CVAR_SAVE)


/*******************************************************************
 * MAPOBJECTPROPSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapObjectPropsPanel::MapObjectPropsPanel
 * MapObjectPropsPanel class constructor
 *******************************************************************/
MapObjectPropsPanel::MapObjectPropsPanel(wxWindow* parent, bool no_apply) : PropsPanelBase(parent)
{
	// Init variables
	last_type = -1;
	group_custom = NULL;
	for (unsigned a = 0; a < 5; a++)
		args[a] = NULL;
	this->no_apply = no_apply;
	hide_flags = false;
	hide_triggers = false;
	udmf = false;

	// Setup sizer
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add item label
	//label_item = new wxStaticText(this, -1, "");
	//sizer->Add(label_item, 0, wxEXPAND|wxALL, 4);
	cb_show_all = new wxCheckBox(this, -1, "Show All");
	cb_show_all->SetValue(mobj_props_show_all);
	sizer->Add(cb_show_all, 0, wxEXPAND|wxALL, 4);
	sizer->AddSpacer(4);

	// Add tabs
	stc_sections = STabCtrl::createControl(this);
	sizer->Add(stc_sections, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add main property grid
	pg_properties = new wxPropertyGrid(stc_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);
	stc_sections->AddPage(pg_properties, "Properties");

	// Create side property grids
	pg_props_side1 = new wxPropertyGrid(stc_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);
	pg_props_side2 = new wxPropertyGrid(stc_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);

	// Add buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add button
	btn_add = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "plus"));
	btn_add->SetToolTip("Add Property");
	hbox->Add(btn_add, 0, wxEXPAND|wxRIGHT, 4);
	hbox->AddStretchSpacer(1);

	// Reset button
	btn_reset = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "close"));
	btn_reset->SetToolTip("Discard Changes");
	hbox->Add(btn_reset, 0, wxEXPAND|wxRIGHT, 4);

	// Apply button
	btn_apply = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "tick"));
	btn_apply->SetToolTip("Apply Changes");
	hbox->Add(btn_apply, 0, wxEXPAND);

	wxPGCell cell;
	cell.SetText("<multiple values>");
	cell.SetFgCol(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
	pg_properties->GetGrid()->SetUnspecifiedValueAppearance(cell);
	pg_props_side1->GetGrid()->SetUnspecifiedValueAppearance(cell);
	pg_props_side2->GetGrid()->SetUnspecifiedValueAppearance(cell);

	// Bind events
	btn_apply->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnApply, this);
	btn_reset->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnReset, this);
	cb_show_all->Bind(wxEVT_CHECKBOX, &MapObjectPropsPanel::onShowAllToggled, this);
	btn_add->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnAdd, this);
	pg_properties->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);
	pg_props_side1->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);
	pg_props_side2->Bind(wxEVT_PG_CHANGED, &MapObjectPropsPanel::onPropertyChanged, this);

	// Hide side property grids
	pg_props_side1->Show(false);
	pg_props_side2->Show(false);

	// Hide apply button if needed
	if (no_apply || mobj_props_auto_apply)
	{
		btn_apply->Show(false);
		btn_reset->Show(false);
	}

	Layout();
}

/* MapObjectPropsPanel::~MapObjectPropsPanel
 * MapObjectPropsPanel class destructor
 *******************************************************************/
MapObjectPropsPanel::~MapObjectPropsPanel()
{
}

/* MapObjectPropsPanel::showAll
 * Returns true if 'Show All' is ticked
 *******************************************************************/
bool MapObjectPropsPanel::showAll()
{
	return cb_show_all->IsChecked();
}

/* MapObjectPropsPanel::addBoolProperty
 * Adds a boolean property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addBoolProperty(wxPGProperty* group, string label, string propname, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGBoolProperty* prop = new MOPGBoolProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addIntProperty
 * Adds an integer property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addIntProperty(wxPGProperty* group, string label, string propname, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGIntProperty* prop = new MOPGIntProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addFloatProperty
 * Adds a float property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addFloatProperty(wxPGProperty* group, string label, string propname, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGFloatProperty* prop = new MOPGFloatProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addStringProperty
 * Adds a string property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addStringProperty(wxPGProperty* group, string label, string propname, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGStringProperty* prop = new MOPGStringProperty(label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addLineFlagProperty
 * Adds a line flag property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addLineFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGLineFlagProperty* prop = new MOPGLineFlagProperty(label, propname, index);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addThingFlagProperty
 * Adds a thing flag property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addThingFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGThingFlagProperty* prop = new MOPGThingFlagProperty(label, propname, index);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::addTextureProperty
 * Adds a texture property cell to the grid under [group] for the
 * object property [propname]
 *******************************************************************/
MOPGProperty* MapObjectPropsPanel::addTextureProperty(wxPGProperty* group, string label, string propname, int textype, bool readonly, wxPropertyGrid* grid, UDMFProperty* udmf_prop)
{
	// Create property
	MOPGTextureProperty* prop = new MOPGTextureProperty(textype, label, propname);
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	// Add it
	properties.push_back(prop);
	if (!grid)	pg_properties->AppendIn(group, prop);
	else		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

/* MapObjectPropsPanel::setBoolProperty
 * Sets the boolean property cell [prop]'s value to [value]
 *******************************************************************/
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

/* MapObjectPropsPanel::addUDMFProperty
 * Adds the UDMF property [prop] to the grid, under [basegroup]. Will
 * add the correct property cell type for the UDMF property
 *******************************************************************/
void MapObjectPropsPanel::addUDMFProperty(UDMFProperty& prop, int objtype, string basegroup, wxPropertyGrid* grid)
{
	// Set grid to add to (main one if grid is NULL)
	if (!grid)
		grid = pg_properties;

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
		properties.push_back(prop_col);
		grid->AppendIn(group, prop_col);
	}
	else if (prop.type() == UDMFProperty::Type::ActionSpecial)
	{
		MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty(prop.name(), propname);
		prop_as->setParent(this);
		prop_as->setUDMFProp(&prop);
		properties.push_back(prop_as);
		grid->AppendIn(group, prop_as);
	}
	else if (prop.type() == UDMFProperty::Type::SectorSpecial)
	{
		MOPGSectorSpecialProperty* prop_ss = new MOPGSectorSpecialProperty(prop.name(), propname);
		prop_ss->setParent(this);
		prop_ss->setUDMFProp(&prop);
		properties.push_back(prop_ss);
		grid->AppendIn(group, prop_ss);
	}
	else if (prop.type() == UDMFProperty::Type::ThingType)
	{
		MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty(prop.name(), propname);
		prop_tt->setParent(this);
		prop_tt->setUDMFProp(&prop);
		properties.push_back(prop_tt);
		grid->AppendIn(group, prop_tt);
	}
	else if (prop.type() == UDMFProperty::Type::Angle)
	{
		MOPGAngleProperty* prop_angle = new MOPGAngleProperty(prop.name(), propname);
		prop_angle->setParent(this);
		prop_angle->setUDMFProp(&prop);
		properties.push_back(prop_angle);
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
		properties.push_back(prop_id);
		grid->AppendIn(group, prop_id);
	}
}

/* MapObjectPropsPanel::setupType
 * Adds all relevant properties to the grid for [objtype]
 *******************************************************************/
void MapObjectPropsPanel::setupType(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type == objtype && !udmf)
		return;

	// Get map format
	int map_format = MapEditor::editContext().mapDesc().format;

	// Clear property grid
	clearGrid();
	btn_add->Show(false);

	// Hide buttons if not needed
	if (no_apply || mobj_props_auto_apply)
	{
		btn_apply->Show(false);
		btn_reset->Show(false);
	}
	else if (!no_apply)
	{
		btn_apply->Show();
		btn_reset->Show();
	}

	// Vertex properties
	if (objtype == MOBJ_VERTEX)
	{
		// Set main tab name
		stc_sections->SetPageText(0, "Vertex");

		// Add 'basic' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add X and Y position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		last_type = MOBJ_VERTEX;
	}

	// Line properties
	else if (objtype == MOBJ_LINE)
	{
		// Set main tab name
		stc_sections->SetPageText(0, "Line");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add side indices
		addIntProperty(g_basic, "Front Side", "sidefront");
		addIntProperty(g_basic, "Back Side", "sideback");

		// Add 'Special' group
		if (!propHidden("special"))
		{
			wxPGProperty* g_special = pg_properties->Append(new wxPropertyCategory("Special"));

			// Add special
			MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties.push_back(prop_as);
			pg_properties->AppendIn(g_special, prop_as);

			// Add args (hexen)
			if (map_format == MAP_HEXEN)
			{
				for (unsigned a = 0; a < 5; a++)
				{
					// Add arg property
					MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_special, S_FMT("Arg%u", a+1), S_FMT("arg%u", a));

					// Link to action special
					args[a] = prop;
				}
			}
			else   // Sector tag otherwise
			{
				//addIntProperty(g_special, "Sector Tag", "arg0");
				MOPGTagProperty* prop_tag = new MOPGTagProperty(MOPGTagProperty::TT_SECTORTAG, "Sector Tag", "arg0");
				prop_tag->setParent(this);
				properties.push_back(prop_tag);
				pg_properties->AppendIn(g_special, prop_tag);
			}

			// Add SPAC
			if (map_format == MAP_HEXEN)
			{
				MOPGSPACTriggerProperty* prop_spac = new MOPGSPACTriggerProperty("Trigger", "spac");
				prop_spac->setParent(this);
				properties.push_back(prop_spac);
				pg_properties->AppendIn(g_special, prop_spac);
			}
		}

		if (!hide_flags)
		{
			// Add 'Flags' group
			wxPGProperty* g_flags = pg_properties->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (int a = 0; a < Game::configuration().nLineFlags(); a++)
				addLineFlagProperty(g_flags, Game::configuration().lineFlag(a).name, S_FMT("flag%u", a), a);
		}

		// --- Sides ---
		pg_props_side1->Show(true);
		pg_props_side2->Show(true);
		stc_sections->AddPage(pg_props_side1, "Front Side");
		stc_sections->AddPage(pg_props_side2, "Back Side");

		// 'General' group 1
		wxPGProperty* subgroup = pg_props_side1->Append(new wxPropertyCategory("General", "side1.general"));
		addIntProperty(subgroup, "Sector", "side1.sector");

		// 'Textures' group 1
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side1->Append(new wxPropertyCategory("Textures", "side1.textures"));
			addTextureProperty(subgroup, "Upper Texture", "side1.texturetop", 0);
			addTextureProperty(subgroup, "Middle Texture", "side1.texturemiddle", 0);
			addTextureProperty(subgroup, "Lower Texture", "side1.texturebottom", 0);
		}

		// 'Offsets' group 1
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side1->Append(new wxPropertyCategory("Offsets", "side1.offsets"));
			addIntProperty(subgroup, "X Offset", "side1.offsetx");
			addIntProperty(subgroup, "Y Offset", "side1.offsety");
		}

		// 'General' group 2
		subgroup = pg_props_side2->Append(new wxPropertyCategory("General", "side2.general"));
		addIntProperty(subgroup, "Sector", "side2.sector");

		// 'Textures' group 2
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side2->Append(new wxPropertyCategory("Textures", "side2.textures"));
			addTextureProperty(subgroup, "Upper Texture", "side2.texturetop", 0);
			addTextureProperty(subgroup, "Middle Texture", "side2.texturemiddle", 0);
			addTextureProperty(subgroup, "Lower Texture", "side2.texturebottom", 0);
		}

		// 'Offsets' group 2
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side2->Append(new wxPropertyCategory("Offsets", "side2.offsets"));
			addIntProperty(subgroup, "X Offset", "side2.offsetx");
			addIntProperty(subgroup, "Y Offset", "side2.offsety");
		}
	}

	// Sector properties
	else if (objtype == MOBJ_SECTOR)
	{
		// Set main tab name
		stc_sections->SetPageText(0, "Sector");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add heights
		if (!propHidden("heightfloor")) addIntProperty(g_basic, "Floor Height", "heightfloor");
		if (!propHidden("heightceiling")) addIntProperty(g_basic, "Ceiling Height", "heightceiling");

		// Add tag
		if (!propHidden("id"))
		{
			MOPGTagProperty* prop_tag = new MOPGTagProperty(MOPGTagProperty::TT_SECTORTAG, "Tag/ID", "id");
			prop_tag->setParent(this);
			properties.push_back(prop_tag);
			pg_properties->AppendIn(g_basic, prop_tag);
		}

		// Add 'Lighting' group
		if (!propHidden("lightlevel"))
		{
			wxPGProperty* g_light = pg_properties->Append(new wxPropertyCategory("Lighting"));

			// Add light level
			addIntProperty(g_light, "Light Level", "lightlevel");
		}

		// Add 'Textures' group
		if (!propHidden("texturefloor"))
		{
			wxPGProperty* g_textures = pg_properties->Append(new wxPropertyCategory("Textures"));

			// Add textures
			addTextureProperty(g_textures, "Floor Texture", "texturefloor", 1);
			addTextureProperty(g_textures, "Ceiling Texture", "textureceiling", 1);
		}

		// Add 'Special' group
		if (!propHidden("special"))
		{
			wxPGProperty* g_special = pg_properties->Append(new wxPropertyCategory("Special"));

			// Add special
			MOPGSectorSpecialProperty* prop_special = new MOPGSectorSpecialProperty("Special", "special");
			prop_special->setParent(this);
			properties.push_back(prop_special);
			pg_properties->AppendIn(g_special, prop_special);
		}
	}

	// Thing properties
	else if (objtype == MOBJ_THING)
	{
		// Set main tab name
		stc_sections->SetPageText(0, "Thing");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

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
			properties.push_back(prop_angle);
			pg_properties->AppendIn(g_basic, prop_angle);
		}

		// Add type
		if (!propHidden("type"))
		{
			MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty("Type", "type");
			prop_tt->setParent(this);
			properties.push_back(prop_tt);
			pg_properties->AppendIn(g_basic, prop_tt);
		}

		// Add id
		if (map_format != MAP_DOOM && !propHidden("id"))
		{
			MOPGTagProperty* prop_id = new MOPGTagProperty(MOPGTagProperty::TT_THINGID, "ID", "id");
			prop_id->setParent(this);
			properties.push_back(prop_id);
			pg_properties->AppendIn(g_basic, prop_id);
		}

		if (map_format == MAP_HEXEN && !propHidden("special"))
		{
			// Add 'Scripting Special' group
			wxPGProperty* g_special = pg_properties->Append(new wxPropertyCategory("Scripting Special"));

			// Add special
			MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties.push_back(prop_as);
			pg_properties->AppendIn(g_special, prop_as);

			// Add 'Args' group
			wxPGProperty* g_args = pg_properties->Append(new wxPropertyCategory("Args"));
			for (unsigned a = 0; a < 5; a++)
			{
				MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_args, S_FMT("Arg%u", a+1), S_FMT("arg%u", a));
				args[a] = prop;
			}
		}

		if (!hide_flags)
		{
			// Add 'Flags' group
			wxPGProperty* g_flags = pg_properties->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (int a = 0; a < Game::configuration().nThingFlags(); a++)
				addThingFlagProperty(g_flags, Game::configuration().thingFlag(a), S_FMT("flag%u", a), a);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	last_type = objtype;
	udmf = false;
	
	Layout();
}

/* MapObjectPropsPanel::setupTypeUDMF
 * Adds all relevant UDMF properties to the grid for [objtype]
 *******************************************************************/
void MapObjectPropsPanel::setupTypeUDMF(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type == objtype && udmf)
		return;

	// Clear property grids
	clearGrid();

	// Hide buttons if not needed
	if (no_apply || mobj_props_auto_apply)
	{
		btn_apply->Show(false);
		btn_reset->Show(false);
	}
	else if (!no_apply)
	{
		btn_apply->Show();
		btn_reset->Show();
	}

	// Set main tab title
	if (objtype == MOBJ_VERTEX)
		stc_sections->SetPageText(0, "Vertex");
	else if (objtype == MOBJ_LINE)
		stc_sections->SetPageText(0, "Line");
	else if (objtype == MOBJ_SECTOR)
		stc_sections->SetPageText(0, "Sector");
	else if (objtype == MOBJ_THING)
		stc_sections->SetPageText(0, "Thing");

	// Go through all possible properties for this type
	auto& props = Game::configuration().allUDMFProperties(objtype);
	for (auto& i : props)
	{
		// Skip if hidden
		if ((hide_flags && i.second.isFlag()) ||
			(hide_triggers && i.second.isTrigger()))
		{
			hide_props.push_back(i.second.propName());
			continue;
		}
		if (VECTOR_EXISTS(hide_props, i.second.propName()))
			continue;

		addUDMFProperty(i.second, objtype);
	}

	// Add side properties if line type
	if (objtype == MOBJ_LINE)
	{
		// Add side tabs
		pg_props_side1->Show(true);
		pg_props_side2->Show(true);
		stc_sections->AddPage(pg_props_side1, "Front Side");
		stc_sections->AddPage(pg_props_side2, "Back Side");

		// Get side properties
		auto& sprops = Game::configuration().allUDMFProperties(MOBJ_SIDE);

		// Front side
		for (auto& i : sprops)
		{
			// Skip if hidden
			if ((hide_flags && i.second.isFlag()) ||
				(hide_triggers && i.second.isTrigger()))
			{
				hide_props.push_back(i.second.propName());
				continue;
			}
			if (VECTOR_EXISTS(hide_props, i.second.propName()))
				continue;

			addUDMFProperty(i.second, objtype, "side1", pg_props_side1);
		}

		// Back side
		for (auto& i : sprops)
		{
			// Skip if hidden
			if ((hide_flags && i.second.isFlag()) ||
				(hide_triggers && i.second.isTrigger()))
			{
				hide_props.push_back(i.second.propName());
				continue;
			}
			if (VECTOR_EXISTS(hide_props, i.second.propName()))
				continue;

			addUDMFProperty(i.second, objtype, "side2", pg_props_side2);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Remember arg properties for passing to type/special properties (or set
	// to NULL if args don't exist)
	for (unsigned arg = 0; arg < 5; arg++)
		args[arg] = pg_properties->GetProperty(S_FMT("arg%u", arg));

	last_type = objtype;
	udmf = true;

	Layout();
}

/* MapObjectPropsPanel::openObject
 * Populates the grid with properties for [object]
 *******************************************************************/
void MapObjectPropsPanel::openObject(MapObject* object)
{
	// Do open multiple objects
	vector<MapObject*> list;
	if (object) list.push_back(object);
	openObjects(list);
}

/* MapObjectPropsPanel::openObject
 * Populates the grid with properties for all MapObjects in [objects]
 *******************************************************************/
void MapObjectPropsPanel::openObjects(vector<MapObject*>& objects)
{
	// Check any objects were given
	if (objects.size() == 0 || objects[0] == NULL)
	{
		this->objects.clear();
		pg_properties->DisableProperty(pg_properties->GetGrid()->GetRoot());
		pg_properties->SetPropertyValueUnspecified(pg_properties->GetGrid()->GetRoot());
		pg_properties->Refresh();
		pg_props_side1->DisableProperty(pg_props_side1->GetGrid()->GetRoot());
		pg_props_side1->SetPropertyValueUnspecified(pg_props_side1->GetGrid()->GetRoot());
		pg_props_side1->Refresh();
		pg_props_side2->DisableProperty(pg_props_side2->GetGrid()->GetRoot());
		pg_props_side2->SetPropertyValueUnspecified(pg_props_side2->GetGrid()->GetRoot());
		pg_props_side2->Refresh();

		return;
	}
	else
		pg_properties->EnableProperty(pg_properties->GetGrid()->GetRoot());

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
				if (VECTOR_EXISTS(hide_props, objprops[b].name))
					continue;

				// Check if property is already on the list
				bool exists = false;
				for (unsigned c = 0; c < properties.size(); c++)
				{
					if (properties[c]->getPropName() == objprops[b].name)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
				{
					// Create custom group if needed
					if (!group_custom)
						group_custom = pg_properties->Append(new wxPropertyCategory("Custom"));

					//LOG_MESSAGE(2, "Add custom property \"%s\"", objprops[b].name);

					// Add property
					switch (objprops[b].value.getType())
					{
					case PROP_BOOL:
						addBoolProperty(group_custom, objprops[b].name, objprops[b].name); break;
					case PROP_INT:
						addIntProperty(group_custom, objprops[b].name, objprops[b].name); break;
					case PROP_FLOAT:
						addFloatProperty(group_custom, objprops[b].name, objprops[b].name); break;
					default:
						addStringProperty(group_custom, objprops[b].name, objprops[b].name); break;
					}
				}
			}
		}
	}

	// Generic properties
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->openObjects(objects);

	// Handle line sides
	if (objects[0]->getObjType() == MOBJ_LINE)
	{
		// Enable/disable side properties
		wxPGProperty* prop = pg_properties->GetProperty("sidefront");
		if (prop && (prop->IsValueUnspecified() || prop->GetValue().GetInteger() >= 0))
			pg_props_side1->EnableProperty(pg_props_side1->GetGrid()->GetRoot());
		else
		{
			pg_props_side1->DisableProperty(pg_props_side1->GetGrid()->GetRoot());
			pg_props_side1->SetPropertyValueUnspecified(pg_props_side1->GetGrid()->GetRoot());
		}
		prop = pg_properties->GetProperty("sideback");
		if (prop && (prop->IsValueUnspecified() || prop->GetValue().GetInteger() >= 0))
			pg_props_side2->EnableProperty(pg_props_side2->GetGrid()->GetRoot());
		else
		{
			pg_props_side2->DisableProperty(pg_props_side2->GetGrid()->GetRoot());
			pg_props_side2->SetPropertyValueUnspecified(pg_props_side2->GetGrid()->GetRoot());
		}
	}

	// Update internal objects list
	if (&objects != &this->objects)
	{
		this->objects.clear();
		for (unsigned a = 0; a < objects.size(); a++)
			this->objects.push_back(objects[a]);
	}

	// Possibly update the argument names and visibility
	updateArgs(NULL);

	pg_properties->Refresh();
	pg_props_side1->Refresh();
	pg_props_side2->Refresh();
}

/* MapObjectPropsPanel::updateArgs
 * Updates the names and visibility of the "arg" properties
 *******************************************************************/
void MapObjectPropsPanel::updateArgs(MOPGIntWithArgsProperty* source)
{
	MOPGProperty* prop;
	MOPGIntWithArgsProperty* prop_with_args;
	MOPGIntWithArgsProperty* owner_prop = source;  // sane default

	// First determine which property owns the args.  Use the last one that has
	// a specified and non-zero value.  ThingType always wins, though, because
	// ThingTypes with args ignore their specials.
	for (unsigned a = 0; a < properties.size(); a++)
	{
		prop = properties[a];

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
		owner_prop->updateArgs(args);
}

/* MapObjectPropsPanel::applyChanges
 * Applies any property changes to the opened object(s)
 *******************************************************************/
void MapObjectPropsPanel::applyChanges()
{
	// Go through all current properties and apply the current value
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->applyValue();
}

/* MapObjectPropsPanel::clearGrid
 * Clears all property grid rows and tabs
 *******************************************************************/
void MapObjectPropsPanel::clearGrid()
{
	// Clear property grids
	for (unsigned a = 0; a < 5; a++)
		args[a] = NULL;
	pg_properties->Clear();
	pg_props_side1->Clear();
	pg_props_side2->Clear();
	group_custom = NULL;
	properties.clear();
	btn_add->Show();

	// Remove side1/2 tabs if they exist
	// Calling RemovePage() changes the focus for no good reason; this is a
	// workaround.  See http://trac.wxwidgets.org/ticket/11333
	stc_sections->Freeze();
	stc_sections->Hide();
	while (stc_sections->GetPageCount() > 1)
		stc_sections->RemovePage(1);
	stc_sections->Show();
	stc_sections->Thaw();
	pg_props_side1->Show(false);
	pg_props_side2->Show(false);
}


/*******************************************************************
 * MAPOBJECTPROPSPANEL CLASS EVENTS
 *******************************************************************/

/* MapObjectPropsPanel::onBtnApply
 * Called when the apply button is clicked
 *******************************************************************/
void MapObjectPropsPanel::onBtnApply(wxCommandEvent& e)
{
	string type;
	if (last_type == MOBJ_VERTEX)
		type = "Vertex";
	else if (last_type == MOBJ_LINE)
		type = "Line";
	else if (last_type == MOBJ_SECTOR)
		type = "Sector";
	else if (last_type == MOBJ_THING)
		type = "Thing";

	// Apply changes
	MapEditor::editContext().beginUndoRecordLocked(S_FMT("Modify %s Properties", CHR(type)), true, false, false);
	applyChanges();
	MapEditor::editContext().endUndoRecord();

	// Refresh map view
	MapEditor::window()->forceRefresh(true);
}

/* MapObjectPropsPanel::onBtnReset
 * Called when the reset button is clicked
 *******************************************************************/
void MapObjectPropsPanel::onBtnReset(wxCommandEvent& e)
{
	// Go through all current properties and reset the value
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->resetValue();
}

/* MapObjectPropsPanel::onShowAllToggled
 * Called when the 'show all' checkbox is toggled
 *******************************************************************/
void MapObjectPropsPanel::onShowAllToggled(wxCommandEvent& e)
{
	mobj_props_show_all = cb_show_all->GetValue();

	// Refresh each property's visibility
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->updateVisibility();

	updateArgs(NULL);
}

/* MapObjectPropsPanel::onBtnAdd
 * Called when the add property button is clicked
 *******************************************************************/
void MapObjectPropsPanel::onBtnAdd(wxCommandEvent& e)
{
	wxDialog dlg(this, -1, "Add UDMF Property");

	// Setup dialog sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(msizer);
	wxGridBagSizer* sizer = new wxGridBagSizer(10, 10);
	msizer->Add(sizer, 1, wxEXPAND|wxALL, 10);

	// Name
	wxTextCtrl* text_name = new wxTextCtrl(&dlg, -1, "");
	sizer->Add(new wxStaticText(&dlg, -1, "Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_name, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Type
	string types[] ={ "Boolean", "String", "Integer", "Float", "Angle", "Texture (Wall)", "Texture (Flat)", "Colour" };
	wxChoice* choice_type = new wxChoice(&dlg, -1, wxDefaultPosition, wxDefaultSize, 7, types);
	choice_type->SetSelection(0);
	sizer->Add(new wxStaticText(&dlg, -1, "Type:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_type, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Buttons
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

	// Show dialog
	dlg.Layout();
	dlg.Fit();
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		// Create custom group if needed
		if (!group_custom)
			group_custom = pg_properties->Append(new wxPropertyCategory("Custom"));

		// Get name entered
		string propname = text_name->GetValue().Lower();
		if (propname == "" || propname.Contains(" "))	// TODO: Proper regex check
		{
			wxMessageBox("Invalid property name", "Error");
			return;
		}

		// Check if existing
		for (unsigned a = 0; a < properties.size(); a++)
		{
			if (properties[a]->getPropName() == propname)
			{
				wxMessageBox(S_FMT("Property \"%s\" already exists", propname), "Error");
				return;
			}
		}

		// Add property
		if (choice_type->GetSelection() == 0)
			addBoolProperty(group_custom, propname, propname);
		else if (choice_type->GetSelection() == 1)
			addStringProperty(group_custom, propname, propname);
		else if (choice_type->GetSelection() == 2)
			addIntProperty(group_custom, propname, propname);
		else if (choice_type->GetSelection() == 3)
			addFloatProperty(group_custom, propname, propname);
		else if (choice_type->GetSelection() == 4)
		{
			MOPGAngleProperty* prop_angle = new MOPGAngleProperty(propname, propname);
			prop_angle->setParent(this);
			properties.push_back(prop_angle);
			pg_properties->AppendIn(group_custom, prop_angle);
		}
		else if (choice_type->GetSelection() == 5)
			addTextureProperty(group_custom, propname, propname, 0);
		else if (choice_type->GetSelection() == 6)
			addTextureProperty(group_custom, propname, propname, 1);
		else if (choice_type->GetSelection() == 7)
		{
			MOPGColourProperty* prop_col = new MOPGColourProperty(propname, propname);
			prop_col->setParent(this);
			properties.push_back(prop_col);
			pg_properties->AppendIn(group_custom, prop_col);
		}
	}
}

/* MapObjectPropsPanel::onPropertyChanged
 * Called when a property value is changed
 *******************************************************************/
void MapObjectPropsPanel::onPropertyChanged(wxPropertyGridEvent& e)
{
	// Ignore if no auto apply
	if (no_apply || !mobj_props_auto_apply)
	{
		e.Skip();
		return;
	}

	// Find property
	string name = e.GetPropertyName();
	for (unsigned a = 0; a < properties.size(); a++)
	{
		if (properties[a]->getPropName() == name)
		{
			// Found, apply value
			string type;
			if (last_type == MOBJ_VERTEX)
				type = "Vertex";
			else if (last_type == MOBJ_LINE)
				type = "Line";
			else if (last_type == MOBJ_SECTOR)
				type = "Sector";
			else if (last_type == MOBJ_THING)
				type = "Thing";
			
			MapEditor::editContext().beginUndoRecordLocked(S_FMT("Modify %s Properties", CHR(type)), true, false, false);
			properties[a]->applyValue();
			MapEditor::editContext().endUndoRecord();
			return;
		}
	}
}
