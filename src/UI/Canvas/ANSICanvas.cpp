
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor/UI/TextureXEditor/TextureXEditor.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Draw2D.h"
#include "Utility/CodePages.h"
#include "General/UI.h"

using namespace slade;


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
ANSICanvas::ANSICanvas(wxWindow* parent) : GLCanvas(parent, BGStyle::Checkered)
{
	view_.setCentered(true);
	view_.setScale(ui::scaleFactor());

	// Get the all-important font data
	auto res_archive = app::archiveManager().programResourceArchive();
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
	gl::Texture::clear(tex_image_);
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
		auto   c    = codepages::ansiColor(picdata_[i]);
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
	// Check image is valid
	if (!picdata_)
		return;

	// Load texture data
	if (!tex_image_)
	{
		vector<uint8_t> rgba_data(width_ * height_ * 4);
		writeRGBAData(rgba_data.data());
		tex_image_ = gl::Texture::createFromData(rgba_data.data(), width_, height_);
	}

	// Determine (texture)coordinates
	auto hx = static_cast<float>(width_) * 0.5f;
	auto hy = static_cast<float>(height_) * 0.5f;

	// Draw the image
	gl::draw2d::RenderOptions opt{ tex_image_ };
	gl::draw2d::drawRect({ -hx, -hy, hx, hy }, opt, &view_);

	// Draw outline
	opt.texture = 0;
	opt.colour.set(0, 0, 0, 64);
	gl::draw2d::drawRectOutline({ -hx, -hy, hx, hy }, opt, &view_);
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
