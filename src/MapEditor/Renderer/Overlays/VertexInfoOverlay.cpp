
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    VertexInfoOverlay.cpp
// Description: VertexInfoOverlay class - a map editor overlay that displays
//              information about the currently highlighted vertex in vertices
//              mode
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
#include "VertexInfoOverlay.h"
#include "General/ColourConfiguration.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "Utility/MathStuff.h"


// -----------------------------------------------------------------------------
//
// VertexInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Updates the overlay with info from [vertex]
// -----------------------------------------------------------------------------
void VertexInfoOverlay::update(MapVertex* vertex)
{
	if (!vertex)
		return;

	// Update info string
	// TODO: pos_frac_ is never set, enable for UDMF?
	if (pos_frac_)
		info_ = wxString::Format("Vertex %d: (%1.4f, %1.4f)", vertex->index(), vertex->xPos(), vertex->yPos());
	else
		info_ = wxString::Format("Vertex %d: (%d, %d)", vertex->index(), (int)vertex->xPos(), (int)vertex->yPos());

	if (Global::debug)
		info_ += wxString::Format(" (%d)", vertex->objId());
}

// -----------------------------------------------------------------------------
// Draws the overlay at [bottom] from 0 to [right]
// -----------------------------------------------------------------------------
void VertexInfoOverlay::draw(int bottom, int right, float alpha) const
{
	// Don't bother if completely faded
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Get colours
	auto col_bg = ColourConfiguration::colour("map_overlay_background");
	auto col_fg = ColourConfiguration::colour("map_overlay_foreground");
	col_fg.a    = col_fg.a * alpha;
	col_bg.a    = col_bg.a * alpha;
	ColRGBA col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += 16 * alpha_inv * alpha_inv;

	// Draw overlay background
	int line_height = 16 * (Drawing::fontSize() / 12.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - line_height - 8, right, bottom + 2, col_bg, col_border);

	// Draw text
	Drawing::drawText(info_, 2, bottom - line_height - 4, col_fg, Drawing::Font::Condensed);

	// Done
	glEnable(GL_LINE_SMOOTH);
}
