#pragma once

namespace slade::map
{
class SlopeSpecials;
class ExtraFloorSpecials;
struct ExtraFloor;

class MapSpecials
{
public:
	explicit MapSpecials(SLADEMap& map);
	~MapSpecials();

	SlopeSpecials&            slopeSpecials() const { return *slope_specials_; }
	ExtraFloorSpecials&       extraFloorSpecials() const { return *extrafloor_specials_; }
	const vector<ExtraFloor>& sectorExtraFloors(const MapSector* sector) const;
	bool                      sectorHasExtraFloors(const MapSector* sector) const;

	void processAllSpecials();
	void processLineSpecial(const MapLine& line) const;
	void processThing(const MapThing& thing) const;

	void lineUpdated(const MapLine& line) const;
	void sectorUpdated(MapSector& sector) const;
	void thingUpdated(const MapThing& thing) const;
	void objectUpdated(MapObject& object) const;
	void objectsUpdated(const vector<MapObject*>& objects);

private:
	SLADEMap*                      map_         = nullptr;
	bool                           bulk_update_ = false;
	unique_ptr<SlopeSpecials>      slope_specials_;
	unique_ptr<ExtraFloorSpecials> extrafloor_specials_;
};
} // namespace slade::map
