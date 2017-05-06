
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Drawing.cpp
 * Description: Various OpenGL drawing functions
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
#include "Drawing.h"
#include "GLTexture.h"
#include "Archive/ArchiveManager.h"
#include "General/Console/Console.h"
#include "Utility/MathStuff.h"
#include "General/Misc.h"
#include "OpenGL.h"

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#else
#include <FTGL/ftgl.h>
#endif

#ifdef __WXGTK20__
#define GSocket GlibGSocket
#include <gtk-2.0/gtk/gtk.h>
#undef GSocket
#endif


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, hud_statusbar, 1, CVAR_SAVE)
CVAR(Bool, hud_center, 1, CVAR_SAVE)
CVAR(Bool, hud_wide, 0, CVAR_SAVE)
CVAR(Bool, hud_bob, 0, CVAR_SAVE)
CVAR(Int, gl_font_size, 12, CVAR_SAVE)

namespace Drawing
{
#ifdef USE_SFML_RENDERWINDOW
	sf::RenderWindow*	render_target = NULL;
	bool				text_state_reset = true;
#endif
	double				text_outline_width = 0;
	rgba_t				text_outline_colour = COL_BLACK;
};


/*******************************************************************
 * FONTMANAGER CLASS
 *******************************************************************/
class FontManager
{
private:
#ifdef USE_SFML_RENDERWINDOW
	sf::Font	font_normal;
	sf::Font	font_condensed;
	sf::Font	font_bold;
	sf::Font	font_boldcondensed;
	sf::Font	font_mono;
	sf::Font	font_small;
#else
	FTFont*		font_normal;
	FTFont*		font_condensed;
	FTFont*		font_bold;
	FTFont*		font_boldcondensed;
	FTFont*		font_mono;
	FTFont*		font_small;
#endif
	static FontManager*	instance;

public:
	FontManager()
	{
#ifndef USE_SFML_RENDERWINDOW
		font_normal = NULL;
		font_condensed = NULL;
		font_bold = NULL;
		font_boldcondensed = NULL;
		font_mono = NULL;
		font_small = NULL;
#endif
	}
	~FontManager()
	{
#ifndef USE_SFML_RENDERWINDOW
		if (font_normal)		{ delete font_normal;			font_normal = NULL;			}
		if (font_condensed)		{ delete font_condensed;		font_condensed = NULL;		}
		if (font_bold)			{ delete font_bold;				font_bold = NULL;			}
		if (font_boldcondensed) { delete font_boldcondensed;	font_boldcondensed = NULL;	}
		if (font_mono)			{ delete font_mono;				font_mono = NULL;			}
		if (font_small)			{ delete font_small;			font_small = NULL;			}
#endif
	}

	static FontManager*	getInstance()
	{
		if (!instance)
			instance = new FontManager();

		return instance;
	}
	int initFonts();

#ifdef USE_SFML_RENDERWINDOW
	sf::Font*	getFont(int font);
#else
	FTFont*		getFont(int font);
#endif

};
#define theFontManager FontManager::getInstance()
FontManager* FontManager::instance = NULL;


/*******************************************************************
 * FONTMANAGER CLASS FUNCTIONS
 *******************************************************************/

#ifdef USE_SFML_RENDERWINDOW

/* FontManager::initFonts
 * Loads all needed fonts for rendering. SFML 2.x implementation
 *******************************************************************/
int FontManager::initFonts()
{
	// --- Load general fonts ---
	int ret = 0;

	// Normal
	ArchiveEntry* entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry) ++ret, font_normal.loadFromMemory((const char*)entry->getData(), entry->getSize());

	// Condensed
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_c.ttf");
	if (entry) ++ret, font_condensed.loadFromMemory((const char*)entry->getData(), entry->getSize());

	// Bold
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_b.ttf");
	if (entry) ++ret, font_bold.loadFromMemory((const char*)entry->getData(), entry->getSize());

	// Condensed Bold
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_cb.ttf");
	if (entry) ++ret, font_boldcondensed.loadFromMemory((const char*)entry->getData(), entry->getSize());

	// Monospace
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_mono.ttf");
	if (entry) ++ret, font_small.loadFromMemory((const char*)entry->getData(), entry->getSize());

	return ret;
}

