
#include "Main.h"
#include "WxStuff.h"
#include "MapObjectPropsPanel.h"
#include "GameConfiguration.h"
#include "SLADEMap.h"
#include "MOPGProperty.h"
#include "MapEditorWindow.h"
#include "Icons.h"
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/gbsizer.h>

CVAR(Bool, mobj_props_show_all, false, CVAR_SAVE)


MapObjectPropsPanel::MapObjectPropsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	last_type = -1;
	group_custom = NULL;

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
	tabs_sections = new wxNotebook(this, -1);
	sizer->Add(tabs_sections, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add main property grid
	pg_properties = new wxPropertyGrid(tabs_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);
	tabs_sections->AddPage(pg_properties, "Properties");

	// Create side property grids
	pg_props_side1 = new wxPropertyGrid(tabs_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);
	pg_props_side2 = new wxPropertyGrid(tabs_sections, -1, wxDefaultPosition, wxDefaultSize, wxPG_TOOLTIPS|wxPG_SPLITTER_AUTO_CENTER);

	// Add buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add button
	btn_add = new wxBitmapButton(this, -1, getIcon("t_plus"));
	btn_add->SetToolTip("Add Property");
	hbox->Add(btn_add, 0, wxEXPAND|wxRIGHT, 4);
	hbox->AddStretchSpacer(1);

	// Reset button
	btn_reset = new wxBitmapButton(this, -1, getIcon("t_close"));
	btn_reset->SetToolTip("Discard Changes");
	hbox->Add(btn_reset, 0, wxEXPAND|wxRIGHT, 4);

	// Apply button
	btn_apply = new wxBitmapButton(this, -1, getIcon("i_tick"));
	btn_apply->SetToolTip("Apply Changes");
	hbox->Add(btn_apply, 0, wxEXPAND);

	wxPGCell cell;
	cell.SetText("<multiple values>");
	pg_properties->GetGrid()->SetUnspecifiedValueAppearance(cell);

	// Bind events
	btn_apply->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnApply, this);
	btn_reset->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnReset, this);
	cb_show_all->Bind(wxEVT_CHECKBOX, &MapObjectPropsPanel::onShowAllToggled, this);
	btn_add->Bind(wxEVT_BUTTON, &MapObjectPropsPanel::onBtnAdd, this);

	// Hide side property grids
	pg_props_side1->Show(false);
	pg_props_side2->Show(false);

	Layout();
}

MapObjectPropsPanel::~MapObjectPropsPanel()
{
}

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

void MapObjectPropsPanel::addUDMFProperty(UDMFProperty* prop, int objtype, string basegroup, wxPropertyGrid* grid)
{
	// Check property was given
	if (!prop)
		return;

	// Set grid to add to (main one if grid is NULL)
	if (!grid)
		grid = pg_properties;

	// Determine group name
	string groupname;
	if (!basegroup.IsEmpty())
		groupname = basegroup + ".";
	groupname += prop->getGroup();

	// Get group to add
	wxPGProperty* group = grid->GetProperty(groupname);
	if (!group)
		group = grid->Append(new wxPropertyCategory(prop->getGroup(), groupname));

	// Determine property name
	string propname;
	if (!basegroup.IsEmpty())
		propname = basegroup + ".";
	propname += prop->getProperty();

	// Add property depending on type
	//MOPGProperty* mopg_prop = NULL;
	if (prop->getType() == UDMFProperty::TYPE_BOOL)
		addBoolProperty(group, prop->getName(), propname, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_INT)
		addIntProperty(group, prop->getName(), propname, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_FLOAT)
		addFloatProperty(group, prop->getName(), propname, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_STRING)
		addStringProperty(group, prop->getName(), propname, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_COLOUR)
	{
		MOPGColourProperty* prop_col = new MOPGColourProperty(prop->getName(), propname);
		prop_col->setParent(this);
		prop_col->setUDMFProp(prop);
		properties.push_back(prop_col);
		grid->AppendIn(group, prop_col);
	}
	else if (prop->getType() == UDMFProperty::TYPE_ASPECIAL)
	{
		MOPGActionSpecialProperty* prop_as = new MOPGActionSpecialProperty(prop->getName(), propname);
		prop_as->setParent(this);
		prop_as->setUDMFProp(prop);
		properties.push_back(prop_as);
		grid->AppendIn(group, prop_as);
	}
	else if (prop->getType() == UDMFProperty::TYPE_SSPECIAL)
	{
		MOPGSectorSpecialProperty* prop_ss = new MOPGSectorSpecialProperty(prop->getName(), propname);
		prop_ss->setParent(this);
		prop_ss->setUDMFProp(prop);
		properties.push_back(prop_ss);
		grid->AppendIn(group, prop_ss);
	}
	else if (prop->getType() == UDMFProperty::TYPE_TTYPE)
	{
		MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty(prop->getName(), propname);
		prop_tt->setParent(this);
		prop_tt->setUDMFProp(prop);
		properties.push_back(prop_tt);
		grid->AppendIn(group, prop_tt);
	}
	else if (prop->getType() == UDMFProperty::TYPE_ANGLE)
	{
		MOPGAngleProperty* prop_angle = new MOPGAngleProperty(prop->getName(), propname);
		prop_angle->setParent(this);
		prop_angle->setUDMFProp(prop);
		properties.push_back(prop_angle);
		grid->AppendIn(group, prop_angle);
	}
	else if (prop->getType() == UDMFProperty::TYPE_TEX_WALL)
		addTextureProperty(group, prop->getName(), propname, 0, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_TEX_FLAT)
		addTextureProperty(group, prop->getName(), propname, 1, false, grid, prop);
	else if (prop->getType() == UDMFProperty::TYPE_ID)
	{
		MOPGTagProperty* prop_id = new MOPGTagProperty(prop->getName(), propname);
		prop_id->setParent(this);
		prop_id->setUDMFProp(prop);
		properties.push_back(prop_id);
		grid->AppendIn(group, prop_id);
	}

	/*if (mopg_prop) {
		mopg_prop->setParent(this);
		mopg_prop->setUDMFProp(prop);
		properties.push_back(mopg_prop);
		grid->AppendIn(group, (wxPGProperty*)mopg_prop);
	}*/
}

