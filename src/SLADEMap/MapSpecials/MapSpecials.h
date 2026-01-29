#pragma once

namespace slade::map
{
enum class SidePart : u8;
enum class SectorPart : u8;
class RenderSpecials;
class SlopeSpecials;
class ExtraFloorSpecials;
struct ExtraFloor;
struct LineTranslucency;

class MapSpecials
{
public:
	explicit MapSpecials(SLADEMap& map);
	~MapSpecials();

	SlopeSpecials&      slopeSpecials() const { return *slope_specials_; }
	ExtraFloorSpecials& extraFloorSpecials() const { return *extrafloor_specials_; }

	const vector<ExtraFloor>& sectorExtraFloors(const MapSector* sector) const;
	bool                      sectorHasExtraFloors(const MapSector* sector) const;
	ColRGBA                   sectorColour(const MapSector& sector, SectorPart where) const;
	float                     sectorFloorHeightAt(const MapSector& sector, Vec3d pos) const;

	optional<LineTranslucency> lineTranslucency(const MapLine& line) const;

	ColRGBA sideColour(const MapSide& side, SidePart where, bool fullbright = false) const;

	void processAllSpecials() const;
	void updateSpecials() const;

	void objectUpdated(MapObject& object) const;
	void objectsUpdated(const vector<MapObject*>& objects) const;

	long specialsLastUpdated() const { return specials_updated_; }

private:
	SLADEMap*                      map_ = nullptr;
	unique_ptr<SlopeSpecials>      slope_specials_;
	unique_ptr<ExtraFloorSpecials> extrafloor_specials_;
	unique_ptr<RenderSpecials>     render_specials_;
	mutable vector<MapObject*>     updated_objects_;
	mutable long                   specials_updated_ = 0;

	void processLineSpecial(const MapLine& line) const;
	void processThing(const MapThing& thing) const;
};
} // namespace slade::map
