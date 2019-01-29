#pragma once

#include "General/Defs.h"
#include "MapObjectList.h"
#include "SLADEMap/MapObject/MapLine.h"

class LineList : public MapObjectList<MapLine>
{
public:
	MapLine*         nearest(Vec2d point, double min = 64) const;
	MapLine*         withVertices(MapVertex* v1, MapVertex* v2, bool reverse = true) const;
	vector<Vec2d>    cutPoints(const Seg2d& cutter) const;
	MapLine*         firstWithId(int id) const;
	void             putAllWithId(int id, vector<MapLine*>& list) const;
	vector<MapLine*> allWithId(int id) const;
	void             putAllTaggingWithId(int id, int type, vector<MapLine*>& list) const;
	int              firstFreeId(MapFormat format) const;
};