void MapObjectPropsPanel::setupType(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type == objtype)
		return;

	// Get map format
	int map_format = theMapEditor->currentMapDesc().format;

	// Clear property grid
	pg_properties->Clear();
	pg_props_side1->Clear();
	pg_props_side2->Clear();
	properties.clear();
	btn_add->Show(false);

	// Remove side1/2 tabs if they exist
	while (tabs_sections->GetPageCount() > 1)
		tabs_sections->RemovePage(1);
	pg_props_side1->Show(false);
	pg_props_side2->Show(false);

	// Vertex properties
	if (objtype == MOBJ_VERTEX)
	{
		// Set main tab name
		tabs_sections->SetPageText(0, "Vertex");

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
		tabs_sections->SetPageText(0, "Line");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add side indices
		addIntProperty(g_basic, "Front Side", "sidefront");
		addIntProperty(g_basic, "Back Side", "sideback");

		// Add 'Special' group
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
				MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_special, S_FMT("Arg%d", a+1), S_FMT("arg%d", a));

				// Link to action special if appropriate
				if (map_format == MAP_HEXEN) prop_as->addArgProperty(prop, a);
			}
		}
		else   // Sector tag otherwise
		{
			//addIntProperty(g_special, "Sector Tag", "arg0");
			MOPGTagProperty* prop_tag = new MOPGTagProperty("Sector Tag", "arg0");
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

		// Add 'Flags' group
		wxPGProperty* g_flags = pg_properties->Append(new wxPropertyCategory("Flags"));

		// Add flags
		for (int a = 0; a < theGameConfiguration->nLineFlags(); a++)
			addLineFlagProperty(g_flags, theGameConfiguration->lineFlag(a), S_FMT("flag%d", a), a);

		// --- Sides ---
		pg_props_side1->Show(true);
		pg_props_side2->Show(true);
		tabs_sections->AddPage(pg_props_side1, "Front Side");
		tabs_sections->AddPage(pg_props_side2, "Back Side");

		// 'General' group 1
		wxPGProperty* subgroup = pg_props_side1->Append(new wxPropertyCategory("General", "side1.general"));
		addIntProperty(subgroup, "Sector", "side1.sector");

		// 'Textures' group 1
		subgroup = pg_props_side1->Append(new wxPropertyCategory("Textures", "side1.textures"));
		addTextureProperty(subgroup, "Upper Texture", "side1.texturetop", 0);
		addTextureProperty(subgroup, "Middle Texture", "side1.texturemiddle", 0);
		addTextureProperty(subgroup, "Lower Texture", "side1.texturebottom", 0);

		// 'Offsets' group 1
		subgroup = pg_props_side1->Append(new wxPropertyCategory("Offsets", "side1.offsets"));
		addIntProperty(subgroup, "X Offset", "side1.offsetx");
		addIntProperty(subgroup, "Y Offset", "side1.offsety");

		// 'General' group 2
		subgroup = pg_props_side2->Append(new wxPropertyCategory("General", "side2.general"));
		addIntProperty(subgroup, "Sector", "side2.sector");

		// 'Textures' group 2
		subgroup = pg_props_side2->Append(new wxPropertyCategory("Textures", "side2.textures"));
		addTextureProperty(subgroup, "Upper Texture", "side2.texturetop", 0);
		addTextureProperty(subgroup, "Middle Texture", "side2.texturemiddle", 0);
		addTextureProperty(subgroup, "Lower Texture", "side2.texturebottom", 0);

		// 'Offsets' group 2
		subgroup = pg_props_side2->Append(new wxPropertyCategory("Offsets", "side2.offsets"));
		addIntProperty(subgroup, "X Offset", "side2.offsetx");
		addIntProperty(subgroup, "Y Offset", "side2.offsety");
	}

	// Sector properties
	else if (objtype == MOBJ_SECTOR)
	{
		// Set main tab name
		tabs_sections->SetPageText(0, "Sector");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add heights
		addIntProperty(g_basic, "Floor Height", "heightfloor");
		addIntProperty(g_basic, "Ceiling Height", "heightceiling");

		// Add tag
		//addIntProperty(g_basic, "Tag/ID", "id");
		MOPGTagProperty* prop_tag = new MOPGTagProperty("Tag/ID", "id");
		prop_tag->setParent(this);
		properties.push_back(prop_tag);
		pg_properties->AppendIn(g_basic, prop_tag);

		// Add 'Lighting' group
		wxPGProperty* g_light = pg_properties->Append(new wxPropertyCategory("Lighting"));

		// Add light level
		addIntProperty(g_light, "Light Level", "lightlevel");

		// Add 'Textures' group
		wxPGProperty* g_textures = pg_properties->Append(new wxPropertyCategory("Textures"));

		// Add textures
		addTextureProperty(g_textures, "Floor Texture", "texturefloor", 1);
		addTextureProperty(g_textures, "Ceiling Texture", "textureceiling", 1);

		// Add 'Special' group
		wxPGProperty* g_special = pg_properties->Append(new wxPropertyCategory("Special"));

		// Add special
		MOPGSectorSpecialProperty* prop_special = new MOPGSectorSpecialProperty("Special", "special");
		prop_special->setParent(this);
		properties.push_back(prop_special);
		pg_properties->AppendIn(g_special, prop_special);
	}

	// Thing properties
	else if (objtype == MOBJ_THING)
	{
		// Set main tab name
		tabs_sections->SetPageText(0, "Thing");

		// Add 'General' group
		wxPGProperty* g_basic = pg_properties->Append(new wxPropertyCategory("General"));

		// Add position
		addIntProperty(g_basic, "X Position", "x");
		addIntProperty(g_basic, "Y Position", "y");

		// Add z height
		if (map_format != MAP_DOOM)
			addIntProperty(g_basic, "Z Height", "height");

		// Add angle
		//addIntProperty(g_basic, "Angle", "angle");
		MOPGAngleProperty* prop_angle = new MOPGAngleProperty("Angle", "angle");
		prop_angle->setParent(this);
		properties.push_back(prop_angle);
		pg_properties->AppendIn(g_basic, prop_angle);

		// Add type
		MOPGThingTypeProperty* prop_tt = new MOPGThingTypeProperty("Type", "type");
		prop_tt->setParent(this);
		properties.push_back(prop_tt);
		pg_properties->AppendIn(g_basic, prop_tt);

		// Add id
		if (map_format != MAP_DOOM)
		{
			MOPGTagProperty* prop_id = new MOPGTagProperty("ID", "id");
			prop_id->setParent(this);
			properties.push_back(prop_id);
			pg_properties->AppendIn(g_basic, prop_id);
		}

		if (map_format == MAP_HEXEN)
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
				MOPGIntProperty* prop = (MOPGIntProperty*)addIntProperty(g_args, S_FMT("Arg%d", a+1), S_FMT("arg%d", a));
				prop_tt->addArgProperty(prop, a);
			}
		}

		// Add 'Flags' group
		wxPGProperty* g_flags = pg_properties->Append(new wxPropertyCategory("Flags"));

		// Add flags
		for (int a = 0; a < theGameConfiguration->nThingFlags(); a++)
			addThingFlagProperty(g_flags, theGameConfiguration->thingFlag(a), S_FMT("flag%d", a), a);
	}

	// Set all bool properties to use checkboxes
	pg_properties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	last_type = objtype;
}

