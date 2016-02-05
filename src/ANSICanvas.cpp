
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ANSICanvas.cpp
 * Description: ANSICanvas class. An OpenGL canvas that displays
 *              an ANSI screen using a VGA font
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
#include "UI/WxStuff.h"
#include "Utility/CodePages.h"
#include "ANSICanvas.h"
#include "ArchiveManager.h"

/*******************************************************************
 * CONSTANTS
 *******************************************************************/

#define NUMROWS 25
#define NUMCOLS 80

/*******************************************************************
 * ANSICANVAS CLASS FUNCTIONS
 *******************************************************************/

/* ANSICanvas::ANSICanvas
 * ANSICanvas class constructor
 *******************************************************************/
ANSICanvas::ANSICanvas(wxWindow* parent, int id)
	: OGLCanvas(parent, id)
{
	// Get the all-important font data
	Archive* res_archive = theArchiveManager->programResourceArchive();
	if (!res_archive)
		return;
	ArchiveEntry* ansi_font = res_archive->entryAtPath("vga-rom-font.16");
	if (!ansi_font || ansi_font->getSize()%256)
		return;

	fontdata = ansi_font->getData();

	// Init variables
	ansidata = NULL;
	tex_image = new GLTexture();
	char_width = 8;
	char_height = ansi_font->getSize()/256;
	width = NUMCOLS * char_width;
	height = NUMROWS * char_height;
	picdata = new uint8_t[width*height];
}

/* ANSICanvas::~ANSICanvas
 * ANSICanvas class destructor
 *******************************************************************/
ANSICanvas::~ANSICanvas()
{
	delete tex_image;
	delete[] picdata;
	// fontdata belongs to the ansi_font ArchiveEntry
	// ansidata belongs to the parent ANSIPanel
}

/* ANSICanvas::writeRGBAData
 * Converts image data into RGBA format
 *******************************************************************/
void ANSICanvas::writeRGBAData(uint8_t* dest)
{
	for (size_t i = 0; i < width*height; ++i)
	{
		size_t j = i<<2;
		rgba_t c = CodePages::ansiColor(picdata[i]);
		dest[j+0] = c.r; dest[j+1] = c.g; dest[j+2] = c.b; dest[j+3] = 0xFF;
	}
}

/* ANSICanvas::draw
 * Draws the image
 *******************************************************************/
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
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

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

/* ANSICanvas::drawImage
 * Draws the image (reloads the image as a texture each time, will
 * change this later...)
 *******************************************************************/
void ANSICanvas::drawImage()
{
	// Check image is valid
	if (!picdata)
		return;

	// Save current matrix
	glPushMatrix();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Load texture data
	uint8_t* RGBAData = new uint8_t[width*height*4];
	writeRGBAData(RGBAData);
	tex_image->loadRawData(RGBAData, width, height);

	// Determine (texture)coordinates
	double x = (double)width;
	double y = (double)height;

	// Draw the image
	OpenGL::setColour(COL_WHITE);
	tex_image->draw2d();

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
	delete[] RGBAData;
}

/* ANSICanvas::drawCharacter
 * Draws a single character. This is called from the parent ANSIPanel
 *******************************************************************/
void ANSICanvas::drawCharacter(size_t index)
{
	if (!ansidata)
		return;

	// Setup some variables
	uint8_t chara = ansidata[index<<1];		// Character
	uint8_t color = ansidata[(index<<1)+1];	// Colour
	uint8_t* pic = picdata + ((index/NUMCOLS)*width*char_height) + ((index%NUMCOLS)*char_width);	// Position on canvas to draw
	const uint8_t* fnt = fontdata + (char_height * chara);	// Position of character in font image

	// Draw character (including background)
	for (int y = 0; y < char_height; ++y)
	{
		for (int x = 0; x < char_width; ++x)
		{
			pic[x+(y*width)] = (fnt[y]&(1<<(char_width-1-x))) ? (color&15) : ((color&112)>>4);
		}
	}
}
