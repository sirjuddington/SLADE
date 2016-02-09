#pragma once

#include "Widget.h"

namespace GLUI
{
	class Panel : public Widget
	{
	private:
		rgba_t	col_bg;

	public:
		Panel(Widget* parent);
		virtual ~Panel();

		rgba_t	getBGCol() { return col_bg; }
		void	setBGCol(rgba_t colour) { col_bg.set(colour); }

		virtual void	drawWidget(point2_t pos);

		static rgba_t getDefaultBGCol();
	};
}
