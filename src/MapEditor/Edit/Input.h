#pragma once

#include "General/KeyBind.h"

class MapEditContext;

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

		Input(MapEditContext& context);

		bool		panning() const { return panning_; }
		MouseState	mouseState() const { return mouse_state_; }
		point2_t	mousePos() const { return mouse_pos_; }
		fpoint2_t	mousePosMap() const { return mouse_pos_map_; }
		point2_t	mouseDownPos() const { return mouse_down_pos_; }
		fpoint2_t	mouseDownPosMap() const { return mouse_down_pos_map_; }
		bool		shiftDown() const { return shift_down_; }
		bool		ctrlDown() const { return ctrl_down_; }
		bool		altDown() const { return alt_down_; }

		void	setPanning(bool pan) { panning_ = pan; } // TODO: Remove

		void	setMouseState(MouseState state) { mouse_state_ = state; }
		void	mouseMove(int new_x, int new_y);
		void	mouseDown();
		void	mouseUp();
		void 	mouseWheel(bool up, double amount);

		void	updateKeyModifiersWx(int modifiers);

		// Keybind handling
		void	onKeyBindPress(string name) override;
		void	onKeyBindRelease(string name) override;
		void	handleKeyBind2dView(const string& name);
		void 	handleKeyBind2d(const string& name);
		void 	handleKeyBind3d(const string& name);
		bool	updateCamera3d(double mult);

	private:
		MapEditContext&	context_;
		bool			panning_;

		// Mouse
		MouseState	mouse_state_;
		point2_t	mouse_pos_;
		fpoint2_t	mouse_pos_map_;
		point2_t	mouse_down_pos_;
		fpoint2_t	mouse_down_pos_map_;
		DragType	mouse_drag_;
		double 		mouse_wheel_speed_;

		// Keyboard
		bool	shift_down_;
		bool	ctrl_down_;
		bool	alt_down_;
	};
}
