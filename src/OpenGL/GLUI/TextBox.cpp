
#include "Main.h"
#include "TextBox.h"
#include "General/ColourConfiguration.h"
#include "OpenGL/Drawing.h"

using namespace GLUI;

TextBox::TextBox(Widget* parent, string text, int font, int alignment, rgba_t colour, double line_height)
: Widget(parent), font(font), alignment(alignment), colour(colour), line_height(line_height)
{
	line_height_pixels = Drawing::getFontLineHeight(font) * line_height;
}

TextBox::~TextBox()
{
}

/* TextBox::split
 * Splits [text] into separate lines (split by newlines), also
 * performs further splitting to word wrap the text within
 * [fit_width]
 *******************************************************************/
void TextBox::splitText(int fit_width)
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
	if (fit_width < 32 && fit_width >= 0)
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
		if (fit_width < 0 || width < fit_width)
		{
			if (fit_width < 0 && width > text_width_full)
				text_width_full = width;

			line++;
			continue;
		}

		// Halve length until it fits in the box
		unsigned c = lines[line].length() - 1;
		while (width >= fit_width)
		{
			if (c <= 1)
				break;

			c *= 0.5;
			width = Drawing::textExtents(lines[line].Mid(0, c), font).x;
		}

		// Increment length until it doesn't fit
		while (width < fit_width)
		{
			c++;
			width = Drawing::textExtents(lines[line].Mid(0, c), font).x;
		}
		c--;

		if (fit_width < 0 && width > text_width_full)
			text_width_full = width;

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
}

int TextBox::getLineHeightPixels()
{
	return line_height_pixels;
}

void TextBox::setText(string text)
{
	this->text = text;

	// First calculate full size of text
	// (without fitting it into the current text box size)
	text_width_full = 0;
	splitText(-1);
	text_height_full = getLineHeightPixels() * lines.size();

	// Split text lines to fit the current width
	splitText(size.x);
}

void TextBox::setLineHeightPixels(int pixels)
{
	int lh = Drawing::getFontLineHeight(font);
	line_height = pixels / lh;
	line_height_pixels = pixels;
	text_height_full = lines.size() * line_height_pixels;
}

void TextBox::setLineHeight(double mult)
{
	line_height = mult;
	line_height_pixels = Drawing::getFontLineHeight(font) * mult;
	text_height_full = lines.size() * line_height_pixels;
}

void TextBox::setFont(int font)
{
	this->font = font;
	line_height_pixels = Drawing::getFontLineHeight(font) * line_height;
	setText(text);
}

void TextBox::drawWidget(point2_t pos, float alpha)
{
	if (lines.empty())
		return;

	// Adjust x for alignment
	if (alignment == Drawing::ALIGN_CENTER)
		pos.x = pos.x + (size.x * 0.5);
	else if (alignment == Drawing::ALIGN_RIGHT)
		pos.x = pos.x + size.x;

	Drawing::enableTextStateReset(false);
	Drawing::setTextState(true);
	rgba_t col(colour.r, colour.g, colour.b, colour.a * alpha, colour.blend);
	for (auto line : lines)
	{
		Drawing::drawText(line, pos.x, pos.y, col, font, alignment);
		pos.y += line_height_pixels;
	}
	Drawing::enableTextStateReset(true);
	Drawing::setTextState(false);
}

void TextBox::updateLayout(dim2_t fit)
{
	if (fit.x >= 0 && fit.x < text_width_full)
	{
		splitText(fit.x);
		setSize(dim2_t(fit.x, line_height_pixels * lines.size()));
	}
	else
	{
		splitText(-1);
		setSize(getFullTextSize());
	}
}

rgba_t TextBox::defaultColour()
{
	return ColourConfiguration::getColour("map_overlay_foreground");
}
