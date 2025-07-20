
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "General/MapFormat.h"
#include "Geometry/Rect.h"
#include "OpenGL/Draw2D.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"

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
	auto pos  = vertex->position();
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
void VertexInfoOverlay::draw(gl::draw2d::Context& dc, float alpha) const
{
	// Don't bother if completely faded
	if (alpha <= 0.0f)
		return;

	// Calculate height
	auto line_height = dc.textLineHeight();
	auto height      = line_height * info_.size() + 8.0f;

	// Slide in/out animation
	auto alpha_inv = 1.0f - alpha;
	auto bottom    = dc.viewSize().y + 16.0f * alpha_inv * alpha_inv;

	// Draw overlay background
	dc.setColourFromConfig("map_overlay_background");
	dc.colour.a *= alpha;
	dc.drawRect({ 0.0f, bottom - height, dc.viewSize().x, bottom });

	// Draw text
	auto y = bottom - height + 4.0f;
	dc.setColourFromConfig("map_overlay_foreground");
	dc.colour.a *= alpha;
	for (const auto& line : info_)
	{
		dc.drawText(line, { 4.0f, y });
		y += line_height;
	}
}
