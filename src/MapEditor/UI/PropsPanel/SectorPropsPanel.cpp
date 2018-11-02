
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SectorPropsPanel.cpp
// Description: UI for editing sector properties
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
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/MapObject.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Drawing.h"
#include "SectorPropsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
// FlatTexCanvas Class Functions
//
// A simple opengl canvas to display a texture
// (will have more advanced functionality later)
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// FlatTexCanvas::FlatTexCanvas
//
// FlatTexCanvas class constructor
// ----------------------------------------------------------------------------
FlatTexCanvas::FlatTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	// Init variables
	SetWindowStyleFlag(wxBORDER_SIMPLE);
	SetInitialSize(WxUtils::scaledSize(136, 136));
}

// ----------------------------------------------------------------------------
// FlatTexCanvas::setTexture
//
// Sets the texture to display
// ----------------------------------------------------------------------------
void FlatTexCanvas::setTexture(string tex)
{
	texname_ = tex;
	if (tex == "-" || tex == "")
		texture_ = nullptr;
	else
		texture_ = MapEditor::textureManager().getFlat(
			tex,
			Game::configuration().featureSupported(Game::Feature::MixTexFlats)
		);

	Refresh();
}

// ----------------------------------------------------------------------------
// FlatTexCanvas::draw
//
// Draws the canvas content
// ----------------------------------------------------------------------------
void FlatTexCanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Draw texture
	if (texture_ && texture_ != &(GLTexture::missingTex()))
	{
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture_, 0, 0, GetSize().x, GetSize().y, 0, 100.0);
	}
	else if (texture_ == &(GLTexture::missingTex()))
	{
		// Draw unknown icon
		GLTexture* tex = MapEditor::textureManager().getEditorImage("thing/unknown");
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(180, 0, 0);
		Drawing::drawTextureWithin(tex, 0, 0, GetSize().x, GetSize().y, 0, 0.25);
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// ----------------------------------------------------------------------------
// FlatComboBox Class Functions
//
// A custom combo box that will show a list of flats matching the current text
// in the control (eg. 'FLAT' will list all flats beginning with FLAT)
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// FlatComboBox::FlatComboBox
//
// FlatComboBox class constructor
// ----------------------------------------------------------------------------
FlatComboBox::FlatComboBox(wxWindow* parent) : wxComboBox(parent, -1)
{
	// Init
	wxArrayString list;
	list.Add("-");

	// Bind events
	Bind(wxEVT_COMBOBOX_DROPDOWN, &FlatComboBox::onDropDown, this);
	Bind(wxEVT_COMBOBOX_CLOSEUP, &FlatComboBox::onCloseUp, this);
	Bind(wxEVT_KEY_DOWN, &FlatComboBox::onKeyDown, this);

	Set(list);
}


// ----------------------------------------------------------------------------
//
// FlatComboBox Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// FlatComboBox::onDropDown
//
// Called when the dropdown list is expanded
// ----------------------------------------------------------------------------
void FlatComboBox::onDropDown(wxCommandEvent& e)
{
	// Get current value
	string text = GetValue().Upper();

	// Populate dropdown with matching flat names
	vector<map_texinfo_t>& textures = MapEditor::textureManager().getAllFlatsInfo();
	wxArrayString list;
	list.Add("-");
	for (unsigned a = 0; a < textures.size(); a++)
	{
		if (textures[a].shortName.StartsWith(text))
		{
			list.Add(textures[a].shortName);
		}
		if (Game::configuration().featureSupported(Game::Feature::LongNames))
		{
			if (textures[a].longName.StartsWith(text))
			{
				list.Add(textures[a].longName);
			}
		}
	}
	Set(list);	// Why does this clear the text box also?
	SetValue(text);

	e.Skip();
}

// ----------------------------------------------------------------------------
// FlatComboBox::onCloseUp
//
// Called when the dropdown list is closed
// ----------------------------------------------------------------------------
void FlatComboBox::onCloseUp(wxCommandEvent& e)
{
	list_down_ = false;
}

// ----------------------------------------------------------------------------
// FlatComboBox::onKeyDown
//
// Called when a key is pressed within the control
// ----------------------------------------------------------------------------
void FlatComboBox::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_DOWN && !list_down_)
	{
		list_down_ = true;
		Popup();
	}
	else
		e.Skip();
}


