
#ifndef __LINE_INFO_OVERLAY_H__
#define __LINE_INFO_OVERLAY_H__

#include "InfoOverlay.h"

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
	void	updateLayout(dim2_t fit) override;
};

class LineInfoOverlay : public InfoOverlay
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
	void	drawWidget(fpoint2_t pos, float alpha, fpoint2_t scale) override;
	void	updateLayout(dim2_t fit) override;
};

#endif//__LINE_INFO_OVERLAY_H__