#else
/* FontManager::initFonts
 * Loads all needed fonts for rendering. Non-SFML implementation
 *******************************************************************/
int FontManager::initFonts()
{
	// --- Load general fonts ---
	int ret = 0;

	if (font_normal)		{ delete font_normal;			font_normal = NULL;			}
	if (font_condensed)		{ delete font_condensed;		font_condensed = NULL;		}
	if (font_bold)			{ delete font_bold;				font_bold = NULL;			}
	if (font_boldcondensed) { delete font_boldcondensed;	font_boldcondensed = NULL;	}
	if (font_mono)			{ delete font_mono;				font_mono = NULL;			}
	if (font_small)			{ delete font_small;			font_small = NULL;			}

	// Normal
	ArchiveEntry* entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_normal = new FTTextureFont(entry->getData(), entry->getSize());
		font_normal->FaceSize(gl_font_size);

		// Check it loaded ok
		if (font_normal->Error())
		{
			delete font_normal;
			font_normal = NULL;
		}
		else ++ ret;
	}

	// Condensed
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_c.ttf");
	if (entry)
	{
		font_condensed = new FTTextureFont(entry->getData(), entry->getSize());
		font_condensed->FaceSize(gl_font_size);

		// Check it loaded ok
		if (font_condensed->Error())
		{
			delete font_condensed;
			font_condensed = NULL;
		}
		else ++ ret;
	}

	// Bold
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_b.ttf");
	if (entry)
	{
		font_bold = new FTTextureFont(entry->getData(), entry->getSize());
		font_bold->FaceSize(gl_font_size);

		// Check it loaded ok
		if (font_bold->Error())
		{
			delete font_bold;
			font_bold = NULL;
		}
		else ++ ret;
	}

	// Condensed bold
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans_cb.ttf");
	if (entry)
	{
		font_boldcondensed = new FTTextureFont(entry->getData(), entry->getSize());
		font_boldcondensed->FaceSize(gl_font_size);

		// Check it loaded ok
		if (font_boldcondensed->Error())
		{
			delete font_boldcondensed;
			font_boldcondensed = NULL;
		}
		else ++ ret;
	}

	// Monospace
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_mono.ttf");
	if (entry)
	{
		font_mono = new FTTextureFont(entry->getData(), entry->getSize());
		font_mono->FaceSize(gl_font_size);

		// Check it loaded ok
		if (font_mono->Error())
		{
			delete font_mono;
			font_mono = NULL;
		}
		else ++ ret;
	}

	// Small
	entry = theArchiveManager->programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_small = new FTTextureFont(entry->getData(), entry->getSize());
		font_small->FaceSize((gl_font_size * 0.6) + 1);

		// Check it loaded ok
		if (font_small->Error())
		{
			delete font_small;
			font_small = NULL;
		}
		else ++ ret;
	}

	return ret;
}
#endif

/* FontManager::getFont
 * Returns a font
 *******************************************************************/
#ifdef USE_SFML_RENDERWINDOW
sf::Font* FontManager::getFont(int font)
{
	switch (font)
	{
	case Drawing::FONT_NORMAL:			return &font_normal;
	case Drawing::FONT_CONDENSED:		return &font_condensed;
	case Drawing::FONT_BOLD:			return &font_bold;
	case Drawing::FONT_BOLDCONDENSED:	return &font_boldcondensed;
	case Drawing::FONT_MONOSPACE:		return &font_mono;
	case Drawing::FONT_SMALL:			return &font_small;
	default:							return &font_normal;
	};
	return NULL;
}
#else // USE_SFML_RENDERWINDOW
FTFont* FontManager::getFont(int font)
{
	switch (font)
	{
	case Drawing::FONT_NORMAL:			return font_normal;
	case Drawing::FONT_CONDENSED:		return font_condensed;
	case Drawing::FONT_BOLD:			return font_bold;
	case Drawing::FONT_BOLDCONDENSED:	return font_boldcondensed;
	case Drawing::FONT_MONOSPACE:		return font_mono;
	case Drawing::FONT_SMALL:			return font_small;
	default:							return font_normal;
	};
	return NULL;
}
#endif // USE_SFML_RENDERWINDOW


