#pragma once

#include "MapEditor/MapEditor.h"

namespace slade
{
class MapEditContext;

class MoveObjects
{
public:
	MoveObjects(MapEditContext& context);

	const vector<mapeditor::Item>& items() const { return items_; }
	Vec2d                          offset() const { return offset_; }

	bool begin(Vec2d mouse_pos);
	void update(Vec2d mouse_pos);
	void end(bool accept = true);

private:
	MapEditContext&         context_;
	Vec2d                   origin_;
	Vec2d                   offset_;
	vector<mapeditor::Item> items_;
	mapeditor::Item         item_closest_ = 0;
};
} // namespace slade
