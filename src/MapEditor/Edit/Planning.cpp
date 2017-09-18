
#include "Main.h"
#include "MapEditor/MapEditContext.h"

using namespace MapEditor;

Planning::Planning(MapEditContext& context) :
	context_{ context }
{
}

void Planning::createLines(vector<fpoint2_t>& points)
{
	for (auto a = 0u; a < points.size() - 1; a++)
	{
		auto v1 = createVertex(points[a].x, points[a].y);
		auto v2 = createVertex(points[a + 1].x, points[a + 1].y);
		lines_.push_back(std::make_unique<MapLine>(v1, v2, nullptr, nullptr));
	}

	updateIndices();
}

MapVertex* Planning::createVertex(double x, double y)
{
	// Check if a vertex already exists at x,y
	for (auto& vertex : vertices_)
		if (vertex->xPos() == x && vertex->yPos() == y)
			return vertex.get();

	// Create vertex
	vertices_.push_back(std::make_unique<MapVertex>(x, y));
	return vertices_.back().get();
}

bool Planning::deleteVertex(MapVertex* vertex)
{
	// Delete lines attached to the vertex
	for (auto a = 0u; a < lines_.size(); ++a)
		if (lines_[a]->v1() == vertex || lines_[a]->v2() == vertex)
		{
			lines_[a]->disconnectFromVertices();
			lines_.erase(lines_.begin() + a);
			a--;
		}

	// Delete the vertex
	bool deleted = false;
	for (auto i = vertices_.begin(); i != vertices_.end(); ++i)
		if (i->get() == vertex)
		{
			vertices_.erase(i);
			deleted = true;
			break;
		}

	updateIndices();
	return deleted;
}

bool Planning::deleteLine(MapLine* line)
{
	// Delete the line
	bool deleted = false;
	for (auto i = lines_.begin(); i != lines_.end(); ++i)
		if (i->get() == line)
		{
			line->disconnectFromVertices();
			lines_.erase(i);
			deleted = true;
			break;
		}

	return deleted;
}

MapObject* Planning::nearestObject(fpoint2_t point, double min)
{
	double min_dist = 999999999;

	// Find nearest planning vertex
	double dist = 0;
	MapVertex* nearest_vertex = nullptr;
	for (auto& vertex : vertices_)
	{
		dist = point.distance_to(vertex->pos());
		if (dist <= min && dist < min_dist)
		{
			nearest_vertex = vertex.get();
			min_dist = dist;
		}
	}

	if (nearest_vertex)
		return nearest_vertex;

	// Find nearest planning line
	MapLine* nearest_line = nullptr;
	for (auto& line : lines_)
	{
		// Check with line bounding box first (since we have a minimum distance)
		fseg2_t bbox = line->seg();
		bbox.expand(min, min);
		if (!bbox.contains(point))
			continue;

		// Calculate distance to line
		dist = line->distanceTo(point);

		// Check if it's nearer than the previous nearest
		if (dist < min_dist && dist <= min)
		{
			nearest_vertex = nullptr;
			nearest_line = line.get();
			min_dist = dist;
		}
	}

	return nearest_vertex ? (MapObject*)nearest_vertex : (MapObject*)nearest_line;
}

void Planning::updateIndices()
{
	for (auto a = 0u; a < vertices_.size(); a++)
		vertices_[a]->setIndex(a);

	for (auto a = 0u; a < lines_.size(); a++)
		lines_[a]->setIndex(a);
}

void Planning::deleteDetachedVertices()
{
	for (auto a = 0u; a < vertices_.size(); ++a)
		if (vertices_[a].get()->nConnectedLines() == 0)
		{
			vertices_.erase(vertices_.begin() + a);
			--a;
		}

	updateIndices();
}
