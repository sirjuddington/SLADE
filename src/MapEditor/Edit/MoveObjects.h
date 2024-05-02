#pragma once

namespace slade::mapeditor
{
struct Item;
class MapEditContext;

class MoveObjects
{
public:
	MoveObjects(MapEditContext& context);

	const vector<Item>& items() const { return items_; }
	Vec2d               offset() const { return offset_; }

	bool begin(const Vec2d& mouse_pos);
	void update(const Vec2d& mouse_pos);
	void end(bool accept = true);

private:
	MapEditContext* context_;
	Vec2d           origin_;
	Vec2d           offset_;
	vector<Item>    items_;
};
} // namespace slade::mapeditor
