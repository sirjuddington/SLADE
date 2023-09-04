#pragma once

namespace slade
{
class MapLine;
namespace gl::draw2d
{
	struct Context;
	class TextBox;
} // namespace gl::draw2d

class LineInfoOverlay
{
public:
	LineInfoOverlay();
	~LineInfoOverlay() = default;

	void update(MapLine* line);
	void draw(gl::draw2d::Context& dc, float alpha = 1.0f);

private:
	float scale_     = 1.0f;
	float last_size_ = 100.0f;

	unique_ptr<gl::draw2d::TextBox> text_box_;

	struct Side
	{
		bool   exists;
		string info;
		string offsets;
		string tex_upper;
		string tex_middle;
		string tex_lower;
		bool   needs_upper;
		bool   needs_middle;
		bool   needs_lower;
	};
	Side side_front_{};
	Side side_back_{};

	void drawSide(gl::draw2d::Context& dc, float bottom, float alpha, const Side& side, float xstart = 0) const;
	void drawTexture(
		gl::draw2d::Context& dc,
		float                alpha,
		float                x,
		float                y,
		string_view          texture,
		bool                 needed,
		string_view          pos = "U") const;
};
} // namespace slade
