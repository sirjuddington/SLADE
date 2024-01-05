#pragma once

#include "MapEditor/MapEditor.h"

// Forward declarations
namespace slade
{
class SLADEMap;
class MapObject;

namespace mapeditor
{
	enum class ItemType;
}
namespace gl::draw2d
{
	struct Context;
}
} // namespace slade

namespace slade
{
class InfoOverlay3D
{
public:
	InfoOverlay3D()  = default;
	~InfoOverlay3D() = default;

	void update(mapeditor::Item item, SLADEMap* map);
	void draw(gl::draw2d::Context& dc, float alpha = 1.0f);
	void reset()
	{
		texture_ = 0;
		object_  = nullptr;
	}

private:
	vector<string>      info_;
	vector<string>      info2_;
	mapeditor::ItemType current_type_;
	mapeditor::Item     current_item_;
	string              texname_;
	unsigned            texture_     = 0;
	bool                thing_icon_  = false;
	MapObject*          object_      = nullptr;
	long                last_update_ = 0;

	void drawTexture(gl::draw2d::Context& dc, float alpha, float x, float y) const;
};
} // namespace slade
