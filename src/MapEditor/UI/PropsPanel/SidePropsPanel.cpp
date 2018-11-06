
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SidePropsPanel.cpp
// Description: UI for editing side properties (textures and offsets)
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
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "SidePropsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
// SideTexCanvas Class Functions
//
// A simple opengl canvas to display a texture
// (will have more advanced functionality later)
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SideTexCanvas::SideTexCanvas
//
// SideTexCanvas class constructor
// ----------------------------------------------------------------------------
SideTexCanvas::SideTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	wxWindow::SetWindowStyleFlag(wxBORDER_SIMPLE);
	SetInitialSize(WxUtils::scaledSize(136, 136));
}

// ----------------------------------------------------------------------------
// SideTexCanvas::setTexture
//
// Sets the texture to display
// ----------------------------------------------------------------------------
void SideTexCanvas::setTexture(string tex)
{
	texname_ = tex;
	if (tex == "-" || tex == "")
		texture_ = nullptr;
	else
		texture_ = MapEditor::textureManager().getTexture(
			tex,
			Game::configuration().featureSupported(Game::Feature::MixTexFlats)
		);

	Refresh();
}

// ----------------------------------------------------------------------------
// SideTexCanvas::draw
//
// Draws the canvas content
// ----------------------------------------------------------------------------
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
	if (texture_ && texture_ != &(GLTexture::missingTex()))
	{
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture_, 0, 0, GetSize().x, GetSize().y, 0);
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
// TextureComboBox Class Functions
//
// A custom combo box that will show a list of textures matching the current
// text in the control (eg. 'BIG' will list all textures beginning with BIG).
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextureComboBox::TextureComboBox
//
// TextureComboBox class constructor
// ----------------------------------------------------------------------------
TextureComboBox::TextureComboBox(wxWindow* parent) : wxComboBox(parent, -1)
{
	// Init
	SetInitialSize(WxUtils::scaledSize(136, -1));
	wxArrayString list;
	list.Add("-");

	// Bind events
	Bind(wxEVT_COMBOBOX_DROPDOWN, &TextureComboBox::onDropDown, this);
	Bind(wxEVT_COMBOBOX_CLOSEUP, &TextureComboBox::onCloseUp, this);
	Bind(wxEVT_KEY_DOWN, &TextureComboBox::onKeyDown, this);

	Set(list);
}


// ----------------------------------------------------------------------------
//
// TextureComboBox Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextureComboBox::onDropDown
//
// Called when the dropdown list is expanded
// ----------------------------------------------------------------------------
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
// TextureComboBox::onCloseUp
//
// Called when the dropdown list is closed
// ----------------------------------------------------------------------------
void TextureComboBox::onCloseUp(wxCommandEvent& e)
{
	list_down_ = false;
}

// ----------------------------------------------------------------------------
// TextureComboBox::onKeyDown
//
// Called when a key is pressed within the control
// ----------------------------------------------------------------------------
void TextureComboBox::onKeyDown(wxKeyEvent& e)
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
// SidePropsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SidePropsPanel::SidePropsPanel
//
// SidePropsPanel class constructor
// ----------------------------------------------------------------------------
SidePropsPanel::SidePropsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxBoxSizer* vbox;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// --- Textures ---
	wxStaticBoxSizer* sizer_tex = new wxStaticBoxSizer(wxVERTICAL, this, "Textures");
	sizer->Add(sizer_tex, 0, wxEXPAND);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	sizer_tex->Add(gb_sizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Upper
	auto pad_min = UI::px(UI::Size::PadMinimum);
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Upper:"), 0, wxALIGN_CENTER|wxBOTTOM, pad_min);
	vbox->Add(gfx_upper_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_upper_ = new TextureComboBox(this), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER);

	// Middle
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 1 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Middle:"), 0, wxALIGN_CENTER|wxBOTTOM, pad_min);
	vbox->Add(gfx_middle_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_middle_ = new TextureComboBox(this), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER);

	// Lower
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 2 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, "Lower:"), 0, wxALIGN_CENTER|wxBOTTOM, pad_min);
	vbox->Add(gfx_lower_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_lower_ = new TextureComboBox(this), { 1, 2 }, { 1, 1 }, wxALIGN_CENTER);

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableCol(2, 1);
	gb_sizer->AddGrowableRow(0, 1);


	// --- Offsets ---
	auto layout_offsets = WxUtils::layoutVertically(
		vector<wxObject*>{
			WxUtils::createLabelHBox(this, "X Offset:", text_offsetx_ = new NumberTextCtrl(this)),
			WxUtils::createLabelHBox(this, "Y Offset:", text_offsety_ = new NumberTextCtrl(this))
		}
	);
	sizer->Add(layout_offsets, 0, wxEXPAND | wxTOP, UI::pad());

	// Bind events
	tcb_upper_->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_middle_->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_lower_ ->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	gfx_upper_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_middle_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_lower_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
