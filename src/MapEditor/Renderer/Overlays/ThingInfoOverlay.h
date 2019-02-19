#pragma once

#include "OpenGL/Drawing.h"

class MapThing;
class GLTexture;

class ThingInfoOverlay
{
public:
	ThingInfoOverlay();
	~ThingInfoOverlay() = default;

	void update(MapThing* thing);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	std::string sprite_;
	std::string translation_;
	std::string palette_;
	std::string icon_;
	int         zeth_icon_ = -1;
	TextBox     text_box_;
	int         last_size_ = 100;
};
