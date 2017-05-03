
#ifndef __OGLCANVAS_H__
#define	__OGLCANVAS_H__

#include "common.h"
#include "OpenGL/OpenGL.h"
#include "Graphics/Palette/Palette.h"

class wxWindow;
#ifdef USE_SFML_RENDERWINDOW
class OGLCanvas : public wxControl, public sf::RenderWindow
{
#else
class OGLCanvas : public wxGLCanvas
{
#endif
protected:
	bool		init_done;
	Palette8bit	palette;
	wxTimer		timer;
	long		last_time;
	bool		recreate;

public:
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true, int timer_interval = 100);
	~OGLCanvas();

	Palette8bit*	getPalette() { return &palette; }
	void			setPalette(Palette8bit* pal) { palette.copyPalette(pal); }
	bool			setContext();
	void			createSFML();
	void			init();
	virtual void	draw() = 0;
	virtual void	update(long frametime) {}
	void			drawCheckeredBackground();
	wxWindow*		toPanel(wxWindow* parent);
	bool			setActive();
	void			setup2D();

#ifdef USE_SFML_RENDERWINDOW
	void	SwapBuffers() { display(); }
#endif

	void	onPaint(wxPaintEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
	void	onTimer(wxTimerEvent& e);
	void	onResize(wxSizeEvent& e);
};

#endif //__OGLCANVAS_H__
