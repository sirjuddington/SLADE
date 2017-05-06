
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GLTexture.cpp
 * Description: Simple class to handle an OpenGL texture
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "GLTexture.h"
#include "OpenGL.h"
#include "Graphics/SImage/SImage.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
GLTexture GLTexture::tex_background;
GLTexture GLTexture::tex_missing;
CVAR(String, bgtx_colour1, "#404050", CVAR_SAVE)
CVAR(String, bgtx_colour2, "#505060", CVAR_SAVE)


/*******************************************************************
 * GLTEXTURE CLASS FUNCTIONS
 *******************************************************************/

/* GLTexture::GLTexture
 * GLTexture class constructor
 *******************************************************************/
GLTexture::GLTexture(bool allow_split)
{
	this->loaded = false;
	this->allow_split = allow_split;
	this->filter = NEAREST;
	this->tiling = true;
	this->scale_x = 1.0;
	this->scale_y = 1.0;
}

/* GLTexture::~GLTexture
 * GLTexture class destructor
 *******************************************************************/
GLTexture::~GLTexture()
{
	// Delete current textures if they exist
	if (loaded)
		clear();
}

/* GLTexture::loadData
 * Builds an opengl texture from [data] (raw RGBA). If [add] is true,
 * the texture is added to the texture list (for split images),
 * otherwise any current texture data is overwritten
 *******************************************************************/
