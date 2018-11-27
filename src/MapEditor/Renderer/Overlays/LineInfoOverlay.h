#pragma once

class MapLine;
class TextBox;

class LineInfoOverlay
{
public:
	LineInfoOverlay();
	~LineInfoOverlay();

	void update(MapLine* line);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	TextBox* text_box_;
	int      last_size_;
	double   scale_;

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
	Side side_front_;
	Side side_back_;

	void drawSide(int bottom, int right, float alpha, Side& side, int xstart = 0);
	void drawTexture(float alpha, int x, int y, string texture, bool needed, string pos = "U");
};
