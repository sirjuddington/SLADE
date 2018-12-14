
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "Graphics/Icons.h"
#include "MapEditor/UI/MapCanvas.h"


// -----------------------------------------------------------------------------
//
// SIconButton Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SIconButton class constructor
// -----------------------------------------------------------------------------
SIconButton::SIconButton(wxWindow* parent, Icons::Type icon_type, const string& icon, const string& tooltip) :
	wxBitmapButton{ parent, -1, wxNullBitmap }
{
	// Create icon
	auto bmp = Icons::getIcon(icon_type, icon, UI::scaleFactor() > 1.);

	// Scale icon if required
	auto size = UI::scalePx(16);
	if (bmp.GetWidth() != size)
	{
		auto img = bmp.ConvertToImage();
		img.Rescale(size, size, wxIMAGE_QUALITY_BICUBIC);
		bmp = wxBitmap(img);
	}

	// Set button image and tooltip
	SetBitmap(bmp);
	if (!tooltip.empty())
		SetToolTip(tooltip);
}
