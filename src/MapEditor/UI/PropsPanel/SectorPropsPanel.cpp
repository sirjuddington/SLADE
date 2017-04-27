
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SectorPropsPanel.cpp
 * Description: UI for editing sector properties
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
#include "SectorPropsPanel.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/MapObject.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Drawing.h"
#include "UI/NumberTextCtrl.h"
#include "UI/STabCtrl.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditContext.h"


/*******************************************************************
 * FLATTEXCANVAS CLASS FUNCTIONS
 *******************************************************************
 * A simple opengl canvas to display a texture (will have more
 * advanced functionality later)
 */

/* FlatTexCanvas::FlatTexCanvas
 * FlatTexCanvas class constructor
 *******************************************************************/
FlatTexCanvas::FlatTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	// Init variables
	texture = nullptr;
	SetWindowStyleFlag(wxBORDER_SIMPLE);

	SetInitialSize(wxSize(136, 136));
}

/* FlatTexCanvas::~FlatTexCanvas
 * FlatTexCanvas class destructor
 *******************************************************************/
FlatTexCanvas::~FlatTexCanvas()
{
}

/* FlatTexCanvas::getTexName
 * Returns the name of the loaded texture
 *******************************************************************/
string FlatTexCanvas::getTexName()
{
	return texname;
}

/* FlatTexCanvas::setTexture
 * Sets the texture to display
 *******************************************************************/
void FlatTexCanvas::setTexture(string tex)
{
	texname = tex;
	if (tex == "-" || tex == "")
		texture = nullptr;
	else
		texture = MapEditor::textureManager().getFlat(tex, theGameConfiguration->mixTexFlats());

	Refresh();
}

/* FlatTexCanvas::draw
 * Draws the canvas content
 *******************************************************************/
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
	if (texture && texture != &(GLTexture::missingTex()))
	{
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture, 0, 0, GetSize().x, GetSize().y, 0, 100.0);
	}
	else if (texture == &(GLTexture::missingTex()))
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


/*******************************************************************
 * FLATCOMBOBOX CLASS FUNCTIONS
 *******************************************************************
 * A custom combo box that will show a list of flats matching the
 * current text in the control (eg. 'FLAT' will list all flats
 * beginning with FLAT). On OSX it will simply have a dropdown list
 * of all flats due to wxWidgets limitations on that platform
 */

/* FlatComboBox::FlatComboBox
 * FlatComboBox class constructor
 *******************************************************************/
FlatComboBox::FlatComboBox(wxWindow* parent) : wxComboBox(parent, -1)
{
	// Init
	list_down = false;
	SetInitialSize(wxSize(136, -1));
	wxArrayString list;
	list.Add("-");

	// Add all flats to dropdown on OSX, since the wxEVT_COMBOBOX_DROPDOWN event isn't supported there
#ifdef __WXOSX__
	vector<map_texinfo_t>& textures = MapEditor::textureManager().getAllFlatsInfo();
	for (unsigned a = 0; a < textures.size(); a++)
		list.Add(textures[a].name);
#else
	// Bind events
	Bind(wxEVT_COMBOBOX_DROPDOWN, &FlatComboBox::onDropDown, this);
	Bind(wxEVT_COMBOBOX_CLOSEUP, &FlatComboBox::onCloseUp, this);
#endif
	Bind(wxEVT_KEY_DOWN, &FlatComboBox::onKeyDown, this);

	Set(list);
}


/*******************************************************************
 * FLATCOMBOBOX CLASS EVENTS
 *******************************************************************/

/* FlatComboBox::onDropDown
 * Called when the dropdown list is expanded
 *******************************************************************/
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
		if (textures[a].name.StartsWith(text))
			list.Add(textures[a].name);
	}
	Set(list);	// Why does this clear the text box also?
	SetValue(text);

	e.Skip();
}

/* FlatComboBox::onCloseUp
 * Called when the dropdown list is closed
 *******************************************************************/
void FlatComboBox::onCloseUp(wxCommandEvent& e)
{
	list_down = false;
}

/* FlatComboBox::onKeyDown
 * Called when a key is pressed within the control
 *******************************************************************/
void FlatComboBox::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_DOWN && !list_down)
	{
		list_down = true;
		Popup();
	}
	else
		e.Skip();
}


/*******************************************************************
 * SECTORPROPSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* SectorPropsPanel::SectorPropsPanel
 * SectorPropsPanel class constructor
 *******************************************************************/
