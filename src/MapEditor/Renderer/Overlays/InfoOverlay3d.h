#pragma once

#include "MapEditor/Edit/Edit3D.h"

class SLADEMap;
class GLTexture;
class MapObject;

class InfoOverlay3D
{
public:
	InfoOverlay3D() = default;
	~InfoOverlay3D() = default;

	void update(int item_index, MapEditor::ItemType item_type, SLADEMap* map);
	void draw(int bottom, int right, int middle, float alpha = 1.0f);
	void drawTexture(float alpha, int x, int y);
	void reset()
	{
		texture_ = nullptr;
		object_  = nullptr;
	}

private:
	vector<string>      info_;
	vector<string>      info2_;
	MapEditor::ItemType current_type_ = MapEditor::ItemType::WallMiddle;
	string              texname_;
	GLTexture*          texture_     = nullptr;
	bool                thing_icon_  = false;
	MapObject*          object_      = nullptr;
	long                last_update_ = 0;
};
