#pragma once

namespace slade
{
enum class SectorSurfaceType : u8;
class MapSector;
struct Plane;

struct SlopeSpecial
{
	SectorSurfaceType surface_type;
	MapSector*        target = nullptr;

	SlopeSpecial(SectorSurfaceType surface_type) : surface_type{ surface_type } {}
	virtual ~SlopeSpecial() = default;

	virtual bool isTarget(const MapSector* sector, SectorSurfaceType surface_type) const
	{
		return sector == target && this->surface_type == surface_type;
	}

	virtual void apply() = 0;
};
} // namespace slade
