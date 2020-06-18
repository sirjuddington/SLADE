
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;


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

	info_.clear();
	bool udmf = vertex->parentMap()->currentFormat() == MapFormat::UDMF;

	// Update info string
	auto pos = vertex->position();
	auto line = fmt::format("Vertex {}: (", vertex->index());
	if (pos.x == static_cast<int>(pos.x))
		line += fmt::format("{}, ", static_cast<int>(pos.x));
	else
		line += fmt::format("{:1.4f}, ", pos.x);
	if (pos.y == static_cast<int>(pos.y))
		line += fmt::format("{})", static_cast<int>(pos.y));
	else
		line += fmt::format("{:1.4f})", pos.y);
	
	if (global::debug)
		line += fmt::format(" ({})", vertex->objId());

	info_.push_back(line);

	// Add vertex heights if they are set
	if (udmf && (vertex->hasProp("zfloor") || vertex->hasProp("zceiling")))
	{
		info_.push_back(fmt::format("Floor Height: {}", vertex->floatProperty("zfloor")));
		info_.push_back(fmt::format("Ceiling Height: {}", vertex->floatProperty("zceiling")));
	}
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
	auto col_bg = colourconfig::colour("map_overlay_background");
	auto col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a    = col_fg.a * alpha;
	col_bg.a    = col_bg.a * alpha;
	ColRGBA col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += 16 * alpha_inv * alpha_inv;

	// Draw overlay background
	int line_height = 16 * (drawing::fontSize() / 12.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawing::drawBorderedRect(0, bottom - (line_height * info_.size()) - 8, right, bottom + 2, col_bg, col_border);

	// Draw text
	int y = bottom - (line_height * info_.size()) - 4;
	for (const auto& line : info_)
	{
		drawing::drawText(line, 2, y, col_fg, drawing::Font::Condensed);
		y += line_height;
	}

	// Done
	glEnable(GL_LINE_SMOOTH);
}
