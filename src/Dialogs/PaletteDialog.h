
#ifndef __PALETTEDIALOG_H__
#define	__PALETTEDIALOG_H__

#include "common.h"

class PaletteCanvas;
class Palette;
class PaletteDialog : public wxDialog
{
private:
	PaletteCanvas*	pal_canvas;

public:
	PaletteDialog(Palette* palette);
	~PaletteDialog();

	rgba_t	getSelectedColour();

	// Events
	void	onLeftDoubleClick(wxMouseEvent& e);
};

#endif//__PALETTEDIALOG_H__
