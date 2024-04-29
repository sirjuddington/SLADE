#pragma once

namespace slade
{
class GfxCanvasBase;
class SToolBarButton;
class CTextureCanvasBase;

namespace ui
{
	class ZoomControl : public wxPanel
	{
	public:
		ZoomControl(wxWindow* parent);
		ZoomControl(wxWindow* parent, GfxCanvasBase* linked_canvas);
		ZoomControl(wxWindow* parent, CTextureCanvasBase* linked_canvas);

		int    zoomPercent() const { return zoom_; }
		double zoomScale() const { return static_cast<double>(zoom_) / 100.; }

		void setZoomPercent(int percent);
		void setZoomScale(double scale);
		void zoomOut(bool fine = false);
		void zoomIn(bool fine = false);

	private:
		wxComboBox*         cb_zoom_               = nullptr;
		SToolBarButton*     btn_zoom_out_          = nullptr;
		SToolBarButton*     btn_zoom_in_           = nullptr;
		GfxCanvasBase*      linked_gfx_canvas_     = nullptr;
		CTextureCanvasBase* linked_texture_canvas_ = nullptr;

		int zoom_ = 100;

		void setup();
		void updateZoomButtons() const;
	};
} // namespace ui
} // namespace slade
