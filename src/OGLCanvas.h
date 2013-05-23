
#ifndef __OGLCANVAS_H__
#define	__OGLCANVAS_H__

#include "OpenGL.h"
#include "Palette.h"
#include <wx/control.h>
#include <wx/timer.h>

#ifdef USE_SFML_RENDERWINDOW
#undef None
#include <SFML/Graphics.hpp>
#else
#include <wx/glcanvas.h>
#endif

class wxPanel;
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
	OGLCanvas(wxWindow* parent, int id, bool handle_timer = true);
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

#ifdef USE_SFML_RENDERWINDOW
#if SFML_VERSION_MAJOR >= 2
	void	SwapBuffers() { display(); }
#else
	void	SwapBuffers() { Display(); }
#endif//SFML_VERSION_MAJOR
#endif//USE_SFML_RENDERWINDOW

	void	onPaint(wxPaintEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
	void	onTimer(wxTimerEvent& e);
	void	onResize(wxSizeEvent& e);
	//void	onIdle(wxIdleEvent& e);
};

#endif //__OGLCANVAS_H__
