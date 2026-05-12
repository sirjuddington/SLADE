#pragma once

#include "Geometry/RectFwd.h"

// Forward declarations
namespace slade
{
enum class GfxView;
class SImage;
namespace gl
{
	class View;
}
} // namespace slade


namespace slade::wxgfx
{
struct Context
{
	unique_ptr<wxGraphicsContext> gc;
	gl::View*                     view;

	Context(wxWindowDC& dc, gl::View* view);

	void applyView() const;

	void setPen(const ColRGBA& colour, double width = 1.0) const;
	void setBrush(const ColRGBA& colour) const;
	void setTransparentBrush() const;
	void setTransparentPen() const;

	void drawLine(const Seg2i& line) const;
	void drawLine(int x1, int y1, int x2, int y2) const;
	void drawLines(const vector<Seg2i>& lines) const;
	void drawRect(const Recti& rect) const;
	void drawRect(int x, int y, int width, int height) const;
	void drawBitmap(const wxBitmap& bitmap, int x, int y, double alpha = 1.0, int width = -1, int height = -1) const;

	void drawOffsetLines(GfxView view_type) const;
};

wxImage createImageFromSVG(const string& svg_text, int width, int height);
wxImage createImageFromSImage(const SImage& image, const Palette* palette);

unique_ptr<wxGraphicsContext> createGraphicsContext(wxWindowDC& dc);
void                          applyViewToGC(const gl::View& view, wxGraphicsContext* gc);
bool                          nearestInterpolationSupported();

void drawOffsetLines(wxGraphicsContext* gc, const gl::View& view, GfxView view_type);
void generateCheckeredBackground(wxBitmap& bitmap, int width, int height);
} // namespace slade::wxgfx
