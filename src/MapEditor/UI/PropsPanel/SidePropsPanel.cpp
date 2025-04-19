
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "SidePropsPanel.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
// SideTexCanvas Class Functions
//
// A simple opengl canvas to display a texture
// (will have more advanced functionality later)
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SideTexCanvas class constructor
// -----------------------------------------------------------------------------
SideTexCanvas::SideTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	wxWindow::SetWindowStyleFlag(wxBORDER_SIMPLE);
	SetInitialSize(wxutil::scaledSize(136, 136));
}

// -----------------------------------------------------------------------------
// Sets the texture to display
// -----------------------------------------------------------------------------
void SideTexCanvas::setTexture(const string& tex)
{
	texname_ = tex;
	if (tex.empty() || tex == "-")
		texture_ = 0;
	else
		texture_ = mapeditor::textureManager()
					   .texture(tex, game::configuration().featureSupported(game::Feature::MixTexFlats))
					   .gl_id;

	Refresh();
}

// -----------------------------------------------------------------------------
// Draws the canvas content
// -----------------------------------------------------------------------------
void SideTexCanvas::draw()
{
	// Setup the viewport
	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, size.x, size.y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (gl::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Draw texture
	if (texture_ && texture_ != gl::Texture::missingTexture())
	{
		glEnable(GL_TEXTURE_2D);
		drawing::drawTextureWithin(texture_, 0, 0, size.x, size.y, 0);
	}
	else if (texture_ == gl::Texture::missingTexture())
	{
		// Draw unknown icon
		auto tex = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
		glEnable(GL_TEXTURE_2D);
		gl::setColour(180, 0, 0);
		drawing::drawTextureWithin(tex, 0, 0, size.x, size.y, 0, 0.25);
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// -----------------------------------------------------------------------------
// TextureComboBox Class Functions
//
// A custom combo box that will show a list of textures matching the current
// text in the control (eg. 'BIG' will list all textures beginning with BIG).
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureComboBox class constructor
// -----------------------------------------------------------------------------
TextureComboBox::TextureComboBox(wxWindow* parent) : wxComboBox(parent, -1)
{
	// Init
	SetInitialSize(wxutil::scaledSize(136, -1));
	wxArrayString list;
	list.Add(wxS("-"));

	// Bind events
	Bind(wxEVT_COMBOBOX_DROPDOWN, &TextureComboBox::onDropDown, this);
	Bind(wxEVT_COMBOBOX_CLOSEUP, &TextureComboBox::onCloseUp, this);
	Bind(wxEVT_KEY_DOWN, &TextureComboBox::onKeyDown, this);

	Set(list);
}


// -----------------------------------------------------------------------------
//
// TextureComboBox Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the dropdown list is expanded
// -----------------------------------------------------------------------------
void TextureComboBox::onDropDown(wxCommandEvent& e)
{
	// Get current value
	auto text = GetValue().Upper().utf8_string();
	if (text == "-")
		text.clear();

	// Populate dropdown with matching texture names
	auto&         textures = mapeditor::textureManager().allTexturesInfo();
	wxArrayString list;
	list.Add(wxS("-"));
	for (auto& texture : textures)
	{
		if (strutil::startsWith(texture.short_name, text))
		{
			list.Add(wxString::FromUTF8(texture.short_name));
		}
		if (game::configuration().featureSupported(game::Feature::LongNames))
		{
			if (strutil::startsWith(texture.long_name, text))
			{
				list.Add(wxString::FromUTF8(texture.long_name));
			}
		}
	}
	Set(list); // Why does this clear the text box also?
	SetValue(wxString::FromUTF8(text));

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the dropdown list is closed
// -----------------------------------------------------------------------------
void TextureComboBox::onCloseUp(wxCommandEvent& e)
{
	list_down_ = false;
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the control
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
//
// SidePropsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SidePropsPanel class constructor
// -----------------------------------------------------------------------------
SidePropsPanel::SidePropsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxBoxSizer* vbox;

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// --- Textures ---
	auto sizer_tex = new wxStaticBoxSizer(wxVERTICAL, this, wxS("Textures"));
	sizer->Add(sizer_tex, 0, wxEXPAND);

	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	sizer_tex->Add(gb_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Upper
	auto pad_min = ui::px(ui::Size::PadMinimum);
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, wxS("Upper:")), 0, wxALIGN_CENTER | wxBOTTOM, pad_min);
	vbox->Add(gfx_upper_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_upper_ = new TextureComboBox(this), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER);

	// Middle
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 1 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, wxS("Middle:")), 0, wxALIGN_CENTER | wxBOTTOM, pad_min);
	vbox->Add(gfx_middle_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_middle_ = new TextureComboBox(this), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER);

	// Lower
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 2 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(new wxStaticText(this, -1, wxS("Lower:")), 0, wxALIGN_CENTER | wxBOTTOM, pad_min);
	vbox->Add(gfx_lower_ = new SideTexCanvas(this), 1, wxEXPAND);
	gb_sizer->Add(tcb_lower_ = new TextureComboBox(this), { 1, 2 }, { 1, 1 }, wxALIGN_CENTER);

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableCol(2, 1);
	gb_sizer->AddGrowableRow(0, 1);


	// --- Offsets ---
	auto layout_offsets = wxutil::layoutVertically(
		vector<wxObject*>{ wxutil::createLabelHBox(this, "X Offset:", text_offsetx_ = new NumberTextCtrl(this)),
						   wxutil::createLabelHBox(this, "Y Offset:", text_offsety_ = new NumberTextCtrl(this)) });
	sizer->Add(layout_offsets, 0, wxEXPAND | wxTOP, ui::pad());

	// Bind events
	tcb_upper_->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_middle_->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	tcb_lower_->Bind(wxEVT_TEXT, &SidePropsPanel::onTextureChanged, this);
	gfx_upper_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_middle_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
	gfx_lower_->Bind(wxEVT_LEFT_DOWN, &SidePropsPanel::onTextureClicked, this);
#ifdef __WXOSX__
	tcb_upper_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_middle_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
	tcb_lower_->Bind(wxEVT_COMBOBOX, &SidePropsPanel::onTextureChanged, this);
#endif
}

// -----------------------------------------------------------------------------
// Loads textures and offsets from [sides]
// -----------------------------------------------------------------------------
void SidePropsPanel::openSides(vector<MapSide*>& sides) const
{
	if (sides.empty())
		return;

	// --- Textures ---

	// Upper
	auto tex_upper = sides[0]->texUpper();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->texUpper() != tex_upper)
		{
			tex_upper = "";
			break;
		}
	}
	gfx_upper_->setTexture(tex_upper);
	tcb_upper_->SetValue(wxString::FromUTF8(tex_upper));

	// Middle
	auto tex_middle = sides[0]->texMiddle();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->texMiddle() != tex_middle)
		{
			tex_middle = "";
			break;
		}
	}
	gfx_middle_->setTexture(tex_middle);
	tcb_middle_->SetValue(wxString::FromUTF8(tex_middle));

	// Lower
	auto tex_lower = sides[0]->texLower();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->texLower() != tex_lower)
		{
			tex_lower = "";
			break;
		}
	}
	gfx_lower_->setTexture(tex_lower);
	tcb_lower_->SetValue(wxString::FromUTF8(tex_lower));


	// --- Offsets ---

	// X
	bool multi = false;
	int  ofs   = sides[0]->texOffsetX();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->texOffsetX() != ofs)
		{
			multi = true;
			break;
		}
	}
	if (!multi)
		text_offsetx_->SetValue(WX_FMT("{}", ofs));

	// Y
	multi = false;
	ofs   = sides[0]->texOffsetY();
	for (unsigned a = 1; a < sides.size(); a++)
	{
		if (sides[a]->texOffsetY() != ofs)
		{
			multi = true;
			break;
		}
	}
	if (!multi)
		text_offsety_->SetValue(WX_FMT("{}", ofs));
}

