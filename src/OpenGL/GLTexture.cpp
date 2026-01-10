
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GLTexture.cpp
// Description: OpenGL texture handling code
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
#include "GLTexture.h"
#include "Geometry/Rect.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, bgtx_colour1, "#404050", CVar::Flag::Save)
CVAR(String, bgtx_colour2, "#505060", CVar::Flag::Save)
namespace
{
std::map<unsigned, gl::Texture> textures;
gl::Texture                     tex_missing;
gl::Texture                     tex_background;
gl::Texture                     tex_white;
unsigned                        last_bound_tex = 0;
} // namespace


// -----------------------------------------------------------------------------
//
// Texture Struct Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if the OpenGL texture [id] exists
// -----------------------------------------------------------------------------
bool gl::Texture::isCreated(unsigned id)
{
	return textures[id].id > 0;
}

// -----------------------------------------------------------------------------
// Returns true if the OpenGL texture [id] exists and has image data
// -----------------------------------------------------------------------------
bool gl::Texture::isLoaded(unsigned id)
{
	auto& inf = textures[id];
	return inf.id > 0 && inf.size.x > 0 && inf.size.y > 0;
}

// -----------------------------------------------------------------------------
// Returns the info struct for the OpenGL texture [id]
// -----------------------------------------------------------------------------
const gl::Texture& gl::Texture::info(unsigned id)
{
	auto& tex = textures[id];
	if (tex.id > 0)
		return tex;

	return tex_missing;
}

// -----------------------------------------------------------------------------
// Returns the 'missing' texture id
// -----------------------------------------------------------------------------
unsigned gl::Texture::missingTexture()
{
	if (!gl::isInitialised())
		return 0;

	// Create the 'missing' texture if necessary
	if (tex_missing.id == 0)
	{
		auto id = create();
		genChequeredTexture(id, 8, ColRGBA::BLACK, ColRGBA::RED);
		tex_missing = textures[id];
	}

	return tex_missing.id;
}

// -----------------------------------------------------------------------------
// Returns the 'background' texture id
// -----------------------------------------------------------------------------
unsigned gl::Texture::backgroundTexture()
{
	if (!gl::isInitialised())
		return 0;

	// Create the 'background' texture if necessary
	if (tex_background.id == 0)
	{
		wxColour col1(bgtx_colour1);
		wxColour col2(bgtx_colour2);

		auto id = create();
		genChequeredTexture(id, 8, ColRGBA{ col1 }, ColRGBA{ col2 });
		tex_background = textures[id];
	}

	return tex_background.id;
}

// -----------------------------------------------------------------------------
// Resets (clears) the 'background' texture
// -----------------------------------------------------------------------------
void gl::Texture::resetBackgroundTexture()
{
	if (!gl::isInitialised())
		return;

	glDeleteTextures(1, &tex_background.id);

	textures[tex_background.id] = {};
	tex_background              = {};
}

// -----------------------------------------------------------------------------
// Creates a new OpenGL texture and returns the id
// -----------------------------------------------------------------------------
unsigned gl::Texture::create(TexFilter filter, bool tiling)
{
	// Check OpenGL is initialised
	if (!gl::isInitialised())
		return 0;

	// Generate the texture id
	unsigned id = 0;
	glGenTextures(1, &id);

	// Check it generated ok
	if (id == 0)
	{
		wxFAIL_MSG("Failed to generate GL texture id - most likely because there is no active context");
		log::error("Failed to generate OpenGL texture id");
		return 0;
	}

	// Set texture info
	textures[id].id     = id;
	textures[id].filter = filter;
	textures[id].tiling = tiling;

	return id;
}

// -----------------------------------------------------------------------------
// Creates a new OpenGL texture with RGBA [data] of [width]x[height]
// -----------------------------------------------------------------------------
unsigned gl::Texture::createFromData(
	const uint8_t* data,
	unsigned       width,
	unsigned       height,
	TexFilter      filter,
	bool           tiling)
{
	auto id = create(filter, tiling);
	if (!loadData(id, data, width, height))
	{
		clear(id);
		return 0;
	}

	return id;
}

// -----------------------------------------------------------------------------
// Creates a new OpenGL texture from [image], using [pal] if necessary
// -----------------------------------------------------------------------------
unsigned gl::Texture::createFromImage(const SImage& image, const Palette* pal, TexFilter filter, bool tiling)
{
	auto id = create(filter, tiling);
	if (!loadImage(id, image, pal))
	{
		clear(id);
		return 0;
	}

	return id;
}

