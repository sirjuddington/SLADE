#pragma once

#include "MapObjectList.h"
#include "SLADEMap/MapObject/MapSide.h"

class SideList : public MapObjectList<MapSide>
{
public:
	// MapObjectList overrides
	void clear() override;
	void add(MapSide* side) override;
	void remove(unsigned index) override;

	void clearTexUsage() const { usage_tex_.clear(); }
	void updateTexUsage(std::string_view tex, int adjust) const;
	int  texUsageCount(std::string_view tex) const;

private:
	mutable std::map<std::string, int> usage_tex_;
};
