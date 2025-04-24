
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Canvas/GL/GLCanvas.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
// SideTexCanvas Class
//
// A simple opengl canvas to display a texture
// (will have more advanced functionality later)
// -----------------------------------------------------------------------------
class slade::SideTexCanvas : public GLCanvas
{
public:
	SideTexCanvas(wxWindow* parent, string_view title) : GLCanvas(parent, BGStyle::Checkered), title_{ title }
	{
		wxWindow::SetWindowStyleFlag(wxBORDER_SIMPLE);
		SetInitialSize(FromDIP(wxSize(136, 136)));
	}

	~SideTexCanvas() override = default;

	string texName() const { return texname_; }

	// Sets the texture to display
	void setTexture(const string& tex)
	{
		activateContext();

		texname_ = tex;
		if (tex.empty() || tex == "-")
			texture_ = 0;
		else
		{
			auto texture = mapeditor::textureManager().texture(
				tex, game::configuration().featureSupported(game::Feature::MixTexFlats));

			texture_ = texture.gl_id;
		}

		Update();
		Refresh();
	}

	// Draws the canvas content
	void draw() override
	{
		gl::draw2d::Context dc(&view_);

		// Draw texture
		if (texture_ && texture_ != gl::Texture::missingTexture())
		{
			dc.texture = texture_;
			dc.drawTextureWithin({ 0.0f, 0.0f, dc.viewSize().x, dc.viewSize().y }, 0.0f, 100.0f);
		}
		else if (texture_ == gl::Texture::missingTexture())
		{
			// Draw unknown icon
			dc.texture = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
			dc.colour.set(180, 0, 0);
			dc.drawTextureWithin({ 0.0f, 0.0f, dc.viewSize().x, dc.viewSize().y }, 0.0f, 0.25f);
		}

		// Draw title
		if (!title_.empty())
		{
			dc.colour.set(255, 255, 255);
			dc.outline_colour.set(0, 0, 0);
			dc.text_alignment = gl::draw2d::Align::Center;
			dc.text_style     = gl::draw2d::TextStyle::Outline;
			dc.font           = gl::draw2d::Font::Condensed;
			dc.drawText(title_, { dc.viewSize().x * 0.5f, 2.0f });
		}
	}

private:
	unsigned texture_ = 0;
	string   texname_;
	string   title_;
};


// -----------------------------------------------------------------------------
// TextureComboBox Class
//
// A custom combo box that will show a list of textures matching the current
// text in the control (eg. 'BIG' will list all textures beginning with BIG).
// -----------------------------------------------------------------------------
class slade::TextureComboBox : public wxComboBox
{
public:
	TextureComboBox(wxWindow* parent) : wxComboBox(parent, -1)
	{
		// Init
		SetInitialSize({ FromDIP(136), -1 });
		wxArrayString list;
		list.Add(wxS("-"));

		// Bind events
		Bind(wxEVT_COMBOBOX_DROPDOWN, &TextureComboBox::onDropDown, this);
		Bind(wxEVT_COMBOBOX_CLOSEUP, &TextureComboBox::onCloseUp, this);
		Bind(wxEVT_KEY_DOWN, &TextureComboBox::onKeyDown, this);

		Set(list);
	}

	~TextureComboBox() override = default;

private:
	bool list_down_ = false;

	// Called when the dropdown list is expanded
	void onDropDown(wxCommandEvent& e)
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

	// Called when the dropdown list is closed
	void onCloseUp(wxCommandEvent& e) { list_down_ = false; }

	// Called when a key is pressed within the control
	void onKeyDown(wxKeyEvent& e)
	{
		if (e.GetKeyCode() == WXK_DOWN && !list_down_)
		{
			list_down_ = true;
			Popup();
		}
		else
			e.Skip();
	}
};


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
	auto lh = ui::LayoutHelper(this);

	wxBoxSizer* vbox;

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// --- Textures ---
	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gb_sizer, lh.sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Upper
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(gfx_upper_ = new SideTexCanvas(this, "Upper"), 1, wxEXPAND);
	gb_sizer->Add(tcb_upper_ = new TextureComboBox(this), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER);

	// Middle
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 1 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(gfx_middle_ = new SideTexCanvas(this, "Middle"), 1, wxEXPAND);
	gb_sizer->Add(tcb_middle_ = new TextureComboBox(this), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER);

	// Lower
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 2 }, { 1, 1 }, wxALIGN_CENTER);
	vbox->Add(gfx_lower_ = new SideTexCanvas(this, "Lower"), 1, wxEXPAND);
	gb_sizer->Add(tcb_lower_ = new TextureComboBox(this), { 1, 2 }, { 1, 1 }, wxALIGN_CENTER);

	// --- Offsets ---
	text_offsetx_ = new NumberTextCtrl(this);
	text_offsetx_->SetInitialSize(lh.size(64, -1));
	text_offsety_ = new NumberTextCtrl(this);
	text_offsety_->SetInitialSize(lh.size(64, -1));
	gb_sizer->Add(vbox = new wxBoxSizer(wxVERTICAL), { 0, 3 }, { 2, 1 }, wxALIGN_TOP);
	vbox->Add(new wxStaticText(this, -1, wxS("Offset")), lh.sfWithSmallBorder(0, wxBOTTOM));
	vbox->Add(wxutil::createLabelHBox(this, "X", text_offsetx_));
	vbox->AddSpacer(lh.pad());
	vbox->Add(wxutil::createLabelHBox(this, "Y", text_offsety_));

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableCol(2, 1);
	gb_sizer->AddGrowableRow(0, 1);

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
void SidePropsPanel::openSides(const vector<MapSide*>& sides) const
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
void SidePropsPanel::applyTo(const vector<MapSide*>& sides) const
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

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

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
