#pragma once

#include "MapObjectList.h"

namespace slade
{
class ThingList : public MapObjectList<MapThing>
{
public:
	MapThing*         nearest(const Vec2d& point, double min = 64) const;
	vector<MapThing*> multiNearest(const Vec2d& point) const;
	BBox              allThingBounds() const;
	void              putAllWithId(int id, vector<MapThing*>& list, unsigned start = 0, int type = 0) const;
	vector<MapThing*> allWithId(int id, unsigned start = 0, int type = 0) const;
	MapThing*         firstWithId(int id, unsigned start = 0, int type = 0, bool ignore_dragon = false) const;
	void              putAllPathed(vector<MapThing*>& list) const;
	void              putAllTaggingWithId(int id, int type, vector<MapThing*>& list, int ttype) const;
	int               firstFreeId() const;
};
} // namespace slade
