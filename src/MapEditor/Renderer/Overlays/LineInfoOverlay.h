#pragma once

namespace slade
{
class TextBox;

class LineInfoOverlay
{
public:
	LineInfoOverlay();
	~LineInfoOverlay();

	void update(MapLine* line);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	double              scale_ = 1.;
	unique_ptr<TextBox> text_box_;
	int                 last_size_ = 100;

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

	void drawSide(int bottom, float alpha, const Side& side, int xstart = 0) const;
	void drawTexture(float alpha, int x, int y, string_view texture, bool needed, string_view pos = "U") const;
};
} // namespace slade
