
#ifndef __MAPCANVAS_H__
#define __MAPCANVAS_H__

#include "common.h"
#include "General/KeyBind.h"
#include "UI/Canvas/OGLCanvas.h"
#include "MapEditor/MapEditContext.h"

class MapCanvas : public OGLCanvas, public KeyBindHandler
{
public:
	MapCanvas(wxWindow* parent, int id, MapEditContext* context);
	~MapCanvas();

	// Drawing
	void	draw() override;

	// Mouse
	void	mouseToCenter();
	void	lockMouse(bool lock);
	void	mouseLook3d();

	// Keybind handling
	void	onKeyBindPress(string name) override;

private:
	MapEditContext*	context_	= nullptr;
	bool			mouse_warp_ = false;
	vector<int>		fps_avg_;
	sf::Clock		sf_clock_;

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
