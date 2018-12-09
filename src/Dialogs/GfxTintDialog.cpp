
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxTintDialog.cpp
// Description: A simple dialog for the 'Tint' function, allows the user to
//              select tint colour + amount and shows a preview of the tinted
//              image
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
#include "GfxTintDialog.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Controls/ColourBox.h"


// -----------------------------------------------------------------------------
//
// GfxTintDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxTintDialog class constructor
// -----------------------------------------------------------------------------
GfxTintDialog::GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, Palette* pal) :
	wxDialog(parent, -1, "Tint", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entry_{ entry },
	palette_{ pal }
{
	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::General, "tint"));
	SetIcon(icon);

	// Setup main sizer
	auto msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND | wxALL, 6);

	// Add colour chooser
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, 4);

	cb_colour_ = new ColourBox(this, -1, false, true);
	cb_colour_->setColour(COL_RED);
	cb_colour_->setPalette(pal);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(cb_colour_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	// Add 'amount' slider
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	slider_amount_ = new wxSlider(this, -1, 50, 0, 100);
	label_amount_  = new wxStaticText(this, -1, "100%");
	hbox->Add(new wxStaticText(this, -1, "Amount:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(slider_amount_, 1, wxEXPAND | wxRIGHT, 4);
	hbox->Add(label_amount_, 0, wxALIGN_CENTER_VERTICAL);

	// Add preview
	gfx_preview_ = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM, 4);

	// Setup preview
	gfx_preview_->setViewType(GfxCanvas::View::Centered);
	gfx_preview_->setPalette(pal);
	gfx_preview_->SetInitialSize(wxSize(256, 256));
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry);
	gfx_preview_->getImage()->tint(colour(), amount(), pal);
	gfx_preview_->updateImageTexture();

	// Init layout
	wxWindowBase::Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxTintDialog::onColourChanged, this);
	slider_amount_->Bind(wxEVT_SLIDER, &GfxTintDialog::onAmountChanged, this);
	Bind(wxEVT_SIZE, &GfxTintDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::SetMinSize(GetSize());
	CenterOnParent();

	// Set values
	label_amount_->SetLabel("50% ");
}

// -----------------------------------------------------------------------------
// Returns the currently selected colour
// -----------------------------------------------------------------------------
ColRGBA GfxTintDialog::colour() const
{
	return cb_colour_->colour();
}

// -----------------------------------------------------------------------------
// Returns the currently selected tint amount
// -----------------------------------------------------------------------------
float GfxTintDialog::amount() const
{
	return (float)slider_amount_->GetValue() * 0.01f;
}

// -----------------------------------------------------------------------------
// Sets the colour and tint amount to use
// -----------------------------------------------------------------------------
void GfxTintDialog::setValues(const string& col, int val)
{
	wxColour wxcol(col);
	cb_colour_->setColour(ColRGBA(COLWX(wxcol)));
	slider_amount_->SetValue(val);
	label_amount_->SetLabel(S_FMT("%d%% ", slider_amount_->GetValue()));
	gfx_preview_->getImage()->tint(colour(), amount(), &palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}


// -----------------------------------------------------------------------------
//
// GfxTintDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the selected colour is changed
// -----------------------------------------------------------------------------
void GfxTintDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry_);
	gfx_preview_->getImage()->tint(colour(), amount(), &palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the tint amount is changed
// -----------------------------------------------------------------------------
void GfxTintDialog::onAmountChanged(wxCommandEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry_);
	gfx_preview_->getImage()->tint(colour(), amount(), &palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
	label_amount_->SetLabel(S_FMT("%d%% ", slider_amount_->GetValue()));
}

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void GfxTintDialog::onResize(wxSizeEvent& e)
{
	OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05f);
	e.Skip();
}
