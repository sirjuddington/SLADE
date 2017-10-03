#pragma once

class GfxCanvas;

class SZoomSlider : public wxPanel
{
public:
	SZoomSlider(wxWindow* parent, GfxCanvas* linked_canvas = nullptr);
	~SZoomSlider() {}

	int		zoomPercent() const;
	double	zoomFactor() const { return (double)zoomPercent() * 0.01; }

	void	setZoom(int percent);
	void	setZoom(double factor);

	void	linkGfxCanvas(GfxCanvas* canvas) { linked_gfx_canvas_ = canvas; }

private:
	wxSlider*		slider_zoom_		= nullptr;
	wxStaticText*	label_zoom_amount_	= nullptr;
	GfxCanvas*		linked_gfx_canvas_	= nullptr;
};
