
#ifndef __PALETTECANVAS_H__
#define	__PALETTECANVAS_H__

#include "OGLCanvas.h"
#include "Palette.h"
#include <wx/colordlg.h>

class PaletteCanvas : public OGLCanvas
{
private:
	int		sel_begin;
	int		sel_end;
	bool	double_width;
	int		allow_selection;	// 0 = no, 1 = one colour only, 2 = any (range)

public:
	PaletteCanvas(wxWindow* parent, int id);
	~PaletteCanvas();

	Palette8bit&	getPalette() { return palette; }
	bool			doubleWidth() { return double_width; }
	int				getSelectionStart() { return sel_begin; }
	int				getSelectionEnd() { return sel_end; }
	int				allowSelection() { return allow_selection; }

	void	doubleWidth(bool dw) { double_width = dw; }
	void	setSelection(int begin, int end = -1);
	void	allowSelection(int sel) { allow_selection = sel; }

	void	draw();
	rgba_t	getSelectedColour();

	// Events
	void	onMouseLeftDown(wxMouseEvent& e);
	void	onMouseRightDown(wxMouseEvent& e);
	void	onMouseMotion(wxMouseEvent& e);
};

#endif //__PALETTECANVAS_H__
