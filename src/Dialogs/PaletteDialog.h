#pragma once

class PaletteCanvas;
class Palette;
class PaletteDialog : public wxDialog
{
public:
	PaletteDialog(Palette* palette);
	~PaletteDialog();

	ColRGBA selectedColour();

private:
	PaletteCanvas* pal_canvas_;

	// Events
	void onLeftDoubleClick(wxMouseEvent& e);
};
