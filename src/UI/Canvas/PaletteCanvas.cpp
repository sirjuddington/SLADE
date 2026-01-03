
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PaletteCanvas.cpp
// Description: PaletteCanvas class. A canvas that displays a palette
//              (256 colours max) and optionally allows selection
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
#include "PaletteCanvas.h"
#include "Graphics/Palette/Palette.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PaletteCanvas Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// PaletteCanvas class constructor
// -----------------------------------------------------------------------------
PaletteCanvas::PaletteCanvas(wxWindow* parent) : wxPanel(parent), palette_{ new Palette }, buffer_{ 1000, 1000, 32 }
{
	SetDoubleBuffered(true);

	// Bind Events
	Bind(wxEVT_PAINT, &PaletteCanvas::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &PaletteCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &PaletteCanvas::onMouseRightDown, this);
	Bind(wxEVT_MOTION, &PaletteCanvas::onMouseMotion, this);

	// Update on resize
	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent&)
		{
			// Update buffer
			updateBuffer();

			// Update offset
			auto mid_x  = GetSize().x / 2;
			auto mid_y  = GetSize().y / 2;
			auto buf_hw = buffer_.GetWidth() / 2;
			auto buf_hh = buffer_.GetHeight() / 2;
			offset_     = { mid_x - buf_hw, mid_y - buf_hh };

			Refresh();
		});
}

// -----------------------------------------------------------------------------
// Returns the currently selected colour, or a completely black + transparent
// colour if nothing is selected
// -----------------------------------------------------------------------------
ColRGBA PaletteCanvas::selectedColour() const
{
	if (sel_begin_ >= 0)
		return palette_->colour(sel_begin_);
	else
		return { 0, 0, 0, 0 };
}

// -----------------------------------------------------------------------------
// Sets the selection range. If [end] is -1 the range is set to a single index
// -----------------------------------------------------------------------------
void PaletteCanvas::setSelection(int begin, int end)
{
	auto prev_begin = sel_begin_;
	auto prev_end   = sel_end_;

	sel_begin_ = begin;
	if (end == -1)
		sel_end_ = begin;
	else
		sel_end_ = end;

	// Refresh if the selection was changed
	if (sel_begin_ != prev_begin || sel_end_ != prev_end)
	{
		updateBuffer(true);
		Update();
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Sets the palette to display
// -----------------------------------------------------------------------------
void PaletteCanvas::setPalette(const Palette* pal)
{
	palette_->copyPalette(pal);
	updateBuffer(true);
	Refresh();
}

// -----------------------------------------------------------------------------
// Updates the buffer bitmap if the size has changed or [force] is true
// -----------------------------------------------------------------------------
void PaletteCanvas::updateBuffer(bool force)
{
	auto width  = GetSize().x;
	auto height = GetSize().y;
	if (width == 0 || height == 0)
		return;

	// Determine square size
	auto rows        = double_width_ ? 8 : 16;
	auto cols        = double_width_ ? 32 : 16;
	int  x_size      = width / cols;
	int  y_size      = height / rows;
	auto square_size = std::min<int>(x_size, y_size);

	// Check canvas is large enough to display the palette
	if (square_size < 3)
		return;

	// If the size hasn't changed we don't need to update the buffer
	if (!force && rows == rows_ && cols == cols_ && square_size == square_size_)
		return;

	// Update variables
	rows_        = rows;
	cols_        = cols;
	square_size_ = square_size;

	// Setup for drawing
	buffer_.Create(square_size_ * cols_, square_size_ * rows_, 32);
	auto       corner_size = square_size * 0.05;
	Vec2i      sel_start{ -1, -1 };
	Vec2i      sel_end{ -1, -1 };
	wxMemoryDC dc(buffer_);
	auto       gc = wxGraphicsContext::Create(dc);
	gc->SetPen(wxPen(*wxBLACK, 2));

	// Draw colour squares to buffer
	int index = 0;
	for (auto row = 0; row < rows_; row++)
	{
		for (auto col = 0; col < cols_; col++)
		{
			gc->SetBrush(wxBrush(palette_->colour(index)));

			if (index == sel_begin_)
				sel_start = { col, row };
			if (index == sel_end_)
				sel_end = { col, row };

			gc->DrawRoundedRectangle(col * square_size, row * square_size, square_size, square_size, corner_size);

			++index;
		}
	}

	// Draw selection outline
	if (sel_start.x >= 0)
	{
		gc->SetBrush(*wxTRANSPARENT_BRUSH);

		// Single-row selection
		if (sel_start.y == sel_end.y)
		{
			// Black inner
			gc->SetPen(wxPen(*wxBLACK, 2));
			gc->DrawRoundedRectangle(
				sel_start.x * square_size + 3,
				sel_start.y * square_size + 3,
				(sel_end.x + 1 - sel_start.x) * square_size - 6,
				square_size - 6,
				corner_size);

			// White outer
			gc->SetPen(wxPen(*wxWHITE, 2));
			gc->DrawRoundedRectangle(
				sel_start.x * square_size + 1,
				sel_start.y * square_size + 1,
				(sel_end.x + 1 - sel_start.x) * square_size - 2,
				square_size - 2,
				corner_size);
		}

		// Multi-row selection
		else
		{
			// First row

			// Black inner
			gc->SetPen(wxPen(*wxBLACK, 2));
			gc->DrawRoundedRectangle(
				sel_start.x * square_size + 3,
				sel_start.y * square_size + 3,
				buffer_.GetWidth() + square_size,
				square_size - 6,
				corner_size);

			// White outer
			gc->SetPen(wxPen(*wxWHITE, 2));
			gc->DrawRoundedRectangle(
				sel_start.x * square_size + 1,
				sel_start.y * square_size + 1,
				buffer_.GetWidth() + square_size,
				square_size - 2,
				corner_size);

			// Last row

			// Black inner
			gc->SetPen(wxPen(*wxBLACK, 2));
			gc->DrawRoundedRectangle(
				-square_size,
				sel_end.y * square_size + 3,
				(sel_end.x + 1) * square_size - 6 + square_size,
				square_size - 6,
				corner_size);

			// White outer
			gc->SetPen(wxPen(*wxWHITE, 2));
			gc->DrawRoundedRectangle(
				-square_size,
				sel_end.y * square_size + 1,
				(sel_end.x + 1) * square_size - 2 + square_size,
				square_size - 2,
				corner_size);

			// Middle row(s)
			for (int row = sel_start.y + 1; row <= sel_end.y - 1; ++row)
			{
				// Black inner
				gc->SetPen(wxPen(*wxBLACK, 2));
				gc->DrawRectangle(-10, row * square_size + 3, buffer_.GetWidth() + 20, square_size - 6);

				// White outer
				gc->SetPen(wxPen(*wxWHITE, 2));
				gc->DrawRectangle(-10, row * square_size + 1, buffer_.GetWidth() + 20, square_size - 2);
			}
		}
	}
}


// -----------------------------------------------------------------------------
//
// PaletteCanvas Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the canvas requires redrawing
// -----------------------------------------------------------------------------
void PaletteCanvas::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);
	dc.DrawBitmap(buffer_, offset_.x, offset_.y, true);
}

