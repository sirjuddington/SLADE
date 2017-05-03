
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ObjectEdit.cpp
 * Description: ObjectEditGroup class - used for the object edit
 *              feature in the map editor, takes a bunch of vertices
 *              or things and applies rotation/translation/scaling
 *              to them. Also keeps track of any connected lines
 *              for visual purposes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "ObjectEdit.h"
#include "Utility/MathStuff.h"
#include "General/KeyBind.h"
#include "General/UI.h"

using namespace MapEditor;


EXTERN_CVAR(Bool, map_merge_undo_step)


/*******************************************************************
 * OBJECTEDITGROUP CLASS FUNCTIONS
 *******************************************************************/

/* ObjectEditGroup::ObjectEditGroup
 * ObjectEditGroup class constructor
 *******************************************************************/
ObjectEditGroup::ObjectEditGroup()
{
	xoff_prev = 0;
	yoff_prev = 0;
	rotation = 0;
	mirrored = false;
}

/* ObjectEditGroup::~ObjectEditGroup
 * ObjectEditGroup class destructor
 *******************************************************************/
ObjectEditGroup::~ObjectEditGroup()
{
}

/* ObjectEditGroup::addVertex
 * Adds [vertex] to the group. If [ignored] is set, the vertex won't
 * be modified by the object edit
 *******************************************************************/
void ObjectEditGroup::addVertex(MapVertex* vertex, bool ignored)
{
	// Add vertex
	vertex_t* v = new vertex_t();
	v->ignored = ignored;
	v->map_vertex = vertex;
	v->position.set(vertex->xPos(), vertex->yPos());
	v->old_position = v->position;
	vertices.push_back(v);

	if (!ignored)
	{
		bbox.extend(v->position.x, v->position.y);
		old_bbox.extend(v->position.x, v->position.y);
		original_bbox.extend(v->position.x, v->position.y);
	}
}

/* ObjectEditGroup::addConnectedLines
 * Builds a list of all lines connected to the group vertices
 *******************************************************************/
void ObjectEditGroup::addConnectedLines()
{
	for (auto vertex : vertices)
	{
		MapVertex* map_vertex = vertex->map_vertex;
		for (unsigned l = 0; l < map_vertex->nConnectedLines(); l++)
		{
			MapLine* map_line = map_vertex->connectedLine(l);
			if (!hasLine(map_line))
			{
				// Create line
				line_t line;
				line.map_line = map_line;
				line.v1 = findVertex(map_line->v1());
				line.v2 = findVertex(map_line->v2());

				// Add extra vertex if needed (will be ignored for editing)
				if (!line.v1)
				{
					addVertex(map_line->v1(), true);
					line.v1 = vertices.back();
				}
				if (!line.v2)
				{
					addVertex(map_line->v2(), true);
					line.v2 = vertices.back();
				}

				// Add line
				lines.push_back(line);
			}
		}
	}
}

/* ObjectEditGroup::addThing
 * Adds [thing] to the group
 *******************************************************************/
void ObjectEditGroup::addThing(MapThing* thing)
{
	// Add thing
	thing_t t;
	t.map_thing = thing;
	t.position.x = thing->xPos();
	t.position.y = thing->yPos();
	t.old_position = t.position;
	t.angle = thing->getAngle();
	things.push_back(t);

	// Update bbox
	bbox.extend(t.position.x, t.position.y);
	old_bbox.extend(t.position.x, t.position.y);
	original_bbox.extend(t.position.x, t.position.y);
}

/* ObjectEditGroup::hasLine
 * Returns true if [line] is connected to the group vertices
 *******************************************************************/
bool ObjectEditGroup::hasLine(MapLine* line)
{
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a].map_line == line)
			return true;
	}

	return false;
}

/* ObjectEditGroup::findVertex
 * Returns the group info about [vertex]
 *******************************************************************/
ObjectEditGroup::vertex_t* ObjectEditGroup::findVertex(MapVertex* vertex)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a]->map_vertex == vertex)
			return vertices[a];
	}

	return NULL;
}

