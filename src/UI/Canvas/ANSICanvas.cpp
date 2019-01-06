
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSICanvas.cpp
// Description: ANSICanvas class. An OpenGL canvas that displays an ANSI screen
//              using a VGA font
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
#include "Archive/ArchiveManager.h"
#include "MainEditor/UI/TextureXEditor/TextureXEditor.h"
#include "OpenGL/GLTexture.h"
#include "Utility/CodePages.h"



// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
namespace
{
const int NUMROWS = 25;
const int NUMCOLS = 80;
} // namespace


// -----------------------------------------------------------------------------
//
// ANSICanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ANSICanvas class constructor
// -----------------------------------------------------------------------------
ANSICanvas::ANSICanvas(wxWindow* parent, int id) : OGLCanvas(parent, id)
{
	// Get the all-important font data
	auto res_archive = App::archiveManager().programResourceArchive();
	if (!res_archive)
		return;
	auto ansi_font = res_archive->entryAtPath("vga-rom-font.16");
	if (!ansi_font || ansi_font->size() % 256)
		return;

	fontdata_ = ansi_font->rawData();

	// Init variables
	char_width_  = 8;
	char_height_ = ansi_font->size() / 256;
	width_       = NUMCOLS * char_width_;
	height_      = NUMROWS * char_height_;
	picdata_     = new uint8_t[width_ * height_];
}

// -----------------------------------------------------------------------------
// ANSICanvas class destructor
// -----------------------------------------------------------------------------
ANSICanvas::~ANSICanvas()
{
	OpenGL::Texture::clear(tex_image_);
	delete[] picdata_;
	// fontdata belongs to the ansi_font ArchiveEntry
	// ansidata belongs to the parent ANSIPanel
}

// -----------------------------------------------------------------------------
// Converts image data into RGBA format
// -----------------------------------------------------------------------------
void ANSICanvas::writeRGBAData(uint8_t* dest) const
{
	for (size_t i = 0; i < width_ * height_; ++i)
	{
		size_t j    = i << 2;
		auto   c    = CodePages::ansiColor(picdata_[i]);
		dest[j + 0] = c.r;
		dest[j + 1] = c.g;
		dest[j + 2] = c.b;
		dest[j + 3] = 0xFF;
	}
}

// -----------------------------------------------------------------------------
// Draws the image
// -----------------------------------------------------------------------------
void ANSICanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Draw the image
	drawImage();

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}

// -----------------------------------------------------------------------------
// Draws the image
// (reloads the image as a texture each time, will change this later...)
// -----------------------------------------------------------------------------
void ANSICanvas::drawImage()
{
	// Check image is valid
	if (!picdata_)
		return;

	// Save current matrix
	glPushMatrix();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Load texture data
	if (!tex_image_)
	{
		vector<uint8_t> rgba_data(width_ * height_ * 4);
		writeRGBAData(rgba_data.data());
		tex_image_ = OpenGL::Texture::createFromData(rgba_data.data(), width_, height_);
	}

	// Determine (texture)coordinates
	double x = (double)width_;
	double y = (double)height_;

	// Draw the image
	OpenGL::setColour(COL_WHITE);
	Drawing::drawTexture(tex_image_);

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Draw outline
	OpenGL::setColour(0, 0, 0, 64);
	glBegin(GL_LINE_LOOP);
	glVertex2d(0, 0);
	glVertex2d(0, y);
	glVertex2d(x, y);
	glVertex2d(x, 0);
	glEnd();

	// Restore previous matrix
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Draws a single character. This is called from the parent ANSIPanel
// -----------------------------------------------------------------------------
void ANSICanvas::drawCharacter(size_t index) const
{
	if (!ansidata_)
		return;

	// Setup some variables
	uint8_t  chara = ansidata_[index << 1];       // Character
	uint8_t  color = ansidata_[(index << 1) + 1]; // Colour
	uint8_t* pic   = picdata_ + ((index / NUMCOLS) * width_ * char_height_)
				   + ((index % NUMCOLS) * char_width_);      // Position on canvas to draw
	const uint8_t* fnt = fontdata_ + (char_height_ * chara); // Position of character in font image

	// Draw character (including background)
	for (int y = 0; y < char_height_; ++y)
		for (int x = 0; x < char_width_; ++x)
			pic[x + (y * width_)] = (fnt[y] & (1 << (char_width_ - 1 - x))) ? (color & 15) : ((color & 112) >> 4);
}
