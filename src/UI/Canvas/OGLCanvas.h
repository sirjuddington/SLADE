#pragma once

#include "Graphics/Palette/Palette.h"
#include "OpenGL/OpenGL.h"

class wxWindow;
class OGLCanvas : public wxGLCanvas
{
public:
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true, int timer_interval = 100);
	~OGLCanvas() = default;

	virtual Palette& palette() { return palette_; }
	void             setPalette(Palette* pal) { palette_.copyPalette(pal); }
	bool             activateContext();
	void             init();
	virtual void     draw() = 0;
	virtual void     update(long frametime) {}
	void             drawCheckeredBackground() const;
	wxWindow*        toPanel(wxWindow* parent);
	void             setup2D() const;

protected:
	bool    init_done_ = false;
	Palette palette_;
	wxTimer timer_;
	long    last_time_ = 0;

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onTimer(wxTimerEvent& e);
};
