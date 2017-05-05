
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LinePropsPanel.cpp
 * Description: UI for editing side properties (textures and offsets)
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
#include "SidePropsPanel.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "UI/NumberTextCtrl.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/MapEditContext.h"


/*******************************************************************
 * SIDETEXCANVAS CLASS FUNCTIONS
 *******************************************************************
 * A simple opengl canvas to display a texture (will have more
 * advanced functionality later)
 */

/* SideTexCanvas::SideTexCanvas
 * SideTexCanvas class constructor
 *******************************************************************/
SideTexCanvas::SideTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	// Init variables
	texture = nullptr;
	SetWindowStyleFlag(wxBORDER_SIMPLE);

	SetInitialSize(wxSize(136, 136));
}

/* SideTexCanvas::~SideTexCanvas
 * SideTexCanvas class destructor
 *******************************************************************/
SideTexCanvas::~SideTexCanvas()
{
}

/* SideTexCanvas::getTexName
 * Returns the name of the loaded texture
 *******************************************************************/
string SideTexCanvas::getTexName()
{
	return texname;
}

/* SideTexCanvas::setTexture
 * Sets the texture to display
 *******************************************************************/
void SideTexCanvas::setTexture(string tex)
{
	texname = tex;
	if (tex == "-" || tex == "")
		texture = nullptr;
	else
		texture = MapEditor::textureManager().getTexture(tex, theGameConfiguration->mixTexFlats());

	Refresh();
}

/* SideTexCanvas::draw
 * Draws the canvas content
 *******************************************************************/
void SideTexCanvas::draw()
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
		Drawing::drawTextureWithin(texture, 0, 0, GetSize().x, GetSize().y, 0);
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
 * TEXTURECOMBOBOX CLASS FUNCTIONS
 *******************************************************************
 * A custom combo box that will show a list of textures matching the
 * current text in the control (eg. 'BIG' will list all textures
 * beginning with BIG). On OSX it will simply have a dropdown list of
 * all textures due to wxWidgets limitations on that platform
 */

/* TextureComboBox::TextureComboBox
 * TextureComboBox class constructor
 *******************************************************************/
TextureComboBox::TextureComboBox(wxWindow* parent) : wxComboBox(parent, -1)
{
	// Init
	list_down = false;
	SetInitialSize(wxSize(136, -1));
	wxArrayString list;
	list.Add("-");

	// Add all textures to dropdown on OSX, since the wxEVT_COMBOBOX_DROPDOWN event isn't supported there
	// (it is in wx 3.0.2, with cocoa at least)
//#ifdef __WXOSX__
//	vector<map_texinfo_t>& textures = MapEditor::textureManager().getAllTexturesInfo();
//	for (unsigned a = 0; a < textures.size(); a++)
//		list.Add(textures[a].name);
//#else
	// Bind events
	Bind(wxEVT_COMBOBOX_DROPDOWN, &TextureComboBox::onDropDown, this);
	Bind(wxEVT_COMBOBOX_CLOSEUP, &TextureComboBox::onCloseUp, this);
//#endif
	Bind(wxEVT_KEY_DOWN, &TextureComboBox::onKeyDown, this);

	Set(list);
}


/*******************************************************************
 * TEXTURECOMBOBOX CLASS EVENTS
 *******************************************************************/

/* TextureComboBox::onDropDown
 * Called when the dropdown list is expanded
 *******************************************************************/
