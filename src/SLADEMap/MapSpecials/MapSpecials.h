#pragma once

namespace slade
{
class SlopeSpecials;

class MapSpecials
{
public:
	explicit MapSpecials(SLADEMap& map);
	~MapSpecials();

	void processAllSpecials();
	void processLineSpecial(const MapLine& line);
	void processThing(const MapThing& thing);

	void lineUpdated(const MapLine& line);
	void sectorUpdated(MapSector& sector);
	void thingUpdated(const MapThing& thing);
	void objectUpdated(MapObject& object);
	void objectsUpdated(const vector<MapObject*>& objects);

private:
	SLADEMap*                 map_         = nullptr;
	bool                      bulk_update_ = false;
	unique_ptr<SlopeSpecials> slope_specials_;
};
} // namespace slade
