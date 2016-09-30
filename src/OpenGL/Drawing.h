
#pragma once

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#endif

#include "common.h"
#include "Fonts.h"
#include "Utility/Structs.h"

class GLTexture;
class FontManager;

namespace Drawing
{
	enum class Align
	{
		Left,
		Right,
		Center
	};

	// Basic drawing
	void drawLine(fpoint2_t start, fpoint2_t end);
	void drawLine(double x1, double y1, double x2, double y2);
	void drawLineTabbed(fpoint2_t start, fpoint2_t end, double tab = 0.1, double tab_max = 16);
	void drawArrow(fpoint2_t p1, fpoint2_t p2, rgba_t color = COL_WHITE, bool twoway = false, double arrowhead_angle = 0.7854f, double arrowhead_length = 25.f);
	void drawRect(fpoint2_t tl, fpoint2_t br);
	void drawRect(double x1, double y1, double x2, double y2);
	void drawFilledRect(fpoint2_t tl, fpoint2_t br);
	void drawFilledRect(double x1, double y1, double x2, double y2);
	void drawBorderedRect(fpoint2_t tl, fpoint2_t br, rgba_t colour, rgba_t border_colour);
	void drawBorderedRect(double x1, double y1, double x2, double y2, rgba_t colour, rgba_t border_colour);
	void drawEllipse(fpoint2_t mid, double radius_x, double radius_y, int sides, rgba_t colour);
	void drawFilledEllipse(fpoint2_t mid, double radius_x, double radius_y, int sides, rgba_t colour);

	// Texture drawing
	frect_t	fitTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);
	void	drawTextureWithin(GLTexture* tex, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);

	// Text drawing
	void drawText(
		string text,
		int x = 0,
		int y = 0,
		rgba_t colour = COL_WHITE,
		Fonts::Font font = Fonts::regularFont(),
		Align alignment = Align::Left,
		frect_t* bounds = nullptr,
		fpoint2_t scale = fpoint2_t(1, 1));
	void enableTextStateReset(bool enable = true);
	void setTextState(bool set = true);
	void setTextOutline(double thickness, rgba_t colour = COL_BLACK);

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

// STextBox class
class STextBox
{
private:
	string			text;
	vector<string>	lines;
	Fonts::Font		font;
	int				width;
	int				height;
	int				line_height;

	void	split(string text);

public:
	STextBox(string text, Fonts::Font& font, int width, int line_height = -1);
	~STextBox() {}

	int		getHeight() { return height; }
	int		getWidth() { return width; }
	int		getLineHeight();
	void	setText(string text);
	void	setSize(int width);
	void	setLineHeight(int height) { line_height = height; }
	void	setFont(Fonts::Font& font);
	void	draw(int x, int y, rgba_t colour = COL_WHITE, Drawing::Align alignment = Drawing::Align::Left);
};