void MapObjectPropsPanel::setupTypeUDMF(int objtype)
{
	// Nothing to do if it was already this type
	if (last_type == objtype)
		return;

	// Clear property grids
	pg_properties->Clear();
	pg_props_side1->Clear();
	pg_props_side2->Clear();
	properties.clear();
	btn_add->Show();

	// Remove side1/2 tabs if they exist
	while (tabs_sections->GetPageCount() > 1)
		tabs_sections->RemovePage(1);
	pg_props_side1->Show(false);
	pg_props_side2->Show(false);

	// Set main tab title
	if (objtype == MOBJ_VERTEX)
		tabs_sections->SetPageText(0, "Vertex");
	else if (objtype == MOBJ_LINE)
		tabs_sections->SetPageText(0, "Line");
	else if (objtype == MOBJ_SECTOR)
		tabs_sections->SetPageText(0, "Sector");
	else if (objtype == MOBJ_THING)
		tabs_sections->SetPageText(0, "Thing");

	// Go through all possible properties for this type
	bool args = false;
	vector<udmfp_t> props = theGameConfiguration->allUDMFProperties(objtype);
	sort(props.begin(), props.end());
	for (unsigned a = 0; a < props.size(); a++)
	{
		addUDMFProperty(props[a].property, objtype);
		if (props[a].property->getProperty() == "arg0")
			args = true;
	}

	// Add side properties if line type
	if (objtype == MOBJ_LINE)
	{
		// Add side tabs
		pg_props_side1->Show(true);
		pg_props_side2->Show(true);
		tabs_sections->AddPage(pg_props_side1, "Front Side");
		tabs_sections->AddPage(pg_props_side2, "Back Side");

		// Get side properties
		vector<udmfp_t> sprops = theGameConfiguration->allUDMFProperties(MOBJ_SIDE);
		sort(sprops.begin(), sprops.end());

		// Front side
		for (unsigned a = 0; a < sprops.size(); a++)
			addUDMFProperty(sprops[a].property, objtype, "side1", pg_props_side1);

		// Back side
		for (unsigned a = 0; a < sprops.size(); a++)
			addUDMFProperty(sprops[a].property, objtype, "side2", pg_props_side2);
	}

	// Set all bool properties to use checkboxes
	pg_properties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Link arg properties to type/special properties (if args exist)
	if (args)
	{
		for (unsigned a = 0; a < properties.size(); a++)
		{
			// Action special
			if (properties[a]->getType() == MOPGProperty::TYPE_ASPECIAL)
			{
				for (unsigned arg = 0; arg < 5; arg++)
					((MOPGActionSpecialProperty*)properties[a])->addArgProperty(pg_properties->GetProperty(S_FMT("arg%d", arg)), arg);
			}

			// Thing type
			else if (properties[a]->getType() == MOPGProperty::TYPE_TTYPE)
			{
				for (unsigned arg = 0; arg < 5; arg++)
					((MOPGThingTypeProperty*)properties[a])->addArgProperty(pg_properties->GetProperty(S_FMT("arg%d", arg)), arg);
			}
		}
	}

	last_type = objtype;
}