/*******************************************************************
 * DRAWING NAMESPACE FUNCTIONS
 *******************************************************************/

/* Drawing::initFonts
 * Creates a FontManager if needed and let it init its own fonts
 *******************************************************************/
void Drawing::initFonts()
{
	theFontManager->initFonts();
}

/* Drawing::drawLine
 * Draws a line from [start] to [end]
 *******************************************************************/
void Drawing::drawLine(fpoint2_t start, fpoint2_t end)
{
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();
}

/* Drawing::drawLine
 * Draws a line from [x1,y1] to [x2,y2]
 *******************************************************************/
void Drawing::drawLine(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glEnd();
}

/* Drawing::drawLineTabbed
 * Draws a line from [start] to [end]
 *******************************************************************/
void Drawing::drawLineTabbed(fpoint2_t start, fpoint2_t end, double tab, double tab_max)
{
	// Draw line
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();

	// Calculate midpoint
	fpoint2_t mid;
	mid.x = start.x + ((end.x - start.x) * 0.5);
	mid.y = start.y + ((end.y - start.y) * 0.5);

	// Calculate tab length
	double tablen = MathStuff::distance(start, end) * tab;
	if (tablen > tab_max) tablen = tab_max;
	if (tablen < 2) tablen = 2;

	// Calculate tab endpoint
	fpoint2_t invdir(-(end.y - start.y), end.x - start.x);
	invdir.normalize();

	// Draw tab
	glBegin(GL_LINES);
	glVertex2d(mid.x, mid.y);
	glVertex2d(mid.x - invdir.x*tablen, mid.y - invdir.y*tablen);
	glEnd();
}

/* Drawing::drawArrow
 * Draws a line from [p1] to [p2] with an arrowhead at the [p1] end.
 * If [twoway] is true, an arrowhead is also drawn at the [p2] end
 *******************************************************************/
void Drawing::drawArrow(fpoint2_t p1, fpoint2_t p2, rgba_t color, bool twoway, double ah_angle, double ah_length)
{
	fpoint2_t a1l, a1r, a2l, a2r;
	fpoint2_t vector = p1 - p2;
	double angle = atan2(-vector.y, vector.x);
	a1l = a1r = p1;
	a1l.x += ah_length * sin(angle - ah_angle); a1l.y += ah_length * cos(angle - ah_angle);
	a1r.x -= ah_length * sin(angle + ah_angle); a1r.y -= ah_length * cos(angle + ah_angle);
	if (twoway)
	{
		vector = p2 - p1;
		angle = atan2(-vector.y, vector.x);
		a2l = a2r = p2;
		a2l.x += ah_length * sin(angle - ah_angle); a2l.y += ah_length * cos(angle - ah_angle);
		a2r.x -= ah_length * sin(angle + ah_angle); a2r.y -= ah_length * cos(angle + ah_angle);
	}
	OpenGL::setColour(color);
	glBegin(GL_LINES);
	glVertex2d(p1.x, p1.y);
	glVertex2d(p2.x, p2.y);
	glVertex2d(p1.x, p1.y);
	glVertex2d(a1l.x, a1l.y);
	glVertex2d(p1.x, p1.y);
	glVertex2d(a1r.x, a1r.y);
	glEnd();
	if (twoway)
	{
		glBegin(GL_LINES);
		glVertex2d(p2.x, p2.y);
		glVertex2d(a2l.x, a2l.y);
		glEnd();
		glBegin(GL_LINES);
		glVertex2d(p2.x, p2.y);
		glVertex2d(a2r.x, a2r.y);
		glEnd();
	}
}