/* ObjectEditGroup::clear
 * Clears all group items
 *******************************************************************/
void ObjectEditGroup::clear()
{
	vertices.clear();
	lines.clear();
	things.clear();
	bbox.reset();
	old_bbox.reset();
	original_bbox.reset();
	xoff_prev = yoff_prev = 0;
	rotation = 0;
}

/* ObjectEditGroup::filterObjects
 * Sets filtering on all group objects to [filter]
 *******************************************************************/
void ObjectEditGroup::filterObjects(bool filter)
{
	// Vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a]->ignored)
			vertices[a]->map_vertex->filter(filter);
	}

	// Lines
	for (unsigned a = 0; a < lines.size(); a++)
		lines[a].map_line->filter(filter);

	// Things
	for (unsigned a = 0; a < things.size(); a++)
		things[a].map_thing->filter(filter);
}

/* ObjectEditGroup::resetPositions
 * Resets the position of all group objects to their original
 * positions (ie. current position on the actual map)
 *******************************************************************/
void ObjectEditGroup::resetPositions()
{
	bbox.reset();

	// Vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		vertices[a]->old_position = vertices[a]->position;

		if (!vertices[a]->ignored)
			bbox.extend(vertices[a]->position.x, vertices[a]->position.y);
	}

	// Things
	for (unsigned a = 0; a < things.size(); a++)
	{
		things[a].old_position = things[a].position;
		bbox.extend(things[a].position.x, things[a].position.y);
	}

	old_bbox = bbox;
	rotation = 0;
}

/* ObjectEditGroup::getNearestLine
 * Finds the nearest line to [pos] (that is closer than [min] in
 * distance), and sets [v1]/[v2] to the line vertices. Returns true
 * if a line was found within the distance specified
 *******************************************************************/
bool ObjectEditGroup::getNearestLine(fpoint2_t pos, double min, fpoint2_t& v1, fpoint2_t& v2)
{
	double min_dist = min;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		double d = MathStuff::distanceToLineFast(
			pos, fseg2_t(lines[a].v1->position, lines[a].v2->position));

		if (d < min_dist)
		{
			min_dist = d;
			v1.set(lines[a].v1->position);
			v2.set(lines[a].v2->position);
		}
	}

	return (min_dist < min);
}

/* ObjectEditGroup::getVerticesToDraw
 * Fills [list] with the positions of all group vertices
 *******************************************************************/
void ObjectEditGroup::getVerticesToDraw(vector<fpoint2_t>& list)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a]->ignored)
			list.push_back(vertices[a]->position);
	}
}

/* ObjectEditGroup::getLinesToDraw
 * Fills [list] with all lines in the group
 *******************************************************************/
void ObjectEditGroup::getLinesToDraw(vector<line_t>& list)
{
	for (unsigned a = 0; a < lines.size(); a++)
		list.push_back(lines[a]);
}

/* ObjectEditGroup::getThingsToDraw
 * Fills [list] with all things in the group
 *******************************************************************/
void ObjectEditGroup::getThingsToDraw(vector<thing_t>& list)
{
	for (unsigned a = 0; a < things.size(); a++)
		list.push_back(things[a]);
}

/* ObjectEditGroup::doMove
 * Moves all group objects by [xoff,yoff]
 *******************************************************************/
void ObjectEditGroup::doMove(double xoff, double yoff)
{
	if (xoff == xoff_prev && yoff == yoff_prev)
		return;

	// Update vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip ignored
		if (vertices[a]->ignored)
			continue;

		vertices[a]->position.x = vertices[a]->old_position.x + xoff;
		vertices[a]->position.y = vertices[a]->old_position.y + yoff;
	}

	// Update things
	for (unsigned a = 0; a < things.size(); a++)
	{
		things[a].position.x = things[a].old_position.x + xoff;
		things[a].position.y = things[a].old_position.y + yoff;
	}

	// Update bbox
	bbox.max.x = old_bbox.max.x + xoff;
	bbox.max.y = old_bbox.max.y + yoff;
	bbox.min.x = old_bbox.min.x + xoff;
	bbox.min.y = old_bbox.min.y + yoff;

	xoff_prev = xoff;
	yoff_prev = yoff;
}

