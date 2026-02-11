
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer2D_Lines.cpp
// Description: MapRenderer2D line rendering functions
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
#include "App.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapRenderer2D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, map2d_line_width, 1.5f, CVar::Flag::Save)
CVAR(Bool, map2d_line_smooth, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)
EXTERN_CVAR(Bool, map_animate_tagged)
EXTERN_CVAR(Bool, map2d_action_lines)


// -----------------------------------------------------------------------------
//
// MapRenderer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Renders map lines, with direction tabs if [show_direction] is true
// -----------------------------------------------------------------------------
void MapRenderer2D::renderLines(bool show_direction, float alpha)
{
	// Check there are any lines to render
	if (map_->nLines() == 0)
		return;

	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	// Update lines buffer if needed
	auto buffer_empty = map2d_line_smooth ? !lines_buffer_ || lines_buffer_->buffer().empty()
										  : !lines_buffer_basic_ || lines_buffer_basic_->buffer().empty();
	if (buffer_empty || show_direction != lines_dirs_ || map_->nLines() != n_lines_
		|| map_->geometryUpdated() > lines_updated_
		|| map_->mapData().modifiedSince(lines_updated_, MapObject::Type::Line))
		updateLinesBuffer(show_direction);

	// Render lines buffer
	if (map2d_line_smooth)
	{
		lines_buffer_->setWidthMult(map2d_line_width);
		lines_buffer_->draw(view_, { 1.0f, 1.0f, 1.0f, alpha });
	}
	else
	{
		auto& shader = gl::draw2d::defaultShader(false);
		lines_buffer_basic_->draw(gl::Primitive::Lines, &shader, view_);
	}
}

// -----------------------------------------------------------------------------
// Renders the line hilight overlay for line [index]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderLineHilight(gl::draw2d::Context& dc, int index, float fade) const
{
	// Check hilight
	auto line = map_->line(index);
	if (!line)
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Render line hilight (+ direction tab)
	auto mid = line->getPoint(MapObject::Point::Mid);
	auto tab = line->dirTabPoint();
	dc.setColourFromConfig("map_hilight", fade);
	dc.line_thickness = map2d_line_width * (colourconfig::lineHilightWidth() * fade);
	dc.drawLines({ { line->start(), line->end() }, { mid, tab } });
}

// -----------------------------------------------------------------------------
// Renders the line selection overlay for line indices in [selection]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderLineSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade) const
{
	// Check anything is selected
	if (selection.empty())
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Build lines list
	vector<Rectf> lines;
	for (const auto& item : selection)
	{
		if (auto line = item.asLine(*map_); line)
		{
			auto mid = line->getPoint(MapObject::Point::Mid);
			auto tab = line->dirTabPoint();
			lines.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());
			lines.emplace_back(mid.x, mid.y, tab.x, tab.y);
		}
	}

	// Render lines
	dc.setColourFromConfig("map_selection", fade);
	dc.line_thickness = map2d_line_width * colourconfig::lineSelectionWidth();
	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Renders the tagged line overlay for lines in [lines]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderTaggedLines(gl::draw2d::Context& dc, const vector<MapLine*>& lines, float fade) const
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Build list of lines & arrows to render
	vector<Rectf> r_lines;
	vector<Rectf> r_arrows;
	auto          object = editContext().selection().hilightedObject();
	for (auto line : lines)
	{
		// Line
		auto mid = line->getPoint(MapObject::Point::Mid);
		auto tab = line->dirTabPoint();
		r_lines.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());
		r_lines.emplace_back(mid.x, mid.y, tab.x, tab.y);

		// Action lines
		if (object && map2d_action_lines)
		{
			auto op = object->getPoint(MapObject::Point::Within);
			auto lp = line->getPoint(MapObject::Point::Within);
			r_arrows.emplace_back(op.x, op.y, lp.x, lp.y);
		}
	}

	// Render tagged lines
	dc.setColourFromConfig("map_tagged", fade);
	dc.line_thickness = map2d_line_width * colourconfig::lineHilightWidth();
	dc.drawLines(r_lines);

	// Render action lines
	dc.line_thickness    = map2d_line_width * 1.5f;
	dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
	dc.drawLines(r_arrows);
	dc.line_arrow_length = 0.0f;
}