/* Drawing::drawRect
 * Draws a rectangle from [tl] to [br]
 *******************************************************************/
void Drawing::drawRect(fpoint2_t tl, fpoint2_t br)
{
	glBegin(GL_LINE_LOOP);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();
}

/* Drawing::drawRect
 * Draws a rectangle from [x1,y1] to [x2,y2]
 *******************************************************************/
void Drawing::drawRect(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINE_LOOP);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();
}

/* Drawing::drawFilledRect
 * Draws a filled rectangle from [tl] to [br]
 *******************************************************************/
void Drawing::drawFilledRect(fpoint2_t tl, fpoint2_t br)
{
	glBegin(GL_QUADS);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();
}

/* Drawing::drawFilledRect
 * Draws a filled rectangle from [x1,y1] to [x2,y2]
 *******************************************************************/
void Drawing::drawFilledRect(double x1, double y1, double x2, double y2)
{
	glBegin(GL_QUADS);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();
}

/* Drawing::drawBorderedRect
 * Draws a filled rectangle with a border from [x1,y1] to [x2,y2]
 *******************************************************************/
void Drawing::drawBorderedRect(fpoint2_t tl, fpoint2_t br, rgba_t colour, rgba_t border_colour)
{
	drawBorderedRect(tl.x, tl.y, br.x, br.y, colour, border_colour);
}

/* Drawing::drawBorderedRect
 * Draws a filled rectangle with a border from [x1,y1] to [x2,y2]
 *******************************************************************/
void Drawing::drawBorderedRect(double x1, double y1, double x2, double y2, rgba_t colour, rgba_t border_colour)
{
	// Rect
	OpenGL::setColour(colour, false);
	glBegin(GL_QUADS);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();

	// Border
	OpenGL::setColour(border_colour, false);
	glBegin(GL_LINE_LOOP);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2-1);
	glVertex2d(x2-1, y2-1);
	glVertex2d(x2-1, y1);
	glEnd();
}

/* Drawing::drawEllipse
 * Draws an ellipse at [mid]
 *******************************************************************/
void Drawing::drawEllipse(fpoint2_t mid, double radius_x, double radius_y, int sides, rgba_t colour)
{
	// Set colour
	OpenGL::setColour(colour, false);

	// Draw circle as line loop
	glBegin(GL_LINE_LOOP);
	double rot = 0;
	for (int a = 0; a < sides; a++)
	{
		glVertex2d(mid.x + sin(rot) * radius_x, mid.y - cos(rot) * radius_y);
		rot -= (3.1415926535897932384626433832795 * 2) / (double)sides;
	}
	glEnd();
}

/* Drawing::drawFilledEllipse
 * Draws a filled ellipse at [mid]
 *******************************************************************/
void Drawing::drawFilledEllipse(fpoint2_t mid, double radius_x, double radius_y, int sides, rgba_t colour)
{
	// Set colour
	OpenGL::setColour(colour, false);

	// Draw circle as line loop
	glBegin(GL_TRIANGLE_FAN);
	glVertex2d(mid.x, mid.y);
	double rot = 0;
	for (int a = 0; a < sides + 1; a++)
	{
		glVertex2d(mid.x + sin(rot) * radius_x, mid.y - cos(rot) * radius_y);
		rot -= (3.1415926535897932384626433832795 * 2) / (double)sides;
	}
	glEnd();
}

/* Drawing::fitTextureWithin
 * Fits [tex] within the rectangle from [x1,y1] to [x2,y2], centered
 * and keeping the correct aspect ratio. If [upscale] is true the
 * texture will be zoomed to fit the rectangle. Returns the resulting
 * texture rectangle coordinates
 *******************************************************************/