void TextureComboBox::onDropDown(wxCommandEvent& e)
{
	// Get current value
	string text = GetValue().Upper();
	if (text == "-")
		text = "";
	
	// Populate dropdown with matching texture names
	vector<map_texinfo_t>& textures = MapEditor::textureManager().getAllTexturesInfo();
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

/* TextureComboBox::onCloseUp
 * Called when the dropdown list is closed
 *******************************************************************/
void TextureComboBox::onCloseUp(wxCommandEvent& e)
{
	list_down = false;
}

/* TextureComboBox::onKeyDown
 * Called when a key is pressed within the control
 *******************************************************************/
void TextureComboBox::onKeyDown(wxKeyEvent& e)
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
 * SIDEPROPSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* SidePropsPanel::SidePropsPanel
 * SidePropsPanel class constructor
 *******************************************************************/
SidePropsPanel::SidePropsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxBoxSizer* vbox;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// --- Textures ---
	wxStaticBox* frame_tex = new wxStaticBox(this, -1, "Textures");
	wxStaticBoxSizer* sizer_tex = new wxStaticBoxSizer(frame_tex, wxVERTICAL);
	sizer->Add(sizer_tex, 0, wxEXPAND|wxALL, 4);
	
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	sizer_tex->Add(gb_sizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Upper
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Upper:"), 0, wxALIGN_CENTER|wxBOTTOM, 2);
	vbox->Add(gfx_upper = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_upper = new TextureComboBox(this), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER);

	// Middle
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Middle:"), 0, wxALIGN_CENTER|wxBOTTOM, 2);
	vbox->Add(gfx_middle = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_middle = new TextureComboBox(this), wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER);

	// Lower
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Lower:"), 0, wxALIGN_CENTER|wxBOTTOM, 2);
	vbox->Add(gfx_lower = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_lower = new TextureComboBox(this), wxGBPosition(1, 2), wxDefaultSpan, wxALIGN_CENTER);

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableCol(2, 1);
	gb_sizer->AddGrowableRow(0, 1);


	// --- Offsets ---
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	
	// X
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "X Offset:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(text_offsetx = new NumberTextCtrl(this), 1, wxEXPAND|wxRIGHT, 4);

	// Y
	sizer->Add(hbox = new wxBoxSizer(wxHORIZONTAL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "Y Offset:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(text_offsety = new NumberTextCtrl(this), 1, wxEXPAND|wxRIGHT, 4);


	// Bind events
	tcb_upper->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_middle->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_lower ->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	gfx_upper->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_middle->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_lower->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
#ifdef __WXOSX__
	tcb_upper->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_middle->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_lower ->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
#endif
}

/* SidePropsPanel::~SidePropsPanel
 * SidePropsPanel class destructor
 *******************************************************************/
SidePropsPanel::~SidePropsPanel()
{
}

/* SidePropsPanel::openSides
 * Loads textures and offsets from [sides]
 *******************************************************************/
void SidePropsPanel::openSides(vector<MapSide*>& sides)
{
	if (sides.empty())
		return;

	// --- Textures ---

	// Upper
	string tex_upper = sides[0]->getTexUpper();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->getTexUpper() != tex_upper)
		{
			tex_upper = "";
			break;
		}
	}
	gfx_upper->setTexture(tex_upper);
	tcb_upper->SetValue(tex_upper);

	// Middle
	string tex_middle = sides[0]->getTexMiddle();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->getTexMiddle() != tex_middle)
		{
			tex_middle = "";
			break;
		}
	}
	gfx_middle->setTexture(tex_middle);
	tcb_middle->SetValue(tex_middle);

	// Lower
	string tex_lower = sides[0]->getTexLower();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->getTexLower() != tex_lower)
		{
			tex_lower = "";
			break;
		}
	}
	gfx_lower->setTexture(tex_lower);
	tcb_lower->SetValue(tex_lower);


	// --- Offsets ---
	
	// X
	bool multi = false;
	int ofs = sides[0]->getOffsetX();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->getOffsetX() != ofs)
		{
			multi = true;
			break;
		}
	}
	if (!multi)
		text_offsetx->SetValue(S_FMT("%d", ofs));

	// Y
	multi = false;
	ofs = sides[0]->getOffsetY();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->getOffsetY() != ofs)
		{
			multi = true;
			break;
		}
	}
	if (!multi)
		text_offsety->SetValue(S_FMT("%d", ofs));
}

/* SidePropsPanel::applyTo
 * Applies current values to [sides]
 *******************************************************************/
void SidePropsPanel::applyTo(vector<MapSide*>& sides)
{
	// Get values
	string tex_upper = tcb_upper->GetValue();
	string tex_middle = tcb_middle->GetValue();
	string tex_lower = tcb_lower->GetValue();

	for (unsigned a = 0; a < sides.size(); a++)
	{
		// Upper Texture
		if (!tex_upper.IsEmpty())
			sides[a]->setStringProperty("texturetop", tex_upper);

		// Middle Texture
		if (!tex_middle.IsEmpty())
			sides[a]->setStringProperty("texturemiddle", tex_middle);

		// Lower Texture
		if (!tex_lower.IsEmpty())
			sides[a]->setStringProperty("texturebottom", tex_lower);

		// X Offset
		if (!text_offsetx->GetValue().IsEmpty())
			sides[a]->setIntProperty("offsetx", text_offsetx->getNumber(sides[a]->getOffsetX()));

		// Y Offset
		if (!text_offsety->GetValue().IsEmpty())
			sides[a]->setIntProperty("offsety", text_offsety->getNumber(sides[a]->getOffsetY()));
	}
}


/*******************************************************************
 * SIDEPROPSPANEL CLASS EVENTS
 *******************************************************************/

/* SidePropsPanel::onTextureChanged
 * Called when a texture name is changed
 *******************************************************************/
void SidePropsPanel::onTextureChanged(wxCommandEvent& e)
{
	// Upper
	if (e.GetEventObject() == tcb_upper)
		gfx_upper->setTexture(tcb_upper->GetValue());

	// Middle
	else if (e.GetEventObject() == tcb_middle)
		gfx_middle->setTexture(tcb_middle->GetValue());

	// Lower
	else if (e.GetEventObject() == tcb_lower)
		gfx_lower->setTexture(tcb_lower->GetValue());

	e.Skip();
}

/* SidePropsPanel::onTextureClicked
 * Called when a texture canvas is clicked
 *******************************************************************/
void SidePropsPanel::onTextureClicked(wxMouseEvent& e)
{
	// Get canvas
	SideTexCanvas* stc = nullptr;
	TextureComboBox* tcb = nullptr;
	if (e.GetEventObject() == gfx_upper)
	{
		stc = gfx_upper;
		tcb = tcb_upper;
	}
	else if (e.GetEventObject() == gfx_middle)
	{
		stc = gfx_middle;
		tcb = tcb_middle;
	}
	else if (e.GetEventObject() == gfx_lower)
	{
		stc = gfx_lower;
		tcb = tcb_lower;
	}

	if (!stc)
	{
		e.Skip();
		return;
	}

	// Browse
	MapTextureBrowser browser(this, 0, stc->getTexName(), &(MapEditor::editContext().map()));
	if (browser.ShowModal() == wxID_OK)
		tcb->SetValue(browser.getSelectedItem()->getName());
}
