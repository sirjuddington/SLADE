
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer2D.cpp
// Description: MapRenderer2D class - handles all rendering related stuff for
//              the map in 2d. See MapRenderer2D_*.cpp files for type-specific
//              rendering functions
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
#include "MapRenderer2D.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/ItemSelection.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/OpenGL.h"
#include "OpenGL/PointSpriteBuffer.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "ThingBuffer2D.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map_animate_hilight, true, CVar::Flag::Save)
CVAR(Bool, map_animate_selection, false, CVar::Flag::Save)
CVAR(Bool, map_animate_tagged, true, CVar::Flag::Save)
CVAR(Bool, map2d_action_lines, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, use_zeth_icons)
EXTERN_CVAR(Float, map2d_line_width)
EXTERN_CVAR(Int, map2d_thing_shape)


// -----------------------------------------------------------------------------
//
// MapRenderer2D Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MapRenderer2D class constructor
// -----------------------------------------------------------------------------
MapRenderer2D::MapRenderer2D(SLADEMap* map, gl::View* view) : map_{ map }, view_{ view }
{
	temp_lines_buffer_          = std::make_unique<gl::LineBuffer>();
	thing_overlay_buffer_       = std::make_unique<gl::PointSpriteBuffer>();
	thing_light_preview_buffer_ = std::make_unique<gl::VertexBuffer2D>();
	temp_things_buffer_         = std::make_unique<gl::ThingBuffer2D>();
}

// -----------------------------------------------------------------------------
// MapRenderer2D class destructor
// -----------------------------------------------------------------------------
MapRenderer2D::~MapRenderer2D() = default;

// -----------------------------------------------------------------------------
// Renders the moving overlay for vertex indices in [vertices], to show movement
// by [move_vec]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderMovingVertices(gl::draw2d::Context& dc, const vector<Item>& vertices, const Vec2d& move_vec)
	const
{
	vector<uint8_t> lines_drawn(map_->nLines(), 0);

	// Determine what lines need drawing (and which of their vertices are being moved)
	for (const auto& item : vertices)
	{
		if (auto v = item.asVertex(*map_))
		{
			for (const auto& line : v->connectedLines())
			{
				if (line->v1() == v)
					lines_drawn[line->index()] |= 1;
				if (line->v2() == v)
					lines_drawn[line->index()] |= 2;
			}
		}
	}

	// Update moving lines buffer with lines attached to moving vertices
	for (unsigned a = 0; a < map_->nLines(); a++)
	{
		auto line  = map_->line(a);
		auto drawn = lines_drawn[line->index()];

		// Skip if not attached to any moving vertices
		if (drawn == 0)
			continue;

		// First vertex
		auto v1 = (drawn & 1) ? glm::vec2{ line->x1() + move_vec.x, line->y1() + move_vec.y }
							  : glm::vec2{ line->x1(), line->y1() };

		// Second vertex
		auto v2 = (drawn & 2) ? glm::vec2{ line->x2() + move_vec.x, line->y2() + move_vec.y }
							  : glm::vec2{ line->x2(), line->y2() };

		// Add to buffer
		temp_lines_buffer_->add2d(v1.x, v1.y, v2.x, v2.y, lineColour(line, true));
	}
	temp_lines_buffer_->push();

	// Draw moving lines
	gl::setBlend(gl::Blend::Normal);
	temp_lines_buffer_->setWidthMult(map2d_line_width);
	temp_lines_buffer_->draw(view_);

	// Get list of moving vertex points
	vector<Vec2f> points;
	for (const auto& item : vertices)
		if (auto v = item.asVertex(*map_))
			points.emplace_back(v->xPos() + move_vec.x, v->yPos() + move_vec.y);

	// Draw moving vertices
	dc.setColourFromConfig("map_moving");
	dc.pointsprite_type   = gl::PointSpriteType::Textured;
	dc.pointsprite_radius = vertexRadius(1.5f);
	dc.texture            = vertexTexture();
	dc.drawPointSprites(points);
}

