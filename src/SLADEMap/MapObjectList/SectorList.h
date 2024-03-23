#pragma once

#include "MapObjectList.h"

namespace slade
{
class MapSector;

class SectorList : public MapObjectList<MapSector>
{
public:
	// MapObjectList overrides
	void clear() override;
	void add(MapSector* sector) override;
	void remove(unsigned index) override;

	MapSector*         atPos(const Vec2d& point) const;
	BBox               allSectorBounds() const;
	void               initPolygons() const;
	void               initBBoxes() const;
	void               putAllWithId(int id, vector<MapSector*>& list) const;
	vector<MapSector*> allWithId(int id) const;
	MapSector*         firstWithId(int id) const;
	int                firstFreeId() const;

	void clearTexUsage() const { usage_tex_.clear(); }
	void updateTexUsage(string_view tex, int adjust) const;
	int  texUsageCount(string_view tex) const;

private:
	mutable std::map<string, int> usage_tex_;
};
} // namespace slade
