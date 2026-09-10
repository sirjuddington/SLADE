#pragma once

namespace slade
{
class GfxCanvasBase;
class CTextureGLCanvas;
} // namespace slade
namespace slade::texeditor
{
class CTextureCanvasBase;
}

namespace slade
{
class SZoomSlider : public wxPanel
{
public:
	SZoomSlider(wxWindow* parent, GfxCanvasBase* linked_canvas = nullptr);
	SZoomSlider(wxWindow* parent, texeditor::CTextureCanvasBase* linked_canvas);
	~SZoomSlider() override = default;

	int    zoomPercent() const;
	double zoomFactor() const { return static_cast<double>(zoomPercent()) * 0.01; }

	void setZoom(int percent) const;
	void setZoom(double factor) const;

	void linkGfxCanvas(GfxCanvasBase* canvas) { linked_gfx_canvas_ = canvas; }
	void linkTextureCanvas(texeditor::CTextureCanvasBase* canvas) { linked_texture_canvas_ = canvas; }

private:
	wxSlider*                      slider_zoom_           = nullptr;
	wxStaticText*                  label_zoom_amount_     = nullptr;
	GfxCanvasBase*                 linked_gfx_canvas_     = nullptr;
	texeditor::CTextureCanvasBase* linked_texture_canvas_ = nullptr;

	void setup();
};
} // namespace slade