// -----------------------------------------------------------------------------
// Renders the moving overlay for line indices in [lines], to show movement by
// [move_vec]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderMovingLines(gl::draw2d::Context& dc, const vector<Item>& lines, const Vec2d& move_vec) const
{
	vector<uint8_t> lines_drawn(map_->nLines(), 0);

	// Determine what lines need drawing (and which of their vertices are being moved)
	for (const auto& item : lines)
	{
		if (auto line = item.asLine(*map_))
		{
			// Check first vertex
			auto v = line->v1();
			for (unsigned l = 0; l < v->nConnectedLines(); l++)
			{
				auto cline = v->connectedLine(l);

				if (cline->v1() == v)
					lines_drawn[cline->index()] |= 1;
				if (cline->v2() == v)
					lines_drawn[cline->index()] |= 2;
			}

			// Check second vertex
			v = line->v2();
			for (unsigned l = 0; l < v->nConnectedLines(); l++)
			{
				auto cline = v->connectedLine(l);

				if (cline->v1() == v)
					lines_drawn[cline->index()] |= 1;
				if (cline->v2() == v)
					lines_drawn[cline->index()] |= 2;
			}
		}
	}

	// Update moving lines buffer with lines attached to moving vertices
	for (unsigned a = 0; a < map_->nLines(); a++)
	{
		auto line  = map_->line(a);
		auto drawn = lines_drawn[line->index()];

		// Skip if not attached to any moving vertices
		if (drawn == 0)
			continue;

		// First vertex
		auto v1 = (drawn & 1) ? glm::vec2{ line->x1() + move_vec.x, line->y1() + move_vec.y }
							  : glm::vec2{ line->x1(), line->y1() };

		// Second vertex
		auto v2 = (drawn & 2) ? glm::vec2{ line->x2() + move_vec.x, line->y2() + move_vec.y }
							  : glm::vec2{ line->x2(), line->y2() };

		// Add to buffer
		temp_lines_buffer_->add2d(v1.x, v1.y, v2.x, v2.y, lineColour(line, true));
	}
	temp_lines_buffer_->push();

	// Draw moving lines
	gl::setBlend(gl::Blend::Normal);
	temp_lines_buffer_->setWidthMult(map2d_line_width);
	temp_lines_buffer_->draw(view_);

	// Build list of moving lines (for overlays)
	vector<Rectf> line_overlays;
	for (const auto& item : lines)
		if (auto line = item.asLine(*map_))
			line_overlays.emplace_back(line->start() + move_vec, line->end() + move_vec);

	// Draw moving line overlays
	dc.setColourFromConfig("map_moving");
	dc.line_thickness = map2d_line_width * 3;
	dc.drawLines(line_overlays);
}

// -----------------------------------------------------------------------------
// Renders the moving overlay for sector indices in [sectors], to show movement
// by [move_vec]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderMovingSectors(gl::draw2d::Context& dc, const vector<Item>& sectors, const Vec2d& move_vec)
	const
{
	// Determine what lines are being moved
	vector<uint8_t> lines_moved(map_->nLines(), 0);
	for (auto item : sectors)
	{
		if (auto sector = item.asSector(*map_))
		{
			// Go through connected sides
			auto& sides = sector->connectedSides();
			for (auto& side : sides)
				lines_moved[side->parentLine()->index()] = 1; // Mark parent line as moved
		}
	}

	// Build list of moving lines
	vector<Item> lines;
	for (unsigned a = 0; a < map_->nLines(); a++)
	{
		if (lines_moved[a] > 0)
			lines.emplace_back(static_cast<int>(a), ItemType::Line);
	}

	// Draw moving lines
	renderMovingLines(dc, lines, move_vec);
}

// -----------------------------------------------------------------------------
// Renders the moving overlay for thing indices in [things], to show movement by
// [move_vec]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderMovingThings(gl::draw2d::Context& dc, const vector<Item>& things, const Vec2d& move_vec) const
{
	vector<MapThing*> moving_things;
	for (const auto& item : things)
		if (auto thing = item.asThing(*map_))
			moving_things.push_back(thing);

	// Render things
	renderThings(moving_things, 1.0f, move_vec);

	// Render overlays
	dc.setColourFromConfig("map_moving");
	renderThingOverlays(dc, moving_things, 0.0f, move_vec);
}

// -----------------------------------------------------------------------------
// Renders pasting overlay for [things] at [pos]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderPasteThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, const Vec2d& pos) const
{
	// Render things
	renderThings(things, 1.0f, pos);

	// Render overlays
	dc.setColourFromConfig("map_linedraw");
	renderThingOverlays(dc, things, 0.0f, pos);
}