/* ObjectEditGroup::doScale
 * Modifies the group bounding box by [xoff]/[yoff], and scales all
 * objects to fit within the resulting bbox. This is used when
 * dragging bbox edges via the mouse
 *******************************************************************/
void ObjectEditGroup::doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom)
{
	if (xoff == xoff_prev && yoff == yoff_prev)
		return;

	// Update bbox
	if (left)
	{
		// Check for left >= right
		if (old_bbox.min.x + xoff >= old_bbox.max.x)
			return;
		else
			bbox.min.x = old_bbox.min.x + xoff;
	}
	if (right)
	{
		// Check for right <= left
		if (old_bbox.max.x + xoff <= old_bbox.min.x)
			return;
		else
			bbox.max.x = old_bbox.max.x + xoff;
	}
	if (top)
	{
		// Check for top <= bottom
		if (old_bbox.max.y + yoff <= old_bbox.min.y)
			return;
		else
			bbox.max.y = old_bbox.max.y + yoff;
	}
	if (bottom)
	{
		// Check for bottom >= top
		if (old_bbox.min.y + yoff >= old_bbox.max.y)
			return;
		else
			bbox.min.y = old_bbox.min.y + yoff;
	}

	// Determine offset and scale values
	double xofs = bbox.min.x - old_bbox.min.x;
	double yofs = bbox.min.y - old_bbox.min.y;
	double xscale = 1;
	double yscale = 1;
	if (old_bbox.width() > 0)
		xscale = bbox.width() / old_bbox.width();
	if (old_bbox.height() > 0)
		yscale = bbox.height() / old_bbox.height();

	// Update vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip ignored
		if (vertices[a]->ignored)
			continue;

		// Scale
		vertices[a]->position.x = old_bbox.min.x + ((vertices[a]->old_position.x - old_bbox.min.x) * xscale);
		vertices[a]->position.y = old_bbox.min.y + ((vertices[a]->old_position.y - old_bbox.min.y) * yscale);

		// Move
		vertices[a]->position.x += xofs;
		vertices[a]->position.y += yofs;
	}

	// Update things
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Scale
		things[a].position.x = old_bbox.min.x + ((things[a].old_position.x - old_bbox.min.x) * xscale);
		things[a].position.y = old_bbox.min.y + ((things[a].old_position.y - old_bbox.min.y) * yscale);

		// Move
		things[a].position.x += xofs;
		things[a].position.y += yofs;
	}

	xoff_prev = xoff;
	yoff_prev = yoff;
}

/* ObjectEditGroup::doRotate
 * Rotates all objects in the group. The rotation angle is calculated
 * from [p1]->mid and mid->[p2]. This is used when rotating via the
 * mouse ([p1] is the drag origin and [p2] is the current point)
 *******************************************************************/
void ObjectEditGroup::doRotate(fpoint2_t p1, fpoint2_t p2, bool lock45)
{
	// Get midpoint
	fpoint2_t mid(old_bbox.min.x + old_bbox.width() * 0.5, old_bbox.min.y + old_bbox.height() * 0.5);

	// Determine angle
	double angle = MathStuff::angle2DRad(p1, mid, p2);
	rotation = MathStuff::radToDeg(angle);

	// Lock to 45 degree increments if needed
	if (lock45)
	{
		rotation = ceil(rotation / 45.0 - 0.5) * 45.0;
		if (rotation > 325 || rotation < 0)
			rotation = 0;
	}

	// Rotate vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a]->ignored)
			vertices[a]->position = MathStuff::rotatePoint(mid, vertices[a]->old_position, rotation);
	}

	// Rotate things
	for (unsigned a = 0; a < things.size(); a++)
		things[a].position = MathStuff::rotatePoint(mid, things[a].old_position, rotation);
}