void MapObjectPropsPanel::openObject(MapObject* object)
{
	// Do open multiple objects
	vector<MapObject*> list;
	if (object) list.push_back(object);
	openObjects(list);
}

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
	if (theMapEditor->currentMapDesc().format == MAP_UDMF)
		setupTypeUDMF(objects[0]->getObjType());
	else
		setupType(objects[0]->getObjType());

	// Find any custom properties (UDMF only)
	if (theMapEditor->currentMapDesc().format == MAP_UDMF)
	{
		for (unsigned a = 0; a < objects.size(); a++)
		{
			// Go through object properties
			vector<MobjPropertyList::prop_t> objprops = objects[a]->props().allProperties();
			for (unsigned b = 0; b < objprops.size(); b++)
			{
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
		if (prop && (prop->GetValue().GetInteger() >= 0 || prop->IsValueUnspecified()))
			pg_props_side1->EnableProperty(pg_props_side1->GetGrid()->GetRoot());
		else
		{
			pg_props_side1->DisableProperty(pg_props_side1->GetGrid()->GetRoot());
			pg_props_side1->SetPropertyValueUnspecified(pg_props_side1->GetGrid()->GetRoot());
		}
		prop = pg_properties->GetProperty("sideback");
		if (prop && (prop->GetValue().GetInteger() >= 0 || prop->IsValueUnspecified()))
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

	pg_properties->Refresh();
	pg_props_side1->Refresh();
	pg_props_side2->Refresh();
}

void MapObjectPropsPanel::showApplyButton(bool show)
{
	btn_apply->Show(show);
}

void MapObjectPropsPanel::applyChanges()
{
	// Go through all current properties and apply the current value
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->applyValue();
}


void MapObjectPropsPanel::onBtnApply(wxCommandEvent& e)
{
	// Apply changes
	applyChanges();

	// Refresh map view
	theMapEditor->forceRefresh();
}

void MapObjectPropsPanel::onBtnReset(wxCommandEvent& e)
{
	// Go through all current properties and reset the value
	for (unsigned a = 0; a < properties.size(); a++)
		properties[a]->resetValue();
}

void MapObjectPropsPanel::onShowAllToggled(wxCommandEvent& e)
{
	mobj_props_show_all = cb_show_all->GetValue();

	// Refresh the list
	openObjects(objects);
}

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
