
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
#include "MapEditor/SLADEMap/MapVertex.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "Utility/MathStuff.h"


// -----------------------------------------------------------------------------
//
// VertexInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// VertexInfoOverlay class constructor
// -----------------------------------------------------------------------------
VertexInfoOverlay::VertexInfoOverlay()
{
	// Init variables
	pos_frac_ = false;
}

// -----------------------------------------------------------------------------
// VertexInfoOverlay class destructor
// -----------------------------------------------------------------------------
VertexInfoOverlay::~VertexInfoOverlay() {}

// -----------------------------------------------------------------------------
// Updates the overlay with info from [vertex]
// -----------------------------------------------------------------------------
void VertexInfoOverlay::update(MapVertex* vertex)
{
	if (!vertex)
		return;

	// Update info string
	if (pos_frac_)
		info_ = S_FMT("Vertex %d: (%1.4f, %1.4f)", vertex->getIndex(), vertex->xPos(), vertex->yPos());
	else
		info_ = S_FMT("Vertex %d: (%d, %d)", vertex->getIndex(), (int)vertex->xPos(), (int)vertex->yPos());

	if (Global::debug)
		info_ += S_FMT(" (%d)", vertex->getId());
}

// -----------------------------------------------------------------------------
// Draws the overlay at [bottom] from 0 to [right]
// -----------------------------------------------------------------------------
void VertexInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if completely faded
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a      = col_fg.a * alpha;
	col_bg.a      = col_bg.a * alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += 16 * alpha_inv * alpha_inv;

	// Draw overlay background
	int line_height = 16 * (Drawing::fontSize() / 12.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - line_height - 8, right, bottom + 2, col_bg, col_border);

	// Draw text
	Drawing::drawText(info_, 2, bottom - line_height - 4, col_fg, Drawing::FONT_CONDENSED);

	// Done
	glEnable(GL_LINE_SMOOTH);
}
