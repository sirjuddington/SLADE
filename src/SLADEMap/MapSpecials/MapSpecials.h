#pragma once

#include "General/Sigslot.h"
#include "Geometry/Plane.h"

namespace slade::map
{
struct SectorLighting;
enum class SidePart : u8;
enum class SectorPart : u8;
class RenderSpecials;
class SlopeSpecials;
class ExtraFloorSpecials;
struct ExtraFloor;
struct LineTranslucency;
struct PointLight;
class PointLights;

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
	ColRGBA                   sectorFadeColour(const MapSector& sector) const;
	float                     sectorFloorHeightAt(const MapSector& sector, Vec3d pos) const;
	SectorLighting            sectorLightingAt(
				   const MapSector& sector,
				   SectorPart       where,
				   optional<Plane>  plane       = {},
				   bool             below_plane = false) const;

	optional<LineTranslucency> lineTranslucency(const MapLine& line) const;

	ColRGBA sideColour(const MapSide& side, SidePart where, bool fullbright = false) const;

	const vector<PointLight>& pointLights() const;
	const PointLight*         pointLightForThing(const MapThing& thing) const;

	void processAllSpecials() const;
	void updateSpecials() const;

	long specialsLastUpdated() const { return specials_updated_; }

private:
	SLADEMap*                      map_ = nullptr;
	unique_ptr<SlopeSpecials>      slope_specials_;
	unique_ptr<ExtraFloorSpecials> extrafloor_specials_;
	unique_ptr<RenderSpecials>     render_specials_;
	unique_ptr<PointLights>        point_lights_;
	mutable vector<MapObject*>     updated_objects_;
	mutable long                   specials_updated_ = 0;
	mutable bool                   specials_updating_ = false;
	ScopedConnectionList           connections_;

	void processLineSpecial(const MapLine& line) const;
	void processThing(const MapThing& thing) const;
};
} // namespace slade::map