// -----------------------------------------------------------------------------
// Loads RGBA [data] of [width]x[height] to the OpenGL texture [id]
// -----------------------------------------------------------------------------
bool gl::Texture::loadData(unsigned id, const uint8_t* data, unsigned width, unsigned height)
{
	// Check OpenGL is initialised
	if (!gl::isInitialised())
		return false;

	// Check given id
	if (id == 0 || id == tex_missing.id || id == tex_background.id)
	{
		log::warning("Unable to load OpenGL texture with id {} - invalid or built-in texture", id);
		return false;
	}

	// Check image dimensions
	if (!validTexDimension(width) || !validTexDimension(height))
	{
		log::warning("Attempt to create OpenGL texture of invalid size {}x{}", width, height);
		return false;
	}

	bind(id);

	// Set texture params
	auto& tex_info = textures[id];
	if (tex_info.tiling)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}

	// Generate the texture
	if (tex_info.filter == TexFilter::Linear)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::Mipmap || tex_info.filter == TexFilter::LinearMipmap)
	{
		/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);*/

		// No mipmaps for now, just do linear
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::NearestMipmap)
	{
		/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);*/

		// No mipmaps for now, just do nearest
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::NearestLinearMin)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		// Default to NEAREST
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	tex_info.size = { static_cast<int>(width), static_cast<int>(height) };

	return true;
}

bool gl::Texture::loadAlphaData(unsigned id, const uint8_t* data, unsigned width, unsigned height)
{
	// Check OpenGL is initialised
	if (!gl::isInitialised())
		return false;

	// Check given id
	if (id == 0 || id == tex_missing.id || id == tex_background.id)
	{
		log::warning("Unable to load OpenGL texture with id {} - invalid or built-in texture", id);
		return false;
	}

	// Check image dimensions
	if (!validTexDimension(width) || !validTexDimension(height))
	{
		log::warning("Attempt to create OpenGL texture of invalid size {}x{}", width, height);
		return false;
	}

	static GLint swizzle_mask[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };

	bind(id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);

	textures[id].tiling = false;
	textures[id].filter = TexFilter::Linear;
	textures[id].size   = { static_cast<int>(width), static_cast<int>(height) };

	return true;
}

// -----------------------------------------------------------------------------
// Loads [image] to the OpenGL texture [id], using [pal] if necessary
// -----------------------------------------------------------------------------
bool gl::Texture::loadImage(unsigned id, const SImage& image, const Palette* pal)
{
	// Check image dimensions
	if (validTexDimension(image.width()) && validTexDimension(image.height()))
	{
		// Get RGBA image data
		MemChunk rgba;
		image.putRGBAData(rgba, pal);

		// Generate GL texture from rgba data
		return loadData(id, rgba.data(), image.width(), image.height());
	}

	log::warning("Attempt to create OpenGL texture of invalid size {}x{}", image.width(), image.height());
	return false;
}

// -----------------------------------------------------------------------------
// Generates a 'chequerboard' texture using colours [col1] and [col2] and loads
// it to OpenGL texture [id]
// -----------------------------------------------------------------------------
bool gl::Texture::genChequeredTexture(unsigned id, uint8_t block_size, ColRGBA col1, ColRGBA col2)
{
	// Check given block size and change if necessary
	for (uint8_t s = 1; s <= 64; s *= 2)
	{
		if (block_size <= s)
		{
			block_size = s;
			break;
		}
	}

	// Generate rgba data
	uint32_t        data_size = (block_size * block_size * 4) * 4;
	vector<uint8_t> data(data_size);
	uint32_t        c = 0;
	for (uint32_t y = 0; y < block_size; y++) // Top half
	{
		for (uint32_t x = 0; x < block_size; x++)
		{
			data[c++] = col1.r;
			data[c++] = col1.g;
			data[c++] = col1.b;
			data[c++] = 255;
		}
		for (uint32_t x = 0; x < block_size; x++)
		{
			data[c++] = col2.r;
			data[c++] = col2.g;
			data[c++] = col2.b;
			data[c++] = 255;
		}
	}
	for (uint32_t y = 0; y < block_size; y++) // Bottom half
	{
		for (uint32_t x = 0; x < block_size; x++)
		{
			data[c++] = col2.r;
			data[c++] = col2.g;
			data[c++] = col2.b;
			data[c++] = 255;
		}
		for (uint32_t x = 0; x < block_size; x++)
		{
			data[c++] = col1.r;
			data[c++] = col1.g;
			data[c++] = col1.b;
			data[c++] = 255;
		}
	}

	// Generate texture from rgba data
	return loadData(id, data.data(), block_size * 2, block_size * 2);
}