/* ObjectEditGroup::doAll
 * Moves all group objects by [xoff,yoff], scales all group objects
 * by [xscale,yscale] and rotates all group objects by [rotation]
 *******************************************************************/
void ObjectEditGroup::doAll(double xoff, double yoff, double xscale, double yscale, double rotation, bool mirror_x, bool mirror_y)
{
	// Update bbox
	bbox = original_bbox;
	
	// Apply offsets
	bbox.min.x += xoff;
	bbox.max.x += xoff;
	bbox.min.y += yoff;
	bbox.max.y += yoff;

	// Apply scale (from center)
	double xgrow = (bbox.width() * xscale) - bbox.width();
	double ygrow = (bbox.height() * yscale) - bbox.height();
	bbox.min.x -= (xgrow * 0.5);
	bbox.max.x += (xgrow * 0.5);
	bbox.min.y -= (ygrow * 0.5);
	bbox.max.y += (ygrow * 0.5);
	old_bbox = bbox;


	// Update vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip ignored
		if (vertices[a]->ignored)
			continue;

		// Reset first
		vertices[a]->position.x = vertices[a]->map_vertex->xPos();
		vertices[a]->position.y = vertices[a]->map_vertex->yPos();

		// Mirror
		if (mirror_x)
			vertices[a]->position.x = original_bbox.mid_x() - (vertices[a]->position.x - original_bbox.mid_x());
		if (mirror_y)
			vertices[a]->position.y = original_bbox.mid_y() - (vertices[a]->position.y - original_bbox.mid_y());

		// Scale
		vertices[a]->position.x = original_bbox.mid_x() + ((vertices[a]->position.x - original_bbox.mid_x()) * xscale);
		vertices[a]->position.y = original_bbox.mid_y() + ((vertices[a]->position.y - original_bbox.mid_y()) * yscale);

		// Move
		vertices[a]->position.x += xoff;
		vertices[a]->position.y += yoff;

		// Rotate
		if (rotation != 0)
			vertices[a]->position = MathStuff::rotatePoint(bbox.mid(), vertices[a]->position, rotation);

		vertices[a]->old_position = vertices[a]->position;
	}


	// Update things
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Reset first
		things[a].position.x = things[a].map_thing->xPos();
		things[a].position.y = things[a].map_thing->yPos();
		things[a].angle = things[a].map_thing->getAngle();

		// Mirror
		if (mirror_x)
		{
			things[a].position.x = original_bbox.mid_x() - (things[a].position.x - original_bbox.mid_x());
			things[a].angle += 90;
			things[a].angle = 360 - things[a].angle;
			things[a].angle -= 90;
			while (things[a].angle < 0) things[a].angle += 360;
		}
		if (mirror_y)
		{
			things[a].position.y = original_bbox.mid_y() - (things[a].position.y - original_bbox.mid_y());
			things[a].angle = 360 - things[a].angle;
			while (things[a].angle < 0) things[a].angle += 360;
		}

		// Scale
		things[a].position.x = original_bbox.min.x + ((things[a].position.x - original_bbox.min.x) * xscale);
		things[a].position.y = original_bbox.min.y + ((things[a].position.y - original_bbox.min.y) * yscale);

		// Move
		things[a].position.x += xoff;
		things[a].position.y += yoff;

		// Rotate
		if (rotation != 0)
			things[a].position = MathStuff::rotatePoint(bbox.mid(), things[a].position, rotation);

		things[a].old_position = things[a].position;
	}

	// Update bbox again for rotation if needed
	if (rotation != 0)
	{
		bbox.reset();
		for (unsigned a = 0; a < vertices.size(); a++)
		{
			if (!vertices[a]->ignored)
				bbox.extend(vertices[a]->position.x, vertices[a]->position.y);
		}
		for (unsigned a = 0; a < things.size(); a++)
			bbox.extend(things[a].position.x, things[a].position.y);
		old_bbox = bbox;
	}

	// Check if mirrored once (ie. lines need flipping)
	if ((mirror_x || mirror_y) && !(mirror_x && mirror_y))
		mirrored = true;
	else
		mirrored = false;
}

