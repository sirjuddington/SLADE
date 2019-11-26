#pragma once

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#endif
#include "Utility/Colour.h"

class FontManager;

namespace Drawing
{
enum class Font
{
	Normal,
	Condensed,
	Bold,
	BoldCondensed,
	Monospace,
	Small
};

enum class Align
{
	Left,
	Right,
	Center
};

// Initialisation
int initFonts();

// Cleanup
void cleanupFonts();

// Info
int fontSize();

// Basic drawing
void drawLine(Vec2d start, Vec2d end);
void drawLine(double x1, double y1, double x2, double y2);
void drawLineTabbed(Vec2d start, Vec2d end, double tab = 0.1, double tab_max = 16);
void drawArrow(
	Vec2d   p1,
	Vec2d   p2,
	ColRGBA color            = ColRGBA::WHITE,
	bool    twoway           = false,
	double  arrowhead_angle  = 0.7854f,
	double  arrowhead_length = 25.f);
void drawRect(Vec2d tl, Vec2d br);
void drawRect(double x1, double y1, double x2, double y2);
void drawFilledRect(Vec2d tl, Vec2d br);
void drawFilledRect(double x1, double y1, double x2, double y2);
void drawBorderedRect(Vec2d tl, Vec2d br, const ColRGBA& colour, const ColRGBA& border_colour);
void drawBorderedRect(double x1, double y1, double x2, double y2, const ColRGBA& colour, const ColRGBA& border_colour);
void drawEllipse(Vec2d mid, double radius_x, double radius_y, int sides, const ColRGBA& colour);
void drawFilledEllipse(Vec2d mid, double radius_x, double radius_y, int sides, const ColRGBA& colour);

// Texture drawing
void  drawTexture(unsigned id, double x = 0, double y = 0, bool flipx = false, bool flipy = false);
void  drawTextureTiled(unsigned id, uint32_t width, uint32_t height);
Rectd fitTextureWithin(unsigned id, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);
void  drawTextureWithin(unsigned id, double x1, double y1, double x2, double y2, double padding, double max_scale = 1);

// Text drawing
void drawText(
	const string& text,
	int           x         = 0,
	int           y         = 0,
	ColRGBA       colour    = ColRGBA::WHITE,
	Font          font      = Font::Normal,
	Align         alignment = Align::Left,
	Rectd*        bounds    = nullptr);
Vec2d textExtents(const string& text, Font font = Font::Normal);
void  enableTextStateReset(bool enable = true);
void  setTextState(bool set = true);
void  setTextOutline(double thickness, const ColRGBA& colour = ColRGBA::BLACK);

// Specific
void drawHud();

// Implementation-specific
#ifdef USE_SFML_RENDERWINDOW
void setRenderTarget(sf::RenderWindow* target);
#endif


// From CodeLite
wxColour systemPanelBGColour();
wxColour systemMenuTextColour();
wxColour systemMenuBarBGColour();
wxColour lightColour(const wxColour& colour, float percent);
wxColour darkColour(const wxColour& colour, float percent);
} // namespace Drawing

// TextBox class
class TextBox
{
public:
	TextBox(string_view text, Drawing::Font font, int width, int line_height = -1);
	~TextBox() = default;

	int  height() const { return height_; }
	int  width() const { return width_; }
	void setText(string_view text);
	void setSize(int width);
	void setLineHeight(int height) { line_height_ = height; }
	void draw(int x, int y, const ColRGBA& colour = ColRGBA::WHITE, Drawing::Align alignment = Drawing::Align::Left);

private:
	string         text_;
	vector<string> lines_;
	Drawing::Font  font_        = Drawing::Font::Normal;
	int            width_       = 0;
	int            height_      = 0;
	int            line_height_ = -1;

	void split(string_view text);
};
