
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PaletteCanvas.cpp
// Description: PaletteCanvas class. An OpenGL canvas that displays a palette
//              (256 colours max)
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
#include "OpenGL/Draw2D.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PaletteCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PaletteCanvas class constructor
// -----------------------------------------------------------------------------
PaletteCanvas::PaletteCanvas(wxWindow* parent) : GLCanvas(parent), vb_palette_{ new gl::VertexBuffer2D() }
{
	// Init with default palette
	palette_ = std::make_unique<Palette>();

	// Bind events
	Bind(wxEVT_LEFT_DOWN, &PaletteCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &PaletteCanvas::onMouseRightDown, this);
	Bind(wxEVT_MOTION, &PaletteCanvas::onMouseMotion, this);

	// Update when resized
	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent& e)
		{
			vb_palette_->buffer().clear();
			e.Skip();
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
	sel_begin_ = begin;
	if (end == -1)
		sel_end_ = begin;
	else
		sel_end_ = end;

	vb_palette_->buffer().clear();
}

// -----------------------------------------------------------------------------
// Sets the palette to display
// -----------------------------------------------------------------------------
void PaletteCanvas::setPalette(const Palette* pal)
{
	GLCanvas::setPalette(pal);
	vb_palette_->buffer().clear();
	Refresh();
}

// -----------------------------------------------------------------------------
// Draws the palette as 16x16 (or 32x8) coloured squares
// -----------------------------------------------------------------------------
void PaletteCanvas::draw()
{
	if (vb_palette_->buffer().empty())
		updatePaletteBuffer();

	// Setup default 2d shader (untextured)
	const auto& shader = gl::draw2d::defaultShader(false);
	view_.setupShader(shader);
	shader.setUniform("colour", glm::vec4{ 1.0f });

	// Draw palette
	vb_palette_->draw(gl::Primitive::Quads);
}

// -----------------------------------------------------------------------------
// Updates the palette vertex buffer
// -----------------------------------------------------------------------------
void PaletteCanvas::updatePaletteBuffer()
{
	// Update variables
	rows_        = double_width_ ? 8 : 16;
	cols_        = double_width_ ? 32 : 16;
	int x_size   = (GetSize().x) / cols_;
	int y_size   = (GetSize().y) / rows_;
	square_size_ = std::min<int>(x_size, y_size);

	// Add vertices to buffer
	int       index     = 0;
	glm::vec4 colour    = { 1.f, 1.f, 1.f, 1.f };
	glm::vec4 col_white = { 1.f, 1.f, 1.f, 1.f };
	glm::vec4 col_black = { 0.f, 0.f, 0.f, 1.f };
	float     x         = 1.f;
	float     y         = 1.f;
	float     sizef     = static_cast<float>(square_size_) - 2.f;
	for (auto row = 0; row < rows_; row++)
	{
		for (auto col = 0; col < cols_; col++)
		{
			// Get colour
			colour.r = palette_->colour(index).fr();
			colour.g = palette_->colour(index).fg();
			colour.b = palette_->colour(index).fb();

			if (index >= sel_begin_ && index <= sel_end_)
			{
				// Selected: White -> Black -> Colour
				vb_palette_->add({ { x, y }, col_white });
				vb_palette_->add({ { x + sizef, y }, col_white });
				vb_palette_->add({ { x + sizef, y + sizef }, col_white });
				vb_palette_->add({ { x, y + sizef }, col_white });

				vb_palette_->add({ { x + 1.f, y + 1.f }, col_black });
				vb_palette_->add({ { x + sizef - 1.f, y + 1.f }, col_black });
				vb_palette_->add({ { x + sizef - 1.f, y + sizef - 1.f }, col_black });
				vb_palette_->add({ { x + 1.f, y + sizef - 1.f }, col_black });

				vb_palette_->add({ { x + 2.f, y + 2.f }, colour });
				vb_palette_->add({ { x + sizef - 2.f, y + 2.f }, colour });
				vb_palette_->add({ { x + sizef - 2.f, y + sizef - 2.f }, colour });
				vb_palette_->add({ { x + 2.f, y + sizef - 2.f }, colour });
			}
			else
			{
				// Not selected
				vb_palette_->add({ { x, y }, colour });
				vb_palette_->add({ { x + sizef, y }, colour });
				vb_palette_->add({ { x + sizef, y + sizef }, colour });
				vb_palette_->add({ { x, y + sizef }, colour });
			}

			// Next column
			++index;
			x += sizef + 2.f;
		}

		// Next row
		y += sizef + 2.f;
		x = 1.f;
	}

	vb_palette_->push();
}


// -----------------------------------------------------------------------------
//
// PaletteCanvas Class Events
//
// -----------------------------------------------------------------------------


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
		const wxSize contentSize = GetSize() * GetContentScaleFactor();
		int          x_size      = contentSize.x / cols;
		int          y_size      = contentSize.y / rows;
		int          size        = std::min<int>(x_size, y_size);
		int          x           = (e.GetX() * GetContentScaleFactor()) / size;
		int          y           = (e.GetY() * GetContentScaleFactor()) / size;

		// If it was within the palette box, select the cell
		if (x >= 0 && x < cols && y >= 0 && y < rows)
			setSelection(y * cols + x);
		else
			setSelection(-1);

		// Redraw
		vb_palette_->buffer().clear();
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
		const wxSize contentSize = GetSize() * GetContentScaleFactor();
		int          x_size      = contentSize.x / cols;
		int          y_size      = contentSize.y / rows;
		int          size        = std::min<int>(x_size, y_size);
		int          x           = (e.GetX() * GetContentScaleFactor()) / size;
		int          y           = (e.GetY() * GetContentScaleFactor()) / size;

		// Set selection accordingly
		if (x >= 0 && x < cols && y >= 0 && y < rows)
		{
			int sel = y * cols + x;
			if (sel > sel_begin_)
				setSelection(sel_begin_, sel);

			vb_palette_->buffer().clear();
			Refresh();
		}
	}
}
