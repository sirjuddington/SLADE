#pragma once

#include "MapObjectList.h"

namespace slade
{
class MapSide;

class SideList : public MapObjectList<MapSide>
{
public:
	// MapObjectList overrides
	void clear() override;
	void add(MapSide* side) override;
	void remove(unsigned index) override;

	void clearTexUsage() const { usage_tex_.clear(); }
	void updateTexUsage(string_view tex, int adjust) const;
	int  texUsageCount(string_view tex) const;

private:
	mutable std::map<string, int> usage_tex_;
};
} // namespace slade
