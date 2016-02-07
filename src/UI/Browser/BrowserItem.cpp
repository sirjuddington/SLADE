
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BrowserItem.cpp
 * Description: A class representing a single browser item. Each
 *              item has a name, index and image associated with
 *              it, and handles drawing itself.
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
#include "BrowserItem.h"
#include "BrowserWindow.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"


/*******************************************************************
 * BROWSERITEM CLASS FUNCTIONS
 *******************************************************************/

/* BrowserItem::BrowserItem
 * BrowserItem class constructor
 *******************************************************************/
BrowserItem::BrowserItem(string name, unsigned index, string type)
{
	this->name = name;
	this->index = index;
	this->type = type;
	this->image = NULL;
	this->blank = false;
	this->text_box = NULL;
}

/* BrowserItem::~BrowserItem
 * BrowserItem class destructor
 *******************************************************************/
BrowserItem::~BrowserItem()
{
	if (text_box)
		delete text_box;
}

/* BrowserItem::loadImage
 * Loads the item image (base class does nothing, must be overridden
 * by child classes to be useful at all)
 *******************************************************************/
bool BrowserItem::loadImage()
{
	return false;
}

/* BrowserItem::draw
 * Draws the item in a [size]x[size] box, keeping the correct aspect
 * ratio of it's image
 *******************************************************************/
void BrowserItem::draw(int size, int x, int y, int font, int nametype, int viewtype, rgba_t colour, bool text_shadow)
{
	// Determine item name string (for normal viewtype)
	string draw_name = "";
	if (nametype == 0)
		draw_name = name;
	else if (nametype == 1)
		draw_name = S_FMT("%d", index);

	// Truncate name if needed
	if (parent->truncateNames() && draw_name.Length() > 8)
	{
		draw_name.Truncate(8);
		draw_name += "...";
	}

	// Item name
	if (viewtype == 0)
	{
		if (text_shadow) Drawing::drawText(draw_name, x+(size*0.5+1), y+size+5, COL_BLACK, font, Drawing::ALIGN_CENTER);
		Drawing::drawText(draw_name, x+(size*0.5), y+size+4, colour, font, Drawing::ALIGN_CENTER);
	}
	else if (viewtype == 1)
	{
		// Create text box if needed
		if (!text_box)
			text_box = new TextBox(S_FMT("%d\n%s", index, name), font, 144, 16);
		
		int top = y;
		top += ((size - text_box->getHeight()) * 0.5);

		if (text_shadow)
			text_box->draw(x + size + 9, top + 1, COL_BLACK);
		text_box->draw(x + size + 8, top, colour);
	}

	// If the item is blank don't bother with the image
	if (blank)
		return;

	// Try to load image if it isn't already
	if (!image || (image && !image->isLoaded()))
		loadImage();

	// If it still isn't just draw a red box with an X
	if (!image || (image && !image->isLoaded()))
	{
		glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT);

		glColor3f(1, 0, 0);
		glDisable(GL_TEXTURE_2D);

		// Outline
		glBegin(GL_LINE_LOOP);
		glVertex2i(x, y);
		glVertex2i(x, y+size);
		glVertex2i(x+size, y+size);
		glVertex2i(x+size, y);
		glEnd();

		// X
		glBegin(GL_LINES);
		glVertex2i(x, y);
		glVertex2i(x+size, y+size);
		glVertex2i(x, y+size);
		glVertex2i(x+size, y);
		glEnd();

		glPopAttrib();

		return;
	}

	// Determine texture dimensions
	double width = image->getWidth();
	double height = image->getHeight();

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
	double top = y + ((double)size * 0.5) - (height * 0.5);
	double left = x + ((double)size * 0.5) - (width * 0.5);

	// Draw
	image->bind();
	OpenGL::setColour(COL_WHITE, false);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(left, top);
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(left, top + height);
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(left + width, top + height);
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(left + width, top);
	glEnd();
}

/* BrowserItem::clearImage
 * Clears the item image
 *******************************************************************/
void BrowserItem::clearImage()
{
	if (image) image->clear();
}
