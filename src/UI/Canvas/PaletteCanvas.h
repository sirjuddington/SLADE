#pragma once

#include "GLCanvas.h"
#include "Graphics/Palette/Palette.h"
#include "OpenGL/VertexBuffer2D.h"

namespace slade
{
namespace gl
{
	class VertexBuffer2D;
}

class PaletteCanvas : public GLCanvas
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

	const Palette& palette() const { return palette_; }
	bool           doubleWidth() const { return double_width_; }
	int            selectionStart() const { return sel_begin_; }
	int            selectionEnd() const { return sel_end_; }
	SelectionType  selectionType() const { return allow_selection_; }

	void doubleWidth(bool dw) { double_width_ = dw; }
	void setSelection(int begin, int end = -1);
	void setSelectionType(SelectionType sel) { allow_selection_ = sel; }
	void setPalette(const Palette* pal);

	ColRGBA selectedColour() const;

	// Events
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseMotion(wxMouseEvent& e);

private:
	Palette       palette_;
	int           sel_begin_       = -1;
	int           sel_end_         = -1;
	bool          double_width_    = false;
	SelectionType allow_selection_ = SelectionType::None;

	unique_ptr<gl::VertexBuffer2D> vb_palette_;
	int                            rows_        = 0;
	int                            cols_        = 0;
	int                            square_size_ = 8;

	void draw() override;
	void updatePaletteBuffer();
};
} // namespace slade
