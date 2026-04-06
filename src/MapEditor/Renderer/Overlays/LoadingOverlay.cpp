
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LoadingOverlay.cpp
// Description:
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
#include "LoadingOverlay.h"
#include "Geometry/Rect.h"
#include "OpenGL/Draw2D.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// LoadingOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// LoadingOverlay constructor
// -----------------------------------------------------------------------------
LoadingOverlay::LoadingOverlay() : MCOverlay(true)
{
	active_ = false;
}

// -----------------------------------------------------------------------------
// LoadingOverlay destructor
// -----------------------------------------------------------------------------
LoadingOverlay::~LoadingOverlay() = default;

// -----------------------------------------------------------------------------
// Sets the overlay message to [message], and activates the overlay if
// [activate] is true
// -----------------------------------------------------------------------------
void LoadingOverlay::setMessage(string_view message, bool activate)
{
	message_ = message;
	if (activate)
		active_ = true;
}

// -----------------------------------------------------------------------------
// Draws the overlay
// -----------------------------------------------------------------------------
void LoadingOverlay::draw(gl::draw2d::Context& dc, float fade)
{
	if (fade <= 0.0f || message_.empty())
		return;

	auto view_size = dc.viewSize();

	// Background
	dc.setColourFromConfig("map_overlay_background", fade);
	dc.drawRect({ 0.0f, 0.0f, view_size.x, view_size.y });

	// Message
	dc.font           = gl::draw2d::Font::Bold;
	dc.text_size      = 36;
	dc.text_alignment = gl::draw2d::Align::Center;
	dc.text_style     = gl::draw2d::TextStyle::DropShadow;
	dc.setColourFromConfig("map_overlay_foreground", fade);

	auto line_height = dc.textLineHeight();
	dc.drawText(message_, { view_size.x * 0.5f, view_size.y * 0.5f - line_height * 0.5f });
}