bool GLTexture::loadData(const uint8_t* data, uint32_t width, uint32_t height, bool add)
{
	// Check data was given
	if (!data)
		return false;

	// Check OpenGL is initialised
	if (!OpenGL::isInitialised())
		return false;

	// Delete current textures if they exist
	if (!add && loaded)
		clear();

	// Create texture struct
	gl_tex_t ntex;
	ntex.width = width;
	ntex.height = height;

	// Generate the texture id
	glGenTextures(1, &ntex.id);
	glBindTexture(GL_TEXTURE_2D, ntex.id);

	// Set texture params
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

	// Force mipmapping if np2 and splitting isn't allowed
	if (!allow_split && (!OpenGL::validTexDimension(width) || !OpenGL::validTexDimension(height)))
	{
		if (filter == LINEAR)
			filter = LINEAR_MIPMAP;
		else if (filter == NEAREST)
			filter = NEAREST_MIPMAP;
	}

	// Generate the texture
	if (filter == LINEAR)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (filter == MIPMAP || filter == LINEAR_MIPMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (filter == NEAREST_MIPMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (filter == NEAREST_LINEAR_MIN)
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

	// Update variables
	loaded = true;
	this->width = width;
	this->height = height;
	this->scale_x = 1.0;
	this->scale_y = 1.0;
	tex.push_back(ntex);

	return true;
}

/* GLTexture::loadRawData
 * Loads raw RGBA data to the texture. If the dimensions are invalid
 * for the system opengl implementation, the data will be split into
 * 128x128 squares. Returns false if the given data is invalid, true
 * otherwise
 *******************************************************************/
bool GLTexture::loadRawData(const uint8_t* data, uint32_t w, uint32_t h)
{
	// Check image was given
	if (!data)
		return false;

	// Clear current texture
	clear();

	// Check image dimensions
	if (OpenGL::validTexDimension(w) && OpenGL::validTexDimension(h))
	{
		// If the image dimensions are valid for OpenGL on this system, just load it as a single texture

		return loadData(data, w, h);
	}
	else
	{
		// Otherwise split the image into 128x128 chunks
		uint8_t* buf = new uint8_t[128*128*4];
		size_t top = 0;
		while (top < h)
		{
			size_t left = 0;
			while (left < w)
			{
				// Load 128x128 portion of image
				memset(buf, 0, 128*128*4);
				size_t rowlen = MIN(128, int(w - left));
				size_t collen = MIN(128, int(h - top));
				for (size_t i = 0; i < collen; ++i)
				{
					size_t doffset = (((top + i) * w) + left) * 4;
					size_t boffset = i * 128 * 4;
					memcpy(buf + boffset, data + doffset, rowlen*4);
				}
				loadData(buf, 128, 128, true);

				// Move right 128px
				left += 128;
			}

			// Move down 128px
			top += 128;
		}
		delete[] buf;

		// Update variables
		width = w;
		height = h;
		scale_x = scale_y = 1.0;

		return true;
	}
}

/* GLTexture::loadImage
 * Loads SImage data to the texture. If the dimensions are invalid
 * for the system opengl implementation, the data will be split into
 * 128x128 squares. Returns false if the given data is invalid, true
 * otherwise
 *******************************************************************/
bool GLTexture::loadImage(SImage* image, Palette8bit* pal)
{
	// Check image was given
	if (!image)
		return false;

	// Check image is valid
	if (!image->isValid())
		return false;

	// Clear current texture
	clear();

	// Check image dimensions
	if (OpenGL::validTexDimension(image->getWidth()) && OpenGL::validTexDimension(image->getHeight()))
	{
		// If the image dimensions are valid for OpenGL on this system, just load it as a single texture

		// Get RGBA image data
		MemChunk rgba;
		image->getRGBAData(rgba, pal);

		// Generate GL texture from rgba data
		return loadData(rgba.getData(), image->getWidth(), image->getHeight());
	}
	else
	{
		// Otherwise split the image into 128x128 chunks
		int top = 0;
		while (top < image->getHeight())
		{
			int left = 0;
			while (left < image->getWidth())
			{
				// Load 128x128 portion of image
				loadImagePortion(image, rect_t(left, top, left + 128, top + 128), pal, true);

				// Move right 128px
				left += 128;
			}

			// Move down 128px
			top += 128;
		}

		// Update variables
		width = image->getWidth();
		height = image->getHeight();
		scale_x = scale_y = 1.0;

		return true;
	}
}

/* GLTexture::loadImagePortion
 * Loads a portion of a SImage to the texture. Only used internally,
 * the portion must be 128x128 in size
 *******************************************************************/
bool GLTexture::loadImagePortion(SImage* image, rect_t rect, Palette8bit* pal, bool add)
{
	// Check image was given
	if (!image)
		return false;

	// Check image is valid
	if (!image->isValid())
		return false;

	// Check portion rect is valid
	if (rect.width() <= 0 || rect.height() <= 0)
		return false;

	// Get RGBA image data
	MemChunk rgba;
	image->getRGBAData(rgba, pal);

	// Init texture data
	MemChunk portion;
	portion.reSize(rect.width() * rect.height() * 4, false);
	portion.fillData(0);

	// Read portion of image if rect isn't completely outside the image
	if (!(rect.left() >= image->getWidth() || rect.right() < 0 || rect.top() >= image->getHeight() || rect.bottom() < 0))
	{
		// Determine start of each row to read
		uint32_t row_start = 0;
		if (rect.left() > 0)
			row_start = rect.left();

		// Determine width of each row to read
		uint32_t row_width = rect.right() - row_start;
		if (rect.right() >= image->getWidth())
			row_width = image->getWidth() - row_start;

		// Determine difference between the left of the portion and the left of the image
		uint32_t skip = 0;
		if (rect.left() < 0)
			skip = (0 - rect.left()) * 4;

		// Create temp row buffer
		uint8_t* buf = new uint8_t[rect.width() * 4];

		// Go through each row
		for (int32_t row = rect.top(); row < rect.bottom(); row++)
		{
			// Clear row buffer
			memset(buf, 0, rect.width() * 4);

			// Check that the current row is within the image
			if (row >= 0 && row < image->getHeight())
			{
				// Seek to current row in image data
				rgba.seek((row * image->getWidth() + row_start) * 4, SEEK_SET);

				// Copy the row data
				rgba.read(buf + skip, row_width * 4);
			}

			// Write the row
			portion.write(buf, rect.width() * 4);
		}

		// Free buffer
		delete[] buf;
	}
	scale_x = scale_y = 1.0;

	// Generate texture from rgba data
	return loadData(portion.getData(), rect.width(), rect.height(), add);
}

/* GLTexture::clear
 * Clears the texture and resets variables
 *******************************************************************/
bool GLTexture::clear()
{
	// Delete texture(s)
	for (size_t a = 0; a < tex.size(); a++)
		glDeleteTextures(1, &tex[a].id);
	tex.clear();

	// Reset variables
	width = 0;
	height = 0;
	loaded = false;
	scale_x = scale_y = 1.0;

	return true;
}

/* GLTexture::genChequeredTexture
 * Generates a chequered pattern, with each square being [size] and
 * alternating between [col1] and [col2]
 *******************************************************************/
bool GLTexture::genChequeredTexture(uint8_t block_size, rgba_t col1, rgba_t col2)
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
	uint32_t data_size = (block_size*block_size*4) * 4;
	uint8_t* data = new uint8_t[data_size];
	uint32_t c = 0;
	for (uint32_t y = 0; y < block_size; y++)  		// Top half
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
	for (uint32_t y = 0; y < block_size; y++)  		// Bottom half
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
	loadData(data, block_size*2, block_size*2);

	// Clean up
	delete[] data;
	scale_x = scale_y = 1.0;

	return true;
}

/* GLTexture::bind
 * Binds the texture for use in opengl. Returns false if the texture
 * isn't loaded, true otherwise
 *******************************************************************/
bool GLTexture::bind()
{
	// Check texture is loaded
	if (!loaded || tex.empty())
		return false;

	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, tex[0].id);

	return true;
}