#ifdef __WXOSX__
	tcb_upper_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_middle_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_lower_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
#endif
}

// ----------------------------------------------------------------------------
// SidePropsPanel::openSides
//
// Loads textures and offsets from [sides]
// ----------------------------------------------------------------------------
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
	gfx_upper_->setTexture(tex_upper);
	tcb_upper_->SetValue(tex_upper);

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
	gfx_middle_->setTexture(tex_middle);
	tcb_middle_->SetValue(tex_middle);

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
	gfx_lower_->setTexture(tex_lower);
	tcb_lower_->SetValue(tex_lower);


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
		text_offsetx_->SetValue(S_FMT("%d", ofs));

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
		text_offsety_->SetValue(S_FMT("%d", ofs));
}

// ----------------------------------------------------------------------------
// SidePropsPanel::applyTo
//
// Applies current values to [sides]
// ----------------------------------------------------------------------------
void SidePropsPanel::applyTo(vector<MapSide*>& sides)
{
	// Get values
	string tex_upper = tcb_upper_->GetValue();
	string tex_middle = tcb_middle_->GetValue();
	string tex_lower = tcb_lower_->GetValue();

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
		if (!text_offsetx_->GetValue().IsEmpty())
			sides[a]->setIntProperty("offsetx", text_offsetx_->getNumber(sides[a]->getOffsetX()));

		// Y Offset
		if (!text_offsety_->GetValue().IsEmpty())
			sides[a]->setIntProperty("offsety", text_offsety_->getNumber(sides[a]->getOffsetY()));
	}
}


// ----------------------------------------------------------------------------
//
// SidePropsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SidePropsPanel::onTextureChanged
//
// Called when a texture name is changed
// ----------------------------------------------------------------------------
void SidePropsPanel::onTextureChanged(wxCommandEvent& e)
{
	// Upper
	if (e.GetEventObject() == tcb_upper_)
		gfx_upper_->setTexture(tcb_upper_->GetValue());

	// Middle
	else if (e.GetEventObject() == tcb_middle_)
		gfx_middle_->setTexture(tcb_middle_->GetValue());

	// Lower
	else if (e.GetEventObject() == tcb_lower_)
		gfx_lower_->setTexture(tcb_lower_->GetValue());

	e.Skip();
}

// ----------------------------------------------------------------------------
// SidePropsPanel::onTextureClicked
//
// Called when a texture canvas is clicked
// ----------------------------------------------------------------------------
void SidePropsPanel::onTextureClicked(wxMouseEvent& e)
{
	// Get canvas
	SideTexCanvas* stc = nullptr;
	TextureComboBox* tcb = nullptr;
	if (e.GetEventObject() == gfx_upper_)
	{
		stc = gfx_upper_;
		tcb = tcb_upper_;
	}
	else if (e.GetEventObject() == gfx_middle_)
	{
		stc = gfx_middle_;
		tcb = tcb_middle_;
	}
	else if (e.GetEventObject() == gfx_lower_)
	{
		stc = gfx_lower_;
		tcb = tcb_lower_;
	}

	if (!stc)
	{
		e.Skip();
		return;
	}

	// Browse
	MapTextureBrowser browser(this, 0, stc->texName(), &(MapEditor::editContext().map()));
	if (browser.ShowModal() == wxID_OK && browser.getSelectedItem())
		tcb->SetValue(browser.getSelectedItem()->name());
}
