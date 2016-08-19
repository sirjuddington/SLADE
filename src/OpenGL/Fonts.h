
#pragma once

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#define GLFONT sf::Font
#else
#include <FTGL/ftgl.h>
#define GLFONT FTFont
#endif

namespace Fonts
{
	struct Font
	{
		string	name;
		int		size;

		Font();
		Font(string name);
		Font(string name, int size);
	};

	GLFONT*		getGLFont(Font& font);
	fpoint2_t	textExtents(string text, Font& font);
	int			getFontLineHeight(Font& font);
	void		resetFontCache();

	// Defaults
	string	defaultFontName();
	string	defaultBoldFontName();
	int		defaultFontSize();

	Font	regularFont();
	Font	regularFont(float size_mult);
	Font	boldFont();
	Font	boldFont(float size_mult);
}
