#pragma once

class Palette;

class ColourBox : public wxPanel
{
public:
	ColourBox(wxWindow* parent, int id = -1, bool enable_alpha = false, bool mode = false);
	ColourBox(wxWindow* parent, int id, rgba_t col, bool enable_alpha = false, bool mode = false);
	~ColourBox() {}

	rgba_t colour() { return colour_; }

	void setPalette(Palette* pal) { palette_ = pal; }
	void setColour(rgba_t col)
	{
		colour_ = col;
		Refresh();
	}

	void popPalette();
	void popColourPicker();
	void popAlphaSlider();

private:
	rgba_t   colour_;
	Palette* palette_ = nullptr;
	bool     alpha_   = false;
	bool     altmode_ = false;

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);

	void sendChangeEvent();
};

DECLARE_EVENT_TYPE(wxEVT_COLOURBOX_CHANGED, -1)
