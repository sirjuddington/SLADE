#pragma once

#include "General/KeyBind.h"

class MapEditContext;
class wxMouseEvent;

#undef None

namespace MapEditor
{
class Input : public KeyBindHandler
{
public:
	enum class MouseState
	{
		Normal,
		Selection,
		Move,
		ThingAngle,
		LineDraw,
		ObjectEdit,
		Paste,
		TagSectors,
		TagThings
	};

	enum class DragType
	{
		None,
		Selection,
		Move
	};

	enum MouseButton
	{
		Left = 0,
		Middle,
		Right,
		Mouse4,
		Mouse5
	};

	Input(MapEditContext& context);

	bool       panning() const { return panning_; }
	MouseState mouseState() const { return mouse_state_; }
	Vec2i      mousePos() const { return mouse_pos_; }
	Vec2d      mousePosMap() const { return mouse_pos_map_; }
	Vec2i      mouseDownPos() const { return mouse_down_pos_; }
	Vec2d      mouseDownPosMap() const { return mouse_down_pos_map_; }
	bool       shiftDown() const { return shift_down_; }
	bool       ctrlDown() const { return ctrl_down_; }
	bool       altDown() const { return alt_down_; }

	// Mouse handling
	void setMouseState(MouseState state) { mouse_state_ = state; }
	bool mouseMove(int new_x, int new_y);
	bool mouseDown(MouseButton button, bool double_click = false);
	bool mouseUp(MouseButton button);
	void mouseWheel(bool up, double amount);
	void mouseLeave();

	// Keyboard handling
	void updateKeyModifiers(bool shift, bool ctrl, bool alt)
	{
		shift_down_ = shift;
		ctrl_down_  = ctrl;
		alt_down_   = alt;
	}
	void updateKeyModifiersWx(int modifiers);
	bool keyDown(std::string_view key) const;
	bool keyUp(std::string_view key) const;

	// Keybind handling
	void onKeyBindPress(const wxString& name) override;
	void onKeyBindRelease(const wxString& name) override;
	void handleKeyBind2dView(std::string_view name);
	void handleKeyBind2d(std::string_view name);
	void handleKeyBind3d(std::string_view name) const;
	bool updateCamera3d(double mult) const;

	static std::string mouseButtonKBName(MouseButton button);

private:
	MapEditContext& context_;

	// Mouse
	MouseState mouse_state_          = MouseState::Normal;
	bool       mouse_button_down_[5] = { false, false, false, false, false };
	Vec2i      mouse_pos_            = { 0, 0 };
	Vec2d      mouse_pos_map_        = { 0, 0 };
	Vec2i      mouse_down_pos_       = { -1, -1 };
	Vec2d      mouse_down_pos_map_   = { -1, -1 };
	DragType   mouse_drag_           = DragType::None;
	double     mouse_wheel_speed_    = 0;
	bool       panning_              = false;

	// Keyboard
	bool shift_down_ = false;
	bool ctrl_down_  = false;
	bool alt_down_   = false;
};
} // namespace MapEditor
