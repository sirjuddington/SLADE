#pragma once

#include "MapEditor/MapEditor.h"

class MapEditContext;

class MoveObjects
{
public:
	MoveObjects(MapEditContext& context);

	const vector<MapEditor::Item>&	items() const { return items_; }
	fpoint2_t						offset() { return offset_; }

	bool	begin(fpoint2_t mouse_pos);
	void	update(fpoint2_t mouse_pos);
	void	end(bool accept = true);

private:
	MapEditContext&			context_;
	fpoint2_t				origin_;
	fpoint2_t				offset_;
	vector<MapEditor::Item>	items_;
	MapEditor::Item			item_closest_ = 0;
};
