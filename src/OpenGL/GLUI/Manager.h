#pragma once

class OGLCanvas;

namespace GLUI
{
	class Widget;
	class Animator;

	class Manager
	{
	public:
		Manager(OGLCanvas* canvas);
		~Manager();

		enum
		{
			DOCK_NONE = 0,
			DOCK_LEFT,
			DOCK_TOP,
			DOCK_RIGHT,
			DOCK_BOTTOM
		};

		struct widget_info_t
		{
			Widget*		widget;
			uint8_t		dock;

			widget_info_t(Widget* widget, uint8_t dock = DOCK_NONE)
				: widget(widget), dock(dock) {}
		};

		void	addWidget(Widget* widget, uint8_t dock = DOCK_NONE);

		void	update(int time);
		void	drawWidgets();

	private:
		OGLCanvas*				canvas;
		vector<widget_info_t>	widgets;
		dim2_t					canvas_size;

		void	applyDocking(widget_info_t& inf);
	};
}