// -----------------------------------------------------------------------------
// Renders the tagging line overlay for lines in [lines]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderTaggingLines(gl::draw2d::Context& dc, const vector<MapLine*>& lines, float fade) const
{
	// Reset fade if tagging animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Build list of lines & arrows to render
	vector<Rectf> r_lines;
	vector<Rectf> r_arrows;
	auto          object = editContext().selection().hilightedObject();
	for (auto line : lines)
	{
		// Line
		auto mid = line->getPoint(MapObject::Point::Mid);
		auto tab = line->dirTabPoint();
		r_lines.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());
		r_lines.emplace_back(mid.x, mid.y, tab.x, tab.y);

		// Action lines
		if (object && map2d_action_lines)
		{
			auto op = object->getPoint(MapObject::Point::Within);
			auto lp = line->getPoint(MapObject::Point::Within);
			r_arrows.emplace_back(lp.x, lp.y, op.x, op.y);
		}
	}

	// Render tagging lines
	dc.setColourFromConfig("map_tagged", fade);
	dc.line_thickness = map2d_line_width * colourconfig::lineHilightWidth();
	dc.drawLines(r_lines);

	// Render action lines
	dc.line_thickness    = map2d_line_width * 1.5f;
	dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
	dc.drawLines(r_arrows);
	dc.line_arrow_length = 0.0f;
}

// -----------------------------------------------------------------------------
// (Re)builds the map lines buffer
// -----------------------------------------------------------------------------
void MapRenderer2D::updateLinesBuffer(bool show_direction)
{
	// Init buffer
	if (map2d_line_smooth && !lines_buffer_)
		lines_buffer_ = std::make_unique<gl::LineBuffer>();
	if (!map2d_line_smooth && !lines_buffer_basic_)
		lines_buffer_basic_ = std::make_unique<gl::VertexBuffer2D>();

	// Add all map lines to buffer
	for (const auto line : map_->lines())
	{
		auto col = lineColour(line);

		if (map2d_line_smooth)
			lines_buffer_->add2d(line->x1(), line->y1(), line->x2(), line->y2(), col, 1.0f);
		else
		{
			lines_buffer_basic_->add(line->start(), col, {});
			lines_buffer_basic_->add(line->end(), col, {});
		}

		// Direction tab if needed
		if (show_direction)
		{
			auto mid = line->getPoint(MapObject::Point::Mid);
			auto tab = line->dirTabPoint();
			if (map2d_line_smooth)
				lines_buffer_->add2d(
					mid.x, mid.y, tab.x, tab.y, { col.fr(), col.fg(), col.fb(), col.fa() * 0.6f }, 1.0f);
			else
			{
				auto dcol = col.ampf(1.0f, 1.0f, 1.0f, 0.6f);
				lines_buffer_basic_->add(mid, dcol, {});
				lines_buffer_basic_->add(tab, dcol, {});
			}
		}
	}
	if (map2d_line_smooth)
		lines_buffer_->push();
	else
		lines_buffer_basic_->push();

	lines_dirs_    = show_direction;
	n_lines_       = map_->nLines();
	lines_updated_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Returns the colour for [line]
// -----------------------------------------------------------------------------
ColRGBA MapRenderer2D::lineColour(const MapLine* line, bool ignore_filter) const
{
	ColRGBA col;

	if (line)
	{
		// Check for special line
		if (line->special() > 0)
			col.set(colourconfig::colour("map_line_special"));
		else if (line->s1())
			col.set(colourconfig::colour("map_line_normal"));
		else
			col.set(colourconfig::colour("map_line_invalid"));

		// Check for two-sided line
		if (line->s2())
			col.a *= 0.6f;

		// Check if filtered
		if (line->isFiltered() && !ignore_filter)
			col.a *= 0.25f;
	}

	return col;
}
