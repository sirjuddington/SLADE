#pragma once

namespace slade
{
class GfxGLCanvas;
class CTextureGLCanvas;

class SZoomSlider : public wxPanel
{
public:
	SZoomSlider(wxWindow* parent, GfxGLCanvas* linked_canvas = nullptr);
	SZoomSlider(wxWindow* parent, CTextureGLCanvas* linked_canvas);
	~SZoomSlider() override = default;

	int    zoomPercent() const;
	double zoomFactor() const { return static_cast<double>(zoomPercent()) * 0.01; }

	void setZoom(int percent) const;
	void setZoom(double factor) const;

	void linkGfxCanvas(GfxGLCanvas* canvas) { linked_gfx_canvas_ = canvas; }
	void linkTextureCanvas(CTextureGLCanvas* canvas) { linked_texture_canvas_ = canvas; }

private:
	wxSlider*         slider_zoom_           = nullptr;
	wxStaticText*     label_zoom_amount_     = nullptr;
	GfxGLCanvas*      linked_gfx_canvas_     = nullptr;
	CTextureGLCanvas* linked_texture_canvas_ = nullptr;

	void setup();
};
} // namespace slade
