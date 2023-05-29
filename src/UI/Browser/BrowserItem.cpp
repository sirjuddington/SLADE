
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BrowserItem.cpp
// Description: A class representing a single browser item. Each item has a
//              name, index and image associated with it, and handles drawing
//              itself.
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
#include "BrowserItem.h"
#include "BrowserWindow.h"
#include "General/UI.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
#include "Utility/StringUtils.h"

using namespace slade;
using NameType = BrowserCanvas::NameType;
using ItemView = BrowserCanvas::ItemView;


// -----------------------------------------------------------------------------
//
// BrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BrowserItem class constructor
// -----------------------------------------------------------------------------
BrowserItem::BrowserItem(const wxString& name, unsigned index, const wxString& type) :
	type_{ type },
	name_{ name },
	index_{ index }
{
}

// -----------------------------------------------------------------------------
// Loads the item image (base class does nothing, must be overridden by child
// classes to be useful at all)
// -----------------------------------------------------------------------------
bool BrowserItem::loadImage()
{
	return false;
}

// -----------------------------------------------------------------------------
// Draws the item in a [size]x[size] box, keeping the correct aspect ratio of
// it's image
// -----------------------------------------------------------------------------
void BrowserItem::draw(
	int            size,
	int            x,
	int            y,
	drawing::Font  font,
	NameType       nametype,
	ItemView       viewtype,
	const ColRGBA& colour,
	bool           text_shadow)
{
	// Determine item name string (for normal viewtype)
	string draw_name;
	if (nametype == NameType::Normal)
		draw_name = name_;
	else if (nametype == NameType::Index)
		draw_name = fmt::format("{}", index_);

	// Truncate name if needed
	if (parent_->truncateNames() && draw_name.size() > 8)
	{
		// textures/aquatex/AQCONC13.png -> t./a./AQCONC13.png
		// textures/AQDIRT01.png -> t./AQDIRT01.png
		string_view draw_name_v{ draw_name };
		if (strutil::contains(draw_name, '/'))
		{
			int    last_pos = 0;
			string new_draw_name;
			while (strutil::contains(draw_name_v.substr(last_pos), '/'))
			{
				new_draw_name.append(draw_name_v.substr(last_pos, 1)).append("./");
				last_pos += draw_name_v.substr(last_pos).find('/') + 1;
			}
			draw_name = new_draw_name + strutil::afterLast(draw_name, '/');
		}
		else
			draw_name = fmt::format("{}...", string_view{ draw_name.data(), 8 });
	}

	// Item name
	if (viewtype == ItemView::Normal)
	{
		if (text_shadow)
			drawing::drawText(
				draw_name, x + (size * 0.5 + 1), y + size + 5, ColRGBA::BLACK, font, drawing::Align::Center);
		drawing::drawText(draw_name, x + (size * 0.5), y + size + 4, colour, font, drawing::Align::Center);
	}
	else if (viewtype == ItemView::Tiles)
	{
		// Create text box if needed
		if (!text_box_)
			text_box_ = std::make_unique<TextBox>(
				fmt::format("{}\n{}", index_, name_.mb_str().data()), font, ui::scalePx(144), ui::scalePx(16));

		int top = y;
		top += ((size - text_box_->height()) * 0.5);

		if (text_shadow)
			text_box_->draw(x + size + 9, top + 1, ColRGBA::BLACK);
		text_box_->draw(x + size + 8, top, colour);
	}

	// If the item is blank don't bother with the image
	if (blank_)
		return;

	// Try to load image if it isn't already
	if (!image_tex_ || (image_tex_ && !gl::Texture::isLoaded(image_tex_)))
		loadImage();

	// If it still isn't just draw a red box with an X
	if (!image_tex_ || (image_tex_ && !gl::Texture::isLoaded(image_tex_)))
	{
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

		glColor3f(1, 0, 0);
		glDisable(GL_TEXTURE_2D);

		// Outline
		glBegin(GL_LINE_LOOP);
		glVertex2i(x, y);
		glVertex2i(x, y + size);
		glVertex2i(x + size, y + size);
		glVertex2i(x + size, y);
		glEnd();

		// X
		glBegin(GL_LINES);
		glVertex2i(x, y);
		glVertex2i(x + size, y + size);
		glVertex2i(x, y + size);
		glVertex2i(x + size, y);
		glEnd();

		glPopAttrib();

		return;
	}

	// Determine texture dimensions
	auto&  tex_info = gl::Texture::info(image_tex_);
	double width    = tex_info.size.x;
	double height   = tex_info.size.y;

	// Scale up if size > 128
	if (size > 128)
	{
		double scale = (double)size / 128.0;
		width *= scale;
		height *= scale;
	}

	if (width > height)
	{
		// Scale down by width
		if (width > size)
		{
			double scale = (double)size / width;
			width *= scale;
			height *= scale;
		}
	}
	else
	{
		// Scale down by height
		if (height > size)
		{
			double scale = (double)size / height;
			width *= scale;
			height *= scale;
		}
	}

	// Determine draw coords
	double top  = y + ((double)size * 0.5) - (height * 0.5);
	double left = x + ((double)size * 0.5) - (width * 0.5);

	// Draw
	gl::Texture::bind(image_tex_);
	gl::setColour(ColRGBA::WHITE);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2d(left, top);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2d(left, top + height);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2d(left + width, top + height);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2d(left + width, top);
	glEnd();
}
