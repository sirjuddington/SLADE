#pragma once

#include "GLUI.h"

namespace GLUI
{
	class Widget
	{
	public:
		enum
		{
			BORDER_NONE,
			BORDER_LINE,
		};

		Widget(Widget* parent);
		virtual ~Widget();

		Widget*			getParent() { return parent; }
		vector<Widget*>	getChildren() { return children; }
		point2_t		getPosition() { return position; }
		point2_t		getAbsolutePosition();
		dim2_t			getSize() { return size; }
		int				getWidth() { return size.x; }
		int				getHeight() { return size.y; }
		bool			isVisible() { return visible; }
		padding_t		getMargin() { return margin; }
		float			getBorderWidth() { return border_width; }
		int				getBorderStyle() { return border_style; }
		rgba_t			getBorderColour() { return border_colour; }

		int	left(bool margin = false) { return position.x - (margin ? this->margin.left : 0); }
		int	top(bool margin = false) { return position.y - (margin ? this->margin.top : 0); }
		int right(bool margin = false) { return position.x + size.x + (margin ? this->margin.right : 0); }
		int bottom(bool margin = false) { return position.y + size.y + (margin ? this->margin.bottom : 0); }
		point2_t middle();

		void	setPosition(point2_t pos) { position = pos; }
		void	setSize(dim2_t dim) { size = dim; }
		void	setVisible(bool vis) { visible = vis; }
		void	setMargin(padding_t margin) { this->margin = margin; }
		void	setBorderWidth(float width) { border_width = width; }
		void	setBorderStyle(int style) { border_style = style; }
		void	setBorderColour(rgba_t colour) { border_colour = colour; }
		void	setBorder(float width, int style, rgba_t colour = COL_WHITE);

		// Drawing
		void			draw(point2_t pos, float alpha = 1.0f);
		virtual void	drawWidget(point2_t pos, float alpha) {}

		// Layout
		virtual void	updateLayout(dim2_t fit = dim2_t(-1, -1)) {}
		void			fitToChildren(padding_t padding = padding_t(0));

	protected:
		Widget*			parent;
		vector<Widget*>	children;
		point2_t		position;
		dim2_t			size;
		bool			visible;
		padding_t		margin;

		// Border
		float	border_width;
		int		border_style;
		rgba_t	border_colour;

		// Display
		float	alpha;
	};
}
