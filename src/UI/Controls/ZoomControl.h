#pragma once

namespace slade
{
class SToolBarButton;
class GfxCanvas;
class CTextureCanvas;

namespace ui
{
	class ZoomControl : public wxPanel
	{
	public:
		ZoomControl(wxWindow* parent);
		ZoomControl(wxWindow* parent, GfxCanvas* linked_canvas);
		ZoomControl(wxWindow* parent, CTextureCanvas* linked_canvas);

		int    zoomPercent() const { return zoom_; }
		double zoomScale() const { return static_cast<double>(zoom_) / 100.; }

		void setZoomPercent(int percent);
		void setZoomScale(double scale);
		void zoomOut();
		void zoomIn();

	private:
		wxComboBox*     cb_zoom_               = nullptr;
		SToolBarButton* btn_zoom_out_          = nullptr;
		SToolBarButton* btn_zoom_in_           = nullptr;
		GfxCanvas*      linked_gfx_canvas_     = nullptr;
		CTextureCanvas* linked_texture_canvas_ = nullptr;

		int zoom_ = 100;

		void setup();
		void updateZoomButtons() const;
	};
} // namespace ui
} // namespace slade