SectorPropsPanel::SectorPropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs = new STabCtrl(this, false);
	sizer->Add(stc_tabs, 1, wxEXPAND);

	// General tab
	stc_tabs->AddPage(setupGeneralPanel(), "General");

	// Special tab
	stc_tabs->AddPage(setupSpecialPanel(), "Special");

	// Other Properties tab
	
	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
	{
		mopp_all_props = new MapObjectPropsPanel(stc_tabs, true);
		mopp_all_props->hideProperty("texturefloor");
		mopp_all_props->hideProperty("textureceiling");
		mopp_all_props->hideProperty("heightfloor");
		mopp_all_props->hideProperty("heightceiling");
		mopp_all_props->hideProperty("lightlevel");
		mopp_all_props->hideProperty("id");
		mopp_all_props->hideProperty("special");
		stc_tabs->AddPage(mopp_all_props, "Other Properties");
	}
	else
		mopp_all_props = nullptr;

	// Bind events
	btn_new_tag->Bind(wxEVT_BUTTON, &SectorPropsPanel::onBtnNewTag, this);
	fcb_floor->Bind(wxEVT_TEXT, &SectorPropsPanel::onTextureChanged, this);
	fcb_ceiling->Bind(wxEVT_TEXT, &SectorPropsPanel::onTextureChanged, this);
	gfx_floor->Bind(wxEVT_LEFT_DOWN, &SectorPropsPanel::onTextureClicked, this);
	gfx_ceiling->Bind(wxEVT_LEFT_DOWN, &SectorPropsPanel::onTextureClicked, this);
}

/* SectorPropsPanel::~SectorPropsPanel
 * SectorPropsPanel class destructor
 *******************************************************************/
SectorPropsPanel::~SectorPropsPanel()
{
}

/* SectorPropsPanel::setupGeneralPanel
 * Creates and sets up the general properties panel
 *******************************************************************/
