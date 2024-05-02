#pragma once

#include "GfxCanvasBase.h"
#include "OpenGL/View.h"

namespace slade
{
class GfxCanvas : public wxPanel, public GfxCanvasBase
{
public:
	using View     = GfxView;
	using EditMode = GfxEditMode;

	GfxCanvas(wxWindow* parent);
	~GfxCanvas() override;

	wxWindow* window() override { return this; }

	gl::View&       view() override { return view_; }
	const gl::View& view() const override { return view_; }

	Palette* palette() override { return palette_.get(); }
	void     setPalette(const Palette* pal) override;

private:
	gl::View            view_;
	unique_ptr<Palette> palette_;
	bool                update_image_    = true;
	bool                image_hilighted_ = false;

	wxBitmap background_bitmap_;
	wxBitmap image_bitmap_;
	wxBitmap brush_bitmap_;

	void generateBrushShadow() override;
	void updateImage(bool hilight);
	void drawImage(wxGraphicsContext* gc);
	void drawImageTiled(wxGraphicsContext* gc);
	void drawCropRect(wxGraphicsContext* gc) const;

	// Events
	void onPaint(wxPaintEvent& e);
};
} // namespace slade
