
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Game/UDMFProperty.h"
#include "MOPGProperty.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/PropertyUtils.h"
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
	PropsPanelBase{ parent },
	last_type_{ map::ObjectType::Object },
	no_apply_{ no_apply }
{
	auto lh = ui::LayoutHelper{ this };

	// Setup sizer
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add item label
	cb_show_all_ = new wxCheckBox(this, -1, wxS("Show All"));
	cb_show_all_->SetValue(mobj_props_show_all);
	sizer->Add(cb_show_all_, lh.sfWithBorder().Expand());

	// Add tabs
	stc_sections_ = STabCtrl::createControl(this);
	sizer->Add(stc_sections_, lh.sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Add main property grid
	pg_properties_ = createPropGrid();
	stc_sections_->AddPage(pg_properties_, wxS("Properties"));

	// Create side property grids
	pg_props_side1_ = createPropGrid();
	pg_props_side2_ = createPropGrid();

	// Add buttons
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Add button
	btn_add_ = new SIconButton(this, "plus", "Add Property");
	hbox->Add(btn_add_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->AddStretchSpacer(1);

	// Reset button
	btn_reset_ = new SIconButton(this, "close", "Discard Changes");
	hbox->Add(btn_reset_, lh.sfWithBorder(0, wxRIGHT).Expand());

	// Apply button
	btn_apply_ = new SIconButton(this, "tick", "Apply Changes");
	hbox->Add(btn_apply_, wxSizerFlags().Expand());

	wxPGCell cell;
	cell.SetText(wxS("<multiple values>"));
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
		stc_sections_,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_TOOLTIPS | wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED);
	propgrid->SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MULTIPLE_SELECTION);
	propgrid->SetCaptionTextColour(inactiveTextColour);
	propgrid->SetCellDisabledTextColour(inactiveTextColour);

	// wxPropertyGrid is very conservative about not stealing focus, and won't do it even if you
	// explicitly click on the category column, which doesn't make sense
	propgrid->Bind(
		wxEVT_LEFT_DOWN,
		[propgrid](wxMouseEvent& e)
		{
			propgrid->SetFocus();
			e.Skip();
		});
	propgrid->Bind(wxEVT_KEY_DOWN, [this, propgrid](wxKeyEvent& e) { onPropGridKeyDown(e, propgrid); });

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
	const wxPGProperty* group,
	T*                  prop,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	prop->setParent(this);
	prop->setUDMFProp(udmf_prop);

	if (udmf_prop)
		prop->SetHelpString(WX_FMT("{}\n({})", udmf_prop->name(), udmf_prop->propName()));

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
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	return addProperty(group, new MOPGBoolProperty(label, propname), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds an integer property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addIntProperty(
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGIntProperty(label, propname), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a float property cell to the grid under [group] for the object property
// [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addFloatProperty(
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGFloatProperty(label, propname), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a string property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addStringProperty(
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGStringProperty(label, propname), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a line flag property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addLineFlagProperty(
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	int                 index,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGLineFlagProperty(label, propname, index), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a thing flag property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addThingFlagProperty(
	const wxPGProperty* group,
	const wxString&     label,
	const wxString&     propname,
	int                 index,
	bool                readonly,
	wxPropertyGrid*     grid,
	game::UDMFProperty* udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGThingFlagProperty(label, propname, index), readonly, grid, udmf_prop);
}

// -----------------------------------------------------------------------------
// Adds a texture property cell to the grid under [group] for the object
// property [propname]
// -----------------------------------------------------------------------------
MOPGProperty* MapObjectPropsPanel::addTextureProperty(
	const wxPGProperty*    group,
	const wxString&        label,
	const wxString&        propname,
	mapeditor::TextureType textype,
	bool                   readonly,
	wxPropertyGrid*        grid,
	game::UDMFProperty*    udmf_prop)
{
	// Create property
	return addProperty(group, new MOPGTextureProperty(textype, label, propname), readonly, grid, udmf_prop);
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
	map::ObjectType     objtype,
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
		groupname = basegroup + wxS(".");
	groupname += wxString::FromUTF8(prop.group());

	// Get group to add
	auto group = grid->GetProperty(groupname);
	if (!group)
	{
		group = grid->Append(new wxPropertyCategory(wxString::FromUTF8(prop.group()), groupname));
		groups_.push_back(group);
	}

	// Determine property name
	wxString propname;
	if (!basegroup.IsEmpty())
		propname = basegroup + wxS(".");
	propname += wxString::FromUTF8(prop.propName());

	// Add property depending on type
	auto prop_name = wxString::FromUTF8(prop.name());
	if (prop.type() == UDMFProperty::Type::Boolean)
		addBoolProperty(group, prop_name, propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Int)
		addIntProperty(group, prop_name, propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Float)
		addFloatProperty(group, prop_name, propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::String)
		addStringProperty(group, prop_name, propname, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Colour)
		addProperty(group, new MOPGColourProperty(prop_name, propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ActionSpecial)
		addProperty(group, new MOPGActionSpecialProperty(prop_name, propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::SectorSpecial)
		addProperty(group, new MOPGSectorSpecialProperty(prop_name, propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ThingType)
		addProperty(group, new MOPGThingTypeProperty(prop_name, propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::Angle)
		addProperty(group, new MOPGAngleProperty(prop_name, propname), false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::TextureWall)
		addTextureProperty(group, prop_name, propname, mapeditor::TextureType::Texture, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::TextureFlat)
		addTextureProperty(group, prop_name, propname, mapeditor::TextureType::Flat, false, grid, &prop);
	else if (prop.type() == UDMFProperty::Type::ID)
	{
		MOPGTagProperty::IdType tagtype;
		if (objtype == map::ObjectType::Line)
			tagtype = MOPGTagProperty::IdType::Line;
		else if (objtype == map::ObjectType::Thing)
			tagtype = MOPGTagProperty::IdType::Thing;
		else
			tagtype = MOPGTagProperty::IdType::Sector;

		addProperty(group, new MOPGTagProperty(tagtype, prop_name, propname), false, grid, &prop);
	}
}

// -----------------------------------------------------------------------------
// Adds all relevant properties to the grid for [objtype]
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::setupType(map::ObjectType objtype)
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
	if (objtype == map::ObjectType::Vertex)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, wxS("Vertex"));

		// Add 'basic' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory(wxS("General")));

		// Add X and Y position
		addIntProperty(g_basic, wxS("X Position"), wxS("x"));
		addIntProperty(g_basic, wxS("Y Position"), wxS("y"));

		last_type_ = map::ObjectType::Vertex;
	}

	// Line properties
	else if (objtype == map::ObjectType::Line)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, wxS("Line"));

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory(wxS("General")));

		// Add side indices
		addIntProperty(g_basic, wxS("Front Side"), wxS("sidefront"));
		addIntProperty(g_basic, wxS("Back Side"), wxS("sideback"));

		// Add 'Special' group
		if (!propHidden("special"))
		{
			auto g_special = pg_properties_->Append(new wxPropertyCategory(wxS("Special")));

			// Add special
			auto prop_as = new MOPGActionSpecialProperty(wxS("Special"), wxS("special"));
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
						addIntProperty(g_special, WX_FMT("Arg{}", a + 1), WX_FMT("arg{}", a)));

					// Link to action special
					args_[a] = prop;
				}
			}
			else // Sector tag otherwise
			{
				auto prop_tag = new MOPGTagProperty(MOPGTagProperty::IdType::Sector, wxS("Sector Tag"), wxS("arg0"));
				prop_tag->setParent(this);
				properties_.push_back(prop_tag);
				pg_properties_->AppendIn(g_special, prop_tag);
			}

			// Add SPAC
			if (map_format == MapFormat::Hexen)
			{
				auto prop_spac = new MOPGSPACTriggerProperty(wxS("Trigger"), wxS("spac"));
				prop_spac->setParent(this);
				properties_.push_back(prop_spac);
				pg_properties_->AppendIn(g_special, prop_spac);
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			auto g_flags = pg_properties_->Append(new wxPropertyCategory(wxS("Flags")));

			// Add flags
			for (unsigned a = 0; a < game::configuration().nLineFlags(); a++)
				addLineFlagProperty(
					g_flags, wxString::FromUTF8(game::configuration().lineFlag(a).name), WX_FMT("flag{}", a), a);
		}

		// --- Sides ---
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, wxS("Front Side"));
		stc_sections_->AddPage(pg_props_side2_, wxS("Back Side"));

		// 'General' group 1
		auto subgroup = pg_props_side1_->Append(new wxPropertyCategory(wxS("General"), wxS("side1.general")));
		addIntProperty(subgroup, wxS("Sector"), wxS("side1.sector"));

		// 'Textures' group 1
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side1_->Append(new wxPropertyCategory(wxS("Textures"), wxS("side1.textures")));
			addTextureProperty(
				subgroup, wxS("Upper Texture"), wxS("side1.texturetop"), mapeditor::TextureType::Texture);
			addTextureProperty(
				subgroup, wxS("Middle Texture"), wxS("side1.texturemiddle"), mapeditor::TextureType::Texture);
			addTextureProperty(
				subgroup, wxS("Lower Texture"), wxS("side1.texturebottom"), mapeditor::TextureType::Texture);
		}

		// 'Offsets' group 1
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side1_->Append(new wxPropertyCategory(wxS("Offsets"), wxS("side1.offsets")));
			addIntProperty(subgroup, wxS("X Offset"), wxS("side1.offsetx"));
			addIntProperty(subgroup, wxS("Y Offset"), wxS("side1.offsety"));
		}

		// 'General' group 2
		subgroup = pg_props_side2_->Append(new wxPropertyCategory(wxS("General"), wxS("side2.general")));
		addIntProperty(subgroup, wxS("Sector"), wxS("side2.sector"));

		// 'Textures' group 2
		if (!propHidden("texturetop"))
		{
			subgroup = pg_props_side2_->Append(new wxPropertyCategory(wxS("Textures"), wxS("side2.textures")));
			addTextureProperty(
				subgroup, wxS("Upper Texture"), wxS("side2.texturetop"), mapeditor::TextureType::Texture);
			addTextureProperty(
				subgroup, wxS("Middle Texture"), wxS("side2.texturemiddle"), mapeditor::TextureType::Texture);
			addTextureProperty(
				subgroup, wxS("Lower Texture"), wxS("side2.texturebottom"), mapeditor::TextureType::Texture);
		}

		// 'Offsets' group 2
		if (!propHidden("offsetx"))
		{
			subgroup = pg_props_side2_->Append(new wxPropertyCategory(wxS("Offsets"), wxS("side2.offsets")));
			addIntProperty(subgroup, wxS("X Offset"), wxS("side2.offsetx"));
			addIntProperty(subgroup, wxS("Y Offset"), wxS("side2.offsety"));
		}
	}

	// Sector properties
	else if (objtype == map::ObjectType::Sector)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, wxS("Sector"));

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory(wxS("General")));

		// Add heights
		if (!propHidden("heightfloor"))
			addIntProperty(g_basic, wxS("Floor Height"), wxS("heightfloor"));
		if (!propHidden("heightceiling"))
			addIntProperty(g_basic, wxS("Ceiling Height"), wxS("heightceiling"));

		// Add tag
		if (!propHidden("id"))
		{
			auto prop_tag = new MOPGTagProperty(MOPGTagProperty::IdType::Sector, wxS("Tag/ID"), wxS("id"));
			prop_tag->setParent(this);
			properties_.push_back(prop_tag);
			pg_properties_->AppendIn(g_basic, prop_tag);
		}

		// Add 'Lighting' group
		if (!propHidden("lightlevel"))
		{
			auto g_light = pg_properties_->Append(new wxPropertyCategory(wxS("Lighting")));

			// Add light level
			addIntProperty(g_light, wxS("Light Level"), wxS("lightlevel"));
		}

		// Add 'Textures' group
		if (!propHidden("texturefloor"))
		{
			auto g_textures = pg_properties_->Append(new wxPropertyCategory(wxS("Textures")));

			// Add textures
			addTextureProperty(g_textures, wxS("Floor Texture"), wxS("texturefloor"), mapeditor::TextureType::Flat);
			addTextureProperty(g_textures, wxS("Ceiling Texture"), wxS("textureceiling"), mapeditor::TextureType::Flat);
		}

		// Add 'Special' group
		if (!propHidden("special"))
		{
			auto g_special = pg_properties_->Append(new wxPropertyCategory(wxS("Special")));

			// Add special
			auto prop_special = new MOPGSectorSpecialProperty(wxS("Special"), wxS("special"));
			prop_special->setParent(this);
			properties_.push_back(prop_special);
			pg_properties_->AppendIn(g_special, prop_special);
		}
	}

	// Thing properties
	else if (objtype == map::ObjectType::Thing)
	{
		// Set main tab name
		stc_sections_->SetPageText(0, wxS("Thing"));

		// Add 'General' group
		auto g_basic = pg_properties_->Append(new wxPropertyCategory(wxS("General")));

		// Add position
		addIntProperty(g_basic, wxS("X Position"), wxS("x"));
		addIntProperty(g_basic, wxS("Y Position"), wxS("y"));

		// Add z height
		if (map_format != MapFormat::Doom && !propHidden("height"))
			addIntProperty(g_basic, wxS("Z Height"), wxS("height"));

		// Add angle
		if (!propHidden("angle"))
		{
			auto prop_angle = new MOPGAngleProperty(wxS("Angle"), wxS("angle"));
			prop_angle->setParent(this);
			properties_.push_back(prop_angle);
			pg_properties_->AppendIn(g_basic, prop_angle);
		}

		// Add type
		if (!propHidden("type"))
		{
			auto prop_tt = new MOPGThingTypeProperty(wxS("Type"), wxS("type"));
			prop_tt->setParent(this);
			properties_.push_back(prop_tt);
			pg_properties_->AppendIn(g_basic, prop_tt);
		}

		// Add id
		if (map_format != MapFormat::Doom && !propHidden("id"))
		{
			auto prop_id = new MOPGTagProperty(MOPGTagProperty::IdType::Thing, wxS("ID"), wxS("id"));
			prop_id->setParent(this);
			properties_.push_back(prop_id);
			pg_properties_->AppendIn(g_basic, prop_id);
		}

		if (map_format == MapFormat::Hexen && !propHidden("special"))
		{
			// Add 'Scripting Special' group
			auto g_special = pg_properties_->Append(new wxPropertyCategory(wxS("Scripting Special")));

			// Add special
			auto prop_as = new MOPGActionSpecialProperty(wxS("Special"), wxS("special"));
			prop_as->setParent(this);
			properties_.push_back(prop_as);
			pg_properties_->AppendIn(g_special, prop_as);

			// Add 'Args' group
			auto g_args = pg_properties_->Append(new wxPropertyCategory(wxS("Args")));
			for (unsigned a = 0; a < 5; a++)
			{
				auto prop = dynamic_cast<MOPGIntProperty*>(
					addIntProperty(g_args, WX_FMT("Arg{}", a + 1), WX_FMT("arg{}", a)));
				args_[a] = prop;
			}
		}

		if (!hide_flags_)
		{
			// Add 'Flags' group
			auto g_flags = pg_properties_->Append(new wxPropertyCategory(wxS("Flags")));

			// Add flags
			for (int a = 0; a < game::configuration().nThingFlags(); a++)
				addThingFlagProperty(
					g_flags, wxString::FromUTF8(game::configuration().thingFlag(a)), WX_FMT("flag{}", a), a);
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
void MapObjectPropsPanel::setupTypeUDMF(map::ObjectType objtype)
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
	if (objtype == map::ObjectType::Vertex)
		stc_sections_->SetPageText(0, wxS("Vertex"));
	else if (objtype == map::ObjectType::Line)
		stc_sections_->SetPageText(0, wxS("Line"));
	else if (objtype == map::ObjectType::Sector)
		stc_sections_->SetPageText(0, wxS("Sector"));
	else if (objtype == map::ObjectType::Thing)
		stc_sections_->SetPageText(0, wxS("Thing"));

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
	if (objtype == map::ObjectType::Line)
	{
		// Add side tabs
		pg_props_side1_->Show(true);
		pg_props_side2_->Show(true);
		stc_sections_->AddPage(pg_props_side1_, wxS("Front Side"));
		stc_sections_->AddPage(pg_props_side2_, wxS("Back Side"));

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

			addUDMFProperty(i->second, objtype, wxS("side1"), pg_props_side1_);
			addUDMFProperty(i->second, objtype, wxS("side2"), pg_props_side2_);
		}
	}

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side1_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
	pg_props_side2_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Remember arg properties for passing to type/special properties (or set
	// to NULL if args don't exist)
	for (unsigned arg = 0; arg < 5; arg++)
		args_[arg] = pg_properties_->GetProperty(WX_FMT("arg{}", arg));

	last_type_ = objtype;
	udmf_      = true;

	Layout();
}

// -----------------------------------------------------------------------------
// Hides groups with no visible children and vice/versa
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::updateGroupVisibility() const
{
	for (auto group : groups_)
#if wxCHECK_VERSION(3, 3, 0)
		group->Hide(!group->HasVisibleChildren(), wxPGPropertyValuesFlags::DontRecurse);
#else
		group->Hide(!group->HasVisibleChildren(), wxPG_DONT_RECURSE);
#endif
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

	// Show all groups initially
	for (auto group : groups_)
#if wxCHECK_VERSION(3, 3, 0)
		group->Hide(false, wxPGPropertyValuesFlags::DontRecurse);
#else
		group->Hide(false, wxPG_DONT_RECURSE);
#endif

	// Setup property grid for the object type
	bool is_udmf      = (mapeditor::editContext().mapDesc().format == MapFormat::UDMF);
	bool type_changed = !(last_type_ == objects[0]->objType() && udmf_ == is_udmf);
	if (is_udmf)
		setupTypeUDMF(objects[0]->objType());
	else
		setupType(objects[0]->objType());

	// Find any custom properties (UDMF only)
	if (is_udmf)
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
						group_custom_ = pg_properties_->Append(new wxPropertyCategory(wxS("Custom")));

					// Add property
					auto pname_wx = wxString::FromUTF8(prop.name);
					switch (property::valueType(prop.value))
					{
					case property::ValueType::Bool:  addBoolProperty(group_custom_, pname_wx, pname_wx); break;
					case property::ValueType::Int:   addIntProperty(group_custom_, pname_wx, pname_wx); break;
					case property::ValueType::Float: addFloatProperty(group_custom_, pname_wx, pname_wx); break;
					default:                         addStringProperty(group_custom_, pname_wx, pname_wx); break;
					}
				}
			}
		}
	}

	// Generic properties
	for (auto property : properties_)
	{
		property->openObjects(objects);
		dynamic_cast<wxPGProperty*>(property)->SetModifiedStatus(false);
	}

	// Handle line sides
	if (objects[0]->objType() == map::ObjectType::Line)
	{
		// Enable/disable side properties
		auto prop = pg_properties_->GetProperty(wxS("sidefront"));
		if (prop && (prop->IsValueUnspecified() || prop->GetValue().GetInteger() >= 0))
			pg_props_side1_->EnableProperty(pg_props_side1_->GetGrid()->GetRoot());
		else
		{
			pg_props_side1_->DisableProperty(pg_props_side1_->GetGrid()->GetRoot());
			pg_props_side1_->SetPropertyValueUnspecified(pg_props_side1_->GetGrid()->GetRoot());
		}
		prop = pg_properties_->GetProperty(wxS("sideback"));
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

	// Hide empty groups
	updateGroupVisibility();

	pg_properties_->Thaw();
	pg_props_side1_->Thaw();
	pg_props_side2_->Thaw();

	// After changing types, the scroll position is meaningless, so scroll back to the top
	if (type_changed)
		pg_properties_->Scroll(0, 0);
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
			prop_with_args = static_cast<MOPGIntWithArgsProperty*>(prop);

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
	for (auto property : properties_)
	{
		auto wxprop = dynamic_cast<wxPGProperty*>(property);
		if (!wxprop->HasFlag(wxPG_PROP_MODIFIED))
			continue;

		property->applyValue();
		wxprop->SetModifiedStatus(false);
	}

	pg_properties_->Refresh();
	pg_props_side1_->Refresh();
	pg_props_side2_->Refresh();
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
	groups_.clear();

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
	last_type_ = map::ObjectType::Object;
}


