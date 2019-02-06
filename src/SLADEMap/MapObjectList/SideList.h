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
	void updateTexUsage(const wxString& tex, int adjust) const { usage_tex_[tex.Upper()] += adjust; }
	int  texUsageCount(const wxString& tex) const { return usage_tex_[tex.Upper()]; }

private:
	mutable std::map<wxString, int> usage_tex_;
};
