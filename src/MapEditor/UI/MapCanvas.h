
#ifndef __MAPCANVAS_H__
#define __MAPCANVAS_H__

#include "common.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "UI/Canvas/OGLCanvas.h"
#include "MapEditor/MapEditContext.h"

class MapCanvas : public OGLCanvas, public KeyBindHandler, public SActionHandler
{
public:
	MapCanvas(wxWindow* parent, int id, MapEditContext* editor);
	~MapCanvas();

	// Drawing
	void	draw() override;

	// Mouse
	void	mouseToCenter();
	void	lockMouse(bool lock);
	void	mouseLook3d();

	// Keybind handling
	void	onKeyBindPress(string name) override;

	// SAction handler
	bool	handleAction(string id) override;

private:
	MapEditContext*	editor;
	int				frametime_last;
	vector<int>		fps_avg;
	int				fr_idle;
	sf::Clock		sfclock;
	double			mwheel_rotation;

	// Mouse stuff
	bool	mouse_selbegin;
	bool	mouse_movebegin;
	bool	mouse_locked;
	bool	mouse_warp;


	// Events
	void	onSize(wxSizeEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyUp(wxKeyEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onMouseUp(wxMouseEvent& e);
	void	onMouseMotion(wxMouseEvent& e);
	void	onMouseWheel(wxMouseEvent& e);
	void	onMouseLeave(wxMouseEvent& e);
	void	onMouseEnter(wxMouseEvent& e);
	void	onIdle(wxIdleEvent& e);
	void	onRTimer(wxTimerEvent& e);
	void	onFocus(wxFocusEvent& e);
};

#endif //__MAPCANVAS_H__
