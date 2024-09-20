
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MapObjectPropsPanel.h"
#include "Game/Configuration.h"
#include "MOPGProperty.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "UI/Controls/SIconButton.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, mobj_props_show_all, false, CVar::Flag::Save)
CVAR(Bool, mobj_props_auto_apply, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapObjectPropsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapObjectPropsPanel class constructor
// -----------------------------------------------------------------------------
MapObjectPropsPanel::MapObjectPropsPanel(wxWindow* parent, bool no_apply) :
	PropsPanelBase{ parent }, no_apply_{ no_apply }
{
	// Setup sizer
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add item label
	cb_show_all_ = new wxCheckBox(this, -1, "Show All");
	cb_show_all_->SetValue(mobj_props_show_all);
	sizer->Add(cb_show_all_, 0, wxEXPAND | wxALL, ui::pad());

	// Add tabs
	stc_sections_ = STabCtrl::createControl(this);
	sizer->Add(stc_sections_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Add main property grid
	pg_properties_ = createPropGrid();
	stc_sections_->AddPage(pg_properties_, "Properties");

	// Create side property grids
	pg_props_side1_ = createPropGrid();
	pg_props_side2_ = createPropGrid();

	// Add buttons
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Add button
	btn_add_ = new SIconButton(this, "plus", "Add Property");
	hbox->Add(btn_add_, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->AddStretchSpacer(1);

	// Reset button
	btn_reset_ = new SIconButton(this, "close", "Discard Changes");
	hbox->Add(btn_reset_, 0, wxEXPAND | wxRIGHT, ui::pad());

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

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Create and set up a property grid
// -----------------------------------------------------------------------------
wxPropertyGrid* MapObjectPropsPanel::createPropGrid()
{
	const auto& inactiveTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

	auto propgrid = new wxPropertyGrid(
		stc_sections_, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS | wxPG_SPLITTER_AUTO_CENTER);
	propgrid->SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS);
	propgrid->SetCaptionTextColour(inactiveTextColour);
	propgrid->SetCellDisabledTextColour(inactiveTextColour);

	// wxPropertyGrid is very conservative about not stealing focus, and won't do it even if you
	// explicitly click on the category column, which doesn't make sense
	propgrid->Bind(wxEVT_LEFT_DOWN, [propgrid](wxMouseEvent& e) {
		propgrid->SetFocus();
		e.Skip();
	});

	return propgrid;
}

// -----------------------------------------------------------------------------
// Returns true if 'Show All' is ticked
// -----------------------------------------------------------------------------
bool MapObjectPropsPanel::showAll() const
{
	return cb_show_all_->IsChecked();
}

// -----------------------------------------------------------------------------
// Add an existing property to the grid.  Helper function for the following.
// Returns the same property, for convenience.
// -----------------------------------------------------------------------------
// We have a strange inheritance problem where every MOPGProperty subclass
// inherits from a wxPGProperty subclass, but MOPGProperty itself doesn't
// inherit from wxPGProperty, so we can't take a MOPGProperty* and call
// wxPGProperty methods on it.  So template it and let god sort 'em out
template<typename T>
T* MapObjectPropsPanel::addProperty(
	wxPGProperty*       group,
	T*                  prop,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	if (udmf_prop)
		prop->SetHelpString(wxString::Format("%s\n(%s)", udmf_prop->name(), udmf_prop->propName()));

	// Add it
	properties_.push_back(prop);
	if (!grid)
		pg_properties_->AppendIn(group, prop);
	else
		grid->AppendIn(group, prop);

	// Set read-only if specified
	if (readonly)
		prop->ChangeFlag(wxPG_PROP_READONLY, true);

	return prop;
}

// -----------------------------------------------------------------------------
// Adds a boolean property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addBoolProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	return addProperty(
		group, new MOPGBoolProperty(label, propname),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds an integer property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addIntProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGIntProperty(label, propname),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a float property cell to the grid under [group] for the object property
// [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addFloatProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGFloatProperty(label, propname),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a string property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addStringProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGStringProperty(label, propname),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a line flag property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addLineFlagProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	int                 index,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGLineFlagProperty(label, propname, index),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a thing flag property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addThingFlagProperty(
	wxPGProperty*       group,
	const wxString&     label,
	const wxString&     propname,
	int                 index,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGThingFlagProperty(label, propname, index),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a texture property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addTextureProperty(
	wxPGProperty*          group,
	const wxString&        label,
	const wxString&        propname,
	mapeditor::TextureType textype,
	bool                   readonly,
	wxPropertyGrid*        grid,
	game::UDMFProperty*    udmf_prop)
{
	// Create property
	return addProperty(
		group, new MOPGTextureProperty(textype, label, propname),
		readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Sets the boolean property cell [prop]'s value to [value]
// -----------------------------------------------------------------------------
bool MapObjectPropsPanel::setBoolProperty(wxPGProperty* prop, bool value, bool force_set) const
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

// -----------------------------------------------------------------------------
// Adds the UDMF property [prop] to the grid, under [basegroup]. Will add the
// correct property cell type for the UDMF property
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::addUDMFProperty(
	game::UDMFProperty& prop,
	MapObject::Type     objtype,
	const wxString&     basegroup,
	wxPropertyGrid*     grid)
{
	using game::UDMFProperty;

	// Set grid to add to (main one if grid is NULL)
	if (!grid)
		grid = pg_properties_;

	// Determine group name
	wxString groupname;
	if (!basegroup.IsEmpty())
		groupname = basegroup + ".";
	groupname += prop.group();

	// Get group to add
	auto group = grid->GetProperty(groupname);
	if (!group)
		group = grid->Append(new wxPropertyCategory(prop.group(), groupname));

	// Determine property name
	wxString propname;
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
		addProperty(group, new MOPGColourProperty(prop.name(), propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ActionSpecial)
		addProperty(group, new MOPGActionSpecialProperty(prop.name(), propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::SectorSpecial)
		addProperty(group, new MOPGSectorSpecialProperty(prop.name(), propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ThingType)
		addProperty(group, new MOPGThingTypeProperty(prop.name(), propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Angle)
		addProperty(group, new MOPGAngleProperty(prop.name(), propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::TextureWall)
		addTextureProperty(group, prop.name(), propname, mapeditor::TextureType::Texture, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::TextureFlat)
		addTextureProperty(group, prop.name(), propname, mapeditor::TextureType::Flat, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ID)
	{
		MOPGTagProperty::IdType tagtype;
		if (objtype == MapObject::Type::Line)
			tagtype = MOPGTagProperty::IdType::Line;
		else if (objtype == MapObject::Type::Thing)
			tagtype = MOPGTagProperty::IdType::Thing;
		else
			tagtype = MOPGTagProperty::IdType::Sector;

		addProperty(group, new MOPGTagProperty(tagtype, prop.name(), propname), false, grid, &prop);
	}
}

// -----------------------------------------------------------------------------
// Adds all relevant properties to the grid for [objtype]
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::setupType(MapObject::Type objtype)
{
	// Nothing to do if it was already this type
	if (last_type_ == objtype && !udmf_)
		return;

	// Get map format
	auto map_format = mapeditor::editContext().mapDesc().format;

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
	if (objtype == MapObject::Type::Vertex)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Vertex");

		// Add 'basic' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add X and Y position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		last_type_ = MapObject::Type::Vertex;
	}

	// Line properties
	else if (objtype == MapObject::Type::Line)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Line");

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add side indices
		addIntProperty(g_basic, "Front Side", "sidefront");
		addIntProperty(g_basic, "Back Side", "sideback");

		// Add 'Special' group
		if (!propHidden("special"))
		{
			auto g_special = pg_properties_->Append(new wxPropertyCategory("Special"));

			// Add special
			auto prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties_.push_back(prop_as);
			pg_properties_->AppendIn(g_special, prop_as);

			// Add args (hexen)
			if (map_format == MapFormat::Hexen)
			{
				for (unsigned a = 0; a < 5; a++)
				{
					// Add arg property
					auto prop = dynamic_cast<MOPGIntProperty*>(
						addIntProperty(g_special, wxString::Format("Arg%u", a + 1), wxString::Format("arg%u", a)));

					// Link to action special
					args_[a] = prop;
				}
			}
			else // Sector tag otherwise
			{
				auto prop_tag = new MOPGTagProperty(MOPGTagProperty::IdType::Sector, "Sector Tag", "arg0");
				prop_tag->setParent(this);
				properties_.push_back(prop_tag);
				pg_properties_->AppendIn(g_special, prop_tag);
			}

			// Add SPAC
			if (map_format == MapFormat::Hexen)
			{
				auto prop_spac = new MOPGSPACTriggerProperty("Trigger", "spac");
				prop_spac->setParent(this);
				properties_.push_back(prop_spac);
				pg_properties_->AppendIn(g_special, prop_spac);
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			auto g_flags = pg_properties_->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (unsigned a = 0; a < game::configuration().nLineFlags(); a++)
				addLineFlagProperty(g_flags, game::configuration().lineFlag(a).name, wxString::Format("flag%u", a), a);
		}

		// --- Sides ---
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, "Front Side");
		stc_sections_->AddPage(pg_props_side2_, "Back Side");

		// 'General' group 1
		auto subgroup = pg_props_side1_->Append(new wxPropertyCategory("General", "side1.general"));
		addIntProperty(subgroup, "Sector", "side1.sector");

		// 'Textures' group 1
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side1_->Append(new wxPropertyCategory("Textures", "side1.textures"));
			addTextureProperty(subgroup, "Upper Texture", "side1.texturetop", mapeditor::TextureType::Texture);
			addTextureProperty(subgroup, "Middle Texture", "side1.texturemiddle", mapeditor::TextureType::Texture);
			addTextureProperty(subgroup, "Lower Texture", "side1.texturebottom", mapeditor::TextureType::Texture);
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
			addTextureProperty(subgroup, "Upper Texture", "side2.texturetop", mapeditor::TextureType::Texture);
			addTextureProperty(subgroup, "Middle Texture", "side2.texturemiddle", mapeditor::TextureType::Texture);
			addTextureProperty(subgroup, "Lower Texture", "side2.texturebottom", mapeditor::TextureType::Texture);
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
	else if (objtype == MapObject::Type::Sector)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Sector");

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add heights
		if (!propHidden("heightfloor"))
			addIntProperty(g_basic, "Floor Height", "heightfloor");
		if (!propHidden("heightceiling"))
			addIntProperty(g_basic, "Ceiling Height", "heightceiling");

		// Add tag
		if (!propHidden("id"))
		{
			auto prop_tag = new MOPGTagProperty(MOPGTagProperty::IdType::Sector, "Tag/ID", "id");
			prop_tag->setParent(this);
			properties_.push_back(prop_tag);
			pg_properties_->AppendIn(g_basic, prop_tag);
		}

		// Add 'Lighting' group
		if (!propHidden("lightlevel"))
		{
			auto g_light = pg_properties_->Append(new wxPropertyCategory("Lighting"));

			// Add light level
			addIntProperty(g_light, "Light Level", "lightlevel");
		}

		// Add 'Textures' group
		if (!propHidden("texturefloor"))
		{
			auto g_textures = pg_properties_->Append(new wxPropertyCategory("Textures"));

			// Add textures
			addTextureProperty(g_textures, "Floor Texture", "texturefloor", mapeditor::TextureType::Flat);
			addTextureProperty(g_textures, "Ceiling Texture", "textureceiling", mapeditor::TextureType::Flat);
		}

		// Add 'Special' group
		if (!propHidden("special"))
		{
			auto g_special = pg_properties_->Append(new wxPropertyCategory("Special"));

			// Add special
			auto prop_special = new MOPGSectorSpecialProperty("Special", "special");
			prop_special->setParent(this);
			properties_.push_back(prop_special);
			pg_properties_->AppendIn(g_special, prop_special);
		}
	}

	// Thing properties
	else if (objtype == MapObject::Type::Thing)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, "Thing");

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory("General"));

		// Add position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		// Add z height
		if (map_format != MapFormat::Doom && !propHidden("height"))
			addIntProperty(g_basic, "Z Height", "height");

		// Add angle
		if (!propHidden("angle"))
		{
			auto prop_angle = new MOPGAngleProperty("Angle", "angle");
			prop_angle->setParent(this);
			properties_.push_back(prop_angle);
			pg_properties_->AppendIn(g_basic, prop_angle);
		}

		// Add type
		if (!propHidden("type"))
		{
			auto prop_tt = new MOPGThingTypeProperty("Type", "type");
			prop_tt->setParent(this);
			properties_.push_back(prop_tt);
			pg_properties_->AppendIn(g_basic, prop_tt);
		}

		// Add id
		if (map_format != MapFormat::Doom && !propHidden("id"))
		{
			auto prop_id = new MOPGTagProperty(MOPGTagProperty::IdType::Thing, "ID", "id");
			prop_id->setParent(this);
			properties_.push_back(prop_id);
			pg_properties_->AppendIn(g_basic, prop_id);
		}

		if (map_format == MapFormat::Hexen && !propHidden("special"))
		{
			// Add 'Scripting Special' group
			auto g_special = pg_properties_->Append(new wxPropertyCategory("Scripting Special"));

			// Add special
			auto prop_as = new MOPGActionSpecialProperty("Special", "special");
			prop_as->setParent(this);
			properties_.push_back(prop_as);
			pg_properties_->AppendIn(g_special, prop_as);

			// Add 'Args' group
			auto g_args = pg_properties_->Append(new wxPropertyCategory("Args"));
			for (unsigned a = 0; a < 5; a++)
			{
				auto prop = dynamic_cast<MOPGIntProperty*>(
					addIntProperty(g_args, wxString::Format("Arg%u", a + 1), wxString::Format("arg%u", a)));
				args_[a] = prop;
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			auto g_flags = pg_properties_->Append(new wxPropertyCategory("Flags"));

			// Add flags
			for (int a = 0; a < game::configuration().nThingFlags(); a++)
				addThingFlagProperty(g_flags, game::configuration().thingFlag(a), wxString::Format("flag%u", a), a);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	last_type_ = objtype;
	udmf_      = false;

	Layout();
}

// -----------------------------------------------------------------------------
// Adds all relevant UDMF properties to the grid for [objtype]
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::setupTypeUDMF(MapObject::Type objtype)
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
	if (objtype == MapObject::Type::Vertex)
		stc_sections_->SetPageText(0, "Vertex");
	else if (objtype == MapObject::Type::Line)
		stc_sections_->SetPageText(0, "Line");
	else if (objtype == MapObject::Type::Sector)
		stc_sections_->SetPageText(0, "Sector");
	else if (objtype == MapObject::Type::Thing)
		stc_sections_->SetPageText(0, "Thing");

	// Go through all possible properties for this type, in configuration order
	auto props = game::configuration().sortedUDMFProperties(objtype);
	for (auto& i : props)
	{
		// Skip if hidden
		if ((hide_flags_ && i->second.isFlag()) || (hide_triggers_ && i->second.isTrigger()))
		{
			hide_props_.emplace_back(i->second.propName());
			continue;
		}
		if (VECTOR_EXISTS(hide_props_, i->second.propName()))
			continue;

		addUDMFProperty(i->second, objtype);
	}

	// Add side properties if line type
	if (objtype == MapObject::Type::Line)
	{
		// Add side tabs
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, "Front Side");
		stc_sections_->AddPage(pg_props_side2_, "Back Side");

		// Get side properties
		auto sprops = game::configuration().sortedUDMFProperties(MapObject::Type::Side);

		// Add to both sides
		for (auto& i : sprops)
		{
			// Skip if hidden
			if ((hide_flags_ && i->second.isFlag()) || (hide_triggers_ && i->second.isTrigger()))
			{
				hide_props_.emplace_back(i->second.propName());
				continue;
			}
			if (VECTOR_EXISTS(hide_props_, i->second.propName()))
				continue;

			addUDMFProperty(i->second, objtype, "side1", pg_props_side1_);
			addUDMFProperty(i->second, objtype, "side2", pg_props_side2_);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Remember arg properties for passing to type/special properties (or set
	// to NULL if args don't exist)
	for (unsigned arg = 0; arg < 5; arg++)
		args_[arg] = pg_properties_->GetProperty(wxString::Format("arg%u", arg));

	last_type_ = objtype;
	udmf_      = true;

	Layout();
}

// -----------------------------------------------------------------------------
// Populates the grid with properties for [object]
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::openObject(MapObject* object)
{
	// Do open multiple objects
	vector<MapObject*> list;
	if (object)
		list.push_back(object);
	openObjects(list);
}

// -----------------------------------------------------------------------------
// Populates the grid with properties for all MapObjects in [objects]
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::openObjects(vector<MapObject*>& objects)
{
	pg_properties_->Freeze();
	pg_props_side1_->Freeze();
	pg_props_side2_->Freeze();

	// Check any objects were given
	if (objects.empty() || objects[0] == nullptr)
	{
		objects_.clear();
		pg_properties_->DisableProperty(pg_properties_->GetGrid()->GetRoot());
		pg_properties_->SetPropertyValueUnspecified(pg_properties_->GetGrid()->GetRoot());
		pg_props_side1_->DisableProperty(pg_props_side1_->GetGrid()->GetRoot());
		pg_props_side1_->SetPropertyValueUnspecified(pg_props_side1_->GetGrid()->GetRoot());
		pg_props_side2_->DisableProperty(pg_props_side2_->GetGrid()->GetRoot());
		pg_props_side2_->SetPropertyValueUnspecified(pg_props_side2_->GetGrid()->GetRoot());

		pg_properties_->Thaw();
		pg_props_side1_->Thaw();
		pg_props_side2_->Thaw();

		return;
	}
	else
		pg_properties_->EnableProperty(pg_properties_->GetGrid()->GetRoot());

	// Setup property grid for the object type
	if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
		setupTypeUDMF(objects[0]->objType());
	else
		setupType(objects[0]->objType());

	// Find any custom properties (UDMF only)
	if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
	{
		for (auto& object : objects)
		{
			// Go through object properties
			auto objprops = object->props().properties();
			for (auto& prop : objprops)
			{
				// Ignore side property
				if (strutil::startsWith(prop.name, "side1.") || strutil::startsWith(prop.name, "side2."))
					continue;

				// Check if hidden
				if (VECTOR_EXISTS(hide_props_, prop.name))
					continue;

				// Check if property is already on the list
				bool exists = false;
				for (auto& property : properties_)
				{
					if (property->propName() == prop.name)
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

					// Add property
					switch (property::valueType(prop.value))
					{
					case property::ValueType::Bool: addBoolProperty(group_custom_, prop.name, prop.name); break;
					case property::ValueType::Int: addIntProperty(group_custom_, prop.name, prop.name); break;
					case property::ValueType::Float: addFloatProperty(group_custom_, prop.name, prop.name); break;
					default: addStringProperty(group_custom_, prop.name, prop.name); break;
					}
				}
			}
		}
	}

	// Generic properties
	for (auto& property : properties_)
		property->openObjects(objects);

	// Handle line sides
	if (objects[0]->objType() == MapObject::Type::Line)
	{
		// Enable/disable side properties
		auto prop = pg_properties_->GetProperty("sidefront");
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
	if (&objects != &objects_)
	{
		objects_.clear();
		for (auto object : objects)
			objects_.push_back(object);
	}

	// Possibly update the argument names and visibility
	updateArgs(nullptr);

	pg_properties_->Thaw();
	pg_props_side1_->Thaw();
	pg_props_side2_->Thaw();
}

// -----------------------------------------------------------------------------
// Updates the names and visibility of the "arg" properties
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::updateArgs(MOPGIntWithArgsProperty* source)
{
	MOPGIntWithArgsProperty* prop_with_args;
	MOPGIntWithArgsProperty* owner_prop = source; // sane default

	// First determine which property owns the args.  Use the last one that has
	// a specified and non-zero value.  ThingType always wins, though, because
	// ThingTypes with args ignore their specials.
	for (auto& prop : properties_)
	{
		if (prop->type() == MOPGProperty::Type::ThingType || prop->type() == MOPGProperty::Type::ActionSpecial)
		{
			prop_with_args = (MOPGIntWithArgsProperty*)prop;

			if (!prop_with_args->IsValueUnspecified() && prop_with_args->GetValue().GetInteger() != 0
				&& prop_with_args->hasArgs())
			{
				owner_prop = prop_with_args;

				if (prop->type() == MOPGProperty::Type::ThingType)
					break;
			}
		}
	}

	if (owner_prop)
		owner_prop->updateArgs(args_);
}

// -----------------------------------------------------------------------------
// Applies any property changes to the opened object(s)
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::applyChanges()
{
	// Go through all current properties and apply the current value
	for (auto& property : properties_)
		property->applyValue();
}

// -----------------------------------------------------------------------------
// Clears all property grid rows and tabs
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::clearGrid()
{
	// Clear property grids
	for (auto& arg : args_)
		arg = nullptr;
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
	last_type_ = MapObject::Type::Object;
}


// -----------------------------------------------------------------------------
//
// MapObjectPropsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the apply button is clicked
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnApply(wxCommandEvent& e)
{
	string type;
	if (last_type_ == MapObject::Type::Vertex)
		type = "Vertex";
	else if (last_type_ == MapObject::Type::Line)
		type = "Line";
	else if (last_type_ == MapObject::Type::Sector)
		type = "Sector";
	else if (last_type_ == MapObject::Type::Thing)
		type = "Thing";

	// Apply changes
	mapeditor::editContext().beginUndoRecordLocked(fmt::format("Modify {} Properties", type), true, false, false);
	applyChanges();
	mapeditor::editContext().endUndoRecord();

	// Refresh map view
	mapeditor::window()->forceRefresh(true);
}

// -----------------------------------------------------------------------------
// Called when the reset button is clicked
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnReset(wxCommandEvent& e)
{
	// Go through all current properties and reset the value
	for (auto& property : properties_)
		property->resetValue();
}

// -----------------------------------------------------------------------------
// Called when the 'show all' checkbox is toggled
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onShowAllToggled(wxCommandEvent& e)
{
	mobj_props_show_all = cb_show_all_->GetValue();

	// Refresh each property's visibility
	for (auto& property : properties_)
		property->updateVisibility();

	updateArgs(nullptr);
}

// -----------------------------------------------------------------------------
// Called when the add property button is clicked
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnAdd(wxCommandEvent& e)
{
	wxDialog dlg(this, -1, "Add UDMF Property");

	// Setup dialog sizer
	auto msizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(msizer);
	auto sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	msizer->Add(sizer, 1, wxEXPAND | wxALL, ui::padLarge());

	// Name
	auto text_name = new wxTextCtrl(&dlg, -1, "");
	sizer->Add(new wxStaticText(&dlg, -1, "Name:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_name, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Type
	wxString types[] = {
		"Boolean", "String", "Integer", "Float", "Angle", "Texture (Wall)", "Texture (Flat)", "Colour"
	};
	auto choice_type = new wxChoice(&dlg, -1, wxDefaultPosition, wxDefaultSize, 7, types);
	choice_type->SetSelection(0);
	sizer->Add(new wxStaticText(&dlg, -1, "Type:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_type, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Buttons
	sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), { 2, 0 }, { 1, 2 }, wxEXPAND);

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
		wxString propname = text_name->GetValue().Lower();
		if (propname.empty() || propname.Contains(" ")) // TODO: Proper regex check
		{
			wxMessageBox("Invalid property name", "Error");
			return;
		}

		// Check if existing
		for (auto& property : properties_)
		{
			if (property->propName() == propname)
			{
				wxMessageBox(wxString::Format("Property \"%s\" already exists", propname), "Error");
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
			auto prop_angle = new MOPGAngleProperty(propname, propname);
			prop_angle->setParent(this);
			properties_.push_back(prop_angle);
			pg_properties_->AppendIn(group_custom_, prop_angle);
		}
		else if (choice_type->GetSelection() == 5)
			addTextureProperty(group_custom_, propname, propname, mapeditor::TextureType::Texture);
		else if (choice_type->GetSelection() == 6)
			addTextureProperty(group_custom_, propname, propname, mapeditor::TextureType::Flat);
		else if (choice_type->GetSelection() == 7)
		{
			auto prop_col = new MOPGColourProperty(propname, propname);
			prop_col->setParent(this);
			properties_.push_back(prop_col);
			pg_properties_->AppendIn(group_custom_, prop_col);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when a property value is changed
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onPropertyChanged(wxPropertyGridEvent& e)
{
	// Ignore if no auto apply
	if (no_apply_ || !mobj_props_auto_apply)
	{
		e.Skip();
		return;
	}

	// Find property
	wxString name = e.GetPropertyName();
	for (auto& property : properties_)
	{
		if (property->propName() == name)
		{
			// Found, apply value
			string type;
			if (last_type_ == MapObject::Type::Vertex)
				type = "Vertex";
			else if (last_type_ == MapObject::Type::Line)
				type = "Line";
			else if (last_type_ == MapObject::Type::Sector)
				type = "Sector";
			else if (last_type_ == MapObject::Type::Thing)
				type = "Thing";

			mapeditor::editContext().beginUndoRecordLocked(
				fmt::format("Modify {} Properties", type), true, false, false);
			property->applyValue();
			mapeditor::editContext().endUndoRecord();
			return;
		}
	}
}
