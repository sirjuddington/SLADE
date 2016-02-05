
#ifndef __PALETTEDIALOG_H__
#define	__PALETTEDIALOG_H__

#include "UI/Canvas/PaletteCanvas.h"

class PaletteDialog : public wxDialog
{
private:
	PaletteCanvas*	pal_canvas;

public:
	PaletteDialog(Palette8bit* palette);
	~PaletteDialog();

	rgba_t	getSelectedColour();

	// Events
	void	onLeftDoubleClick(wxMouseEvent& e);
};

#endif//__PALETTEDIALOG_H__
