#pragma once

#include "MapObjectList.h"

namespace slade
{
enum class MapFormat;

class LineList : public MapObjectList<MapLine>
{
public:
	MapLine*         nearest(const Vec2d& point, double min = 64) const;
	MapLine*         withVertices(const MapVertex* v1, const MapVertex* v2, bool reverse = true) const;
	vector<Vec2d>    cutPoints(const Seg2d& cutter) const;
	MapLine*         firstWithId(int id) const;
	void             putAllWithId(int id, vector<MapLine*>& list) const;
	vector<MapLine*> allWithId(int id) const;
	void             putAllTaggingWithId(int id, int type, vector<MapLine*>& list) const;
	int              firstFreeId(MapFormat format) const;
};
} // namespace slade
