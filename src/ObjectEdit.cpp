
#include "Main.h"
#include "SLADEMap.h"
#include "ObjectEdit.h"
#include "MathStuff.h"

ObjectEditGroup::ObjectEditGroup()
{
	xoff_prev = 0;
	yoff_prev = 0;
}

ObjectEditGroup::~ObjectEditGroup()
{
}

void ObjectEditGroup::addVertex(MapVertex* vertex, bool ignored)
{
	// Add vertex
	vertex_t v;
	v.ignored = ignored;
	v.map_vertex = vertex;
	v.position.set(vertex->xPos(), vertex->yPos());
	v.old_position = v.position;
	vertices.push_back(v);

	if (!ignored)
	{
		bbox.extend(v.position.x, v.position.y);
		old_bbox.extend(v.position.x, v.position.y);
		original_bbox.extend(v.position.x, v.position.y);
	}
}

void ObjectEditGroup::addConnectedLines()
{
	unsigned n_vertices = vertices.size();
	for (unsigned a = 0; a < n_vertices; a++)
	{
		MapVertex* map_vertex = vertices[a].map_vertex;
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
					line.v1 = &(vertices.back());
				}
				if (!line.v2)
				{
					addVertex(map_line->v2(), true);
					line.v2 = &(vertices.back());
				}

				// Add line
				lines.push_back(line);
			}
		}
	}
}

void ObjectEditGroup::addThing(MapThing* thing)
{
	// Add thing
	thing_t t;
	t.map_thing = thing;
	t.position.x = thing->xPos();
	t.position.y = thing->yPos();
	t.old_position = t.position;
	things.push_back(t);

	// Update bbox
	bbox.extend(t.position.x, t.position.y);
	old_bbox.extend(t.position.x, t.position.y);
	original_bbox.extend(t.position.x, t.position.y);
}

bool ObjectEditGroup::hasLine(MapLine* line)
{
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a].map_line == line)
			return true;
	}

	return false;
}

ObjectEditGroup::vertex_t* ObjectEditGroup::findVertex(MapVertex* vertex)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a].map_vertex == vertex)
			return &(vertices[a]);
	}

	return NULL;
}

void ObjectEditGroup::clear()
{
	vertices.clear();
	lines.clear();
	things.clear();
	bbox.reset();
	old_bbox.reset();
	original_bbox.reset();
	xoff_prev = yoff_prev = 0;
}

void ObjectEditGroup::filterObjects(bool filter)
{
	// Vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a].ignored)
			vertices[a].map_vertex->filter(filter);
	}

	// Lines
	for (unsigned a = 0; a < lines.size(); a++)
		lines[a].map_line->filter(filter);

	// Things
	for (unsigned a = 0; a < things.size(); a++)
		things[a].map_thing->filter(filter);
}

void ObjectEditGroup::resetPositions()
{
	// Vertices
	for (unsigned a = 0; a < vertices.size(); a++)
		vertices[a].old_position = vertices[a].position;

	// Things
	for (unsigned a = 0; a < things.size(); a++)
		things[a].old_position = things[a].position;

	old_bbox = bbox;
}

void ObjectEditGroup::getVerticesToDraw(vector<fpoint2_t>& list)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a].ignored)
			list.push_back(vertices[a].position);
	}
}

void ObjectEditGroup::getLinesToDraw(vector<line_t>& list)
{
	for (unsigned a = 0; a < lines.size(); a++)
		list.push_back(lines[a]);
}

void ObjectEditGroup::getThingsToDraw(vector<thing_t>& list)
{
	for (unsigned a = 0; a < things.size(); a++)
		list.push_back(things[a]);
}

void ObjectEditGroup::doMove(double xoff, double yoff)
{
	if (xoff == xoff_prev && yoff == yoff_prev)
		return;

	// Update vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip ignored
		if (vertices[a].ignored)
			continue;

		vertices[a].position.x = vertices[a].old_position.x + xoff;
		vertices[a].position.y = vertices[a].old_position.y + yoff;
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
		if (vertices[a].ignored)
			continue;

		// Scale
		vertices[a].position.x = old_bbox.min.x + ((vertices[a].old_position.x - old_bbox.min.x) * xscale);
		vertices[a].position.y = old_bbox.min.y + ((vertices[a].old_position.y - old_bbox.min.y) * yscale);

		// Move
		vertices[a].position.x += xofs;
		vertices[a].position.y += yofs;
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

void ObjectEditGroup::doAll(double xoff, double yoff, double xscale, double yscale, double rotation)
{
	// Update bbox
	bbox = original_bbox;
	bbox.max.x = original_bbox.min.x + (original_bbox.width() * xscale);
	bbox.max.y = original_bbox.min.y + (original_bbox.height() * yscale);
	bbox.min.x += xoff;
	bbox.max.x += xoff;
	bbox.min.y += yoff;
	bbox.max.y += yoff;
	old_bbox = bbox;
	fpoint2_t bbox_midpoint(bbox.min.x + (bbox.width() * 0.5), bbox.min.y + (bbox.height() * 0.5));

	// Update vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip ignored
		if (vertices[a].ignored)
			continue;

		// Scale
		vertices[a].position.x = original_bbox.min.x + ((vertices[a].map_vertex->xPos() - original_bbox.min.x) * xscale);
		vertices[a].position.y = original_bbox.min.y + ((vertices[a].map_vertex->yPos() - original_bbox.min.y) * yscale);

		// Move
		vertices[a].position.x += xoff;
		vertices[a].position.y += yoff;

		// Rotate
		if (rotation != 0)
			vertices[a].position = MathStuff::rotatePoint(bbox_midpoint, vertices[a].position, -rotation);

		vertices[a].old_position = vertices[a].position;
	}

	// Update things
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Scale
		things[a].position.x = original_bbox.min.x + ((things[a].map_thing->xPos() - original_bbox.min.x) * xscale);
		things[a].position.y = original_bbox.min.y + ((things[a].map_thing->yPos() - original_bbox.min.y) * yscale);

		// Move
		things[a].position.x += xoff;
		things[a].position.y += yoff;

		// Rotate
		if (rotation != 0)
			things[a].position = MathStuff::rotatePoint(bbox_midpoint, things[a].position, -rotation);

		things[a].old_position = things[a].position;
	}

	// Update bbox again for rotation if needed
	if (rotation != 0)
	{
		bbox.reset();
		for (unsigned a = 0; a < vertices.size(); a++)
			bbox.extend(vertices[a].position.x, vertices[a].position.y);
		for (unsigned a = 0; a < things.size(); a++)
			bbox.extend(things[a].position.x, things[a].position.y);
		old_bbox = bbox;
	}
}

void ObjectEditGroup::applyEdit()
{
	// Get map
	SLADEMap* map = NULL;
	if (!vertices.empty())
		map = vertices[0].map_vertex->getParentMap();
	else if (!things.empty())
		map = things[0].map_thing->getParentMap();
	else
		return;

	// Move vertices
	for (unsigned a = 0; a < vertices.size(); a++)
		map->moveVertex(vertices[a].map_vertex->getIndex(), vertices[a].position.x, vertices[a].position.y);

	// Move things
	for (unsigned a = 0; a < things.size(); a++)
		map->moveThing(things[a].map_thing->getIndex(), things[a].position.x, things[a].position.y);
}