// -----------------------------------------------------------------------------
// Applies current values to [sides]
// -----------------------------------------------------------------------------
void SidePropsPanel::applyTo(vector<MapSide*>& sides) const
{
	// Get values
	auto tex_upper  = tcb_upper_->GetValue().utf8_string();
	auto tex_middle = tcb_middle_->GetValue().utf8_string();
	auto tex_lower  = tcb_lower_->GetValue().utf8_string();

	for (auto& side : sides)
	{
		// Upper Texture
		if (!tex_upper.empty())
			side->setTexUpper(tex_upper);

		// Middle Texture
		if (!tex_middle.empty())
			side->setTexMiddle(tex_middle);

		// Lower Texture
		if (!tex_lower.empty())
			side->setTexLower(tex_lower);

		// X Offset
		if (!text_offsetx_->GetValue().IsEmpty())
			side->setTexOffsetX(text_offsetx_->number(side->texOffsetX()));

		// Y Offset
		if (!text_offsety_->GetValue().IsEmpty())
			side->setTexOffsetY(text_offsety_->number(side->texOffsetY()));
	}
}


// -----------------------------------------------------------------------------
//
// SidePropsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a texture name is changed
// -----------------------------------------------------------------------------
void SidePropsPanel::onTextureChanged(wxCommandEvent& e)
{
	// Upper
	if (e.GetEventObject() == tcb_upper_)
		gfx_upper_->setTexture(tcb_upper_->GetValue().utf8_string());

	// Middle
	else if (e.GetEventObject() == tcb_middle_)
		gfx_middle_->setTexture(tcb_middle_->GetValue().utf8_string());

	// Lower
	else if (e.GetEventObject() == tcb_lower_)
		gfx_lower_->setTexture(tcb_lower_->GetValue().utf8_string());

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a texture canvas is clicked
// -----------------------------------------------------------------------------
void SidePropsPanel::onTextureClicked(wxMouseEvent& e)
{
	// Get canvas
	SideTexCanvas*   stc = nullptr;
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
	MapTextureBrowser browser(this, mapeditor::TextureType::Texture, stc->texName(), &(mapeditor::editContext().map()));
	if (browser.ShowModal() == wxID_OK && browser.selectedItem())
		tcb->SetValue(wxString::FromUTF8(browser.selectedItem()->name()));
}
