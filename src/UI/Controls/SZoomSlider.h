#pragma once

namespace slade
{
class GfxCanvas;
class CTextureCanvas;

class SZoomSlider : public wxPanel
{
public:
	SZoomSlider(wxWindow* parent, GfxCanvas* linked_canvas = nullptr);
	SZoomSlider(wxWindow* parent, CTextureCanvas* linked_canvas);
	~SZoomSlider() override = default;

	int    zoomPercent() const;
	double zoomFactor() const { return static_cast<double>(zoomPercent()) * 0.01; }

	void setZoom(int percent) const;
	void setZoom(double factor) const;

	void linkGfxCanvas(GfxCanvas* canvas) { linked_gfx_canvas_ = canvas; }
	void linkTextureCanvas(CTextureCanvas* canvas) { linked_texture_canvas_ = canvas; }

private:
	wxSlider*       slider_zoom_           = nullptr;
	wxStaticText*   label_zoom_amount_     = nullptr;
	GfxCanvas*      linked_gfx_canvas_     = nullptr;
	CTextureCanvas* linked_texture_canvas_ = nullptr;

	void setup();
};
} // namespace slade
