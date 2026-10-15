
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
	wxWindow*     parent,
	icons::Type   icon_type,
	const string& icon,
	const string& tooltip,
	int           icon_size,
	int           button_size) :
	wxBitmapButton{ parent, -1, wxNullBitmap, wxDefaultPosition, wxSize(button_size, button_size) }
{
	auto bmp = icons::getIcon(icon_type, icon, icon_size);

	// Set button image and tooltip
	SetBitmap(bmp);
	if (!tooltip.empty())
		SetToolTip(wxString::FromUTF8(tooltip));

	if (button_size > 0)
		SetMinSize(wxSize(button_size, button_size));
	else
	{
		auto default_height = GetDefaultSize().y;
		SetMinSize(wxSize(default_height, default_height));
	}
}

// -----------------------------------------------------------------------------
// Override of DoSetSize to Enforce square sizing
// -----------------------------------------------------------------------------
void SIconButton::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
	if (width != -1 || height != -1)
	{
		if (width == -1)
			width = height;
		else if (height == -1)
			height = width;
		else
		{
			auto full_size = std::max(width, height);
			auto cur_size  = std::min(width, height);

			// Ask parent sizer for a wider/taller slot on the next layout pass
			if (full_size > cur_size)
			{
				auto min_size = GetMinSize();
				if (min_size.x < full_size || min_size.y < full_size)
				{
					SetMinSize(wxSize(full_size, full_size));
					if (auto parent = GetParent(); parent && parent->GetSizer())
						parent->CallAfter([parent]() { parent->Layout(); });
				}
			}

			// Keep this pass inside the allocated slot so we don't overlap
			// neighbouring controls
			width = height = cur_size;
		}
	}

	wxBitmapButton::DoSetSize(x, y, width, height, sizeFlags);
}