// -----------------------------------------------------------------------------
// Returns the average colour of the OpenGL texture [id] within [area]
// -----------------------------------------------------------------------------
ColRGBA gl::Texture::averageColour(unsigned id, Recti area)
{
	// Check texture is loaded
	if (!isLoaded(id))
		return ColRGBA::BLACK;

	// Empty area rect means full texture
	const auto& tex_info = textures[id];
	if (area.tl.x == area.br.x && area.tl.y == area.br.y)
		area.set(0, 0, tex_info.size.x, tex_info.size.y);

	// Clamp area to texture
	if (area.tl.x < 0)
		area.tl.x = 0;
	if (area.tl.y < 0)
		area.tl.y = 0;
	if (area.br.x > tex_info.size.x)
		area.br.x = tex_info.size.x;
	if (area.br.y > tex_info.size.y)
		area.br.y = tex_info.size.y;

	// Get texture pixels
	uint8_t* pixels = new uint8_t[tex_info.size.x * tex_info.size.y * 8];
	bind(tex_info.id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Add colour values
	unsigned red   = 0;
	unsigned green = 0;
	unsigned blue  = 0;
	unsigned npix  = 0;
	// Go through area
	for (int y = area.tl.y; y < area.br.y; y++)
	{
		for (int x = area.tl.x; x < area.br.x; x++)
		{
			// Add pixel
			unsigned c = (y * tex_info.size.x * 4) + (x * 4);
			red += pixels[c++];
			green += pixels[c++];
			blue += pixels[c++];

			npix++;
		}
	}

	// Clean up
	delete[] pixels;

	// Return average colour
	return {
		static_cast<uint8_t>(red / npix), static_cast<uint8_t>(green / npix), static_cast<uint8_t>(blue / npix), 255
	};
}

// -----------------------------------------------------------------------------
// Binds the OpenGL texture [id] for use (unless it is already bound)
// -----------------------------------------------------------------------------
void gl::Texture::bind(unsigned id, bool force)
{
	if (!gl::isInitialised())
		return;

	if (force)
	{
		glBindTexture(GL_TEXTURE_2D, id);
		last_bound_tex = id;
	}
	else if (id != last_bound_tex)
	{
		glBindTexture(GL_TEXTURE_2D, id);
		last_bound_tex = id;
	}
}

// -----------------------------------------------------------------------------
// Returns the 'plain white' texture id
// Used to simulate something being untextured since by default sampling a
// non-existant texture results in all black (and 0 alpha)
// -----------------------------------------------------------------------------
unsigned gl::Texture::whiteTexture()
{
	if (!gl::isInitialised())
		return 0;

	// Create the 'white' texture if necessary
	if (tex_white.id == 0)
	{
		auto id = create();
		genChequeredTexture(id, 8, ColRGBA::WHITE, ColRGBA::WHITE);
		tex_white = textures[id];
	}

	return tex_white.id;
}

// -----------------------------------------------------------------------------
// Deletes the OpenGL texture [id]
// -----------------------------------------------------------------------------
void gl::Texture::clear(unsigned id)
{
	if (!gl::isInitialised() || id == 0 || id == tex_missing.id || id == tex_background.id || textures.empty())
		return;

	textures[id] = {};
	glDeleteTextures(1, &id);
}

// -----------------------------------------------------------------------------
// Deletes all OpenGL textures
// -----------------------------------------------------------------------------
void gl::Texture::clearAll()
{
	if (!gl::isInitialised())
		return;

	for (auto& tex : textures)
		glDeleteTextures(1, &tex.second.id);

	textures.clear();
	tex_missing    = {};
	tex_background = {};
}

// -----------------------------------------------------------------------------
// Sets texture [id]'s [tiling] flag (ie. GL_TEXTURE_WRAP_* value)
// -----------------------------------------------------------------------------
void gl::Texture::setTiling(unsigned id, bool tiling)
{
	auto& tex_info = textures[id];
	if (tex_info.tiling == tiling)
		return;

	bind(id);

	if (tiling)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	tex_info.tiling = tiling;
}
