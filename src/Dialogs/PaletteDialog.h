#pragma once

class PaletteCanvas;
class Palette;
class PaletteDialog : public wxDialog
{
public:
	PaletteDialog(Palette* palette);
	~PaletteDialog();

	rgba_t selectedColour();

private:
	PaletteCanvas* pal_canvas_;

	// Events
	void onLeftDoubleClick(wxMouseEvent& e);
};
