// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSICanvas.cpp
// Description: ANSICanvas class. Canvas for displaying ANSI art
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
#include "ANSICanvas.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Graphics/ANSIScreen.h"
#include "Utility/CodePages.h"
#include <utility>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// ANSICanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ANSICanvas class constructor
// -----------------------------------------------------------------------------
ANSICanvas::ANSICanvas(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS)
{
	SetDoubleBuffered(true);

	// Get the all-important font data
	auto res_archive = app::archiveManager().programResourceArchive();
	if (!res_archive)
		return;
	auto ansi_font = res_archive->entryAtPath("vga-rom-font.16");
	if (!ansi_font || ansi_font->size() == 0 || ansi_font->size() % 256 != 0)
		return;
	fontdata_ = ansi_font->rawData();

	// Init variables
	char_width_  = 8;
	char_height_ = ansi_font->size() / 256;
	width_       = gfx::ANSIScreen::NUMCOLS * char_width_;
	height_      = gfx::ANSIScreen::NUMROWS * char_height_;
	picdata_.resize(width_ * height_);

	// Bind Events
	Bind(wxEVT_PAINT, &ANSICanvas::onPaint, this);
}

// -----------------------------------------------------------------------------
// Loads ANSI [screen] to the canvas
// -----------------------------------------------------------------------------
void ANSICanvas::openScreen(gfx::ANSIScreen* screen)
{
	ansi_screen_ = screen;

	// Connect signals
	sig_connections_.connections.clear();
	sig_connections_ += ansi_screen_->signals().char_changed.connect( // Single character change
		[this](int index)
		{
			drawCharacter(index);
			image_.reset();
			Refresh();
		});
	sig_connections_ += ansi_screen_->signals().chars_changed.connect( // Multiple character change
		[this](const vector<unsigned>& indices)
		{
			for (auto idx : indices)
				drawCharacter(idx);
			image_.reset();
			Refresh();
		});
	sig_connections_ += ansi_screen_->signals().selection_changed.connect( // Selection change
		[this]
		{
			Update();
			Refresh();
		});

	// Draw entire screen
	image_.reset();
	for (size_t i = 0; i < gfx::ANSIScreen::SIZE; i++)
		drawCharacter(i);
}

// -----------------------------------------------------------------------------
// Sets the [scale]
// -----------------------------------------------------------------------------
void ANSICanvas::setScale(u8 scale)
{
	scale_ = scale;
	image_.reset();
	Refresh();
}

// -----------------------------------------------------------------------------
// Draws a single character at [index] to the canvas
// -----------------------------------------------------------------------------
void ANSICanvas::drawCharacter(unsigned index)
{
	if (!ansi_screen_)
		return;

	// Setup some variables
	uint8_t  chara = ansi_screen_->characterAt(index);
	uint8_t  color = ansi_screen_->colourAt(index);
	uint8_t* pic   = picdata_.data() + index / gfx::ANSIScreen::NUMCOLS * width_ * char_height_
				   + index % gfx::ANSIScreen::NUMCOLS * char_width_; // Position on canvas to draw
	const uint8_t* fnt = fontdata_ + char_height_ * chara;           // Position of character in font image

	// Draw character (including background)
	for (u8 y = 0; y < char_height_; ++y)
		for (u8 x = 0; x < char_width_; ++x)
			pic[x + y * width_] = fnt[y] & 1 << (char_width_ - 1 - x) ? color & 15 : (color & 112) >> 4;
}

// -----------------------------------------------------------------------------
// Returns the index of the character at point [pt], or nullopt if outside
// -----------------------------------------------------------------------------
std::optional<unsigned> ANSICanvas::hitTest(const wxPoint& pt) const
{
	// Determine position within canvas
	auto disp_w = width_ * scale_;
	auto disp_h = height_ * scale_;
	auto ox     = GetSize().x / 2 - disp_w / 2;
	auto oy     = GetSize().y / 2 - disp_h / 2;
	auto x      = pt.x - ox;
	auto y      = pt.y - oy;

	// Check if within range
	if (x < 0 || y < 0 || x >= disp_w || y >= disp_h)
		return std::nullopt;

	// Return index
	int col = x / (char_width_ * scale_);
	int row = y / (char_height_ * scale_);
	return row * gfx::ANSIScreen::NUMCOLS + col;
}


// -----------------------------------------------------------------------------
//
// ANSICanvas Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the canvas needs to be repainted
// -----------------------------------------------------------------------------
void ANSICanvas::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Load wx image
	if (!image_)
	{
		vector<uint8_t> rgb_data(width_ * height_ * 3);
		auto            rgb_index = 0;
		for (unsigned p = 0; p < width_ * height_; ++p)
		{
			auto c                = codepages::ansiColor(picdata_[p]);
			rgb_data[rgb_index++] = c.r;
			rgb_data[rgb_index++] = c.g;
			rgb_data[rgb_index++] = c.b;
		}

		wxImage img(width_, height_, rgb_data.data(), true);

		// Scale if needed
		if (scale_ > 1)
			img = img.Scale(width_ * scale_, height_ * scale_, wxIMAGE_QUALITY_NEAREST);

		image_ = std::make_unique<wxBitmap>(img);
	}

	// Draw image
	auto dx = GetSize().x / 2 - image_->GetWidth() / 2;
	auto dy = GetSize().y / 2 - image_->GetHeight() / 2;
	dc.DrawBitmap(*image_, dx, dy);

	// Draw selection outline
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxPENSTYLE_SOLID));
	auto c_w = char_width_ * scale_;
	auto c_h = char_height_ * scale_;

	// Draw outline by checking edge boundaries
	for (auto col = 0u; col < gfx::ANSIScreen::NUMCOLS; ++col)
	{
		for (auto row = 0u; row < gfx::ANSIScreen::NUMROWS; ++row)
		{
			if (!ansi_screen_->isSelected(col, row))
				continue;

			auto x = dx + col * c_w;
			auto y = dy + row * c_h;

			// Check each edge and draw if it's a boundary

			// Top edge
			if (row == 0 || !ansi_screen_->isSelected(col, row - 1))
				dc.DrawLine(x, y, x + c_w, y);

			// Bottom edge
			if (row == gfx::ANSIScreen::NUMROWS - 1 || !ansi_screen_->isSelected(col, row + 1))
				dc.DrawLine(x, y + c_h, x + c_w, y + c_h);

			// Left edge
			if (col == 0 || !ansi_screen_->isSelected(col - 1, row))
				dc.DrawLine(x, y, x, y + c_h);

			// Right edge
			if (col == gfx::ANSIScreen::NUMCOLS - 1 || !ansi_screen_->isSelected(col + 1, row))
				dc.DrawLine(x + c_w, y, x + c_w, y + c_h);
		}
	}
}
