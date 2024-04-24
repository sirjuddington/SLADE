#pragma once

namespace slade
{
class PaletteCanvas : public wxPanel
{
public:
	enum class SelectionType
	{
		None,
		One,
		Range
	};

	PaletteCanvas(wxWindow* parent);
	~PaletteCanvas() override = default;

	const Palette* palette() const { return palette_.get(); }
	bool           doubleWidth() const { return double_width_; }
	int            selectionStart() const { return sel_begin_; }
	int            selectionEnd() const { return sel_end_; }
	SelectionType  selectionType() const { return allow_selection_; }

	void doubleWidth(bool dw) { double_width_ = dw; }
	void setSelection(int begin, int end = -1);
	void setSelectionType(SelectionType sel) { allow_selection_ = sel; }
	void setPalette(const Palette* pal);

	ColRGBA selectedColour() const;

	// Events (public)
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);

private:
	unique_ptr<Palette> palette_;
	int                 sel_begin_       = -1;
	int                 sel_end_         = -1;
	int                 sel_base_        = -1; // The 'base' index of the selection (for range select via shift+click)
	bool                double_width_    = false;
	SelectionType       allow_selection_ = SelectionType::None;
	int                 rows_            = 0;
	int                 cols_            = 0;
	int                 square_size_     = 8;
	Vec2i               offset_;
	wxBitmap            buffer_;

	void updateBuffer(bool force = false);

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseMotion(wxMouseEvent& e);
};
} // namespace slade
