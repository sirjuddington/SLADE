#pragma once

namespace slade
{
class TextBox;
class MapThing;
class GLTexture;

class ThingInfoOverlay
{
public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void update(MapThing* thing);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	string              sprite_;
	string              translation_;
	string              palette_;
	string              icon_;
	int                 zeth_icon_ = -1;
	unique_ptr<TextBox> text_box_;
	int                 last_size_ = 100;
};
} // namespace slade
