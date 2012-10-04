
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PaletteCanvas.cpp
 * Description: PaletteCanvas class. An OpenGL canvas that displays
 *              a palette (256 colours max)
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
#include "WxStuff.h"
#include "PaletteCanvas.h"


/*******************************************************************
 * PALETTECANVAS CLASS FUNCTIONS
 *******************************************************************/

/* PaletteCanvas::PaletteCanvas
 * PaletteCanvas class constructor
 *******************************************************************/
PaletteCanvas::PaletteCanvas(wxWindow* parent, int id)
: OGLCanvas(parent, id) {
	sel_begin = -1;
	sel_end = -1;
	double_width = false;
	allow_selection = 0;

	// Bind events
	Bind(wxEVT_LEFT_DOWN,  &PaletteCanvas::onMouseLeftDown,  this);
	Bind(wxEVT_RIGHT_DOWN, &PaletteCanvas::onMouseRightDown, this);
	Bind(wxEVT_MOTION, &PaletteCanvas::onMouseMotion, this);
}

/* PaletteCanvas::~PaletteCanvas
 * PaletteCanvas class destructor
 *******************************************************************/
PaletteCanvas::~PaletteCanvas() {
}

/* PaletteCanvas::draw
 * Draws the palette as 16x16 (or 32x8) coloured squares
 *******************************************************************/
void PaletteCanvas::draw() {
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Setup some variables
	int rows = 16;
	int cols = 16;
	if (double_width) {
		rows = 8;
		cols = 32;
	}
	int x_size = (GetSize().x) / cols;
	int y_size = (GetSize().y) / rows;
	int size = MIN(x_size, y_size);

	// Draw palette
	int c = 0;
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			// Set colour
			palette.colour(c).set_gl();

			// Draw square
			glBegin(GL_QUADS);
			glVertex2d(x*size+1, y*size+1);
			glVertex2d(x*size+1, y*size+size-1);
			glVertex2d(x*size+size-1, y*size+size-1);
			glVertex2d(x*size+size-1, y*size+1);
			glEnd();

			// Draw selection outline if needed
			if (c >= sel_begin && c <= sel_end) {
				COL_WHITE.set_gl();
				glBegin(GL_LINES);
				glVertex2d(x*size, y*size);
				glVertex2d(x*size+size, y*size);
				glVertex2d(x*size, y*size+size-1);
				glVertex2d(x*size+size, y*size+size-1);
				glEnd();

				COL_BLACK.set_gl();
				glBegin(GL_LINES);
				glVertex2d(x*size+1, y*size+1);
				glVertex2d(x*size+size-1, y*size+1);
				glVertex2d(x*size+1, y*size+size-2);
				glVertex2d(x*size+size-1, y*size+size-2);
				glEnd();

				// Selection beginning
				if (c == sel_begin) {
					COL_WHITE.set_gl();
					glBegin(GL_LINES);
					glVertex2d(x*size, y*size);
					glVertex2d(x*size, y*size+size);
					glEnd();

					COL_BLACK.set_gl();
					glBegin(GL_LINES);
					glVertex2d(x*size+1, y*size+1);
					glVertex2d(x*size+1, y*size+size-1);
					glEnd();
				}

				// Selection ending
				if (c == sel_end) {
					COL_WHITE.set_gl();
					glBegin(GL_LINES);
					glVertex2d(x*size+size-1, y*size+size-2);
					glVertex2d(x*size+size-1, y*size);
					glEnd();

					COL_BLACK.set_gl();
					glBegin(GL_LINES);
					glVertex2d(x*size+size-2, y*size+1);
					glVertex2d(x*size+size-2, y*size+size-1);
					glEnd();
				}
			}

			// Next colour
			c++;

			if (c > 255)
				break;
		}

		if (c > 255)
			break;
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}

/* PaletteCanvas::getSelectedColour
 * Returns the currently selected colour, or a completely black +
 * transparent colour if nothing is selected
 *******************************************************************/
rgba_t PaletteCanvas::getSelectedColour() {
	if (sel_begin >= 0)
		return palette.colour(sel_begin);
	else
		return rgba_t(0, 0, 0, 0);
}

/* PaletteCanvas::setSelection
 * Sets the selection range. If [end] is -1 the range is set to a
 * single index
 *******************************************************************/
void PaletteCanvas::setSelection(int begin, int end) {
	sel_begin = begin;
	if (end == -1)
		sel_end = begin;
	else
		sel_end = end;
}


/*******************************************************************
 * PALETTECANVAS EVENTS
 *******************************************************************/

/* PaletteCanvas::onMouseLeftDown
 * Called when the palette canvas is left clicked
 *******************************************************************/
void PaletteCanvas::onMouseLeftDown(wxMouseEvent& e) {
	// Handle selection if needed
	if (allow_selection > 0) {
		// Figure out what 'grid' position was clicked
		int rows = 16;
		int cols = 16;
		if (double_width) {
			rows = 8;
			cols = 32;
		}
		int x_size = (GetSize().x) / cols;
		int y_size = (GetSize().y) / rows;
		int size = MIN(x_size, y_size);
		int x = e.GetX() / size;
		int y = e.GetY() / size;

		// If it was within the palette box, select the cell
		if (x >= 0 && x < cols && y >= 0 && y < rows)
			setSelection(y * cols + x);
		else
			setSelection(-1);

		// Redraw
		Refresh();
	}

	// Do normal left click stuff
	e.Skip();
}

/* PaletteCanvas::onMouseRightDown
 * Called when the palette canvas is right clicked
 *******************************************************************/
void PaletteCanvas::onMouseRightDown(wxMouseEvent& e) {
	// Do normal right click stuff
	e.Skip();
}

/* PaletteCanvas::onMouseMotion
 * Called when the mouse cursor is moved over the palette canvas
 *******************************************************************/
void PaletteCanvas::onMouseMotion(wxMouseEvent& e) {
	// Check for dragging selection
	if (e.LeftIsDown() && allow_selection > 1) {
		// Figure out what 'grid' position the cursor is over
		int rows = 16;
		int cols = 16;
		if (double_width) {
			rows = 8;
			cols = 32;
		}
		int x_size = (GetSize().x) / cols;
		int y_size = (GetSize().y) / rows;
		int size = MIN(x_size, y_size);
		int x = e.GetX() / size;
		int y = e.GetY() / size;

		// Set selection accordingly
		if (x >= 0 && x < cols && y >= 0 && y < rows) {
			int sel = y * cols + x;
			if (sel > sel_begin)
				setSelection(sel_begin, sel);

			Refresh();
		}
	}
}