// -----------------------------------------------------------------------------
// Called when the palette canvas is left clicked
// -----------------------------------------------------------------------------
void PaletteCanvas::onMouseLeftDown(wxMouseEvent& e)
{
	// Handle selection if needed
	if (allow_selection_ != SelectionType::None)
	{
		// Figure out what 'grid' position was clicked
		int rows = 16;
		int cols = 16;
		if (double_width_)
		{
			rows = 8;
			cols = 32;
		}
		int x = (e.GetX() - offset_.x) / square_size_;
		int y = (e.GetY() - offset_.y) / square_size_;

		// If it was within the palette box, select the cell
		if (x >= 0 && x < cols && y >= 0 && y < rows)
		{
			auto index = y * cols + x;

			if (e.GetModifiers() == wxMOD_SHIFT && allow_selection_ == SelectionType::Range && sel_base_ >= 0)
			{
				// Range select
				if (sel_base_ < index)
					setSelection(sel_base_, index);
				else
					setSelection(index, sel_base_);
			}
			else
			{
				// Single select
				sel_base_ = index;
				setSelection(sel_base_);
			}
		}
		else
		{
			sel_base_ = -1;
			setSelection(-1);
		}

		// Redraw
		updateBuffer(true);
		Refresh();
	}

	// Do normal left click stuff
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the palette canvas is right clicked
// -----------------------------------------------------------------------------
void PaletteCanvas::onMouseRightDown(wxMouseEvent& e)
{
	// Do normal right click stuff
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the mouse cursor is moved over the palette canvas
// -----------------------------------------------------------------------------
void PaletteCanvas::onMouseMotion(wxMouseEvent& e)
{
	// Check for dragging selection
	if (e.LeftIsDown() && allow_selection_ == SelectionType::Range)
	{
		// Figure out what 'grid' position the cursor is over
		int rows = 16;
		int cols = 16;
		if (double_width_)
		{
			rows = 8;
			cols = 32;
		}
		int x = (e.GetX() - offset_.x) / square_size_;
		int y = (e.GetY() - offset_.y) / square_size_;

		// Set selection accordingly
		if (x >= 0 && x < cols && y >= 0 && y < rows)
		{
			int sel = y * cols + x;
			if (sel > sel_begin_)
				setSelection(sel_begin_, sel);
		}
	}
}
