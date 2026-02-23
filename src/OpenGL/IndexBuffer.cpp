
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    IndexBuffer.cpp
// Description: IndexBuffer class - buffer for storing vertex indices for
//              indexed drawing in OpenGL
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
#include "IndexBuffer.h"
#include "OpenGL.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// IndexBuffer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// IndexBuffer class destructor
// -----------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
	if (ebo_ > 0)
		deleteEBO(ebo_);
}

// -----------------------------------------------------------------------------
// Binds the index buffer for use. Creates the buffer if it doesn't exist yet.
// Returns true if the buffer was successfully bound.
// -----------------------------------------------------------------------------
bool IndexBuffer::bind()
{
	if (!getContext())
		return false;

	if (ebo_ == 0)
		ebo_ = createBuffer();

	bindEBO(ebo_);

	return ebo_ != 0;
}

// -----------------------------------------------------------------------------
// Updates the buffer with [data] starting at [offset].
// Returns true if the buffer was successfully updated.
// -----------------------------------------------------------------------------
bool IndexBuffer::update(unsigned offset, const vector<uint32_t>& data)
{
	if (!getContext())
		return false;

	if (offset >= index_count_ || offset + data.size() > index_count_)
		return false;

	if (bind())
	{
		glBufferSubData(
			GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(uint32_t), data.size() * sizeof(uint32_t), data.data());
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Uploads [data] to the buffer, replacing any existing data.
// Returns true if the buffer was successfully updated.
// -----------------------------------------------------------------------------
bool IndexBuffer::upload(const vector<uint32_t>& data)
{
	if (!getContext())
		return false;

	if (!bind())
		return false;

	// Only allocate new buffer if we are uploading more data than the buffer currently holds
	if (data.size() > index_count_)
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size() * sizeof(uint32_t), data.data(), GL_STATIC_DRAW);
	else
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, data.size() * sizeof(uint32_t), data.data());

	index_count_ = data.size();

	return true;
}
