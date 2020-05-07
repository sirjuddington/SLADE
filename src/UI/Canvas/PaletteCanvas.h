#pragma once

#include "OGLCanvas.h"

namespace slade
{
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
	~PaletteCanvas() = default;

	Palette&      palette() override { return palette_; }
	bool          doubleWidth() const { return double_width_; }
	int           selectionStart() const { return sel_begin_; }
	int           selectionEnd() const { return sel_end_; }
	SelectionType selectionType() const { return allow_selection_; }

	void doubleWidth(bool dw) { double_width_ = dw; }
	void setSelection(int begin, int end = -1);
	void setSelectionType(SelectionType sel) { allow_selection_ = sel; }

	void    draw() override;
	ColRGBA selectedColour() const;

	// Events
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseMotion(wxMouseEvent& e);

private:
	int           sel_begin_       = -1;
	int           sel_end_         = -1;
	bool          double_width_    = false;
	SelectionType allow_selection_ = SelectionType::None;
};
} // namespace slade
