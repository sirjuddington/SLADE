
#ifndef __LINE_INFO_OVERLAY_H__
#define __LINE_INFO_OVERLAY_H__

class MapLine;
class TextBox;

class LineInfoOverlay
{
private:
	TextBox*	text_box;
	int			last_size;

	struct side_t
	{
		bool	exists;
		string	info;
		string	offsets;
		string	tex_upper;
		string	tex_middle;
		string	tex_lower;
	};
	side_t	side_front;
	side_t	side_back;

public:
	LineInfoOverlay();
	~LineInfoOverlay();

	void	update(MapLine* line);
	void	draw(int bottom, int right, float alpha = 1.0f);
	void	drawSide(int bottom, int right, float alpha, side_t& side, int xstart = 0);
	void	drawTexture(float alpha, int x, int y, string texture, string pos = "U");
};

#endif//__LINE_INFO_OVERLAY_H__
