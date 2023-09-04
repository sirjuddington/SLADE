#pragma once

namespace slade
{
class MapSector;
namespace gl::draw2d
{
	struct Context;
	class TextBox;
} // namespace gl::draw2d

class SectorInfoOverlay
{
public:
	SectorInfoOverlay();
	~SectorInfoOverlay() = default;

	void update(const MapSector* sector);
	void draw(gl::draw2d::Context& dc, float alpha = 1.0f);

private:
	unique_ptr<gl::draw2d::TextBox> text_box_;
	string                          ftex_;
	string                          ctex_;
	int                             last_size_ = 100;

	void drawTexture(
		gl::draw2d::Context& dc,
		float                alpha,
		float                x,
		float                y,
		string_view          texture,
		string_view          pos = "Upper") const;
};
} // namespace slade
