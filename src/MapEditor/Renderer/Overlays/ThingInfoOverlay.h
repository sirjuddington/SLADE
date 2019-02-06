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
	wxString sprite_;
	wxString translation_;
	wxString palette_;
	wxString icon_;
	int      zeth_icon_ = -1;
	TextBox  text_box_;
	int      last_size_ = 100;
};