/* GLTexture::draw2d
 * Draws the texture as a 2d image at [x], [y]. Returns false if the
 * texture isn't loaded, true otherwise
 *******************************************************************/
bool GLTexture::draw2d(double x, double y, bool flipx, bool flipy)
{
	// Can't draw if texture not loaded
	if (!loaded || tex.empty())
		return false;

	// Flipping?
	if (flipx)
		x += width;
	if (flipy)
		y += height;

	// If the texture isn't split, just draw it straight
	if (OpenGL::validTexDimension(width) && OpenGL::validTexDimension(height))
	{
		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, tex[0].id);

		// Setup metrics
		double h = (double)width;
		double v = (double)height;
		if (flipx) h = -h;
		if (flipy) v = -v;

		// Translate to position
		glPushMatrix();
		glTranslated(x, y, 0);

		// Draw
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0);	glVertex2d(0, 0);
		glTexCoord2d(0, 1);	glVertex2d(0, v);
		glTexCoord2d(1, 1);	glVertex2d(h, v);
		glTexCoord2d(1, 0); glVertex2d(h, 0);
		glEnd();

		glPopMatrix();
	}

	// Otherwise draw the 128x128 chunks
	else
	{
		// Translate to position
		glPushMatrix();
		glTranslated(x, y, 0);

		double stepx = 128;
		if (flipx) stepx = -128;
		double stepy = 128;
		if (flipy) stepy = -128;

		size_t tex_index = 0;
		double top = 0;
		while (top < height && top >= 0)
		{
			double left = 0;
			while (left < width && left >= 0)
			{
				if (tex_index < tex.size())
				{
					// Bind the texture
					glBindTexture(GL_TEXTURE_2D, tex[tex_index].id);

					// Draw
					glBegin(GL_QUADS);
					glTexCoord2d(0, 0);	glVertex2d(left, top);
					glTexCoord2d(0, 1);	glVertex2d(left, top+stepy);
					glTexCoord2d(1, 1);	glVertex2d(left+stepx, top+stepy);
					glTexCoord2d(1, 0); glVertex2d(left+stepx, top);
					glEnd();
				}

				// Move right 128px
				left += stepx;
				tex_index++;
			}

			// Move down 128px
			top += stepy;
		}

		glPopMatrix();
	}

	return true;
}

