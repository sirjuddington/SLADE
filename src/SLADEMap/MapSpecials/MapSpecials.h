#pragma once

namespace slade
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

	void processAllSpecials();
	void processLineSpecial(const MapLine& line);
	void processThing(const MapThing& thing);

	void lineUpdated(const MapLine& line);
	void sectorUpdated(MapSector& sector);
	void thingUpdated(const MapThing& thing);
	void objectUpdated(MapObject& object);
	void objectsUpdated(const vector<MapObject*>& objects);

private:
	SLADEMap*                      map_         = nullptr;
	bool                           bulk_update_ = false;
	unique_ptr<SlopeSpecials>      slope_specials_;
	unique_ptr<ExtraFloorSpecials> extrafloor_specials_;
};
} // namespace slade
