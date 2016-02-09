#pragma once

#include "Widget.h"

class GLTexture;
class SImage;
class Palette8bit;

namespace GLUI
{
	class ImageBox : public Widget
	{
	private:
		GLTexture*	texture;
		int			background;
		double		max_scale;
		rgba_t		image_colour;

	public:
		ImageBox(Widget* parent);
		virtual ~ImageBox();

		GLTexture*	getTexture() { return texture; }
		int			getBackgroundStyle() { return background; }
		double		getMaxImageScale() { return max_scale; }
		rgba_t		getImageColour() { return image_colour; }

		void	setTexture(GLTexture* texture);
		void	setBackgroundStyle(int style) { background = style; }
		void	setMaxImageScale(double scale) { max_scale = scale; }
		void	setImageColour(rgba_t colour) { image_colour = colour; }
		void	setSizeFromImage();

		bool	loadImage(SImage* image, Palette8bit* palette = NULL);

		void	drawWidget(point2_t pos);

		enum
		{
			BACKGROUND_NONE = 0,
			BACKGROUND_CHECKERBOARD,
		};
	};
}
