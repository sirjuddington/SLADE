
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LineDraw.cpp
// Description: Map Editor line drawing implementation
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
#include "LineDraw.h"
#include "General/KeyBind.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "Utility/MathStuff.h"

using namespace MapEditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, shapedraw_shape, 0, CVAR_SAVE)
CVAR(Bool, shapedraw_centered, false, CVAR_SAVE)
CVAR(Bool, shapedraw_lockratio, false, CVAR_SAVE)
CVAR(Int, shapedraw_sides, 16, CVAR_SAVE)


// -----------------------------------------------------------------------------
//
// LineDraw Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the line drawing point at [index]
// -----------------------------------------------------------------------------
fpoint2_t LineDraw::point(unsigned index)
{
	// Check index
	if (index >= draw_points_.size())
		return fpoint2_t(0, 0);

	return draw_points_[index];
}

// -----------------------------------------------------------------------------
// Adds a line drawing point at [point], or at the nearest vertex to [point] if
// [nearest] is true
// -----------------------------------------------------------------------------
bool LineDraw::addPoint(fpoint2_t point, bool nearest)
{
	// Snap to nearest vertex if necessary
	if (nearest)
	{
		int vertex = context_.map().nearestVertex(point);
		if (vertex >= 0)
			point = context_.map().getVertex(vertex)->point();
	}

	// Otherwise, snap to grid if necessary
	else if (context_.gridSnap())
	{
		point.x = context_.snapToGrid(point.x);
		point.y = context_.snapToGrid(point.y);
	}

	// Check if this is the same as the last point
	if (draw_points_.size() > 0 && point.x == draw_points_.back().x && point.y == draw_points_.back().y)
	{
		// End line drawing
		end(true);
		MapEditor::showShapeDrawPanel(false);
		return true;
	}

	// Add point
	draw_points_.push_back(point);

	// Check if first and last points match
	if (draw_points_.size() > 1 && point.x == draw_points_[0].x && point.y == draw_points_[0].y)
	{
		end(true);
		MapEditor::showShapeDrawPanel(false);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Removes the most recent line drawing point, or cancels line drawing if there
// are no points
// -----------------------------------------------------------------------------
void LineDraw::removePoint()
{
	if (draw_points_.empty())
	{
		end(false);
		MapEditor::showShapeDrawPanel(false);
	}
	else
	{
		draw_points_.pop_back();
	}
}

// -----------------------------------------------------------------------------
// Sets the shape drawing origin to [point], or the nearest vertex to [point] if
// [nearest] is true
// -----------------------------------------------------------------------------
void LineDraw::setShapeOrigin(fpoint2_t point, bool nearest)
{
	// Snap to nearest vertex if necessary
	if (nearest)
	{
		int vertex = context_.map().nearestVertex(point);
		if (vertex >= 0)
			point = context_.map().getVertex(vertex)->point();
	}

	// Otherwise, snap to grid if necessary
	else if (context_.gridSnap())
	{
		point.x = context_.snapToGrid(point.x);
		point.y = context_.snapToGrid(point.y);
	}

	draw_origin_ = point;
}

// -----------------------------------------------------------------------------
// Builds the current shape as line drawing points using the shape draw origin
// and [point] for the size
// -----------------------------------------------------------------------------
void LineDraw::updateShape(fpoint2_t point)
{
	// Clear line draw points
	draw_points_.clear();

	// Snap edge to grid if needed
	if (context_.gridSnap())
	{
		point.x = context_.snapToGrid(point.x);
		point.y = context_.snapToGrid(point.y);
	}

	// Lock width:height at 1:1 if needed
	fpoint2_t origin = draw_origin_;
	double    width  = fabs(point.x - origin.x);
	double    height = fabs(point.y - origin.y);
	if (shapedraw_lockratio)
	{
		if (width < height)
		{
			if (origin.x < point.x)
				point.x = origin.x + height;
			else
				point.x = origin.x - height;
		}

		if (height < width)
		{
			if (origin.y < point.y)
				point.y = origin.y + width;
			else
				point.y = origin.y - width;
		}
	}

	// Center on origin if needed
	if (shapedraw_centered)
	{
		origin.x -= (point.x - origin.x);
		origin.y -= (point.y - origin.y);
	}

	// Get box from tl->br
	fpoint2_t tl(min(origin.x, point.x), min(origin.y, point.y));
	fpoint2_t br(max(origin.x, point.x), max(origin.y, point.y));
	width  = br.x - tl.x;
	height = br.y - tl.y;

	// Rectangle
	if (shapedraw_shape == 0)
	{
		draw_points_.push_back(fpoint2_t(tl.x, tl.y));
		draw_points_.push_back(fpoint2_t(tl.x, br.y));
		draw_points_.push_back(fpoint2_t(br.x, br.y));
		draw_points_.push_back(fpoint2_t(br.x, tl.y));
		draw_points_.push_back(fpoint2_t(tl.x, tl.y));
	}

	// Ellipse
	else if (shapedraw_shape == 1)
	{
		// Get midpoint
		fpoint2_t mid;
		mid.x = tl.x + ((br.x - tl.x) * 0.5);
		mid.y = tl.y + ((br.y - tl.y) * 0.5);

		// Get x/y radius
		width *= 0.5;
		height *= 0.5;

		// Add ellipse points
		double    rot = 0;
		fpoint2_t start;
		for (int a = 0; a < shapedraw_sides; a++)
		{
			// Calculate point (rounded)
			fpoint2_t p(MathStuff::round(mid.x + sin(rot) * width), MathStuff::round(mid.y - cos(rot) * height));

			// Add point
			draw_points_.push_back(p);
			rot -= (3.1415926535897932384626433832795 * 2) / (double)shapedraw_sides;

			if (a == 0)
				start = p;
		}

		// Close ellipse
		draw_points_.push_back(start);
	}
}

// -----------------------------------------------------------------------------
// Begins a line or shape (if [shape] = true) drawing operation
// -----------------------------------------------------------------------------
void LineDraw::begin(bool shape)
{
	// Setup state
	state_current_ = shape ? State::ShapeOrigin : State::Line;
	context_.input().setMouseState(Input::MouseState::LineDraw);

	// Setup help text
	string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
	string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
	if (shape)
	{
		context_.setFeatureHelp({ "Shape Drawing",
								  S_FMT("%s = Accept", key_accept),
								  S_FMT("%s = Cancel", key_cancel),
								  "Left Click = Draw point",
								  "Right Click = Undo previous point" });
		MapEditor::showShapeDrawPanel(true);
	}
	else
	{
		context_.setFeatureHelp({ "Line Drawing",
								  S_FMT("%s = Accept", key_accept),
								  S_FMT("%s = Cancel", key_cancel),
								  "Left Click = Draw point",
								  "Right Click = Undo previous point",
								  "Shift = Snap to nearest vertex" });
	}
}

// -----------------------------------------------------------------------------
// Ends the line drawing operation and applies changes if [accept] is true
// -----------------------------------------------------------------------------
void LineDraw::end(bool apply)
{
	// Hide shape draw panel
	MapEditor::showShapeDrawPanel(true);

	// Do nothing if we don't need to create any lines
	if (!apply || draw_points_.size() <= 1)
	{
		draw_points_.clear();
		context_.setFeatureHelp({});
		return;
	}

	// Begin undo level
	context_.beginUndoRecord("Line Draw");

	// Add extra points if any lines overlap existing vertices
	auto& map = context_.map();
	for (unsigned a = 0; a < draw_points_.size() - 1; a++)
	{
		auto v =
			map.lineCrossVertex(draw_points_[a].x, draw_points_[a].y, draw_points_[a + 1].x, draw_points_[a + 1].y);
		while (v)
		{
			draw_points_.insert(draw_points_.begin() + a + 1, fpoint2_t(v->xPos(), v->yPos()));
			a++;
			v = map.lineCrossVertex(draw_points_[a].x, draw_points_[a].y, draw_points_[a + 1].x, draw_points_[a + 1].y);
		}
	}

	// Create vertices
	for (unsigned a = 0; a < draw_points_.size(); a++)
		map.createVertex(draw_points_[a].x, draw_points_[a].y, 1);

	// Create lines
	unsigned nl_start = map.nLines();
	for (unsigned a = 0; a < draw_points_.size() - 1; a++)
	{
		// Check for intersections
		auto intersect =
			map.cutLines(draw_points_[a].x, draw_points_[a].y, draw_points_[a + 1].x, draw_points_[a + 1].y);
		LOG_MESSAGE(2, "%lu intersect points", intersect.size());

		// Create line normally if no intersections
		if (intersect.size() == 0)
			map.createLine(draw_points_[a].x, draw_points_[a].y, draw_points_[a + 1].x, draw_points_[a + 1].y, 1);
		else
		{
			// Intersections exist, create multiple lines between intersection points

			// From first point to first intersection
			map.createLine(draw_points_[a].x, draw_points_[a].y, intersect[0].x, intersect[0].y, 1);

			// Between intersection points
			for (unsigned p = 0; p < intersect.size() - 1; p++)
				map.createLine(intersect[p].x, intersect[p].y, intersect[p + 1].x, intersect[p + 1].y, 1);

			// From last intersection to next point
			map.createLine(intersect.back().x, intersect.back().y, draw_points_[a + 1].x, draw_points_[a + 1].y, 1);
		}
	}

	// Build new sectors
	vector<MapLine*> new_lines;
	for (unsigned a = nl_start; a < map.nLines(); a++)
		new_lines.push_back(map.getLine(a));
	map.correctSectors(new_lines);

	// Check for and attempt to correct invalid lines
	vector<MapLine*> invalid_lines;
	for (unsigned a = 0; a < new_lines.size(); a++)
	{
		if (new_lines[a]->s1())
			continue;

		new_lines[a]->flip();
		invalid_lines.push_back(new_lines[a]);
	}
	map.correctSectors(invalid_lines);

	// End recording undo level
	context_.endUndoRecord(true);

	// Clear draw points
	draw_points_.clear();

	// Clear feature help text
	context_.setFeatureHelp({});
}
