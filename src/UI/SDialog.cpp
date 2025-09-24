
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SDialog.cpp
// Description: Simple base class for dialogs that handles saved size and
//              position info
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
#include "SDialog.h"
#include "UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SDialog class constructor
// -----------------------------------------------------------------------------
SDialog::SDialog(wxWindow* parent, const string& title, const string& id, int width, int height, int x, int y) :
	wxDialog(
		parent,
		-1,
		wxString::FromUTF8(title),
		wxPoint(x, y),
		wxSize(width, height),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	id_{ id }
{
	// Init size/pos
	auto info = ui::getWindowInfo(this, id_);
	if (!info.id.empty())
	{
		SetClientSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		ui::setWindowInfo(this, id_, width, height, x, y);

	// Bind events
	if (!id.empty())
	{
		// Bind(wxEVT_SIZE, &SDialog::onSize, this);
		// Bind(wxEVT_MOVE, &SDialog::onMove, this);
	}
	Bind(wxEVT_SHOW, &SDialog::onShow, this);
}

// -----------------------------------------------------------------------------
// SDialog class destructor
// -----------------------------------------------------------------------------
SDialog::~SDialog()
{
	if (!id_.empty())
		ui::setWindowInfo(this, id_, GetClientSize().x, GetClientSize().y, GetPosition().x, GetPosition().y);
}

// -----------------------------------------------------------------------------
// Resizes the dialog to its saved size
// -----------------------------------------------------------------------------
void SDialog::setSavedSize(int def_width, int def_height)
{
	auto info = ui::getWindowInfo(this, id_);
	if (!info.id.empty())
	{
		SetInitialSize(wxSize(info.width, info.height));
		SetClientSize(info.width, info.height);
	}
	else
	{
		SetInitialSize(wxSize(def_width, def_height));
		SetClientSize(def_width, def_height);
	}
}


// -----------------------------------------------------------------------------
//
// SDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void SDialog::onSize(wxSizeEvent& e)
{
	// Update window size settings
	const wxSize client_size = GetClientSize();
	ui::setWindowInfo(this, id_, client_size.x, client_size.y, -2, -2);
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the dialog is moved
// -----------------------------------------------------------------------------
void SDialog::onMove(wxMoveEvent& e)
{
	// Update window position settings
	ui::setWindowInfo(this, id_, -2, -2, GetPosition().x, GetPosition().y);
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the dialog is shown
// -----------------------------------------------------------------------------
void SDialog::onShow(wxShowEvent& e)
{
	if (e.IsShown())
		CenterOnParent();
	e.Skip();
}
