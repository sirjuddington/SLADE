#pragma once

#include "OpenGL/GLHeaders.h"

namespace slade
{
class Palette;
class OGLCanvas : public wxGLCanvas
{
public:
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true, int timer_interval = 100);
	~OGLCanvas() override;

	virtual Palette& palette() { return *palette_; }
	void             setPalette(const Palette* pal) const;
	bool             setContext();
	void             init();
	virtual void     draw() = 0;
	virtual void     update(long frametime) {}
	void             drawCheckeredBackground() const;
	bool             setActive();
	void             setup2D() const;

protected:
	bool                init_done_ = false;
	unique_ptr<Palette> palette_;
	wxTimer             timer_;
	long                last_time_ = 0;

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onTimer(wxTimerEvent& e);
};
} // namespace slade
