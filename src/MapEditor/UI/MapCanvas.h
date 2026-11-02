#pragma once

#include "General/KeyBind.h"
#include "UI/Canvas/GL/GLCanvas.h"

namespace sf
{
class Clock;
}

namespace slade
{
namespace mapeditor
{
	class MapEditContext;
}


class MapCanvas : public GLCanvas, public KeyBindHandler
{
public:
	MapCanvas(wxWindow* parent, mapeditor::MapEditContext* context);
	~MapCanvas() override = default;

	// Drawing
	void draw() override;

	// Mouse
	void warpMouse(const Vec2i& pos);
	void mouseToCenter();
	void lockMouse(bool lock);
	void mouseLook3d();

	// Keybind handling
	void onKeyBindPress(string_view name) override;

private:
	mapeditor::MapEditContext* context_                = nullptr;
	bool                       mouse_warp_             = false;
	int                        mouse_look_skip_frames_ = 0;
	vector<int>                fps_avg_;
	unique_ptr<sf::Clock>      sf_clock_;
	wxTimer                    timer_;
	long                       next_frame_ms_        = 0;
	long                       last_wheel_timestamp_ = -1;
	bool                       mouse_looking_        = false;
	Vec2i                      mouse_locked_pos_{ -1, -1 };

	void update();

	// Events
	void onSize(wxSizeEvent& e);
	void onKeyDown(wxKeyEvent& e);
	void onKeyUp(wxKeyEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onMouseUp(wxMouseEvent& e);
	void onMouseMotion(wxMouseEvent& e);
	void onMouseWheel(wxMouseEvent& e);
	void onMouseLeave(wxMouseEvent& e);
	void onMouseEnter(wxMouseEvent& e);
	void onIdle(wxIdleEvent& e);
	void onRefreshTimer(wxTimerEvent& e);
	void onFocus(wxFocusEvent& e);
};
} // namespace slade