frect_t Drawing::fitTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale)
{
	// Ignore null texture
	if (!tex)
		return frect_t();

	double width = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	double x_dim = (double)tex->getWidth();
	double y_dim = (double)tex->getHeight();

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - padding) / x_dim;
	double y_scale = ((double)width - padding) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = MIN(x_scale, y_scale);

	// Clamp scale to maximum desired scale
	if (scale > max_scale)
		scale = max_scale;

	// Return the fitted rectangle
	return frect_t(x1 + width*0.5 - (scale*tex->getWidth()*0.5),
	               y1 + height*0.5 - (scale*tex->getHeight()*0.5),
	               x1 + width*0.5 + (scale*tex->getWidth()*0.5),
	               y1 + height*0.5 + (scale*tex->getHeight()*0.5));
}

/* Drawing::drawTextureWithin
 * Draws [tex] within the rectangle from [x1,y1] to [x2,y2], centered
 * and keeping the correct aspect ratio. If [upscale] is true the
 * texture will be zoomed to fit the rectangle
 *******************************************************************/
void Drawing::drawTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale)
{
	// Ignore null texture
	if (!tex)
		return;

	double width = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	double x_dim = (double)tex->getWidth();
	double y_dim = (double)tex->getHeight();

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - padding) / x_dim;
	double y_scale = ((double)height - padding) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = MIN(x_scale, y_scale);

	// Clamp scale to maximum desired scale
	if (scale > max_scale)
		scale = max_scale;

	// Now draw the texture
	glPushMatrix();
	glTranslated(x1 + width*0.5, y1 + height*0.5, 0);	// Translate to middle of area
	glScaled(scale, scale, scale);						// Scale to fit within area
	tex->draw2d(tex->getWidth()*-0.5, tex->getHeight()*-0.5);
	glPopMatrix();
}

#ifdef USE_SFML_RENDERWINDOW
/*******************************************************************
 * SFML 2.x TEXT FUNCTION IMPLEMENTATIONS
 *******************************************************************/

/* Drawing::drawText
 * Draws [text] at [x,y]. If [bounds] is not null, the bounding
 * coordinates of the rendered text string are written to it.
 *******************************************************************/
void Drawing::drawText(string text, int x, int y, rgba_t colour, int font, int alignment, frect_t* bounds)
{
	// Setup SFML string
	sf::Text sf_str;
	sf_str.setString(UTF8(text));
	sf_str.setColor(sf::Color(colour.r, colour.g, colour.b, colour.a));

	// Set font
	sf::Font* f = theFontManager->getFont(font);
	sf_str.setFont(*f);
	if (font == FONT_SMALL)
		sf_str.setCharacterSize((gl_font_size * 0.6) + 1);
	else
		sf_str.setCharacterSize(gl_font_size);

	// Setup alignment
	if (alignment != ALIGN_LEFT)
	{
		float width = sf_str.getLocalBounds().width;

		if (alignment == ALIGN_CENTER)
			x -= MathStuff::round(width*0.5);
		else
			x -= width;
	}
	sf_str.setPosition(x, y);

	// Set bounds rect
	if (bounds)
	{
		sf::FloatRect rect = sf_str.getGlobalBounds();
		bounds->set(rect.left, rect.top, rect.left+rect.width, rect.top+rect.height);
	}

	// Draw the string
	if (render_target)
	{
		if (text_state_reset)
			setTextState(true);

		if (text_outline_width > 0)
		{
#if (SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR >= 4) || SFML_VERSION_MAJOR > 2
			// Set text outline if SFML version is 2.4 or later
			sf_str.setOutlineThickness(text_outline_width);
			sf_str.setOutlineColor(
				sf::Color(
					text_outline_colour.r,
					text_outline_colour.g,
					text_outline_colour.b,
					text_outline_colour.a
				)
			);
#else
			// On SFML < 2.4, use old hacky outline method
			sf_str.setColor(
				sf::Color(
					text_outline_colour.r,
					text_outline_colour.g,
					text_outline_colour.b,
					text_outline_colour.a
				)
			);
			sf_str.setPosition(x - 2, y - 1);
			render_target->draw(sf_str);
			sf_str.setPosition(x - 2, y + 1);
			render_target->draw(sf_str);
			sf_str.setPosition(x + 2, y + 1);
			render_target->draw(sf_str);
			sf_str.setPosition(x + 2, y - 1);
			render_target->draw(sf_str);
			sf_str.setPosition(x, y);
			sf_str.setColor(sf::Color(colour.r, colour.g, colour.b, colour.a));
#endif
		}

		// Draw
		render_target->draw(sf_str);

		if (text_state_reset)
			setTextState(false);
	}
}

