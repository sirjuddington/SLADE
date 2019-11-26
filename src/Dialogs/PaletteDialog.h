#pragma once

#include "Utility/Colour.h"

class PaletteCanvas;
class Palette;

class PaletteDialog : public wxDialog
{
public:
	PaletteDialog(Palette* palette);
	~PaletteDialog() = default;

	ColRGBA selectedColour() const;

private:
	PaletteCanvas* pal_canvas_ = nullptr;
};