// ----------------------------------------------------------------------------
//
// SectorPropsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SectorPropsPanel::SectorPropsPanel
//
// SectorPropsPanel class constructor
// ----------------------------------------------------------------------------
SectorPropsPanel::SectorPropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND);

	// General tab
	stc_tabs_->AddPage(setupGeneralPanel(), "General");

	// Special tab
	stc_tabs_->AddPage(setupSpecialPanel(), "Special");

	// Other Properties tab

	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
	{
		mopp_all_props_ = new MapObjectPropsPanel(stc_tabs_, true);
		mopp_all_props_->hideProperty("texturefloor");
		mopp_all_props_->hideProperty("textureceiling");
		mopp_all_props_->hideProperty("heightfloor");
		mopp_all_props_->hideProperty("heightceiling");
		mopp_all_props_->hideProperty("lightlevel");
		mopp_all_props_->hideProperty("id");
		mopp_all_props_->hideProperty("special");
		stc_tabs_->AddPage(mopp_all_props_, "Other Properties");
	}
	else
		mopp_all_props_ = nullptr;

	// Bind events
	btn_new_tag_->Bind(wxEVT_BUTTON, &SectorPropsPanel::onBtnNewTag, this);
	fcb_floor_->Bind(wxEVT_TEXT, &SectorPropsPanel::onTextureChanged, this);
	fcb_ceiling_->Bind(wxEVT_TEXT, &SectorPropsPanel::onTextureChanged, this);
	gfx_floor_->Bind(wxEVT_LEFT_DOWN, &SectorPropsPanel::onTextureClicked, this);
	gfx_ceiling_->Bind(wxEVT_LEFT_DOWN, &SectorPropsPanel::onTextureClicked, this);
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::setupGeneralPanel
//
// Creates and sets up the general properties panel
// ----------------------------------------------------------------------------
wxPanel* SectorPropsPanel::setupGeneralPanel()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs_);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Floor ---
	wxBoxSizer* m_hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_hbox, 0, wxEXPAND|wxALL, UI::pad());
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Floor");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_hbox->Add(framesizer, 1, wxALIGN_CENTER|wxRIGHT, UI::pad());

	// Texture
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, UI::pad());
	gb_sizer->Add(gfx_floor_ = new FlatTexCanvas(panel), { 0, 0 }, { 1, 2 }, wxALIGN_CENTER);
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(fcb_floor_ = new FlatComboBox(panel), { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Height
	gb_sizer->Add(new wxStaticText(panel, -1, "Height:"), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_height_floor_ = new NumberTextCtrl(panel), { 2, 1 }, { 1, 1 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);


	// --- Ceiling ---
	frame = new wxStaticBox(panel, -1, "Ceiling");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_hbox->Add(framesizer, 1, wxALIGN_CENTER);

	// Texture
	gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, UI::pad());
	gb_sizer->Add(gfx_ceiling_ = new FlatTexCanvas(panel), { 0, 0 }, { 1, 2 }, wxALIGN_CENTER);
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(fcb_ceiling_ = new FlatComboBox(panel), { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Height
	gb_sizer->Add(new wxStaticText(panel, -1, "Height:"), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_height_ceiling_ = new NumberTextCtrl(panel), { 2, 1 }, { 1, 1 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);


	// -- General ---
	frame = new wxStaticBox(panel, -1, "General");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, UI::pad());
	gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, UI::pad());

	// Light level
	gb_sizer->Add(new wxStaticText(panel, -1, "Light Level:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_light_ = new NumberTextCtrl(panel), { 0, 1 }, { 1, 2 }, wxEXPAND);

	// Tag
	gb_sizer->Add(new wxStaticText(panel, -1, "Tag:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_tag_ = new NumberTextCtrl(panel), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(btn_new_tag_ = new wxButton(panel, -1, "New Tag"), { 1, 2 }, { 1, 1 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);

	return panel;
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::setupSpecialPanel
//
// Creates and sets up the special properties panel
// ----------------------------------------------------------------------------
wxPanel* SectorPropsPanel::setupSpecialPanel()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs_);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Add special panel
	sizer->Add(panel_special_ = new SectorSpecialPanel(panel), 1, wxEXPAND|wxALL, UI::pad());

	// Add override checkbox
	sizer->Add(
		cb_override_special_ = new wxCheckBox(panel, -1, "Override Special"),
		0,
		wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,
		UI::pad()
	);
	cb_override_special_->SetToolTip(
		"Differing specials detected, tick this to set the special for all selected sectors"
	);

	return panel;
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::openObjects
//
// Loads values from [objects]
// ----------------------------------------------------------------------------
void SectorPropsPanel::openObjects(vector<MapObject*>& objects)
{
	if (objects.empty())
		return;

	int ival;
	string sval;

	// Special
	if (MapObject::multiIntProperty(objects, "special", ival))
	{
		panel_special_->setup(ival);
		cb_override_special_->Show(false);
		cb_override_special_->SetValue(true);
	}
	else
		cb_override_special_->SetValue(false);

	// Floor texture
	if (MapObject::multiStringProperty(objects, "texturefloor", sval))
	{
		gfx_floor_->setTexture(sval);
		fcb_floor_->SetValue(sval);
	}

	// Ceiling texture
	if (MapObject::multiStringProperty(objects, "textureceiling", sval))
	{
		gfx_ceiling_->setTexture(sval);
		fcb_ceiling_->SetValue(sval);
	}

	// Floor height
	if (MapObject::multiIntProperty(objects, "heightfloor", ival))
		text_height_floor_->SetValue(S_FMT("%d", ival));

	// Ceiling height
	if (MapObject::multiIntProperty(objects, "heightceiling", ival))
		text_height_ceiling_->SetValue(S_FMT("%d", ival));

	// Light level
	if (MapObject::multiIntProperty(objects, "lightlevel", ival))
		text_light_->SetValue(S_FMT("%d", ival));

	// Tag
	if (MapObject::multiIntProperty(objects, "id", ival))
		text_tag_->SetValue(S_FMT("%d", ival));

	// Load other properties
	if (mopp_all_props_)
		mopp_all_props_->openObjects(objects);

	// Update internal objects list
	this->objects_.clear();
	for (unsigned a = 0; a < objects.size(); a++)
		this->objects_.push_back(objects[a]);

	// Update layout
	Layout();
	Refresh();
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::applyChanges
//
// Apply values to opened objects
// ----------------------------------------------------------------------------
void SectorPropsPanel::applyChanges()
{
	for (unsigned a = 0; a < objects_.size(); a++)
	{
		MapSector* sector = (MapSector*)objects_[a];

		// Special
		if (cb_override_special_->GetValue())
			sector->setIntProperty("special", panel_special_->getSelectedSpecial());

		// Floor texture
		if (!fcb_floor_->GetValue().IsEmpty())
			sector->setStringProperty("texturefloor", fcb_floor_->GetValue());

		// Ceiling texture
		if (!fcb_ceiling_->GetValue().IsEmpty())
			sector->setStringProperty("textureceiling", fcb_ceiling_->GetValue());

		// Floor height
		if (!text_height_floor_->GetValue().IsEmpty())
			sector->setIntProperty("heightfloor", text_height_floor_->getNumber(sector->getFloorHeight()));

		// Ceiling height
		if (!text_height_ceiling_->GetValue().IsEmpty())
			sector->setIntProperty("heightceiling", text_height_ceiling_->getNumber(sector->getCeilingHeight()));

		// Light level
		if (!text_light_->GetValue().IsEmpty())
			sector->setIntProperty("lightlevel", text_light_->getNumber(sector->getLightLevel()));

		// Tag
		if (!text_tag_->GetValue().IsEmpty())
			sector->setIntProperty("id", text_tag_->getNumber(sector->getTag()));
	}

	if (mopp_all_props_)
		mopp_all_props_->applyChanges();
}


// ----------------------------------------------------------------------------
//
// SectorPropsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SectorPropsPanel::onTextureChanged
//
// Called when a texture name is changed
// ----------------------------------------------------------------------------
void SectorPropsPanel::onTextureChanged(wxCommandEvent& e)
{
	// Floor
	if (e.GetEventObject() == fcb_floor_)
		gfx_floor_->setTexture(fcb_floor_->GetValue());

	// Ceiling
	else if (e.GetEventObject() == fcb_ceiling_)
		gfx_ceiling_->setTexture(fcb_ceiling_->GetValue());

	e.Skip();
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::onTextureClicked
//
// Called when a texture canvas is clicked
// ----------------------------------------------------------------------------
void SectorPropsPanel::onTextureClicked(wxMouseEvent& e)
{
	// Get canvas
	FlatTexCanvas* tc = nullptr;
	FlatComboBox* cb = nullptr;
	if (e.GetEventObject() == gfx_floor_)
	{
		tc = gfx_floor_;
		cb = fcb_floor_;
	}
	else if (e.GetEventObject() == gfx_ceiling_)
	{
		tc = gfx_ceiling_;
		cb = fcb_ceiling_;
	}

	if (!tc)
	{
		e.Skip();
		return;
	}

	// Browse
	MapTextureBrowser browser(this, 1, tc->texName(), &(MapEditor::editContext().map()));
	if (browser.ShowModal() == wxID_OK && browser.getSelectedItem())
		cb->SetValue(browser.getSelectedItem()->name());
}

// ----------------------------------------------------------------------------
// SectorPropsPanel::onBtnNewTag
//
// Called when the 'New Tag' button is clicked
// ----------------------------------------------------------------------------
void SectorPropsPanel::onBtnNewTag(wxCommandEvent& e)
{
	int tag = MapEditor::editContext().map().findUnusedSectorTag();
	text_tag_->SetValue(S_FMT("%d", tag));
}
