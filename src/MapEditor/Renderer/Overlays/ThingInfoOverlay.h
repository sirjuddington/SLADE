#pragma once

namespace slade
{
class TextBox;
class GLTexture;
namespace gl::draw2d
{
	struct Context;
	class TextBox;
} // namespace gl::draw2d

class ThingInfoOverlay
{
public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void update(MapThing* thing);
	void draw(gl::draw2d::Context& dc, float alpha = 1.0f);

private:
	string                          sprite_;
	string                          translation_;
	string                          palette_;
	string                          icon_;
	int                             zeth_icon_ = -1;
	unique_ptr<gl::draw2d::TextBox> text_box_;
	int                             last_size_ = 100;
};
} // namespace slade
