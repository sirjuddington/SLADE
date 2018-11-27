#pragma once

class MapThing;
class GLTexture;
class TextBox;

class ThingInfoOverlay
{
public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void update(MapThing* thing);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	string   sprite_;
	string   translation_;
	string   palette_;
	string   icon_;
	int      zeth_;
	TextBox* text_box_;
	int      last_size_;
};
