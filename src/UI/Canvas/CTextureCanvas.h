#pragma once

#include "CTextureCanvasBase.h"
#include "OpenGL/View.h"

namespace slade
{
namespace wxgfx
{
	struct Context;
}

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

	void drawTexture(wxgfx::Context& ctx, Vec2d scale, Vec2i offset, bool draw_patches);
	void drawTextureBorder(wxgfx::Context& ctx, Vec2d scale, Vec2i offset) const;
	void drawPatch(const wxgfx::Context& ctx, int index);

	// Events
	void onPaint(wxPaintEvent& e);
};
} // namespace slade
