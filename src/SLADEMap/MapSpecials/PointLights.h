#pragma once

namespace slade::game
{
class ThingType;
}
namespace slade::map
{
struct PointLight
{
	enum class Type : u8
	{
		Normal,
		Attenuated,
		Additive,
		Subtractive
	};

	const MapThing* thing;
	Vec3d           position;
	u8              r, g, b;
	unsigned        radius;
	Type            type;
};

class PointLights
{
public:
	PointLights(const SLADEMap& map);
	~PointLights();

	const vector<PointLight>& pointLights() const { return point_lights_; }

	void processThing(const MapThing& thing);

	void thingUpdated(const MapThing& thing);
	void thingDeleted(const MapThing& thing);

	void clear() { point_lights_.clear(); }

private:
	const SLADEMap*    map_ = nullptr;
	vector<PointLight> point_lights_;

	void addPointLight(
		const MapThing&        thing,
		const game::ThingType& tt,
		u8                     r,
		u8                     g,
		u8                     b,
		unsigned               radius,
		PointLight::Type       type               = PointLight::Type::Normal,
		bool                   radius_sector_sync = false);
};
} // namespace slade::map
