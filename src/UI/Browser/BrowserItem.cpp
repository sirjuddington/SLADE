
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Geometry/Rect.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace browser;

// -----------------------------------------------------------------------------
//
// BrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BrowserItem class constructor
// -----------------------------------------------------------------------------
BrowserItem::BrowserItem(const string& name, unsigned index, const string& type) :
	type_{ type },
	name_{ name },
	index_{ index }
{
}

// -----------------------------------------------------------------------------
// BrowserItem class destructor
// -----------------------------------------------------------------------------
BrowserItem::~BrowserItem() = default;

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
void BrowserItem::draw(int size, gl::draw2d::Context& dc, NameType nametype, ItemView viewtype)
{
	auto sizef = static_cast<float>(size);

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
		dc.text_alignment = gl::draw2d::Align::Center;
		dc.drawText(draw_name, { sizef * 0.5f, sizef + 4.0f });
	}
	else if (viewtype == ItemView::Tiles)
	{
		// Create text box if needed
		if (!text_box_)
			text_box_ = std::make_unique<gl::draw2d::TextBox>(fmt::format("{}\n{}", index_, name_), 144, dc.font, 16);

		Vec2f pos;
		pos.x = sizef + 8.0f;
		pos.y = (sizef - text_box_->height()) * 0.5f;
		text_box_->draw(pos, dc);
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
		static vector<Rectf> lines_box_x;
		if (lines_box_x.empty())
		{
			lines_box_x.emplace_back(0.0f, 0.0f, 0.0f, sizef);
			lines_box_x.emplace_back(0.0f, sizef, sizef, sizef);
			lines_box_x.emplace_back(sizef, sizef, sizef, 0.0f);
			lines_box_x.emplace_back(sizef, 0.0f, 0.0f, 0.0f);
			lines_box_x.emplace_back(0.0f, 0.0f, sizef, sizef);
			lines_box_x.emplace_back(0.0f, sizef, sizef, 0.0f);
		}

		dc.line_thickness = 2.0f;
		dc.colour         = ColRGBA::RED;
		dc.drawLines(lines_box_x);

		return;
	}

	// Determine texture dimensions
	auto& tex_info = gl::Texture::info(image_tex_);
	float width    = tex_info.size.x;
	float height   = tex_info.size.y;

	// Scale up if size > 128
	if (size > 128)
	{
		float scale = sizef / 128.0;
		width *= scale;
		height *= scale;
	}

	if (width > height)
	{
		// Scale down by width
		if (width > sizef)
		{
			float scale = sizef / width;
			width *= scale;
			height *= scale;
		}
	}
	else
	{
		// Scale down by height
		if (height > sizef)
		{
			float scale = sizef / height;
			width *= scale;
			height *= scale;
		}
	}

	// Determine draw coords
	float top  = sizef * 0.5 - height * 0.5;
	float left = sizef * 0.5 - width * 0.5;

	// Disable tiling for texture
	bool tiling = tex_info.tiling;
	if (tiling)
		gl::Texture::setTiling(image_tex_, false);

	// Draw
	dc.texture = image_tex_;
	dc.colour  = ColRGBA::WHITE;
	dc.drawRect({ left, top, left + width, top + height });

	// Re-enable tiling for texture if needed
	if (tiling)
		gl::Texture::setTiling(image_tex_, true);
}
