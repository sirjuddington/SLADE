
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#include "Graphics/SImage/SImage.h"
#include "OpenGL.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, bgtx_colour1, "#404050", CVar::Flag::Save)
CVAR(String, bgtx_colour2, "#505060", CVar::Flag::Save)
namespace
{
std::map<unsigned, OpenGL::Texture> textures;
OpenGL::Texture                     tex_missing;
OpenGL::Texture                     tex_background;
unsigned                            last_bound_tex = 0;
} // namespace


// -----------------------------------------------------------------------------
//
// Texture Struct Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if the OpenGL texture [id] exists
// -----------------------------------------------------------------------------
bool OpenGL::Texture::isCreated(unsigned id)
{
	return textures[id].id > 0;
}

// -----------------------------------------------------------------------------
// Returns true if the OpenGL texture [id] exists and has image data
// -----------------------------------------------------------------------------
bool OpenGL::Texture::isLoaded(unsigned id)
{
	auto& inf = textures[id];
	return inf.id > 0 && inf.size.x > 0 && inf.size.y > 0;
}

// -----------------------------------------------------------------------------
// Returns the info struct for the OpenGL texture [id]
// -----------------------------------------------------------------------------
const OpenGL::Texture& OpenGL::Texture::info(unsigned id)
{
	auto& tex = textures[id];
	if (tex.id > 0)
		return tex;

	return tex_missing;
}

// -----------------------------------------------------------------------------
// Returns the 'missing' texture id
// -----------------------------------------------------------------------------
unsigned OpenGL::Texture::missingTexture()
{
	if (!OpenGL::isInitialised())
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
unsigned OpenGL::Texture::backgroundTexture()
{
	if (!OpenGL::isInitialised())
		return 0;

	// Create the 'background' texture if necessary
	if (tex_background.id == 0)
	{
		wxColour col1(bgtx_colour1);
		wxColour col2(bgtx_colour2);

		auto id = create();
		genChequeredTexture(id, 8, { COLWX(col1), 255 }, { COLWX(col2), 255 });
		tex_background = textures[id];
	}

	return tex_background.id;
}

// -----------------------------------------------------------------------------
// Resets (clears) the 'background' texture
// -----------------------------------------------------------------------------
void OpenGL::Texture::resetBackgroundTexture()
{
	glDeleteTextures(1, &tex_background.id);

	textures[tex_background.id] = {};
	tex_background              = {};
}

// -----------------------------------------------------------------------------
// Creates a new OpenGL texture and returns the id
// -----------------------------------------------------------------------------
unsigned OpenGL::Texture::create(TexFilter filter, bool tiling)
{
	// Check OpenGL is initialised
	if (!OpenGL::isInitialised())
		return 0;

	// Generate the texture id
	unsigned id;
	glGenTextures(1, &id);

	// Set texture info
	textures[id].id     = id;
	textures[id].filter = filter;
	textures[id].tiling = tiling;

	return id;
}

// -----------------------------------------------------------------------------
// Creates a new OpenGL texture with RGBA [data] of [width]x[height]
// -----------------------------------------------------------------------------
unsigned OpenGL::Texture::createFromData(
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
unsigned OpenGL::Texture::createFromImage(const SImage& image, Palette* pal, TexFilter filter, bool tiling)
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
bool OpenGL::Texture::loadData(unsigned id, const uint8_t* data, unsigned width, unsigned height)
{
	// Check OpenGL is initialised
	if (!OpenGL::isInitialised())
		return false;

	// Check given id
	if (id == 0 || id == tex_missing.id || id == tex_background.id)
	{
		Log::warning(S_FMT("Unable to load OpenGL texture with id %d - invalid or built-in texture", id));
		return false;
	}

	// Check image dimensions
	if (!validTexDimension(width) || !validTexDimension(height))
	{
		Log::warning(S_FMT("Attempt to create OpenGL texture of invalid size %ldx%ld", width, height));
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
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Generate the texture
	if (tex_info.filter == TexFilter::Linear)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::Mipmap || tex_info.filter == TexFilter::LinearMipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::NearestMipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (tex_info.filter == TexFilter::NearestLinearMin)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		// Default to NEAREST
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	tex_info.size = { (int)width, (int)height };

	return true;
}

// -----------------------------------------------------------------------------
// Loads [image] to the OpenGL texture [id], using [pal] if necessary
// -----------------------------------------------------------------------------
bool OpenGL::Texture::loadImage(unsigned id, const SImage& image, Palette* pal)
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

	Log::warning(S_FMT("Attempt to create OpenGL texture of invalid size %ldx%ld", image.width(), image.height()));
	return false;
}

// -----------------------------------------------------------------------------
// Generates a 'chequerboard' texture using colours [col1] and [col2] and loads
// it to OpenGL texture [id]
// -----------------------------------------------------------------------------
bool OpenGL::Texture::genChequeredTexture(unsigned id, uint8_t block_size, ColRGBA col1, ColRGBA col2)
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
ColRGBA OpenGL::Texture::averageColour(unsigned id, Recti area)
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
	return { uint8_t(red / npix), uint8_t(green / npix), uint8_t(blue / npix), 255 };
}

// -----------------------------------------------------------------------------
// Binds the OpenGL texture [id] for use (unless it is already bound)
// -----------------------------------------------------------------------------
void OpenGL::Texture::bind(unsigned id, bool force)
{
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
// Deletes the OpenGL texture [id]
// -----------------------------------------------------------------------------
void OpenGL::Texture::clear(unsigned id)
{
	if (id == 0 || id == tex_missing.id || id == tex_background.id || textures.empty())
		return;

	textures[id] = {};
	glDeleteTextures(1, &id);
}

// -----------------------------------------------------------------------------
// Deletes all OpenGL textures
// -----------------------------------------------------------------------------
void OpenGL::Texture::clearAll()
{
	for (auto& tex : textures)
		glDeleteTextures(1, &tex.second.id);

	textures.clear();
	tex_missing    = {};
	tex_background = {};
}
