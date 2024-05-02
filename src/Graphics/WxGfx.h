#pragma once

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
wxImage createImageFromSVG(const string& svg_text, int width, int height);
wxImage createImageFromSImage(const SImage& image, const Palette* palette);

unique_ptr<wxGraphicsContext> createGraphicsContext(wxWindowDC& dc);
void                          applyViewToGC(const gl::View& view, wxGraphicsContext* gc);
bool                          nearestInterpolationSupported();

void drawOffsetLines(wxGraphicsContext* gc, const gl::View& view, GfxView view_type);
void generateCheckeredBackground(wxBitmap& bitmap, int width, int height);
} // namespace slade::wxgfx