/* Drawing::textExtents
 * Returns the width and height of [text] when drawn with [font]
 *******************************************************************/
fpoint2_t Drawing::textExtents(string text, int font)
{
	// Setup SFML string
	sf::Text sf_str;
	sf_str.setString(CHR(text));

	// Set font
	sf::Font* f = theFontManager->getFont(font);
	sf_str.setFont(*f);
	if (font == FONT_SMALL)
		sf_str.setCharacterSize((gl_font_size * 0.6) + 1);
	else
		sf_str.setCharacterSize(gl_font_size);

	// Return width and height of text
	sf::FloatRect rect = sf_str.getGlobalBounds();
	return fpoint2_t(rect.width, rect.height);
}

#else
/*******************************************************************
 * FTGL TEXT FUNCTION IMPLEMENTATIONS
 *******************************************************************/

/* Drawing::drawText
 * Draws [text] at [x,y]. If [bounds] is not null, the bounding
 * coordinates of the rendered text string are written to it.
 *******************************************************************/
void Drawing::drawText(string text, int x, int y, rgba_t colour, int font, int alignment, frect_t* bounds)
{
	// Get desired font
	FTFont* ftgl_font = theFontManager->getFont(font);

	// If FTGL font is invalid, do nothing
	if (!ftgl_font)
		return;

	// Setup alignment
	FTBBox bbox = ftgl_font->BBox(CHR(text), -1);
	int xpos = x;
	int ypos = y;
	float width = bbox.Upper().X() - bbox.Lower().X();
	float height = ftgl_font->LineHeight();
	if (alignment != ALIGN_LEFT)
	{
		if (alignment == ALIGN_CENTER)
			xpos -= MathStuff::round(width*0.5);
		else
			xpos -= width;
	}

	// Set bounds rect
	if (bounds)
	{
		bbox = ftgl_font->BBox(CHR(text), -1, FTPoint(xpos, ypos));
		bounds->set(bbox.Lower().X(), bbox.Lower().Y(), bbox.Upper().X(), bbox.Lower().Y() + height);
	}

	// Draw the string
	glPushMatrix();
	glTranslatef(xpos, ypos + ftgl_font->FaceSize(), 0.0f);
	glTranslatef(-0.375f, -0.375f, 0);
	glScalef(1.0f, -1.0f, 1.0f);
	if (text_outline_width > 0)
	{
		// Draw outline if set
		OpenGL::setColour(text_outline_colour);
		glTranslatef(-2.0f, -1.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(0.0f, 2.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(4.0f, 0.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(0.0f, -2.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(-2.0f, 1.0f, 0.0f);
	}
	OpenGL::setColour(colour);
	ftgl_font->Render(CHR(text), -1);
	glPopMatrix();
}

/* Drawing::textExtents
 * Returns the width and height of [text] when drawn with [font]
 *******************************************************************/
fpoint2_t Drawing::textExtents(string text, int font)
{
	// Get desired font
	FTFont* ftgl_font = theFontManager->getFont(font);

	// If FTGL font is invalid, return empty
	if (!ftgl_font)
		return fpoint2_t(0,0);

	// Return width and height of text
	FTBBox bbox = ftgl_font->BBox(CHR(text), -1);
	return fpoint2_t(bbox.Upper().X() - bbox.Lower().X(), ftgl_font->LineHeight());
}

#endif

/* Drawing::enableTextStateReset
 * When enabled, the OpenGL state is set for text rendering each time
 * drawText is called and restored after (SFML only)
 *******************************************************************/
void Drawing::enableTextStateReset(bool enable)
{
#ifdef USE_SFML_RENDERWINDOW
	text_state_reset = enable;
#endif
}

/* Drawing::setTextState
 * Sets or restores (depending on [set]) the OpenGL state for SFML
 * text rendering
 *******************************************************************/
void Drawing::setTextState(bool set)
{
#ifdef USE_SFML_RENDERWINDOW
	if (set)
	{
		// Push related states
		glPushMatrix();
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glPushAttrib(GL_VIEWPORT_BIT);
		render_target->resetGLStates();
	}
	else
	{
		// Pop related states
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
#endif
}

/* Drawing::setTextOutline
 * Sets the [thickness] and [colour] of the outline to use when
 * drawing text
 *******************************************************************/
void Drawing::setTextOutline(double thickness, rgba_t colour)
{
	text_outline_width = thickness;
	text_outline_colour = colour;
}

/* Drawing::drawHud
 * Draws doom hud offset guide lines, from the center
 *******************************************************************/
void Drawing::drawHud()
{
	// Determine some variables
	int hw = 160;
	int hh = 100;
	if (hud_wide) hw = 177;

	// Draw 320x200 screen outline
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glLineWidth(1.5f);
	drawRect(-hw, -hh, hw, hh);

	// Draw statusbar line if needed
	glLineWidth(1.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	if (hud_statusbar)
	{
		drawLine(-hw, 68, hw, 68);	// Doom's status bar: 32 pixels tall
		drawLine(-hw, 62, hw, 62);	// Hexen: 38 pixels
		drawLine(-hw, 58, hw, 58);	// Heretic: 42 pixels
	}

	// Draw center lines if needed
	if (hud_center)
	{
		drawLine(-hw, 0, hw, 0);
		drawLine(0, -hh, 0, hh);
	}

	// Draw normal screen edge guides if widescreen
	if (hud_wide)
	{
		drawLine(-160, -100, -160, 100);
		drawLine(160, -100, 160, 100);
	}

	// Draw weapon bobbing guides
	if (hud_bob)
	{
		glLineWidth(0.8f);
		drawRect(-hw - 16, -hh - 16, hw + 16, hh + 16);
	}
}

#ifdef USE_SFML_RENDERWINDOW
/* Drawing::setRenderTarget
 * Sets the SFML render target to [target]
 *******************************************************************/
void Drawing::setRenderTarget(sf::RenderWindow* target)
{
	render_target = target;
}
#endif


// The following functions are taken from CodeLite (http://codelite.org)

wxColour Drawing::getPanelBGColour()
{
#ifdef __WXGTK__
	static bool     intitialized(false);
	static wxColour bgColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

	if( !intitialized )
	{
		// try to get the background colour from a menu
		GtkWidget* menu = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		GtkStyle*   def = gtk_rc_get_style( menu );
		if(!def)
			def = gtk_widget_get_default_style();

		if(def)
		{
			GdkColor col = def->bg[GTK_STATE_NORMAL];
			bgColour = wxColour(col);
		}
		gtk_widget_destroy( menu );
		intitialized = true;
	}
	return bgColour;
#else
	return wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif
}

wxColour Drawing::getMenuTextColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
}

wxColour Drawing::getMenuBarBGColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
}

wxColour Drawing::lightColour(const wxColour& colour, float percent)
{
	if(percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	hsl_t hsl = Misc::rgbToHsl(rgba_t(COLWX(colour)));

	// Increase luminance
	hsl.l += (float)((percent * 5.0)/100.0);
	if (hsl.l > 1.0) hsl.l = 1.0;

	rgba_t rgb = Misc::hslToRgb(hsl);
	return wxColour(rgb.r, rgb.g, rgb.b);
}

wxColour Drawing::darkColour(const wxColour& colour, float percent)
{
	if(percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	hsl_t hsl = Misc::rgbToHsl(rgba_t(COLWX(colour)));

	// Decrease luminance
	hsl.l -= (float)((percent * 5.0)/100.0);
	if (hsl.l < 0) hsl.l = 0;

	rgba_t rgb = Misc::hslToRgb(hsl);
	return wxColour(rgb.r, rgb.g, rgb.b);
}


/*******************************************************************
 * TEXTBOX CLASS FUNCTIONS
 *******************************************************************/

/* TextBox::TextBox
 * TextBox class constructor
 *******************************************************************/
TextBox::TextBox(string text, int font, int width, int line_height)
{
	this->font = font;
	this->width = width;
	this->height = 0;
	this->line_height = line_height;
	setText(text);
}

/* TextBox::split
 * Splits [text] into separate lines (split by newlines), also
 * performs further splitting to word wrap the text within the box
 *******************************************************************/
void TextBox::split(string text)
{
	// Clear current text lines
	lines.clear();

	// Do nothing for empty string
	if (text.IsEmpty())
		return;

	// Split at newlines
	wxArrayString split = wxSplit(text, '\n');
	for (unsigned a = 0; a < split.Count(); a++)
		lines.push_back(split[a]);

	// Don't bother wrapping if width is really small
	if (width < 32)
		return;

	// Word wrap
	unsigned line = 0;
	while (line < lines.size())
	{
		// Ignore empty or single-character line
		if (lines[line].length() < 2)
		{
			line++;
			continue;
		}

		// Get line width
		double width = Drawing::textExtents(lines[line], font).x;

		// Continue to next line if within box
		if (width < this->width)
		{
			line++;
			continue;
		}

		// Halve length until it fits in the box
		unsigned c = lines[line].length() - 1;
		while (width >= this->width)
		{
			if (c <= 1)
				break;

			c *= 0.5;
			width = Drawing::textExtents(lines[line].Mid(0, c), font).x;
		}

		// Increment length until it doesn't fit
		while (width < this->width)
		{
			c++;
			width = Drawing::textExtents(lines[line].Mid(0, c), font).x;
		}
		c--;

		// Find previous space
		int sc = c;
		while (sc >= 0)
		{
			if (lines[line][sc] == ' ')
				break;
			sc--;
		}
		if (sc <= 0)
			sc = c;
		else
			sc++;

		// Split line
		string nl = lines[line].Mid(sc);
		lines[line] = lines[line].Mid(0, sc);
		lines.insert(lines.begin() + line + 1, nl);
		line++;
	}

	// Update height
	if (line_height < 0)
		height = Drawing::textExtents(lines[0], font).y * lines.size();
	else
		height = line_height * lines.size();
}

/* TextBox::setText
 * Sets the text box text
 *******************************************************************/
void TextBox::setText(string text)
{
	this->text = text;
	split(text);
}

/* TextBox::setSize
 * Sets the text box width
 *******************************************************************/
void TextBox::setSize(int width)
{
	this->width = width;
	split(this->text);
}

/* TextBox::draw
 * Draws the text box
 *******************************************************************/
void TextBox::draw(int x, int y, rgba_t colour, int alignment)
{
	frect_t b;
	Drawing::enableTextStateReset(false);
	Drawing::setTextState(true);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		Drawing::drawText(lines[a], x, y, colour, font, alignment, &b);

		if (line_height < 0)
			y += b.height();
		else
			y += line_height;
	}
	Drawing::enableTextStateReset(true);
	Drawing::setTextState(false);
}

/*
CONSOLE_COMMAND(d_testfont, 1) {
	ArchiveEntry* entry = theArchiveManager->programResourceArchive()->entryAtPath(S_FMT("fonts/%s.ttf", args[0]));
	if (entry) {
		if (Drawing::font_condensed) {
			delete Drawing::font_condensed;
			Drawing::font_condensed = NULL;
		}

		long size = 12;
		if (args.size() > 1)
			args[1].ToLong(&size);

		Drawing::font_condensed = new FTTextureFont(entry->getData(), entry->getSize());
		Drawing::font_condensed->FaceSize(size);
	}
}
*/
