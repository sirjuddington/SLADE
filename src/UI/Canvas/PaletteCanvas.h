#pragma once

#include "OGLCanvas.h"

class PaletteCanvas : public OGLCanvas
{
public:
	enum class SelectionType
	{
		None,
		One,
		Range
	};

	PaletteCanvas(wxWindow* parent, int id);
	~PaletteCanvas();

	Palette&      palette() { return palette_; }
	bool          doubleWidth() { return double_width_; }
	int           selectionStart() { return sel_begin_; }
	int           selectionEnd() { return sel_end_; }
	SelectionType selectionType() { return allow_selection_; }

	void doubleWidth(bool dw) { double_width_ = dw; }
	void setSelection(int begin, int end = -1);
	void setSelectionType(SelectionType sel) { allow_selection_ = sel; }

	void   draw();
	ColRGBA selectedColour();

	// Events
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseMotion(wxMouseEvent& e);

private:
	int           sel_begin_;
	int           sel_end_;
	bool          double_width_;
	SelectionType allow_selection_;
};
