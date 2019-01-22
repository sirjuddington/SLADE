#pragma once

#include "MapObjectList.h"
#include "SLADEMap/MapObject/MapVertex.h"

class VertexList : public MapObjectList<MapVertex>
{
public:
	MapVertex* nearest(Vec2f point, double min = 64) const;
	MapVertex* vertexAt(double x, double y) const;
	MapVertex* firstCrossed(const Seg2f& line) const;
};
