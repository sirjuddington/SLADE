
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapPreviewCanvas.cpp
// Description: Canvas that shows a basic map preview
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
#include "MapPreviewCanvas.h"
#include "General/ColourConfiguration.h"
#include "General/MapPreviewData.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_view_things)


// -----------------------------------------------------------------------------
//
// MapPreviewCanvas Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MapPreviewCanvas class constructor
// -----------------------------------------------------------------------------
MapPreviewCanvas::MapPreviewCanvas(wxWindow* parent, MapPreviewData* data) : wxPanel(parent), data_{ data }
{
	SetDoubleBuffered(true);

	// Bind Events
	Bind(wxEVT_PAINT, &MapPreviewCanvas::onPaint, this);
	Bind(wxEVT_SIZE, [this](wxSizeEvent&) { Refresh(); });
}

// -----------------------------------------------------------------------------
// Updates the map preview buffer bitmap
// -----------------------------------------------------------------------------
void MapPreviewCanvas::updateBuffer()
{
	if (!data_)
		return;

	auto width  = GetSize().x;
	auto height = GetSize().y;
	if (width == 0 || height == 0)
		return;

	auto mid_x   = width / 2;
	auto mid_y   = height / 2;
	auto map_mid = data_->bounds.mid();

	// Determine scale
	auto scale_x = static_cast<double>(width) / data_->bounds.width();
	auto scale_y = static_cast<double>(height) / data_->bounds.height();
	auto scale   = glm::min(scale_x, scale_y) * 0.95;

	buffer_.Create(width, height);

	wxMemoryDC dc(buffer_);
	auto       gc = wxGraphicsContext::Create(dc);

	// Background
	gc->SetBrush(wxBrush(colourconfig::colour("map_view_background")));
	gc->DrawRectangle(0, 0, width, height);

	// Build lines to draw via wxGraphicsContext
	vector<wxPoint2DDouble> lines_1s_v1;
	vector<wxPoint2DDouble> lines_1s_v2;
	vector<wxPoint2DDouble> lines_2s_v1;
	vector<wxPoint2DDouble> lines_2s_v2;
	vector<wxPoint2DDouble> lines_special_v1;
	vector<wxPoint2DDouble> lines_special_v2;
	vector<wxPoint2DDouble> lines_macro_v1;
	vector<wxPoint2DDouble> lines_macro_v2;
	for (auto& line : data_->lines)
	{
		auto& v1 = data_->vertices[line.v1];
		auto& v2 = data_->vertices[line.v2];

		// Transform vertex positions
		auto x1 = mid_x + (v1.x - map_mid.x) * scale;
		auto y1 = mid_y - (v1.y - map_mid.y) * scale;
		auto x2 = mid_x + (v2.x - map_mid.x) * scale;
		auto y2 = mid_y - (v2.y - map_mid.y) * scale;

		if (line.twosided)
		{
			lines_2s_v1.emplace_back(x1, y1);
			lines_2s_v2.emplace_back(x2, y2);
		}
		else if (line.special)
		{
			lines_special_v1.emplace_back(x1, y1);
			lines_special_v2.emplace_back(x2, y2);
		}
		else if (line.macro)
		{
			lines_macro_v1.emplace_back(x1, y1);
			lines_macro_v2.emplace_back(x2, y2);
		}
		else
		{
			lines_1s_v1.emplace_back(x1, y1);
			lines_1s_v2.emplace_back(x2, y2);
		}
	}

	double line_width = 1.51;
	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
	gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

	// 2-Sided lines
	if (!lines_2s_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(colourconfig::colour("map_view_line_2s"), line_width)));
		gc->StrokeLines(lines_2s_v1.size(), lines_2s_v1.data(), lines_2s_v2.data());
	}

	// 1-Sided lines
	if (!lines_1s_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(colourconfig::colour("map_view_line_1s"), line_width)));
		gc->StrokeLines(lines_1s_v1.size(), lines_1s_v1.data(), lines_1s_v2.data());
	}

	// Special lines
	if (!lines_special_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(colourconfig::colour("map_view_line_special"), line_width)));
		gc->StrokeLines(lines_special_v1.size(), lines_special_v1.data(), lines_special_v2.data());
	}

	// Macro lines
	if (!lines_macro_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(colourconfig::colour("map_view_line_macro"), line_width)));
		gc->StrokeLines(lines_macro_v1.size(), lines_macro_v1.data(), lines_macro_v2.data());
	}

	// Things
	if (!data_->things.empty() && map_view_things)
	{
		auto thing_size     = glm::clamp(32.0 * scale, 6.0, 32.0);
		auto thing_halfsize = thing_size * 0.5;

		gc->SetBrush(wxBrush(colourconfig::colour("map_view_thing")));
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(colourconfig::colour("map_view_background"), 1)));
		for (const auto& thing : data_->things)
		{
			auto x = mid_x + (thing.x - map_mid.x) * scale;
			auto y = mid_y - (thing.y - map_mid.y) * scale;

			gc->DrawEllipse(x - thing_halfsize, y - thing_halfsize, thing_size, thing_size);
		}
	}

	// Update variables
	buffer_updated_time = data_->updated_time;
	buffer_things       = map_view_things;
}

// -----------------------------------------------------------------------------
// Returns true if the buffer bitmap needs to be updated
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::shouldUpdateBuffer() const
{
	if (!data_)
		return false;

	// Check buffer
	if (!buffer_.IsOk() || buffer_.GetSize() != GetSize())
		return true;

	// Check buffer content
	return buffer_updated_time < data_->updated_time || buffer_things != map_view_things;
}

// -----------------------------------------------------------------------------
// Called when the canvas needs to be (re)drawn
// -----------------------------------------------------------------------------
void MapPreviewCanvas::onPaint(wxPaintEvent& e)
{
	if (shouldUpdateBuffer())
		updateBuffer();

	wxPaintDC dc(this);

	if (buffer_.IsOk())
		dc.DrawBitmap(buffer_, 0, 0);
	else
	{
		dc.SetBrush(wxBrush(colourconfig::colour("map_view_background")));
		dc.DrawRectangle({ 0, 0 }, GetSize());
	}
}
