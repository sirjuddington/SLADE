#pragma once

#include "MapObjectList.h"
#include "SLADEMap/MapObject/MapVertex.h"

namespace slade
{
class VertexList : public MapObjectList<MapVertex>
{
public:
	MapVertex* nearest(Vec2d point, double min = 64) const;
	MapVertex* vertexAt(double x, double y) const;
	MapVertex* firstCrossed(const Seg2d& line) const;
};
} // namespace slade
