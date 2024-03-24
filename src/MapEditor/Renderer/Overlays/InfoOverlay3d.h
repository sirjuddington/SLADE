#pragma once

#include "MapEditor/Item.h"

namespace slade
{
class InfoOverlay3D
{
public:
	InfoOverlay3D()  = default;
	~InfoOverlay3D() = default;

	void update(mapeditor::Item item, SLADEMap* map);
	void draw(int bottom, int right, int middle, float alpha = 1.0f);
	void drawTexture(float alpha, int x, int y) const;
	void reset()
	{
		texture_ = 0;
		object_  = nullptr;
	}

private:
	vector<string>      info_;
	vector<string>      info2_;
	mapeditor::ItemType current_type_ = mapeditor::ItemType::WallMiddle;
	mapeditor::Item     current_item_;
	string              texname_;
	unsigned            texture_     = 0;
	bool                thing_icon_  = false;
	MapObject*          object_      = nullptr;
	long                last_update_ = 0;
};
} // namespace slade
