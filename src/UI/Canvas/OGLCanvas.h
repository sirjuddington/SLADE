#pragma once

#include "Graphics/Palette/Palette.h"
#include "OpenGL/OpenGL.h"

class wxWindow;

namespace slade
{
class OGLCanvas : public wxGLCanvas
{
public:
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true, int timer_interval = 100);
	~OGLCanvas() = default;

	virtual Palette& palette() { return palette_; }
	void             setPalette(Palette* pal) { palette_.copyPalette(pal); }
	bool             setContext();
	void             init();
	virtual void     draw() = 0;
	virtual void     update(long frametime) {}
	void             drawCheckeredBackground() const;
	bool             setActive();
	void             setup2D() const;

protected:
	bool    init_done_ = false;
	Palette palette_;
	wxTimer timer_;
	long    last_time_ = 0;
	bool    recreate_  = false;

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onTimer(wxTimerEvent& e);
};
} // namespace slade
