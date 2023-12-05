
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ObjectEdit.cpp
// Description: ObjectEditGroup class - used for the object edit feature in the
//              map editor, takes a bunch of vertices or things and applies
//              rotation/translation/scaling to them. Also keeps track of any
//              connected lines for visual purposes
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
#include "ObjectEdit.h"
#include "General/KeyBind.h"
#include "General/UI.h"
#include "Input.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/Renderer/Renderer.h"
#include "OpenGL/View.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_merge_undo_step)


// -----------------------------------------------------------------------------
//
// ObjectEditGroup Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds [vertex] to the group.
// If [ignored] is set, the vertex won't be modified by the object edit
// -----------------------------------------------------------------------------
void ObjectEditGroup::addVertex(MapVertex* vertex, bool ignored)
{
	auto v = new Vertex{ { vertex->xPos(), vertex->yPos() }, { vertex->xPos(), vertex->yPos() }, vertex, ignored };

	if (!ignored)
	{
		bbox_.extend(v->position.x, v->position.y);
		old_bbox_.extend(v->position.x, v->position.y);
		original_bbox_.extend(v->position.x, v->position.y);
	}

	vertices_.emplace_back(v);
}

// -----------------------------------------------------------------------------
// Builds a list of all lines connected to the group vertices
// -----------------------------------------------------------------------------
void ObjectEditGroup::addConnectedLines()
{
	const auto n_v = vertices_.size();
	for (unsigned v = 0; v < n_v; ++v) // Can't use a range-for loop here due to addVertex usage
	{
		auto map_vertex = vertices_[v]->map_vertex;
		for (unsigned l = 0; l < map_vertex->nConnectedLines(); l++)
		{
			const auto map_line = map_vertex->connectedLine(l);
			if (!hasLine(map_line))
			{
				// Create line
				Line line{ findVertex(map_line->v1()), findVertex(map_line->v2()), map_line };

				// Add extra vertex if needed (will be ignored for editing)
				if (!line.v1)
				{
					addVertex(map_line->v1(), true);
					line.v1 = vertices_.back().get();
				}
				if (!line.v2)
				{
					addVertex(map_line->v2(), true);
					line.v2 = vertices_.back().get();
				}

				// Add line
				lines_.push_back(line);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Adds [thing] to the group
// -----------------------------------------------------------------------------
void ObjectEditGroup::addThing(MapThing* thing)
{
	// Add thing
	Thing t;
	t.map_thing    = thing;
	t.position.x   = thing->xPos();
	t.position.y   = thing->yPos();
	t.old_position = t.position;
	t.angle        = thing->angle();
	things_.push_back(t);

	// Update bbox
	bbox_.extend(t.position.x, t.position.y);
	old_bbox_.extend(t.position.x, t.position.y);
	original_bbox_.extend(t.position.x, t.position.y);
}

// -----------------------------------------------------------------------------
// Returns true if [line] is connected to the group vertices
// -----------------------------------------------------------------------------
bool ObjectEditGroup::hasLine(MapLine* line)
{
	for (auto& l : lines_)
		if (l.map_line == line)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the group info about [vertex]
// -----------------------------------------------------------------------------
ObjectEditGroup::Vertex* ObjectEditGroup::findVertex(MapVertex* vertex)
{
	for (auto& v : vertices_)
		if (v->map_vertex == vertex)
			return v.get();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Clears all group items
// -----------------------------------------------------------------------------
void ObjectEditGroup::clear()
{
	vertices_.clear();
	lines_.clear();
	things_.clear();
	bbox_.reset();
	old_bbox_.reset();
	original_bbox_.reset();
	offset_prev_ = { 0, 0 };
	rotation_    = 0;
}

// -----------------------------------------------------------------------------
// Sets filtering on all group objects to [filter]
// -----------------------------------------------------------------------------
void ObjectEditGroup::filterObjects(bool filter)
{
	// Vertices
	for (auto& vertex : vertices_)
		if (!vertex->ignored)
			vertex->map_vertex->filter(filter);

	// Lines
	for (auto& line : lines_)
		line.map_line->filter(filter);

	// Things
	for (auto& thing : things_)
		thing.map_thing->filter(filter);
}

// -----------------------------------------------------------------------------
// Resets the position of all group objects to their original positions
// (ie. current position on the actual map)
// -----------------------------------------------------------------------------
void ObjectEditGroup::resetPositions()
{
	bbox_.reset();

	// Vertices
	for (auto& vertex : vertices_)
	{
		vertex->old_position = vertex->position;

		if (!vertex->ignored)
			bbox_.extend(vertex->position.x, vertex->position.y);
	}

	// Things
	for (auto& thing : things_)
	{
		thing.old_position = thing.position;
		bbox_.extend(thing.position.x, thing.position.y);
	}

	old_bbox_ = bbox_;
	rotation_ = 0;
}

// -----------------------------------------------------------------------------
// Finds the nearest line to [pos] (that is closer than [min] in distance),
// and sets [v1]/[v2] to the line vertices.
// Returns true if a line was found within the distance specified
// -----------------------------------------------------------------------------
bool ObjectEditGroup::nearestLineEndpoints(Vec2d pos, double min, Vec2d& v1, Vec2d& v2)
{
	double min_dist = min;
	for (auto& line : lines_)
	{
		double d = math::distanceToLineFast(pos, { line.v1->position, line.v2->position });

		if (d < min_dist)
		{
			min_dist = d;
			v1.set(line.v1->position);
			v2.set(line.v2->position);
		}
	}

	return (min_dist < min);
}

// -----------------------------------------------------------------------------
// Fills [list] with the positions of all group vertices
// -----------------------------------------------------------------------------
void ObjectEditGroup::putVerticesToDraw(vector<Vec2d>& list)
{
	for (auto& vertex : vertices_)
		if (!vertex->ignored)
			list.push_back(vertex->position);
}

// -----------------------------------------------------------------------------
// Fills [list] with all lines in the group
// -----------------------------------------------------------------------------
void ObjectEditGroup::putLinesToDraw(vector<Line>& list)
{
	for (auto line : lines_)
		list.push_back(line);
}

// -----------------------------------------------------------------------------
// Fills [list] with all things in the group
// -----------------------------------------------------------------------------
void ObjectEditGroup::putThingsToDraw(vector<Thing>& list)
{
	for (const auto& thing : things_)
		list.push_back(thing);
}

// -----------------------------------------------------------------------------
// Moves all group objects by [xoff,yoff]
// -----------------------------------------------------------------------------
void ObjectEditGroup::doMove(double xoff, double yoff)
{
	if (xoff == offset_prev_.x && yoff == offset_prev_.y)
		return;

	// Update vertices
	for (auto& vertex : vertices_)
	{
		// Skip ignored
		if (vertex->ignored)
			continue;

		vertex->position.x = vertex->old_position.x + xoff;
		vertex->position.y = vertex->old_position.y + yoff;
	}

	// Update things
	for (auto& thing : things_)
	{
		thing.position.x = thing.old_position.x + xoff;
		thing.position.y = thing.old_position.y + yoff;
	}

	// Update bbox
	bbox_.max.x = old_bbox_.max.x + xoff;
	bbox_.max.y = old_bbox_.max.y + yoff;
	bbox_.min.x = old_bbox_.min.x + xoff;
	bbox_.min.y = old_bbox_.min.y + yoff;

	offset_prev_.x = xoff;
	offset_prev_.y = yoff;
}

// -----------------------------------------------------------------------------
// Modifies the group bounding box by [xoff]/[yoff], and scales all objects to
// fit within the resulting bbox.
// This is used when dragging bbox edges via the mouse
// -----------------------------------------------------------------------------
void ObjectEditGroup::doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom)
{
	if (xoff == offset_prev_.x && yoff == offset_prev_.y)
		return;

	// Update bbox
	if (left)
	{
		// Check for left >= right
		if (old_bbox_.min.x + xoff >= old_bbox_.max.x)
			return;
		else
			bbox_.min.x = old_bbox_.min.x + xoff;
	}
	if (right)
	{
		// Check for right <= left
		if (old_bbox_.max.x + xoff <= old_bbox_.min.x)
			return;
		else
			bbox_.max.x = old_bbox_.max.x + xoff;
	}
	if (top)
	{
		// Check for top <= bottom
		if (old_bbox_.max.y + yoff <= old_bbox_.min.y)
			return;
		else
			bbox_.max.y = old_bbox_.max.y + yoff;
	}
	if (bottom)
	{
		// Check for bottom >= top
		if (old_bbox_.min.y + yoff >= old_bbox_.max.y)
			return;
		else
			bbox_.min.y = old_bbox_.min.y + yoff;
	}

	// Determine offset and scale values
	double xofs   = bbox_.min.x - old_bbox_.min.x;
	double yofs   = bbox_.min.y - old_bbox_.min.y;
	double xscale = 1;
	double yscale = 1;
	if (old_bbox_.width() > 0)
		xscale = bbox_.width() / old_bbox_.width();
	if (old_bbox_.height() > 0)
		yscale = bbox_.height() / old_bbox_.height();

	// Update vertices
	for (auto& vertex : vertices_)
	{
		// Skip ignored
		if (vertex->ignored)
			continue;

		// Scale
		vertex->position.x = old_bbox_.min.x + ((vertex->old_position.x - old_bbox_.min.x) * xscale);
		vertex->position.y = old_bbox_.min.y + ((vertex->old_position.y - old_bbox_.min.y) * yscale);

		// Move
		vertex->position.x += xofs;
		vertex->position.y += yofs;
	}

	// Update things
	for (auto& thing : things_)
	{
		// Scale
		thing.position.x = old_bbox_.min.x + ((thing.old_position.x - old_bbox_.min.x) * xscale);
		thing.position.y = old_bbox_.min.y + ((thing.old_position.y - old_bbox_.min.y) * yscale);

		// Move
		thing.position.x += xofs;
		thing.position.y += yofs;
	}

	offset_prev_.x = xoff;
	offset_prev_.y = yoff;
}

// -----------------------------------------------------------------------------
// Rotates all objects in the group.
// The rotation angle is calculated from [p1]->mid and mid->[p2].
// This is used when rotating via the mouse ([p1] is the drag origin and [p2]
// is the current point)
// -----------------------------------------------------------------------------
void ObjectEditGroup::doRotate(Vec2d p1, Vec2d p2, bool lock45)
{
	// Get midpoint
	Vec2d mid(old_bbox_.min.x + old_bbox_.width() * 0.5, old_bbox_.min.y + old_bbox_.height() * 0.5);

	// Determine angle
	double angle = math::angle2DRad(p1, mid, p2);
	rotation_    = math::radToDeg(angle);

	// Lock to 45 degree increments if needed
	if (lock45)
	{
		rotation_ = ceil(rotation_ / 45.0 - 0.5) * 45.0;
		if (rotation_ > 325 || rotation_ < 0)
			rotation_ = 0;
	}

	// Rotate vertices
	for (auto& vertex : vertices_)
	{
		if (!vertex->ignored)
			vertex->position = math::rotatePoint(mid, vertex->old_position, rotation_);
	}

	// Rotate things
	for (auto& thing : things_)
		thing.position = math::rotatePoint(mid, thing.old_position, rotation_);
}

// -----------------------------------------------------------------------------
// Moves all group objects by [xoff,yoff], scales all group objects by
// [xscale,yscale] and rotates all group objects by [rotation]
// -----------------------------------------------------------------------------
void ObjectEditGroup::doAll(
	double xoff,
	double yoff,
	double xscale,
	double yscale,
	double rotation,
	bool   mirror_x,
	bool   mirror_y)
{
	// Update bbox
	bbox_ = original_bbox_;

	// Apply offsets
	bbox_.min.x += xoff;
	bbox_.max.x += xoff;
	bbox_.min.y += yoff;
	bbox_.max.y += yoff;

	// Apply scale (from center)
	double xgrow = (bbox_.width() * xscale) - bbox_.width();
	double ygrow = (bbox_.height() * yscale) - bbox_.height();
	bbox_.min.x -= (xgrow * 0.5);
	bbox_.max.x += (xgrow * 0.5);
	bbox_.min.y -= (ygrow * 0.5);
	bbox_.max.y += (ygrow * 0.5);
	old_bbox_ = bbox_;


	// Update vertices
	for (auto& vertex : vertices_)
	{
		// Skip ignored
		if (vertex->ignored)
			continue;

		// Reset first
		vertex->position.x = vertex->map_vertex->xPos();
		vertex->position.y = vertex->map_vertex->yPos();

		// Mirror
		if (mirror_x)
			vertex->position.x = original_bbox_.midX() - (vertex->position.x - original_bbox_.midX());
		if (mirror_y)
			vertex->position.y = original_bbox_.midY() - (vertex->position.y - original_bbox_.midY());

		// Scale
		vertex->position.x = original_bbox_.midX() + (vertex->position.x - original_bbox_.midX()) * xscale;
		vertex->position.y = original_bbox_.midY() + (vertex->position.y - original_bbox_.midY()) * yscale;

		// Move
		vertex->position.x += xoff;
		vertex->position.y += yoff;

		// Rotate
		if (rotation != 0)
			vertex->position = math::rotatePoint(bbox_.mid(), vertex->position, rotation);

		vertex->old_position = vertex->position;
	}


	// Update things
	for (auto& thing : things_)
	{
		// Reset first
		thing.position.x = thing.map_thing->xPos();
		thing.position.y = thing.map_thing->yPos();
		thing.angle      = thing.map_thing->angle();

		// Mirror
		if (mirror_x)
		{
			thing.position.x = original_bbox_.midX() - (thing.position.x - original_bbox_.midX());
			thing.angle += 90;
			thing.angle = 360 - thing.angle;
			thing.angle -= 90;
			while (thing.angle < 0)
				thing.angle += 360;
		}
		if (mirror_y)
		{
			thing.position.y = original_bbox_.midY() - (thing.position.y - original_bbox_.midY());
			thing.angle      = 360 - thing.angle;
			while (thing.angle < 0)
				thing.angle += 360;
		}

		// Scale
		thing.position.x = original_bbox_.midX() + (thing.position.x - original_bbox_.midX()) * xscale;
		thing.position.y = original_bbox_.midY() + (thing.position.y - original_bbox_.midY()) * yscale;

		// Move
		thing.position.x += xoff;
		thing.position.y += yoff;

		// Rotate
		if (rotation != 0)
			thing.position = math::rotatePoint(bbox_.mid(), thing.position, rotation);

		thing.old_position = thing.position;
	}

	// Update bbox again for rotation if needed
	if (rotation != 0)
	{
		bbox_.reset();
		for (auto& vertex : vertices_)
			if (!vertex->ignored)
				bbox_.extend(vertex->position.x, vertex->position.y);
		for (auto& thing : things_)
			bbox_.extend(thing.position.x, thing.position.y);
		old_bbox_ = bbox_;
	}

	// Check if mirrored once (ie. lines need flipping)
	if ((mirror_x || mirror_y) && !(mirror_x && mirror_y))
		mirrored_ = true;
	else
		mirrored_ = false;
}

// -----------------------------------------------------------------------------
// Applies new group object positions to the actual map objects being edited
// -----------------------------------------------------------------------------
void ObjectEditGroup::applyEdit()
{
	// Get map
	SLADEMap* map;
	if (!vertices_.empty())
		map = vertices_[0]->map_vertex->parentMap();
	else if (!things_.empty())
		map = things_[0].map_thing->parentMap();
	else
		return;

	// Move vertices
	for (auto& vertex : vertices_)
		vertex->map_vertex->move(vertex->position.x, vertex->position.y);

	// Move things
	for (auto& thing : things_)
	{
		thing.map_thing->move(thing.position);
		thing.map_thing->setAngle(thing.angle);
	}

	// Flip lines if needed
	if (mirrored_)
	{
		for (auto& line : lines_)
		{
			if (!line.isExtra())
				line.map_line->flip(false);
		}
	}
}

// -----------------------------------------------------------------------------
// Adds all group vertices to [list]
// -----------------------------------------------------------------------------
void ObjectEditGroup::putMapVertices(vector<MapVertex*>& list)
{
	for (auto& vertex : vertices_)
	{
		if (!vertex->ignored)
			list.push_back(vertex->map_vertex);
	}
}


// -----------------------------------------------------------------------------
//
// ObjectEdit Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Begins an object edit operation
// -----------------------------------------------------------------------------
bool ObjectEdit::begin()
{
	// Things mode
	if (context_.editMode() == Mode::Things)
	{
		// Get selected things
		auto edit_objects = context_.selection().selectedObjects();

		// Setup object group
		group_.clear();
		for (auto& object : edit_objects)
			group_.addThing((MapThing*)object);

		// Filter objects
		group_.filterObjects(true);
	}
	else
	{
		vector<MapObject*> edit_objects;

		// Vertices mode
		if (context_.editMode() == Mode::Vertices)
		{
			// Get selected vertices
			edit_objects = context_.selection().selectedObjects();
		}

		// Lines mode
		else if (context_.editMode() == Mode::Lines)
		{
			// Get vertices of selected lines
			auto lines = context_.selection().selectedLines();
			for (auto& line : lines)
			{
				VECTOR_ADD_UNIQUE(edit_objects, line->v1());
				VECTOR_ADD_UNIQUE(edit_objects, line->v2());
			}
		}

		// Sectors mode
		else if (context_.editMode() == Mode::Sectors)
		{
			// Get vertices of selected sectors
			auto sectors = context_.selection().selectedSectors();
			for (auto& sector : sectors)
				sector->putVertices(edit_objects);
		}

		// Setup object group
		group_.clear();
		for (auto& object : edit_objects)
			group_.addVertex((MapVertex*)object);
		group_.addConnectedLines();

		// Filter objects
		group_.filterObjects(true);
	}

	if (group_.empty())
		return false;

	mapeditor::showObjectEditPanel(true, &group_);

	context_.input().setMouseState(Input::MouseState::ObjectEdit);
	context_.renderer().forceUpdate(true, false);

	// Setup help text
	auto key_accept = KeyBind::bind("map_edit_accept").keysAsString();
	auto key_cancel = KeyBind::bind("map_edit_cancel").keysAsString();
	auto key_toggle = KeyBind::bind("me2d_begin_object_edit").keysAsString();
	context_.setFeatureHelp({ "Object Edit",
							  fmt::format("{} = Accept", key_accept),
							  fmt::format("{} or {} = Cancel", key_cancel, key_toggle),
							  "Shift = Disable grid snapping",
							  "Ctrl = Rotate" });

	return true;
}

// -----------------------------------------------------------------------------
// Ends the object edit operation and applies changes if [accept] is true
// -----------------------------------------------------------------------------
void ObjectEdit::end(bool accept)
{
	// Un-filter objects
	group_.filterObjects(false);

	// Apply change if accepted
	if (accept)
	{
		// Begin recording undo level
		context_.beginUndoRecord(fmt::format("Edit {}", context_.modeString()));

		// Apply changes
		group_.applyEdit();

		// Do merge
		bool merge = true;
		if (context_.editMode() != Mode::Things)
		{
			// Begin extra 'Merge' undo step if wanted
			if (map_merge_undo_step)
			{
				context_.endUndoRecord(true);
				context_.beginUndoRecord("Merge");
			}

			vector<MapVertex*> vertices;
			group_.putMapVertices(vertices);
			merge = context_.map().mergeArch(vertices);
		}

		// Clear selection
		context_.selection().clear();

		context_.endUndoRecord(merge || !map_merge_undo_step);
	}

	mapeditor::showObjectEditPanel(false, nullptr);
	context_.setFeatureHelp({});
}

// -----------------------------------------------------------------------------
// Determines the current object edit state depending on the mouse cursor
// position relative to the object edit bounding box
// -----------------------------------------------------------------------------
void ObjectEdit::determineState()
{
	// Get object edit bbox
	auto bbox     = group_.bbox();
	int  bbox_pad = 8;
	int  left     = context_.renderer().view().screenX(bbox.min.x) - bbox_pad;
	int  right    = context_.renderer().view().screenX(bbox.max.x) + bbox_pad;
	int  top      = context_.renderer().view().screenY(bbox.max.y) - bbox_pad;
	int  bottom   = context_.renderer().view().screenY(bbox.min.y) + bbox_pad;
	rotating_     = context_.input().ctrlDown();

	// Check if mouse is outside the bbox
	auto mouse_pos = context_.input().mousePos();
	if (mouse_pos.x < left || mouse_pos.x > right || mouse_pos.y < top || mouse_pos.y > bottom)
	{
		// Outside bbox
		state_ = State::None;
		context_.setCursor(ui::MouseCursor::Normal);
		return;
	}

	// Left side
	if (mouse_pos.x < left + bbox_pad && bbox.width() > 0)
	{
		// Top left
		if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
		{
			state_ = State::TopLeft;
			context_.setCursor(rotating_ ? ui::MouseCursor::Cross : ui::MouseCursor::SizeNWSE);
		}

		// Bottom left
		else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
		{
			state_ = State::BottomLeft;
			context_.setCursor(rotating_ ? ui::MouseCursor::Cross : ui::MouseCursor::SizeNESW);
		}

		// Left
		else if (!rotating_)
		{
			state_ = State::Left;
			context_.setCursor(ui::MouseCursor::SizeWE);
		}
	}

	// Right side
	else if (mouse_pos.x > right - bbox_pad && bbox.width() > 0)
	{
		// Top right
		if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
		{
			state_ = State::TopRight;
			context_.setCursor(rotating_ ? ui::MouseCursor::Cross : ui::MouseCursor::SizeNESW);
		}

		// Bottom right
		else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
		{
			state_ = State::BottomRight;
			context_.setCursor(rotating_ ? ui::MouseCursor::Cross : ui::MouseCursor::SizeNWSE);
		}

		// Right
		else if (!rotating_)
		{
			state_ = State::Right;
			context_.setCursor(ui::MouseCursor::SizeWE);
		}
	}

	// Top
	else if (mouse_pos.y < top + bbox_pad && bbox.height() > 0 && !rotating_)
	{
		state_ = State::Top;
		context_.setCursor(ui::MouseCursor::SizeNS);
	}

	// Bottom
	else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0 && !rotating_)
	{
		state_ = State::Bottom;
		context_.setCursor(ui::MouseCursor::SizeNS);
	}

	// Middle
	else
	{
		state_ = rotating_ ? State::None : State::Move;
		context_.setCursor(rotating_ ? ui::MouseCursor::Normal : ui::MouseCursor::Move);
	}
}
