
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapGeometryBuffer3D.cpp
// Description:
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
#include "MapGeometryBuffer3D.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// MapGeometryBuffer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initializes the vertex array object for this buffer
// -----------------------------------------------------------------------------
void MapGeometryBuffer3D::initVAO()
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

	buffer_.bind();

	constexpr auto SIZE = 9 * sizeof(float);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, SIZE, (void*)0);
	glEnableVertexAttribArray(0);

	// Texture Coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, SIZE, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Brightness
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, SIZE, (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Normal
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, SIZE, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(3);

	gl::bindVAO(0);
}
