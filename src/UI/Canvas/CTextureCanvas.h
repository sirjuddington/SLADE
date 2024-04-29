#pragma once

#include "CTextureCanvasBase.h"
#include "OpenGL/View.h"

namespace slade
{
class CTextureCanvas : public wxPanel, public CTextureCanvasBase
{
public:
	CTextureCanvas(wxWindow* parent);
	~CTextureCanvas() override;

	wxWindow* window() override { return this; }

	gl::View&       view() override { return view_; }
	const gl::View& view() const override { return view_; }

	Palette* palette() override { return palette_.get(); }
	void     setPalette(const Palette* pal) override;

	void refreshPatch(unsigned index) override;

private:
	unique_ptr<Palette> palette_;
	gl::View            view_;

	vector<wxBitmap> patch_bitmaps_;
	wxBitmap         background_bitmap_;
	wxBitmap         tex_bitmap_;

	void drawTexture(wxGraphicsContext* gc, Vec2d scale, Vec2d offset, bool draw_patches);
	void drawTextureBorder(wxGraphicsContext* gc, Vec2d scale, Vec2d offset) const;
	void drawPatch(wxGraphicsContext* gc, int index);

	// Events
	void onPaint(wxPaintEvent& e);
};
} // namespace slade
