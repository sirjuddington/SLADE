#pragma once

#include "MapEditor/MapEditor.h"

class MapEditContext;

class MoveObjects
{
public:
	MoveObjects(MapEditContext& context);

	const vector<MapEditor::Item>& items() const { return items_; }
	Vec2f                          offset() { return offset_; }

	bool begin(Vec2f mouse_pos);
	void update(Vec2f mouse_pos);
	void end(bool accept = true);

private:
	MapEditContext&         context_;
	Vec2f                   origin_;
	Vec2f                   offset_;
	vector<MapEditor::Item> items_;
	MapEditor::Item         item_closest_ = 0;
};