wxPanel* SectorPropsPanel::setupGeneralPanel()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Floor ---
	wxBoxSizer* m_hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_hbox, 0, wxEXPAND|wxALL, 4);
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Floor");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_hbox->Add(framesizer, 1, wxALIGN_CENTER|wxRIGHT, 4);

	// Texture
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);
	gb_sizer->Add(gfx_floor = new FlatTexCanvas(panel), wxGBPosition(0, 0), wxGBSpan(1, 2), wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(fcb_floor = new FlatComboBox(panel), wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Height
	gb_sizer->Add(new wxStaticText(panel, -1, "Height:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_height_floor = new NumberTextCtrl(panel), wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);


	// --- Ceiling ---
	frame = new wxStaticBox(panel, -1, "Ceiling");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_hbox->Add(framesizer, 1, wxALIGN_CENTER);

	// Texture
	gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);
	gb_sizer->Add(gfx_ceiling = new FlatTexCanvas(panel), wxGBPosition(0, 0), wxGBSpan(1, 2), wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(fcb_ceiling = new FlatComboBox(panel), wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Height
	gb_sizer->Add(new wxStaticText(panel, -1, "Height:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_height_ceiling = new NumberTextCtrl(panel), wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);


	// -- General ---
	frame = new wxStaticBox(panel, -1, "General");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);
	gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);

	// Light level
	gb_sizer->Add(new wxStaticText(panel, -1, "Light Level:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_light = new NumberTextCtrl(panel), wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);

	// Tag
	gb_sizer->Add(new wxStaticText(panel, -1, "Tag:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_tag = new NumberTextCtrl(panel), wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(btn_new_tag = new wxButton(panel, -1, "New Tag"), wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);

	return panel;
}

/* SectorPropsPanel::setupSpecialPanel
 * Creates and sets up the special properties panel
 *******************************************************************/
wxPanel* SectorPropsPanel::setupSpecialPanel()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Add special panel
	sizer->Add(panel_special = new SectorSpecialPanel(panel), 1, wxEXPAND|wxALL, 4);

	// Add override checkbox
	sizer->Add(cb_override_special = new wxCheckBox(panel, -1, "Override Special"), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	cb_override_special->SetToolTip("Differing specials detected, tick this to set the special for all selected sectors");

	return panel;
}

/* SectorPropsPanel::openObjects
 * Loads values from [objects]
 *******************************************************************/
void SectorPropsPanel::openObjects(vector<MapObject*>& objects)
{
	if (objects.empty())
		return;

	int ival;
	string sval;

	// Special
	if (MapObject::multiIntProperty(objects, "special", ival))
	{
		panel_special->setup(ival);
		cb_override_special->Show(false);
		cb_override_special->SetValue(true);
	}
	else
		cb_override_special->SetValue(false);

	// Floor texture
	if (MapObject::multiStringProperty(objects, "texturefloor", sval))
	{
		gfx_floor->setTexture(sval);
		fcb_floor->SetValue(sval);
	}

	// Ceiling texture
	if (MapObject::multiStringProperty(objects, "textureceiling", sval))
	{
		gfx_ceiling->setTexture(sval);
		fcb_ceiling->SetValue(sval);
	}

	// Floor height
	if (MapObject::multiIntProperty(objects, "heightfloor", ival))
		text_height_floor->SetValue(S_FMT("%d", ival));
	
	// Ceiling height
	if (MapObject::multiIntProperty(objects, "heightceiling", ival))
		text_height_ceiling->SetValue(S_FMT("%d", ival));

	// Light level
	if (MapObject::multiIntProperty(objects, "lightlevel", ival))
		text_light->SetValue(S_FMT("%d", ival));

	// Tag
	if (MapObject::multiIntProperty(objects, "id", ival))
		text_tag->SetValue(S_FMT("%d", ival));

	// Load other properties
	if (mopp_all_props)
		mopp_all_props->openObjects(objects);

	// Update internal objects list
	this->objects.clear();
	for (unsigned a = 0; a < objects.size(); a++)
		this->objects.push_back(objects[a]);

	// Update layout
	Layout();
	Refresh();
}

/* SectorPropsPanel::applyChanges
 * Apply values to opened objects
 *******************************************************************/
void SectorPropsPanel::applyChanges()
{
	for (unsigned a = 0; a < objects.size(); a++)
	{
		MapSector* sector = (MapSector*)objects[a];

		// Special
		if (cb_override_special->GetValue())
			sector->setIntProperty("special", panel_special->getSelectedSpecial());

		// Floor texture
		if (!fcb_floor->GetValue().IsEmpty())
			sector->setStringProperty("texturefloor", fcb_floor->GetValue());

		// Ceiling texture
		if (!fcb_ceiling->GetValue().IsEmpty())
			sector->setStringProperty("textureceiling", fcb_ceiling->GetValue());

		// Floor height
		if (!text_height_floor->GetValue().IsEmpty())
			sector->setIntProperty("heightfloor", text_height_floor->getNumber(sector->getFloorHeight()));

		// Ceiling height
		if (!text_height_ceiling->GetValue().IsEmpty())
			sector->setIntProperty("heightceiling", text_height_ceiling->getNumber(sector->getCeilingHeight()));

		// Light level
		if (!text_light->GetValue().IsEmpty())
			sector->setIntProperty("lightlevel", text_light->getNumber(sector->getLightLevel()));

		// Tag
		if (!text_tag->GetValue().IsEmpty())
			sector->setIntProperty("id", text_tag->getNumber(sector->getTag()));
	}

	if (mopp_all_props)
		mopp_all_props->applyChanges();
}


/*******************************************************************
 * SECTORPROPSPANEL CLASS EVENTS
 *******************************************************************/

/* SectorPropsPanel::onTextureChanged
 * Called when a texture name is changed
 *******************************************************************/
void SectorPropsPanel::onTextureChanged(wxCommandEvent& e)
{
	// Floor
	if (e.GetEventObject() == fcb_floor)
		gfx_floor->setTexture(fcb_floor->GetValue());

	// Ceiling
	else if (e.GetEventObject() == fcb_ceiling)
		gfx_ceiling->setTexture(fcb_ceiling->GetValue());

	e.Skip();
}

/* SectorPropsPanel::onTextureClicked
 * Called when a texture canvas is clicked
 *******************************************************************/
void SectorPropsPanel::onTextureClicked(wxMouseEvent& e)
{
	// Get canvas
	FlatTexCanvas* tc = nullptr;
	FlatComboBox* cb = nullptr;
	if (e.GetEventObject() == gfx_floor)
	{
		tc = gfx_floor;
		cb = fcb_floor;
	}
	else if (e.GetEventObject() == gfx_ceiling)
	{
		tc = gfx_ceiling;
		cb = fcb_ceiling;
	}

	if (!tc)
	{
		e.Skip();
		return;
	}

	// Browse
	MapTextureBrowser browser(this, 1, tc->getTexName(), &(MapEditor::editContext().map()));
	if (browser.ShowModal() == wxID_OK)
		cb->SetValue(browser.getSelectedItem()->getName());
}

/* SectorPropsPanel::onBtnNewTag
 * Called when the 'New Tag' button is clicked
 *******************************************************************/
void SectorPropsPanel::onBtnNewTag(wxCommandEvent& e)
{
	int tag = MapEditor::editContext().map().findUnusedSectorTag();
	text_tag->SetValue(S_FMT("%d", tag));
}
