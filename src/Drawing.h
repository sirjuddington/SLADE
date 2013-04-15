
#ifndef __DRAWING_H__
#define __DRAWING_H__

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#endif

#include <wx/colour.h>

class GLTexture;
class FontManager;

namespace Drawing {
	enum {
		// Text fonts
		FONT_NORMAL = 0,
		FONT_CONDENSED,
		FONT_BOLD,
		FONT_BOLDCONDENSED,
		FONT_MONOSPACE,
		FONT_SMALL,

		// Text alignment
		ALIGN_LEFT = 0,
		ALIGN_RIGHT = 1,
		ALIGN_CENTER = 2,
	};

	// Initialisation
	void initFonts();

	// Basic drawing
	void drawLine(fpoint2_t start, fpoint2_t end);
	void drawLine(double x1, double y1, double x2, double y2);
	void drawLineTabbed(fpoint2_t start, fpoint2_t end, double tab = 0.1, double tab_max = 16);
	void drawLineTabbed(double x1, double y1, double x2, double y2, double tab = 0.1, double tab_max = 16);
	void drawRect(fpoint2_t tl, fpoint2_t br);
	void drawRect(double x1, double y1, double x2, double y2);
	void drawFilledRect(fpoint2_t tl, fpoint2_t br);
	void drawFilledRect(double x1, double y1, double x2, double y2);
	void drawBorderedRect(fpoint2_t tl, fpoint2_t br, rgba_t colour, rgba_t border_colour);
	void drawBorderedRect(double x1, double y1, double x2, double y2, rgba_t colour, rgba_t border_colour);

	// Texture drawing
	frect_t	fitTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);
	void	drawTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);

	// Text drawing
	void drawText(string text, int x = 0, int y = 0, rgba_t colour = COL_WHITE, int font = FONT_NORMAL, int alignment = ALIGN_LEFT, frect_t* bounds = NULL);
	fpoint2_t textExtents(string text, int font = FONT_NORMAL);

	// Specific
	void drawHud();

	// Implementation-specific
#ifdef USE_SFML_RENDERWINDOW
	void setRenderTarget(sf::RenderWindow* target);
#endif


	// From CodeLite
	wxColour getPanelBGColour();
	wxColour getMenuTextColour();
	wxColour getMenuBarBGColour();
	wxColour lightColour(const wxColour& colour, float percent);
	wxColour darkColour(const wxColour& colour, float percent);
}

#endif//__DRAWING_H__