/* GLTexture::draw2dTiled
 * Draws the texture tiled within an area [width]x[height]. Returns
 * false if the texture isn't loaded, true otherwise
 *******************************************************************/
bool GLTexture::draw2dTiled(uint32_t width, uint32_t height)
{
	// Can't draw if texture not loaded
	if (!loaded || tex.empty())
		return false;

	// If the texture isn't split, just draw it straight
	if (OpenGL::validTexDimension(width) && OpenGL::validTexDimension(height))
	{
		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, tex[0].id);

		// Calculate texture coordinates
		double tex_x = (double)width / (double)this->width;
		double tex_y = (double)height / (double)this->height;

		// Draw
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0);			glVertex2d(0, 0);
		glTexCoord2d(0, tex_y);		glVertex2d(0, height);
		glTexCoord2d(tex_x, tex_y);	glVertex2d(width, height);
		glTexCoord2d(tex_x, 0);		glVertex2d(width, 0);
		glEnd();
	}

	// Otherwise draw the 128x128 chunks
	else
	{
		uint32_t x = 0;
		while (x < width)
		{
			uint32_t y = 0;
			while (y < height)
			{
				// Draw texture
				draw2d(x, y);

				// Move down
				y += this->height;
			}

			// Move right
			x += this->width;
		}
	}

	return true;
}

/* GLTexture::averageColour
 * Returns the average colour of the texture
 *******************************************************************/
rgba_t GLTexture::averageColour(rect_t area)
{
	// Check texture is loaded
	if (!loaded || tex.empty())
		return COL_BLACK;

	// Empty area rect means full texture
	if (area.tl.x == area.br.x && area.tl.y == area.br.y)
		area.set(0, 0, width, height);

	// Clamp area to texture
	if (area.tl.x < 0)					area.tl.x = 0;
	if (area.tl.y < 0)					area.tl.y = 0;
	if ((unsigned)area.br.x > width)	area.br.x = width;
	if ((unsigned)area.br.y > height)	area.br.y = height;

	// Get texture pixels
	uint8_t* pixels = new uint8_t[width*height*8];
	glBindTexture(GL_TEXTURE_2D, tex[0].id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Add colour values
	unsigned red = 0;
	unsigned green = 0;
	unsigned blue = 0;
	unsigned npix = 0;
	// Go through area
	for (int y = area.tl.y; y < area.br.y; y++)
	{
		for (int x = area.tl.x; x < area.br.x; x++)
		{
			// Add pixel
			unsigned c = (y * width * 4) + (x * 4);
			red += pixels[c++];
			green += pixels[c++];
			blue += pixels[c++];

			npix++;
		}
	}

	// Clean up
	delete[] pixels;

	// Return average colour
	return rgba_t(red/npix, green/npix, blue/npix, 255);
}


/*******************************************************************
 * GLTEXTURE STATIC FUNCTIONS
 *******************************************************************/

/* GLTexture::bgTex
 * Returns the global chequered 'background' texture
 *******************************************************************/
GLTexture& GLTexture::bgTex()
{
	if (!tex_background.isLoaded())
	{
		wxColour col1(bgtx_colour1);
		wxColour col2(bgtx_colour2);
		tex_background.genChequeredTexture(8, rgba_t(COLWX(col1), 255), rgba_t(COLWX(col2), 255));
	}
	return tex_background;
}

GLTexture& GLTexture::missingTex()
{
	if (!tex_missing.isLoaded())
		tex_missing.genChequeredTexture(8, rgba_t(0, 0, 0), rgba_t(255, 0, 0));
	return tex_missing;
}

/* GLTexture::resetBgTex
 * Resets the global chequered 'background' texture
 *******************************************************************/
void GLTexture::resetBgTex()
{
	if (tex_background.isLoaded())
		tex_background.clear();
}
