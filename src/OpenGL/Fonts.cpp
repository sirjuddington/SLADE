
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Fonts.cpp
 * Description: OpenGL fonts handling
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "Fonts.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Float, gl_ui_scale)


/*******************************************************************
 * FONT STRUCT FUNCTIONS
 *******************************************************************/

/* Font::Font
 * Font struct default constructor
 *******************************************************************/
Fonts::Font::Font()
{
	name = defaultFontName();
	size = defaultFontSize();
}

/* Font::Font
 * Font struct constructor (name and default size)
 *******************************************************************/
Fonts::Font::Font(string name) : name(name)
{
	size = defaultFontSize();
}

/* Font::Font
 * Font struct constructor (name and size)
 *******************************************************************/
Fonts::Font::Font(string name, int size) : name(name), size(size)
{
}


/*******************************************************************
 * FONTS NAMESPACE FUNCTIONS (SFML IMPLEMENTATION)
 *******************************************************************/
#ifdef USE_SFML_RENDERWINDOW
namespace Fonts
{
	std::map<string, std::unique_ptr<GLFONT>> fonts;
}

/* Fonts::getGLFont
 * Returns an OpenGL font matching [font], creating one if necessary
 *******************************************************************/
GLFONT* Fonts::getGLFont(Font& font)
{
	// Find existing font
	// We can ignore the requested size here as SFML sets this when drawing with sf::Text
	if (fonts[font.name] != nullptr)
		return fonts[font.name].get();

	// Existing font not found, try to load it
	ArchiveEntry* entry =
		theArchiveManager->programResourceArchive()->entryAtPath(S_FMT("fonts/%s.ttf", font.name));
	if (entry)
	{
		sf::Font* f = new sf::Font();
		if (f->loadFromMemory((const char*)entry->getData(), entry->getSize()))
		{
			fonts[font.name] = std::unique_ptr<GLFONT>(f);
			return f;
		}
	}

	// Font not found in program resource or was an invalid format
	return nullptr;
}

/* Fonts::textExtents
 * Returns the width and height of [text] when drawn with [font]
 *******************************************************************/
fpoint2_t Fonts::textExtents(string text, Font& font)
{
	// Setup SFML string
	sf::Text sf_str;
	sf_str.setString(CHR(text));

	// Set font
	sf::Font* f = getGLFont(font);
	sf_str.setFont(*f);
	sf_str.setCharacterSize(font.size);

	// Return width and height of text
	sf::FloatRect rect = sf_str.getGlobalBounds();
	return fpoint2_t(rect.width, rect.height);
}

/* Fonts::getFontLineHeight
 * Returns the line height of [font]
 *******************************************************************/
int Fonts::getFontLineHeight(Font& font)
{
	return getGLFont(font)->getLineSpacing(font.size) * gl_ui_scale;
}

/* Fonts::resetFontCache
 * Clears out the font cache
 *******************************************************************/
void Fonts::resetFontCache()
{
	fonts.clear();
}


/*******************************************************************
 * FONTS NAMESPACE FUNCTIONS (FTGL IMPLEMENTATION)
 *******************************************************************/
#else
namespace Fonts
{
	vector<FTFont*>	fonts;
}

/* Fonts::getGLFont
 * Returns an OpenGL font matching [font], creating one if necessary
 *******************************************************************/
GLFONT* Fonts::getGLFont(string name, int size)
{
	// TODO
	return nullptr;
}

/* Fonts::textExtents
 * Returns the width and height of [text] when drawn with [font]
 *******************************************************************/
fpoint2_t Fonts::textExtents(string text, string font, int size)
{
	// Get desired font
	FTFont* ftgl_font = getFont(font, size);

	// If FTGL font is invalid, return empty
	if (!ftgl_font)
		return fpoint2_t(0,0);

	// Return width and height of text
	FTBBox bbox = ftgl_font->BBox(CHR(text), -1);
	return fpoint2_t(bbox.Upper().X() - bbox.Lower().X(), ftgl_font->LineHeight());
}

/* Fonts::getFontLineHeight
 * Returns the line height of [font]
 *******************************************************************/
int Drawing::getFontLineHeight(string font, int size)
{
	return textExtents("Wg", font, size).y;
}

/* Fonts::resetFontCache
 * Clears out the font cache
 *******************************************************************/
void Fonts::resetFontCache()
{
	fonts.clear();
}

#endif


/*******************************************************************
 * FONTS NAMESPACE FUNCTIONS (GENERAL)
 *******************************************************************/

/* Fonts::defaultFontName
 * Returns the default font name
 *******************************************************************/
string Fonts::defaultFontName()
{
	return "Ubuntu-Medium";
}

/* Fonts::defaultBoldFontName
 * Returns the default bold font name
 *******************************************************************/
string Fonts::defaultBoldFontName()
{
	return "Ubuntu-Bold";
}

/* Fonts::defaultFontSize
 * Returns the default font size
 *******************************************************************/
int Fonts::defaultFontSize()
{
	return 12;
}

/* Fonts::regularFont
 * Returns a default regular font of default size
 *******************************************************************/
Fonts::Font	Fonts::regularFont()
{
	return Font(defaultFontName());
}

/* Fonts::regularFont
 * Returns a default regular font of default size * [size_mult]
 *******************************************************************/
Fonts::Font	Fonts::regularFont(float size_mult)
{
	return Font(defaultFontName(), MathStuff::round(defaultFontSize() * size_mult));
}

/* Fonts::boldFont
 * Returns a default bold font of default size
 *******************************************************************/
Fonts::Font	Fonts::boldFont()
{
	return Font(defaultBoldFontName());
}

/* Fonts::boldFont
 * Returns a default bold font of default size * [size_mult]
 *******************************************************************/
Fonts::Font	Fonts::boldFont(float size_mult)
{
	return Font(defaultBoldFontName(), MathStuff::round(defaultFontSize() * size_mult));
}
