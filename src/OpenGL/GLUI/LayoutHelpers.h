#pragma once

#include "GLUI.h"

namespace GLUI
{
	enum
	{
		USE_MARGIN = -1,
	};

	namespace LayoutHelpers
	{
		void	placeWidgetAbove(Widget* widget, Widget* base, int padding = USE_MARGIN, int align = ALIGN_NONE);
		void	placeWidgetBelow(Widget* widget, Widget* base, int padding = USE_MARGIN, int align = ALIGN_NONE);
		void	placeWidgetToLeft(Widget* widget, Widget* base, int padding = USE_MARGIN, int align = ALIGN_NONE);
		void	placeWidgetToRight(Widget* widget, Widget* base, int padding = USE_MARGIN, int align = ALIGN_NONE);
		void	placeWidgetWithin(Widget* widget, rect_t rect, int align_h = ALIGN_LEFT, int align_v = ALIGN_TOP, padding_t padding = padding_t(0));
		void	placeWidgetWithinParent(Widget* widget, int align_h = ALIGN_LEFT, int align_v = ALIGN_TOP, padding_t padding = padding_t(0));

		void	alignLefts(Widget* widget, Widget* base);
		void	alignTops(Widget* widget, Widget* base);
		void	alignRights(Widget* widget, Widget* base);
		void	alignBottoms(Widget* widget, Widget* base);
		void	alignMiddlesV(Widget* widget, Widget* base);
		void	alignMiddlesH(Widget* widget, Widget* base);

		void	sameWidth(Widget* widget, Widget* base);
		void	sameWidthLargest(Widget* w1, Widget* w2);
		void	sameHeight(Widget* widget, Widget* base);
		void	sameHeightLargest(Widget* w1, Widget* w2);
	}
}
