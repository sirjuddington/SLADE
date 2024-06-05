
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WxUtils.cpp
// Description: WxWidgets-related graphics utility functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "WxGfx.h"
#include "OpenGL/View.h"
#include "SImage/SImage.h"
#include "UI/Canvas/GfxCanvasBase.h"

using namespace slade;
using namespace wxgfx;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
bool nearest_interp_supported = true;
}


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, bgtx_colour1)
EXTERN_CVAR(String, bgtx_colour2)
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_wide)
EXTERN_CVAR(Bool, hud_bob)


Context::Context(wxWindowDC& dc, gl::View* view) : view{ view }
{
	gc = createGraphicsContext(dc);
}

void Context::applyView() const
{
	if (view && gc)
		applyViewToGC(*view, gc.get());
}

void Context::setPen(const ColRGBA& colour, double width) const
{
	if (gc)
	{
		auto w       = gc->GetWindow();
		auto p_width = (width / (view ? view->scale().x : 1.0)) / w->GetContentScaleFactor();
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo{ colour, p_width }));
	}
}

void Context::setBrush(const ColRGBA& colour) const
{
	if (gc)
		gc->SetBrush(wxBrush{ colour });
}

void Context::setTransparentBrush() const
{
	if (gc)
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
}

void Context::setTransparentPen() const
{
	if (gc)
		gc->SetPen(*wxTRANSPARENT_PEN);
}

void Context::drawLine(const Seg2i& line) const
{
	drawLine(line.x1(), line.y1(), line.x2(), line.y2());
}

void Context::drawLine(int x1, int y1, int x2, int y2) const
{
	if (!gc)
		return;

	const auto w = gc->GetWindow();
	gc->StrokeLine(w->FromPhys(x1), w->FromPhys(y1), w->FromPhys(x2), w->FromPhys(y2));
}

void Context::drawLines(const vector<Seg2i>& lines) const
{
	if (!gc)
		return;

	vector<wxPoint2DDouble> begin_points;
	vector<wxPoint2DDouble> end_points;
	const auto              w = gc->GetWindow();

	for (const auto& line : lines)
	{
		begin_points.emplace_back(w->FromPhys(line.x1()), w->FromPhys(line.y1()));
		end_points.emplace_back(w->FromPhys(line.x2()), w->FromPhys(line.y2()));
	}

	gc->StrokeLines(lines.size(), begin_points.data(), end_points.data());
}

void Context::drawRect(const Recti& rect) const
{
	drawRect(rect.tl.x, rect.tl.y, rect.width(), rect.height());
}

void Context::drawRect(int x, int y, int width, int height) const
{
	if (!gc)
		return;

	const auto scale = gc->GetContentScaleFactor();
	gc->DrawRectangle(x / scale, y / scale, width / scale, height / scale);
}

void Context::drawBitmap(const wxBitmap& bitmap, int x, int y, double alpha, int width, int height) const
{
	if (!gc)
		return;

	const auto scale = gc->GetContentScaleFactor();

	if (alpha < 1.)
		gc->BeginLayer(alpha);

	if (width < 0)
		width = bitmap.GetWidth();
	if (height < 0)
		height = bitmap.GetHeight();

	gc->DrawBitmap(bitmap, x / scale, y / scale, width / scale, height / scale);

	if (alpha < 1.)
		gc->EndLayer();
}

