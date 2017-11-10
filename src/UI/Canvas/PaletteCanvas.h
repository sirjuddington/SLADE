#pragma once

#include "GLCanvas.h"
#include "Graphics/Palette/Palette.h"

class PaletteCanvas : public GLCanvas
{
public:
	PaletteCanvas(wxWindow* parent, int id);
	~PaletteCanvas() {}

	Palette&	palette() { return palette_; }
	bool		doubleWidth() const { return double_width_; }
	int			selectionStart() const { return sel_begin_; }
	int			selectionEnd() const { return sel_end_; }
	int			allowSelection() const { return allow_selection_; }

	void	doubleWidth(bool dw) { double_width_ = dw; }
	void	setSelection(int begin, int end = -1);
	void	allowSelection(int sel) { allow_selection_ = sel; }
	void	setPalette(Palette* pal) { palette_.copyPalette(pal); }

	void	drawContent() override;
	rgba_t	getSelectedColour();

	// Events
	void	onMouseLeftDown(wxMouseEvent& e);
	void	onMouseRightDown(wxMouseEvent& e);
	void	onMouseMotion(wxMouseEvent& e);

private:
	int		sel_begin_			= -1;
	int		sel_end_			= -1;
	bool	double_width_		= false;
	int		allow_selection_	= 0;	// 0 = no, 1 = one colour only, 2 = any (range)
	Palette	palette_;
};
