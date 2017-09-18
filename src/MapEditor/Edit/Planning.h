#pragma once

#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapVertex.h"

class MapEditContext;

namespace MapEditor
{
	
class Planning
{
public:
	struct Note
	{
		point2_t	position;
		string		text;
		rgba_t		colour;
		string		icon;
		point2_t	target;
	};

	Planning(MapEditContext& context);

	const vector<MapVertex::UPtr>&	vertices() const { return vertices_; }
	const vector<MapLine::UPtr>&	lines() const { return lines_; }

	MapLine*	line(unsigned index) const { return index < lines_.size() ? lines_[index].get() : nullptr; }
	MapVertex*	vertex(unsigned index) const { return index < vertices_.size() ? vertices_[index].get() : nullptr; }

	void		createLines(vector<fpoint2_t>& points);
	MapVertex*	createVertex(double x, double y);

	bool	deleteVertex(MapVertex* vertex);
	bool	deleteLine(MapLine* line);

	MapObject*	nearestObject(fpoint2_t point, double min);
	void		updateIndices();
	void		deleteDetachedVertices();

private:
	MapEditContext&			context_;
	vector<MapVertex::UPtr>	vertices_;
	vector<MapLine::UPtr>	lines_;
};

}