void Context::drawOffsetLines(GfxView view_type) const
{
	if (!gc || !view)
		return;

	auto psize_thick  = 1.51;
	auto psize_normal = 1.0;
	auto iq           = gc->GetInterpolationQuality();

	if (view_type == GfxView::Sprite)
	{
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
		setPen({ 0, 0, 0, 150 }, psize_thick);
		drawLine(view->visibleRegion().left(), 0, view->visibleRegion().right(), 0);
		drawLine(0, view->visibleRegion().top(), 0, view->visibleRegion().bottom());
	}
	else if (view_type == GfxView::HUD)
	{
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		// (320/354)x200 screen outline
		auto right  = hud_wide ? 337 : 320;
		auto left   = hud_wide ? -17 : 0;
		auto top    = 0;
		auto bottom = 200;
		setPen({ 0, 0, 0, 190 }, psize_thick);
		drawLine(left, top, left, bottom);
		drawLine(left, bottom, right, bottom);
		drawLine(right, bottom, right, top);
		drawLine(right, top, left, top);

		// Statusbar line(s)
		setPen({ 0, 0, 0, 128 }, psize_normal);
		if (hud_statusbar)
		{
			drawLine(left, 168, right, 168); // Doom's status bar: 32 pixels tall
			drawLine(left, 162, right, 162); // Hexen: 38 pixels
			drawLine(left, 158, right, 158); // Heretic: 42 pixels
		}

		// Center lines
		if (hud_center)
		{
			drawLine(left, 100, right, 100);
			drawLine(160, top, 160, bottom);
		}

		// Normal screen edge guides if widescreen
		if (hud_wide)
		{
			drawLine(0, top, 0, bottom);
			drawLine(320, top, 320, bottom);
		}

		// Weapon bobbing guides
		if (hud_bob)
		{
			setPen({ 0, 0, 0, 128 }, psize_normal);
			drawLine(left - 16.0, top - 16.0, left - 16.0, bottom + 16.0);
			drawLine(left - 16.0, bottom + 16.0, right + 16.0, bottom + 16.0);
			drawLine(right + 16.0, bottom + 16.0, right + 16.0, top - 16.0);
			drawLine(right + 16.0, top - 16.0, left - 16.0, top - 16.0);
		}
	}

	// Restore gc state
	gc->SetInterpolationQuality(iq);
}










// -----------------------------------------------------------------------------
// Creates a wxImage from the given [svg_text] data, sized to [width x height].
// Returns an invalid (empty) wxImage if the SVG data was invalid
// -----------------------------------------------------------------------------
wxImage wxgfx::createImageFromSVG(const string& svg_text, int width, int height)
{
	auto bundle = wxBitmapBundle::FromSVG(svg_text.c_str(), { width, height });
	return bundle.GetBitmap({ width, height }).ConvertToImage();
}

// -----------------------------------------------------------------------------
// Creates a wxImage from the given S[image] and [palette] (optional)
// -----------------------------------------------------------------------------
wxImage wxgfx::createImageFromSImage(const SImage& image, const Palette* palette)
{
	if (!image.isValid())
		return {};

	// Get image RGB and Alpha data separately because we can't create a wxImage straight from RGBA data
	MemChunk rgb, alpha;
	image.putRGBData(rgb, palette);
	image.putAlphaData(alpha);

	// Create wx bitmap
	return { image.width(), image.height(), rgb.releaseData(), alpha.releaseData() };
}

// -----------------------------------------------------------------------------
// Creates a platform-appropriate wxGraphicsContext for [dc], contained in a
// unique_ptr to ensure it's deleted after use
// -----------------------------------------------------------------------------
unique_ptr<wxGraphicsContext> wxgfx::createGraphicsContext(wxWindowDC& dc)
{
	wxGraphicsContext* gc = nullptr;

#ifdef WIN32
	// Use Direct2d on Windows instead of GDI+
	gc = wxGraphicsRenderer::GetDirect2DRenderer()->CreateContext(dc);
#else
	gc = wxGraphicsContext::Create(dc);
#endif

	// Check if 'nearest' interpolation (wxINTERPOLATION_NONE) is supported by the wxGraphicsContext
	nearest_interp_supported = gc->SetInterpolationQuality(wxINTERPOLATION_NONE);
	gc->SetInterpolationQuality(wxINTERPOLATION_DEFAULT);

	return unique_ptr<wxGraphicsContext>{ gc };
}

// -----------------------------------------------------------------------------
// Applies the given [view] (scale and offset/translation) to [gc]
// -----------------------------------------------------------------------------
void wxgfx::applyViewToGC(const gl::View& view, wxGraphicsContext* gc)
{
	auto scale = gc->GetContentScaleFactor();
	if (view.centered())
		gc->Translate((view.size().x * 0.5) / scale, (view.size().y * 0.5) / scale);
	gc->Scale(view.scale().x, view.scale().y);
	gc->Translate(-view.offset().x / scale, -view.offset().y / scale);
}

