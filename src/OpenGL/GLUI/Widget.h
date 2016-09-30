#pragma once

#include "GLUI.h"
#include "Animator.h"
#include "Event.h"

namespace GLUI
{
	class Widget
	{
	public:
		typedef unique_ptr<Widget> Ptr;

		enum class Border
		{
			None,
			Line
		};

		enum class StdAnim
		{
			Visible = 0,
			MouseOver = 1,
		};

		Widget(Widget* parent);
		virtual ~Widget() {}

		Widget*			getParent() { return parent; }
		vector<Widget*>	getChildren();
		point2_t		getPosition() { return position; }
		point2_t		getAbsolutePosition();
		dim2_t			getSize() { return size; }
		int				getWidth() { return size.x; }
		int				getHeight() { return size.y; }
		bool			isVisible();
		padding_t		getMargin() { return margin; }
		float			getBorderWidth() { return border_width; }
		Border			getBorderStyle() { return border_style; }
		rgba_t			getBorderColour() { return border_colour; }
		bool			mouseIsOver() { return mouse_over; }

		int	left(bool margin = false) { return position.x - (margin ? this->margin.left : 0); }
		int	top(bool margin = false) { return position.y - (margin ? this->margin.top : 0); }
		int right(bool margin = false) { return position.x + size.x + (margin ? this->margin.right : 0); }
		int bottom(bool margin = false) { return position.y + size.y + (margin ? this->margin.bottom : 0); }
		point2_t middle();

		void	setPosition(point2_t pos);
		void	setSize(dim2_t dim);
		void	setVisible(bool vis, bool animate = true);
		void	setMargin(padding_t margin) { this->margin = margin; }
		void	setBorderWidth(float width) { border_width = width; }
		void	setBorderStyle(Border style) { border_style = style; }
		void	setBorderColour(rgba_t colour) { border_colour = colour; }
		void	setBorder(float width, Border style, rgba_t colour = COL_WHITE);

		// Drawing
		void			draw(fpoint2_t pos, float alpha = 1.0f, fpoint2_t scale = fpoint2_t (1, 1));
		virtual void	drawWidget(fpoint2_t pos, float alpha, fpoint2_t scale) {}

		// Layout
		virtual void	updateLayout(dim2_t fit = dim2_t(-1, -1)) {}
		void			fitToChildren(padding_t padding = padding_t(0), bool include_invisible = false);

		// Animation
		fpoint2_t	getAnimatedOffset();
		fpoint2_t	getAnimatedScale();
		float		getAnimatedAlpha();
		void		animate(int time);
		Animator*	getStandardAnimation(StdAnim anim) { return anim_standard[anim]; }
		void		setStandardAnimation(StdAnim anim, Animator::Ptr animator);

		// Input
		void	mouseMove(int x, int y);
		void	mouseOver(bool is_over);
		void	mouseButtonDown(MouseBtn button, int x, int y);
		void	mouseButtonUp(MouseBtn button, int x, int y);
		bool	keyDown(string key, bool shift, bool ctrl, bool alt);
		bool	keyUp(string key, bool shift, bool ctrl, bool alt);

		// Events
		EventHandler<EventInfo&>&		evtPosChanged() { return evt_pos_changed; }
		EventHandler<EventInfo&>&		evtSizeChanged() { return evt_size_changed; }
		EventHandler<EventInfo&>&		evtVisibleChanged() { return evt_visible_changed; }
		EventHandler<MouseEventInfo&>&	evtMouseMove() { return evt_mouse_move; }
		EventHandler<EventInfo&>&		evtMouseEnter() { return evt_mouse_enter; }
		EventHandler<EventInfo&>&		evtMouseLeave() { return evt_mouse_leave; }
		EventHandler<MouseEventInfo&>&	evtMouseDown() { return evt_mouse_down; }
		EventHandler<MouseEventInfo&>&	evtMouseUp() { return evt_mouse_up; }

	protected:
		Widget*		parent;
		vector<Ptr>	children;
		point2_t	position;
		dim2_t		size;
		bool		visible;
		padding_t	margin;

		// Border
		float	border_width;
		Border	border_style;
		rgba_t	border_colour;

		// Display/Animation
		float							alpha;
		vector<Animator::Ptr>			animators;
		std::map<StdAnim, Animator*>	anim_standard;

		// Input
		bool	mouse_over;

		// Event handlers
		EventHandler<EventInfo&>		evt_pos_changed;
		EventHandler<EventInfo&>		evt_size_changed;
		EventHandler<EventInfo&>		evt_visible_changed;
		EventHandler<MouseEventInfo&>	evt_mouse_move;
		EventHandler<EventInfo&>		evt_mouse_enter;
		EventHandler<EventInfo&>		evt_mouse_leave;
		EventHandler<MouseEventInfo&>	evt_mouse_down;
		EventHandler<MouseEventInfo&>	evt_mouse_up;
		EventHandler<KeyEventInfo&>		evt_key_down;
		EventHandler<KeyEventInfo&>		evt_key_up;
	};
}
