
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SIconButton.cpp
// Description: A simple button that displays an icon
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
#include "SIconButton.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SIconButton Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SIconButton class constructor
// -----------------------------------------------------------------------------
SIconButton::SIconButton(
	wxWindow*       parent,
	icons::Type     icon_type,
	const wxString& icon,
	const wxString& tooltip,
	int             icon_size) :
	wxBitmapButton{ parent, -1, wxNullBitmap }
{
	auto bmp = icons::getIcon(icon_type, icon.ToStdString(), icon_size);

	// Set button image and tooltip
	SetBitmap(bmp);
	if (!tooltip.empty())
		SetToolTip(tooltip);
}
