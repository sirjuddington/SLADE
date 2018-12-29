#pragma once

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#endif

class GLTexture;
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
void drawLine(Vec2f start, Vec2f end);
void drawLine(double x1, double y1, double x2, double y2);
void drawLineTabbed(Vec2f start, Vec2f end, double tab = 0.1, double tab_max = 16);
void drawArrow(
	Vec2f   p1,
	Vec2f   p2,
	ColRGBA color            = COL_WHITE,
	bool    twoway           = false,
	double  arrowhead_angle  = 0.7854f,
	double  arrowhead_length = 25.f);
void drawRect(Vec2f tl, Vec2f br);
void drawRect(double x1, double y1, double x2, double y2);
void drawFilledRect(Vec2f tl, Vec2f br);
void drawFilledRect(double x1, double y1, double x2, double y2);
void drawBorderedRect(Vec2f tl, Vec2f br, ColRGBA colour, ColRGBA border_colour);
void drawBorderedRect(double x1, double y1, double x2, double y2, ColRGBA colour, ColRGBA border_colour);
void drawEllipse(Vec2f mid, double radius_x, double radius_y, int sides, ColRGBA colour);
void drawFilledEllipse(Vec2f mid, double radius_x, double radius_y, int sides, ColRGBA colour);

// Texture drawing
Rectf fitTextureWithin(
	GLTexture* tex,
	double     x1,
	double     y1,
	double     x2,
	double     y2,
	double     padding,
	double     max_scale = 1);
void drawTextureWithin(
	GLTexture* tex,
	double     x1,
	double     y1,
	double     x2,
	double     y2,
	double     padding,
	double     max_scale = 1);

// Text drawing
void drawText(
	const string& text,
	int           x         = 0,
	int           y         = 0,
	ColRGBA       colour    = COL_WHITE,
	Font          font      = Font::Normal,
	Align         alignment = Align::Left,
	Rectf*        bounds    = nullptr);
Vec2f textExtents(const string& text, Font font = Font::Normal);
void  enableTextStateReset(bool enable = true);
void  setTextState(bool set = true);
void  setTextOutline(double thickness, ColRGBA colour = COL_BLACK);

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
	TextBox(const string& text, Drawing::Font font, int width, int line_height = -1);
	~TextBox() = default;

	int  height() const { return height_; }
	int  width() const { return width_; }
	void setText(const string& text);
	void setSize(int width);
	void setLineHeight(int height) { line_height_ = height; }
	void draw(int x, int y, ColRGBA colour = COL_WHITE, Drawing::Align alignment = Drawing::Align::Left);

private:
	string         text_;
	vector<string> lines_;
	Drawing::Font  font_        = Drawing::Font::Normal;
	int            width_       = 0;
	int            height_      = 0;
	int            line_height_ = -1;

	void split(const string& text);
};