/* ObjectEditGroup::applyEdit
 * Applies new group object positions to the actual map objects
 * being edited
 *******************************************************************/
void ObjectEditGroup::applyEdit()
{
	// Get map
	SLADEMap* map = NULL;
	if (!vertices.empty())
		map = vertices[0]->map_vertex->getParentMap();
	else if (!things.empty())
		map = things[0].map_thing->getParentMap();
	else
		return;

	// Move vertices
	for (auto vertex : vertices)
		map->moveVertex(vertex->map_vertex->getIndex(), vertex->position.x, vertex->position.y);

	// Move things
	for (auto& thing : things)
	{
		map->moveThing(thing.map_thing->getIndex(), thing.position.x, thing.position.y);
		thing.map_thing->setIntProperty("angle", thing.angle);
	}

	// Flip lines if needed
	if (mirrored)
	{
		for (auto& line : lines)
		{
			if (!line.isExtra())
				line.map_line->flip(false);
		}
	}
}

/* ObjectEditGroup::getVertices
 * Adds all group vertices to [list]
 *******************************************************************/
void ObjectEditGroup::getVertices(vector<MapVertex*>& list)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a]->ignored)
			list.push_back(vertices[a]->map_vertex);
	}
}





ObjectEdit::ObjectEdit(MapEditContext& context) :
	context_{ context },
	state_{ State::None },
	rotating_{ false }
{
}

bool ObjectEdit::stateLeft(bool move) const
{
	return (state_ == State::Left ||
		state_ == State::TopLeft ||
		state_ == State::BottomLeft ||
		(move && state_ == State::Move));
}

bool ObjectEdit::stateTop(bool move) const
{
	return (state_ == State::Top ||
		state_ == State::TopLeft ||
		state_ == State::TopRight ||
		(move && state_ == State::Move));
}

bool ObjectEdit::stateRight(bool move) const
{
	return (state_ == State::Right ||
		state_ == State::TopRight ||
		state_ == State::BottomRight ||
		(move && state_ == State::Move));
}

bool ObjectEdit::stateBottom(bool move) const
{
	return (state_ == State::Bottom ||
		state_ == State::BottomRight ||
		state_ == State::BottomLeft ||
		(move && state_ == State::Move));
}


/* ObjectEdit::begin
 * Begins an object edit operation
 *******************************************************************/
bool ObjectEdit::begin()
{
	// Things mode
	if (context_.editMode() == Mode::Things)
	{
		// Get selected things
		auto edit_objects = context_.selection().selectedObjects();

		// Setup object group
		group_.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			group_.addThing((MapThing*)edit_objects[a]);

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
			for (unsigned a = 0; a < lines.size(); a++)
			{
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v1());
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v2());
			}
		}

		// Sectors mode
		else if (context_.editMode() == Mode::Sectors)
		{
			// Get vertices of selected sectors
			auto sectors = context_.selection().selectedSectors();
			for (unsigned a = 0; a < sectors.size(); a++)
				sectors[a]->getVertices(edit_objects);
		}

		// Setup object group
		group_.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			group_.addVertex((MapVertex*)edit_objects[a]);
		group_.addConnectedLines();

		// Filter objects
		group_.filterObjects(true);
	}

	if (group_.empty())
		return false;

	MapEditor::showObjectEditPanel(true, &group_);

	context_.input().setMouseState(Input::MouseState::ObjectEdit);
	context_.renderer().renderer2D().forceUpdate();

	// Setup help text
	string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
	string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
	string key_toggle = KeyBind::getBind("me2d_begin_object_edit").keysAsString();
	context_.setFeatureHelp({
		"Object Edit",
		S_FMT("%s = Accept", key_accept),
		S_FMT("%s or %s = Cancel", key_cancel, key_toggle),
		"Shift = Disable grid snapping",
		"Ctrl = Rotate"
	});

	return true;
}

