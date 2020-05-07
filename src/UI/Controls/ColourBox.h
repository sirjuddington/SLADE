#pragma once

#include "Utility/Colour.h"

namespace slade
{
class Palette;

class ColourBox : public wxPanel
{
public:
	ColourBox(wxWindow* parent, int id = -1, bool enable_alpha = false, bool mode = false);
	ColourBox(wxWindow* parent, int id, ColRGBA col, bool enable_alpha = false, bool mode = false);
	~ColourBox() = default;

	ColRGBA colour() const { return colour_; }

	void setPalette(Palette* pal) { palette_ = pal; }
	void setColour(ColRGBA col)
	{
		colour_ = col;
		Refresh();
	}

	void popPalette();
	void popColourPicker();
	void popAlphaSlider();

private:
	ColRGBA  colour_  = ColRGBA::BLACK;
	Palette* palette_ = nullptr;
	bool     alpha_   = false;
	bool     altmode_ = false;

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);

	void sendChangeEvent();
};
} // namespace slade

DECLARE_EVENT_TYPE(wxEVT_COLOURBOX_CHANGED, -1)