bool wxgfx::nearestInterpolationSupported()
{
	return nearest_interp_supported;
}

void wxgfx::drawOffsetLines(wxGraphicsContext* gc, const gl::View& view, GfxView view_type)
{
	auto psize_thick  = 1.51 / view.scale().x;
	auto psize_normal = 1.0 / view.scale().x;

	if (view_type == GfxView::Sprite)
	{
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(wxColour(0, 0, 0, 190), psize_thick)));
		gc->StrokeLine(view.visibleRegion().left(), 0, view.visibleRegion().right(), 0);
		gc->StrokeLine(0, view.visibleRegion().top(), 0, view.visibleRegion().bottom());
	}
	else if (view_type == GfxView::HUD)
	{
		gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

		// (320/354)x200 screen outline
		auto right  = hud_wide ? 337.0 : 320.0;
		auto left   = hud_wide ? -17.0 : 0.0;
		auto top    = 0;
		auto bottom = 200;
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(wxColour(0, 0, 0, 190), psize_thick)));
		gc->StrokeLine(left, top, left, bottom);
		gc->StrokeLine(left, bottom, right, bottom);
		gc->StrokeLine(right, bottom, right, top);
		gc->StrokeLine(right, top, left, top);

		// Statusbar line(s)
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(wxColour(0, 0, 0, 128), psize_normal)));
		if (hud_statusbar)
		{
			gc->StrokeLine(left, 168, right, 168); // Doom's status bar: 32 pixels tall
			gc->StrokeLine(left, 162, right, 162); // Hexen: 38 pixels
			gc->StrokeLine(left, 158, right, 158); // Heretic: 42 pixels
		}

		// Center lines
		if (hud_center)
		{
			gc->StrokeLine(left, 100, right, 100);
			gc->StrokeLine(160, top, 160, bottom);
		}

		// Normal screen edge guides if widescreen
		if (hud_wide)
		{
			gc->StrokeLine(0, top, 0, bottom);
			gc->StrokeLine(320, top, 320, bottom);
		}

		// Weapon bobbing guides
		if (hud_bob)
		{
			gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(wxColour(0, 0, 0, 128), psize_normal)));
			gc->StrokeLine(left - 16.0, top - 16.0, left - 16.0, bottom + 16.0);
			gc->StrokeLine(left - 16.0, bottom + 16.0, right + 16.0, bottom + 16.0);
			gc->StrokeLine(right + 16.0, bottom + 16.0, right + 16.0, top - 16.0);
			gc->StrokeLine(right + 16.0, top - 16.0, left - 16.0, top - 16.0);
		}
	}
}

// -----------------------------------------------------------------------------
// Generates a checkered pattern of [width]x[height] into [bitmap].
// If the bitmap is already larger than the requested size, does nothing
// -----------------------------------------------------------------------------
void wxgfx::generateCheckeredBackground(wxBitmap& bitmap, int width, int height)
{
	// Check size
	if (width <= 0 || height <= 0)
		return;

	// Do nothing if the bitmap doesn't need updating
	if (bitmap.IsOk() && bitmap.GetWidth() > width && bitmap.GetHeight() > height)
		return;

	wxColour col1(bgtx_colour1);
	wxColour col2(bgtx_colour2);

	bitmap.Create(width, height);
	wxMemoryDC dc(bitmap);

	// First colour
	dc.SetBrush(wxBrush(col1));
	dc.SetPen(*wxTRANSPARENT_PEN);
	int  x       = 0;
	int  y       = 0;
	bool odd_row = false;
	while (y < height)
	{
		x = odd_row ? 8 : 0;

		while (x < width)
		{
			dc.DrawRectangle(x, y, 8, 8);
			x += 16;
		}

		// Next row
		y += 8;
		odd_row = !odd_row;
	}

	// Second colour
	dc.SetBrush(wxBrush(col2));
	y       = 0;
	odd_row = false;
	while (y < height)
	{
		x = odd_row ? 0 : 8;

		while (x < width)
		{
			dc.DrawRectangle(x, y, 8, 8);
			x += 16;
		}

		// Next row
		y += 8;
		odd_row = !odd_row;
	}
}