// -----------------------------------------------------------------------------
// Renders object edit group overlay for [group]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderObjectEditGroup(gl::draw2d::Context& dc, const ObjectEditGroup* group) const
{
	// Simple test
	vector<Vec2d> vertex_points;
	group->putVerticesToDraw(vertex_points);
	vector<ObjectEditGroup::Line> lines;
	group->putLinesToDraw(lines);

	// --- Lines ---

	// Lines
	for (auto& line : lines)
	{
		temp_lines_buffer_->add2d(
			line.v1->position.x,
			line.v1->position.y,
			line.v2->position.x,
			line.v2->position.y,
			lineColour(line.map_line, true));
	}
	temp_lines_buffer_->push();
	temp_lines_buffer_->setWidthMult(map2d_line_width);
	temp_lines_buffer_->draw(view_);

	// Edit overlay
	vector<Rectf> overlay_lines;
	for (auto& line : lines)
		if (!line.isExtra())
			overlay_lines.emplace_back(
				line.v1->position.x, line.v1->position.y, line.v2->position.x, line.v2->position.y);
	dc.setColourFromConfig("map_object_edit");
	dc.line_thickness = map2d_line_width * 3;
	dc.drawLines(overlay_lines);


	// --- Vertices ---

	dc.pointsprite_type   = gl::PointSpriteType::Textured;
	dc.pointsprite_radius = vertexRadius();
	dc.texture            = vertexTexture();
	dc.drawPointSprites(vertex_points);


	// --- Things ---

	// Get things to draw
	vector<ObjectEditGroup::Thing> things;
	group->putThingsToDraw(things);

	if (!things.empty())
	{
		// Draw things
		MapThing* thing;
		double    x, y, angle;
		for (auto& item : things)
		{
			// Get thing info
			thing = item.map_thing;
			x     = item.position.x;
			y     = item.position.y;
			angle = thing->angle();

			// Setup temp. thing buffer with type properties from game configuration
			setupThingBuffer(*temp_things_buffer_, game::configuration().thingType(thing->type()));

			// Draw thing
			temp_things_buffer_->add(x, y, angle);
			temp_things_buffer_->push();
			temp_things_buffer_->draw(view_, glm::vec4{ 1.0f }, map2d_thing_shape == 1, true);
		}

		// Draw thing overlays
		for (auto& item : things)
		{
			thing              = item.map_thing;
			const auto& tt     = game::configuration().thingType(thing->type());
			double      radius = tt.radius();
			if (tt.shrinkOnZoom())
				radius = scaledRadius(radius);

			thing_overlay_buffer_->add({ item.position.x, item.position.y }, radius + 4.0f);
		}
		thing_overlay_buffer_->push();
		thing_overlay_buffer_->setColour(colourconfig::colour("map_object_edit"));
		thing_overlay_buffer_->setFillOpacity(0.25f);
		thing_overlay_buffer_->setOutlineWidth(std::min(3.0f / static_cast<float>(view_->scale().x), 4.0f));
		thing_overlay_buffer_->draw(
			map2d_thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
			view_);
	}
}

// -----------------------------------------------------------------------------
// Updates all VBOs and other cached data
// -----------------------------------------------------------------------------
void MapRenderer2D::forceUpdate(bool flats_ceilings)
{
	// Update variables
	flats_.clear();
	thing_paths_.clear();

	// Update buffers
	updateFlatsBuffer(flats_ceilings);
	updateLinesBuffer(lines_dirs_);
	updateVerticesBuffer();
	updateThingBuffers();
}

// -----------------------------------------------------------------------------
// Returns [radius] scaled such that it stays the same size on screen at all
// zoom levels
// -----------------------------------------------------------------------------
double MapRenderer2D::scaledRadius(int radius) const
{
	// if (radius > 16)
	//	radius = 16;

	if (view_->scale().x > 1.0)
		return radius / view_->scale().x;
	else
		return radius;
}

// -----------------------------------------------------------------------------
// Clears cached flat texture data
// -----------------------------------------------------------------------------
void MapRenderer2D::clearTextureCache()
{
	for (auto& flat : flats_)
		flat.texture = 0;
	flat_groups_.clear();
}
