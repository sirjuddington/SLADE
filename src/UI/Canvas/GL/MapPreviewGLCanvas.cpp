
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapPreviewGLCanvas.cpp
// Description: OpenGL Canvas that shows a basic map preview which can be
//              optionally zoomed/panned
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
#include "MapPreviewGLCanvas.h"
#include "Archive/EntryType/EntryType.h"
#include "General/ColourConfiguration.h"
#include "General/MapPreviewData.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/PointSpriteBuffer.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map_view_things, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapPreviewGLCanvas Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MapPreviewGLCanvas class constructor
// -----------------------------------------------------------------------------
MapPreviewGLCanvas::MapPreviewGLCanvas(wxWindow* parent, MapPreviewData* data, bool allow_zoom, bool allow_pan) :
	GLCanvas(parent),
	data_{ data }
{
	// Centered view
	setView({ true, true, false });

	// Mousewheel zoom
	if (allow_zoom)
		setupMousewheelZoom();

	// View Panning
	if (allow_pan)
		setupMousePanning();
}

// -----------------------------------------------------------------------------
// Draws the map
// -----------------------------------------------------------------------------
void MapPreviewGLCanvas::draw()
{
	setBackground(BGStyle::Colour, colourconfig::colour("map_view_background"));

	// Reset buffers if data is updated since last draw
	if (buffer_updated_time_ < data_->updated_time)
	{
		if (lines_buffer_)
			lines_buffer_->buffer().clear();
		if (things_buffer_)
			things_buffer_->buffer().clear();
		view_init_           = false;
		buffer_updated_time_ = data_->updated_time;
	}

	// Update buffer if needed
	if (!lines_buffer_ || lines_buffer_->buffer().empty())
		updateLinesBuffer();

	// Zoom/offset to show full map
	if (!view_init_)
	{
		view_.fitTo(data_->bounds, 1.1);
		view_.zoom(0.95);
		view_init_ = true;
	}

	// Setup drawing
	auto& shader = gl::LineBuffer::shader();
	view_.setupShader(shader);

	// Draw lines
	lines_buffer_->draw();

	// Draw things
	if (map_view_things)
	{
		// Update buffer if needed
		if (!things_buffer_ || things_buffer_->buffer().empty())
			updateThingsBuffer();

		// Draw things
		things_buffer_->setPointRadius(20.0f);
		things_buffer_->setColour(colourconfig::colour("map_view_thing"));
		things_buffer_->draw(gl::PointSpriteType::Circle, &view_);
	}
}

// -----------------------------------------------------------------------------
// Updates the lines vertex buffer
// -----------------------------------------------------------------------------
void MapPreviewGLCanvas::updateLinesBuffer()
{
	if (!data_)
		return;

	// Setup colours
	glm::vec4 col_view_line_1s      = colourconfig::colour("map_view_line_1s");
	glm::vec4 col_view_line_2s      = colourconfig::colour("map_view_line_2s");
	glm::vec4 col_view_line_special = colourconfig::colour("map_view_line_special");
	glm::vec4 col_view_line_macro   = colourconfig::colour("map_view_line_macro");

	if (!lines_buffer_)
		lines_buffer_ = std::make_unique<gl::LineBuffer>();

	gl::LineBuffer::Line lb_line;
	for (auto& line : data_->lines)
	{
		// Check ends
		if (line.v1 >= data_->vertices.size() || line.v2 >= data_->vertices.size())
			continue;

		// Set colour
		if (line.special)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_special;
		else if (line.macro)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_macro;
		else if (line.twosided)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_2s;
		else
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_1s;

		// Add to buffer
		lb_line.v1_pos_width = {
			data_->vertices[line.v1].x, data_->vertices[line.v1].y, 0.0f, line.twosided ? 1.5f : 2.0f
		};
		lb_line.v2_pos_width = {
			data_->vertices[line.v2].x, data_->vertices[line.v2].y, 0.0f, line.twosided ? 1.5f : 2.0f
		};
		lines_buffer_->add(lb_line);
	}
	lines_buffer_->push();
}

// -----------------------------------------------------------------------------
// Updates the things vertex buffer
// -----------------------------------------------------------------------------
void MapPreviewGLCanvas::updateThingsBuffer()
{
	if (!data_)
		return;

	if (!things_buffer_)
		things_buffer_ = std::make_unique<gl::PointSpriteBuffer>();

	for (auto& thing : data_->things)
		things_buffer_->add({ thing.x, thing.y });

	things_buffer_->push();
}