/* ObjectEdit::end
 * Ends the object edit operation and applies changes if [accept] is
 * true
 *******************************************************************/
void ObjectEdit::end(bool accept)
{
	// Un-filter objects
	group_.filterObjects(false);

	// Apply change if accepted
	if (accept)
	{
		// Begin recording undo level
		context_.beginUndoRecord(S_FMT("Edit %s", context_.modeString()));

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
			group_.getVertices(vertices);
			merge = context_.map().mergeArch(vertices);
		}

		// Clear selection
		context_.selection().clear();

		context_.endUndoRecord(merge || !map_merge_undo_step);
	}

	MapEditor::showObjectEditPanel(false, nullptr);
	context_.setFeatureHelp({});
}

/* ObjectEdit::determineState
 * Determines the current object edit state depending on the mouse
 * cursor position relative to the object edit bounding box
 *******************************************************************/
void ObjectEdit::determineState()
{
	// Get object edit bbox
	auto bbox = group_.getBBox();
	int bbox_pad = 8;
	int left = context_.renderer().view().screenX(bbox.min.x) - bbox_pad;
	int right = context_.renderer().view().screenX(bbox.max.x) + bbox_pad;
	int top = context_.renderer().view().screenY(bbox.max.y) - bbox_pad;
	int bottom = context_.renderer().view().screenY(bbox.min.y) + bbox_pad;
	rotating_ = context_.input().ctrlDown();

	// Check if mouse is outside the bbox
	auto mouse_pos = context_.input().mousePos();
	if (mouse_pos.x < left || mouse_pos.x > right || mouse_pos.y < top || mouse_pos.y > bottom)
	{
		// Outside bbox
		state_ = State::None;
		context_.setCursor(UI::MouseCursor::Normal);
		return;
	}

	// Left side
	if (mouse_pos.x < left + bbox_pad && bbox.width() > 0)
	{
		// Top left
		if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
		{
			state_ = State::TopLeft;
			context_.setCursor(rotating_ ? UI::MouseCursor::Cross : UI::MouseCursor::SizeNWSE);
		}
			
		// Bottom left
		else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
		{
			state_ = State::BottomLeft;
			context_.setCursor(rotating_ ? UI::MouseCursor::Cross : UI::MouseCursor::SizeNESW);
		}
			
		// Left
		else if (!rotating_)
		{
			state_ = State::Left;
			context_.setCursor(UI::MouseCursor::SizeWE);
		}
	}

	// Right side
	else if (mouse_pos.x > right - bbox_pad && bbox.width() > 0)
	{
		// Top right
		if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
		{
			state_ = State::TopRight;
			context_.setCursor(rotating_ ? UI::MouseCursor::Cross : UI::MouseCursor::SizeNESW);
		}

		// Bottom right
		else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
		{
			state_ = State::BottomRight;
			context_.setCursor(rotating_ ? UI::MouseCursor::Cross : UI::MouseCursor::SizeNWSE);
		}

		// Right
		else if (!rotating_)
		{
			state_ = State::Right;
			context_.setCursor(UI::MouseCursor::SizeWE);
		}
	}

	// Top
	else if (mouse_pos.y < top + bbox_pad && bbox.height() > 0 && !rotating_)
	{
		state_ = State::Top;
		context_.setCursor(UI::MouseCursor::SizeNS);
	}

	// Bottom
	else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0 && !rotating_)
	{
		state_ = State::Bottom;
		context_.setCursor(UI::MouseCursor::SizeNS);
	}

	// Middle
	else
	{
		state_ = rotating_ ? State::None : State::Move;
		context_.setCursor(rotating_ ? UI::MouseCursor::Normal : UI::MouseCursor::Move);
	}
}
