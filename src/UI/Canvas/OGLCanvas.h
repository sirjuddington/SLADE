#pragma once

#include "Graphics/Palette/Palette.h"
#include "OpenGL/OpenGL.h"

class wxWindow;
#ifdef USE_SFML_RENDERWINDOW
class OGLCanvas : public wxControl, public sf::RenderWindow
{
#else
class OGLCanvas : public wxGLCanvas
{
#endif
public:
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true, int timer_interval = 100);
	~OGLCanvas();

	Palette*     getPalette() { return &palette_; }
	void         setPalette(Palette* pal) { palette_.copyPalette(pal); }
	bool         setContext();
	void         createSFML();
	void         init();
	virtual void draw() = 0;
	virtual void update(long frametime) {}
	void         drawCheckeredBackground();
	wxWindow*    toPanel(wxWindow* parent);
	bool         setActive();
	void         setup2D();

#ifdef USE_SFML_RENDERWINDOW
	void SwapBuffers() { display(); }
#endif

protected:
	bool    init_done_;
	Palette palette_;
	wxTimer timer_;
	long    last_time_;
	bool    recreate_;

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onTimer(wxTimerEvent& e);
	void onResize(wxSizeEvent& e);
};
