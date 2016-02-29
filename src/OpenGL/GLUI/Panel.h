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

		rgba_t getBGCol(float alpha = 1.0f)
		{
			return rgba_t(col_bg.r, col_bg.g, col_bg.b, col_bg.a * alpha, col_bg.blend);
		}
		void setBGCol(rgba_t colour) { col_bg.set(colour); }

		virtual void	drawWidget(point2_t pos, float alpha) override;

		static rgba_t getDefaultBGCol();
	};
}
