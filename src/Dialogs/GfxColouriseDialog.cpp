
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxColouriseDialog.cpp
// Description: A simple dialog for the 'Colourise' function, allows the user to
//              select a colour and shows a preview of the colourised image
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
#include "GfxColouriseDialog.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Controls/ColourBox.h"


// -----------------------------------------------------------------------------
//
// GfxColouriseDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxColouriseDialog class constructor
// -----------------------------------------------------------------------------
GfxColouriseDialog::GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, const Palette& pal) :
	wxDialog(parent, -1, "Colourise", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entry_{ entry },
	palette_{ pal }
{
	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::General, "colourise"));
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
	cb_colour_->setColour(ColRGBA::RED);
	cb_colour_->setPalette(&palette_);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(cb_colour_, 0, wxEXPAND);

	// Add preview
	gfx_preview_ = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview_, 1, wxEXPAND | wxALL, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM, 4);

	// Setup preview
	gfx_preview_->setViewType(GfxCanvas::View::Centered);
	gfx_preview_->setPalette(&palette_);
	gfx_preview_->SetInitialSize(wxSize(192, 192));
	Misc::loadImageFromEntry(&gfx_preview_->image(), entry);
	auto col = cb_colour_->colour();
	gfx_preview_->image().colourise(col, &palette_);
	gfx_preview_->updateImageTexture();

	// Init layout
	wxWindowBase::Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxColouriseDialog::onColourChanged, this);
	Bind(wxEVT_SIZE, &GfxColouriseDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::SetMinSize(GetSize());
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Returns the currently selected colour
// -----------------------------------------------------------------------------
ColRGBA GfxColouriseDialog::colour() const
{
	return cb_colour_->colour();
}

// -----------------------------------------------------------------------------
// Sets the colour to use
// -----------------------------------------------------------------------------
void GfxColouriseDialog::setColour(const string& col)
{
	wxColour colour(col);
	ColRGBA  rgba = ColRGBA(COLWX(colour));
	cb_colour_->setColour(rgba);
	gfx_preview_->image().colourise(rgba, &palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}


// -----------------------------------------------------------------------------
//
// GfxColouriseDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the selected colour is changed
// -----------------------------------------------------------------------------
void GfxColouriseDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(&gfx_preview_->image(), entry_);
	gfx_preview_->image().colourise(cb_colour_->colour(), &palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void GfxColouriseDialog::onResize(wxSizeEvent& e)
{
	OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05f);
	e.Skip();
}
