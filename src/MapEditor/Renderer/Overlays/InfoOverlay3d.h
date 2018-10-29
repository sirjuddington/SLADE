#pragma once

#include "MapEditor/Edit/Edit3D.h"

class SLADEMap;
class GLTexture;
class MapObject;

class InfoOverlay3D
{
public:
	InfoOverlay3D();
	~InfoOverlay3D();

	void update(MapEditor::Item item, SLADEMap* map);
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
	MapEditor::ItemType current_type_;
	int                 current_floor_index_;
	MapEditor::Item     current_item_; // TODO
	string              texname_;
	GLTexture*          texture_;
	bool                thing_icon_;
	MapObject*          object_;
	long                last_update_;
};
