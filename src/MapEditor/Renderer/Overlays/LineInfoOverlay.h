
#ifndef __LINE_INFO_OVERLAY_H__
#define __LINE_INFO_OVERLAY_H__

#include "OpenGL/GLUI/Panel.h"

// Forward declarations
class TextureBox;
class MapLine;
class MapSide;
namespace GLUI { class TextBox; }

class LineSideGLPanel : public GLUI::Panel
{
private:
	string			side;
	TextureBox*		tex_lower;
	TextureBox*		tex_middle;
	TextureBox*		tex_upper;
	int				bg_height;
	GLUI::TextBox*	text_info;

public:
	LineSideGLPanel(GLUI::Widget* parent, string side);
	virtual ~LineSideGLPanel();

	void	update(MapSide* side, bool needs_upper = false, bool needs_middle = false, bool needs_lower = false);
	void	setBGHeight(int height) { bg_height = height; }

	// Widget
	void	drawWidget(point2_t pos);
	void	updateLayout(dim2_t fit);
};

class LineInfoOverlay : public GLUI::Panel
{
private:
	GLUI::TextBox*		text_info;
	LineSideGLPanel*	ls_front;
	LineSideGLPanel*	ls_back;

public:
	LineInfoOverlay();
	virtual ~LineInfoOverlay();

	void	update(MapLine* line, int map_format);

	// Widget
	void	drawWidget(point2_t pos);
	void	updateLayout(dim2_t fit);
};

#if 0

class MapLine;
class TextBox;

class LineInfoOverlay
{
private:
	TextBox*	text_box;
	int			last_size;
	double      scale;

	struct side_t
	{
		bool	exists;
		string	info;
		string	offsets;
		string	tex_upper;
		string	tex_middle;
		string	tex_lower;
		bool	needs_upper;
		bool	needs_middle;
		bool	needs_lower;
	};
	side_t	side_front;
	side_t	side_back;

public:
	LineInfoOverlay();
	~LineInfoOverlay();

	void	update(MapLine* line);
	void	draw(int bottom, int right, float alpha = 1.0f);
	void	drawSide(int bottom, int right, float alpha, side_t& side, int xstart = 0);
	void	drawTexture(float alpha, int x, int y, string texture, bool needed, string pos = "U");
};

#endif

#endif//__LINE_INFO_OVERLAY_H__
