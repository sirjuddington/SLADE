#pragma once

#include "Widget.h"

//class TextBox;

namespace GLUI
{
	class TextBox : public Widget
	{
	private:
		string			text;
		vector<string>	lines;
		int				alignment;
		rgba_t			colour;
		double			line_height;
		int				font;
		double			prev_scale;

		int text_width_full;
		int text_height_full;
		int	text_max_line_width;
		int	line_height_pixels;

		void	splitText(int fit_width);

	public:
		TextBox(Widget* parent, string text, int font = 1, int alignment = 0, rgba_t colour = COL_WHITE, double line_height = 1.1);
		virtual ~TextBox();

		int		getLineHeightPixels();
		dim2_t	getFullTextSize() { return dim2_t(text_width_full, text_height_full); }

		void	setText(string text);
		void	setAlignment(int alignment) { this->alignment = alignment; }
		void	setLineHeight(double mult);
		void	setLineHeightPixels(int line_height);
		void	setColour(rgba_t colour) { this->colour = colour; }
		void	setFont(int font);

		// Widget
		void	drawWidget(point2_t pos, float alpha) override;
		void	updateLayout(dim2_t fit = dim2_t(-1, -1)) override;

		// Static
		static rgba_t	defaultColour();
	};
}