// -----------------------------------------------------------------------------
//
// MapObjectPropsPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the apply button is clicked
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnApply(wxCommandEvent& e)
{
	string type;
	if (last_type_ == map::ObjectType::Vertex)
		type = "Vertex";
	else if (last_type_ == map::ObjectType::Line)
		type = "Line";
	else if (last_type_ == map::ObjectType::Sector)
		type = "Sector";
	else if (last_type_ == map::ObjectType::Thing)
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
	updateGroupVisibility();
}

// -----------------------------------------------------------------------------
// Called when the add property button is clicked
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onBtnAdd(wxCommandEvent& e)
{
	wxDialog dlg(this, -1, wxS("Add UDMF Property"));
	auto     lh = ui::LayoutHelper(&dlg);

	// Setup dialog sizer
	auto msizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(msizer);
	auto sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	msizer->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	// Name
	auto text_name = new wxTextCtrl(&dlg, -1, wxEmptyString);
	sizer->Add(new wxStaticText(&dlg, -1, wxS("Name:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_name, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Type
	wxString types[]     = { wxS("Boolean"), wxS("String"),         wxS("Integer"),        wxS("Float"),
							 wxS("Angle"),   wxS("Texture (Wall)"), wxS("Texture (Flat)"), wxS("Colour") };
	auto     choice_type = new wxChoice(&dlg, -1, wxDefaultPosition, wxDefaultSize, 7, types);
	choice_type->SetSelection(0);
	sizer->Add(new wxStaticText(&dlg, -1, wxS("Type:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
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
			group_custom_ = pg_properties_->Append(new wxPropertyCategory(wxS("Custom")));

		// Get name entered
		wxString propname = text_name->GetValue().Lower();
		if (propname.empty() || propname.Contains(wxS(" "))) // TODO: Proper regex check
		{
			wxMessageBox(wxS("Invalid property name"), wxS("Error"));
			return;
		}

		// Check if existing
		auto pname_str = propname.utf8_string();
		for (auto& property : properties_)
		{
			if (property->propName() == pname_str)
			{
				wxMessageBox(WX_FMT("Property \"){}\" already exists", pname_str), wxS("Error"));
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
	auto name = e.GetPropertyName().utf8_string();
	for (auto& property : properties_)
	{
		if (property->propName() == name)
		{
			// Found, apply value
			string type;
			if (last_type_ == map::ObjectType::Vertex)
				type = "Vertex";
			else if (last_type_ == map::ObjectType::Line)
				type = "Line";
			else if (last_type_ == map::ObjectType::Sector)
				type = "Sector";
			else if (last_type_ == map::ObjectType::Thing)
				type = "Thing";

			mapeditor::editContext().beginUndoRecordLocked(
				fmt::format("Modify {} Properties", type), true, false, false);
			property->applyValue();
			mapeditor::editContext().endUndoRecord();
			return;
		}
	}
}


// -----------------------------------------------------------------------------
// Handle a keypress on a propgrid itself (not a property editor)
// -----------------------------------------------------------------------------
void MapObjectPropsPanel::onPropGridKeyDown(wxKeyEvent& e, wxPropertyGrid* propgrid)
{
	if (e.GetKeyCode() == WXK_DELETE)
	{
		if (objects_.empty())
			return;

		// Clear the selected properties, and remove them entirely if custom
		auto selection = propgrid->GetSelectedProperties();
		propgrid->Freeze();
		for (auto& prop : selection)
		{
			auto mopg_prop = dynamic_cast<MOPGProperty*>(prop);
			if (mopg_prop)
				mopg_prop->clearValue();
		}
		propgrid->Thaw();
	}
	else
		e.Skip();
}
