
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    CTextureCanvas.cpp
 * Description: An OpenGL canvas that displays a composite texture
 *              (ie from doom's TEXTUREx)
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
#include "CTextureCanvas.h"
#include "Misc.h"
#include "Drawing.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
wxDEFINE_EVENT(EVT_DRAG_END, wxCommandEvent);
CVAR(Bool, tx_arc, false, CVAR_SAVE)
EXTERN_CVAR(Bool, gfx_show_border)


/*******************************************************************
 * CTEXTURECANVAS CLASS FUNCTIONS
 *******************************************************************/

/* CTextureCanvas::CTextureCanvas
 * CTextureCanvas class constructor
 *******************************************************************/
CTextureCanvas::CTextureCanvas(wxWindow* parent, int id)
	: OGLCanvas(parent, id)
{
	// Init variables
	scale = 1.0;
	hilight_patch = -1;
	draw_outside = true;
	dragging = false;
	texture = NULL;
	show_grid = false;
	blend_rgba = false;
	view_type = 0;
	tex_scale = false;

	// Bind events
	Bind(wxEVT_MOTION, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &CTextureCanvas::onMouseEvent, this);
}

/* CTextureCanvas::~CTextureCanvas
 * CTextureCanvas class destructor
 *******************************************************************/
CTextureCanvas::~CTextureCanvas()
{
	clearPatchTextures();
}

/* CTextureCanvas::selectPatch
 * Selects the patch at [index]
 *******************************************************************/
void CTextureCanvas::selectPatch(int index)
{
	// Check patch index is ok
	if (index < 0 || (unsigned) index >= texture->nPatches())
		return;

	// Select the patch
	selected_patches[index] = true;
}

/* CTextureCanvas::deSelectPatch
 * De-Selects the patch at [index]
 *******************************************************************/
void CTextureCanvas::deSelectPatch(int index)
{
	// Check patch index is ok
	if (index < 0 || (unsigned) index >= texture->nPatches())
		return;

	// De-Select the patch
	selected_patches[index] = false;
}

/* CTextureCanvas::patchSelected
 * Returns true if the patch at [index] is selected, false otherwise
 *******************************************************************/
bool CTextureCanvas::patchSelected(int index)
{
	// Check index is ok
	if (index < 0 || (unsigned)index >= texture->nPatches())
		return false;

	// Return if patch index is selected
	return selected_patches[index];
}

/* CTextureCanvas::clearTexture
 * Clears the current texture and the patch textures list
 *******************************************************************/
void CTextureCanvas::clearTexture()
{
	// Stop listening to the current texture (if it exists)
	if (texture)
		stopListening(texture);

	// Clear texture;
	texture = NULL;

	// Clear patch textures
	clearPatchTextures();

	// Reset view offset
	resetOffsets();

	// Clear patch selection
	selected_patches.clear();

	// Clear full preview
	tex_preview.clear();

	// Refresh canvas
	Refresh();
}

/* CTextureCanvas::clearPatchTextures
 * Clears the patch textures list
 *******************************************************************/
void CTextureCanvas::clearPatchTextures()
{
	for (size_t a = 0; a < patch_textures.size(); a++)
		delete patch_textures[a];
	patch_textures.clear();

	// Refresh canvas
	Refresh();
}

/* CTextureCanvas::updatePatchTextures
 * Unloads all patch textures, so they are reloaded on next draw
 *******************************************************************/
void CTextureCanvas::updatePatchTextures()
{
	// Unload single patch textures
	for (size_t a = 0; a < patch_textures.size(); a++)
		patch_textures[a]->clear();

	// Unload full preview
	tex_preview.clear();
}

/* CTextureCanvas::updateTexturePreview
 * Unloads the full preview texture, so it is reloaded on next draw
 *******************************************************************/
void CTextureCanvas::updateTexturePreview()
{
	// Unload full preview
	tex_preview.clear();
}

/* CTextureCanvas::openTexture
 * Loads a composite texture to be displayed
 *******************************************************************/
bool CTextureCanvas::openTexture(CTexture* tex, Archive* parent)
{
	// Clear the current texture
	clearTexture();

	// Set texture
	texture = tex;
	this->parent = parent;

	// Init patches
	clearPatchTextures();
	for (uint32_t a = 0; a < tex->nPatches(); a++)
	{
		// Create GL texture
		patch_textures.push_back(new GLTexture());

		// Set selection
		selected_patches.push_back(false);
	}

	// Listen to it
	listenTo(tex);

	// Redraw
	Refresh();

	return true;
}

/* CTextureCanvas::draw
 * Draws the canvas contents
 *******************************************************************/
void CTextureCanvas::draw()
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
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Pan by view offset
	glTranslated(offset.x, offset.y, 0);

	// Draw texture
	if (texture)
		drawTexture();

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}

/* CTextureCanvas::drawTexture
 * Draws the currently opened composite texture
 *******************************************************************/
void CTextureCanvas::drawTexture()
{
	// Push matrix
	glPushMatrix();

	// Calculate top-left position of texture (for glScissor, since it ignores the current translation/scale)
	double left = offset.x + (GetSize().x * 0.5) - (texture->getWidth() * 0.5 * scale);
	double top = -offset.y + (GetSize().y * 0.5) - (texture->getHeight() * 0.5 * scale);

	// Translate to middle of the canvas
	glTranslated(GetSize().x * 0.5, GetSize().y * 0.5, 0);

	// Zoom
	double yscale = (tx_arc ? scale * 1.2 : scale);
	glScaled(scale, yscale, 1);

	// Draw offset guides if needed
	drawOffsetLines();

	// Apply texture scale
	double tscalex = 1;
	double tscaley = 1;
	if (tex_scale)
	{
		tscalex = texture->getScaleX();
		if (tscalex == 0) tscalex = 1;
		tscaley = texture->getScaleY();
		if (tscaley == 0) tscaley = 1;
		glScaled(1.0 / tscalex, 1.0 / tscaley, 1);
	}

	// Translate by offsets if needed
	if (view_type == 0)
		glTranslated(texture->getWidth() * -0.5, texture->getHeight() * -0.5, 0);	// No offsets
	if (view_type >= 1)
		glTranslated(-texture->getOffsetX(), -texture->getOffsetY(), 0);			// Sprite offsets
	if (view_type == 2)
		glTranslated(-160*tscalex, -100*tscaley, 0);								// HUD offsets

	// Draw the texture border
	//if (gfx_show_border)
	drawTextureBorder();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// First, draw patches semitransparently (for anything outside the texture)
	// But only if we are drawing stuff outside the texture area
	if (draw_outside)
	{
		for (uint32_t a = 0; a < texture->nPatches(); a++)
			drawPatch(a, true);
	}

	// Reset colouring
	OpenGL::setColour(COL_WHITE);

	// If we're currently dragging, draw a 'basic' preview of the texture using opengl
	if (dragging)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(left, top, texture->getWidth() * scale, texture->getHeight() * scale);
		for (uint32_t a = 0; a < texture->nPatches(); a++)
			drawPatch(a);
		glDisable(GL_SCISSOR_TEST);
	}

	// Otherwise, draw the fully generated texture
	else
	{
		// Generate if needed
		if (!tex_preview.isLoaded())
		{
			// Determine image type
			SIType type = PALMASK;
			if (blend_rgba) type = RGBA;

			// CTexture -> temp Image -> GLTexture
			SImage temp(type);
			texture->toImage(temp, parent, &palette, blend_rgba);
			tex_preview.loadImage(&temp, &palette);
		}

		// Draw it
		tex_preview.draw2d();
	}

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Now loop through selected patches and draw selection outlines
	OpenGL::setColour(70, 210, 220, 255, BLEND_NORMAL);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.5f);
	for (size_t a = 0; a < selected_patches.size(); a++)
	{
		// Skip if not selected
		if (!selected_patches[a])
			continue;

		// Get patch
		CTPatch* patch = texture->getPatch(a);
		CTPatchEx* epatch = (CTPatchEx*)patch;

		// Check for rotation
		if (texture->isExtended() && (epatch->getRotation() == 90 || epatch->getRotation() == -90))
		{
			// Draw outline, width/height swapped
			glBegin(GL_LINE_LOOP);
			glVertex2i(patch->xOffset(), patch->yOffset());
			glVertex2i(patch->xOffset(), patch->yOffset() + (int)patch_textures[a]->getWidth());
			glVertex2i(patch->xOffset() + (int)patch_textures[a]->getHeight(), patch->yOffset() + (int)patch_textures[a]->getWidth());
			glVertex2i(patch->xOffset() + (int)patch_textures[a]->getHeight(), patch->yOffset());
			glEnd();
		}
		else
		{
			// Draw outline
			glBegin(GL_LINE_LOOP);
			glVertex2i(patch->xOffset(), patch->yOffset());
			glVertex2i(patch->xOffset(), patch->yOffset() + (int)patch_textures[a]->getHeight());
			glVertex2i(patch->xOffset() + (int)patch_textures[a]->getWidth(), patch->yOffset() + (int)patch_textures[a]->getHeight());
			glVertex2i(patch->xOffset() + (int)patch_textures[a]->getWidth(), patch->yOffset());
			glEnd();
		}
	}

	// Finally, draw a hilight outline if anything is hilighted
	if (hilight_patch >= 0)
	{
		// Set colour
		OpenGL::setColour(255, 255, 255, 150, BLEND_ADDITIVE);

		// Get patch
		CTPatch* patch = texture->getPatch(hilight_patch);
		CTPatchEx* epatch = (CTPatchEx*)patch;
		GLTexture* patch_texture = patch_textures[hilight_patch];

		// Check for rotation
		if (texture->isExtended() && (epatch->getRotation() == 90 || epatch->getRotation() == -90))
		{
			// Draw outline, width/height swapped
			glBegin(GL_LINE_LOOP);
			glVertex2i(patch->xOffset(), patch->yOffset());
			glVertex2i(patch->xOffset(), patch->yOffset() + (int)patch_texture->getWidth());
			glVertex2i(patch->xOffset() + (int)patch_texture->getHeight(), patch->yOffset() + (int)patch_texture->getWidth());
			glVertex2i(patch->xOffset() + (int)patch_texture->getHeight(), patch->yOffset());
			glEnd();
		}
		else
		{
			// Draw outline
			glBegin(GL_LINE_LOOP);
			glVertex2i(patch->xOffset(), patch->yOffset());
			glVertex2i(patch->xOffset(), patch->yOffset() + (int)patch_texture->getHeight());
			glVertex2i(patch->xOffset() + (int)patch_texture->getWidth(), patch->yOffset() + (int)patch_texture->getHeight());
			glVertex2i(patch->xOffset() + (int)patch_texture->getWidth(), patch->yOffset());
			glEnd();
		}
	}
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);

	// Pop matrix
	glPopMatrix();
}

/* CTextureCanvas::drawPatch
 * Draws the patch at index [num] in the composite texture
 *******************************************************************/
void CTextureCanvas::drawPatch(int num, bool outside)
{
	// Get patch to draw
	CTPatch* patch = texture->getPatch(num);

	// Check it exists
	if (!patch)
		return;

	// Load the patch as an opengl texture if it isn't already
	if (!patch_textures[num]->isLoaded())
	{
		SImage temp(PALMASK);
		if (texture->loadPatchImage(num, temp, parent, &palette))
		{
			// Load the image as a texture
			patch_textures[num]->loadImage(&temp, &palette);
		}
		else
			patch_textures[num]->genChequeredTexture(8, COL_RED, COL_BLACK);
	}

	// Translate to position
	glPushMatrix();
	glTranslated(patch->xOffset(), patch->yOffset(), 0);

	// Setup rendering options
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Setup extended features
	bool flipx = false;
	bool flipy = false;
	double alpha = 1.0;
	bool shade_select = true;
	rgba_t col = COL_WHITE;
	if (texture->isExtended())
	{
		// Get extended patch
		CTPatchEx* epatch = (CTPatchEx*)patch;

		// Flips
		if (epatch->flipX())
			flipx = true;
		if (epatch->flipY())
			flipy = true;

		// Rotation
		if (epatch->getRotation() == 90)
		{
			glTranslated(patch_textures[num]->getHeight(), 0, 0);
			glRotated(90, 0, 0, 1);
		}
		else if (epatch->getRotation() == 180)
		{
			glTranslated(patch_textures[num]->getWidth(), patch_textures[num]->getHeight(), 0);
			glRotated(180, 0, 0, 1);
		}
		else if (epatch->getRotation() == -90)
		{
			glTranslated(0, patch_textures[num]->getWidth(), 0);
			glRotated(-90, 0, 0, 1);
		}
	}

	// Set colour
	if (outside)
		glColor4f(0.8f, 0.2f, 0.2f, 0.3f);
	else
		glColor4f(col.fr(), col.fg(), col.fb(), alpha);

	// Draw the patch
	patch_textures[num]->draw2d(0, 0, flipx, flipy);

	glPopMatrix();
}

/* CTextureCanvas::drawTextureBorder
 * Draws a black border around the texture
 *******************************************************************/
void CTextureCanvas::drawTextureBorder()
{
	// Draw the texture border
	double ext = 0.11;
	glLineWidth(2.0f);
	OpenGL::setColour(COL_BLACK);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-ext, -ext);
	glVertex2d(-ext, texture->getHeight()+ext);
	glVertex2d(texture->getWidth()+ext, texture->getHeight()+ext);
	glVertex2d(texture->getWidth()+ext, -ext);
	glEnd();
	glLineWidth(1.0f);

	// Draw vertical ticks
	int y = 0;
	glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
	while (y <= texture->getHeight())
	{
		glBegin(GL_LINES);
		glVertex2i(-4, y);
		glVertex2i(0, y);
		glVertex2i(texture->getWidth(), y);
		glVertex2i(texture->getWidth() + 4, y);
		glEnd();

		y += 8;
	}

	// Draw horizontal ticks
	int x = 0;
	while (x <= texture->getWidth())
	{
		glBegin(GL_LINES);
		glVertex2i(x, -4);
		glVertex2i(x, 0);
		glVertex2i(x, texture->getHeight());
		glVertex2i(x, texture->getHeight() + 4);
		glEnd();

		x += 8;
	}

	// Draw grid
	if (show_grid)
	{
		// Draw inverted grid lines
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// Vertical
		y = 8;
		while (y <= texture->getHeight() - 8)
		{
			glBegin(GL_LINES);
			glVertex2i(0, y);
			glVertex2i(texture->getWidth(), y);
			glEnd();

			y += 8;
		}

		// Horizontal
		x = 8;
		while (x <= texture->getWidth() - 8)
		{
			glBegin(GL_LINES);
			glVertex2i(x, 0);
			glVertex2i(x, texture->getHeight());
			glEnd();

			x += 8;
		}


		// Darken grid lines
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

		// Vertical
		y = 8;
		while (y <= texture->getHeight() - 8)
		{
			glBegin(GL_LINES);
			glVertex2i(0, y);
			glVertex2i(texture->getWidth(), y);
			glEnd();

			y += 8;
		}

		// Horizontal
		x = 8;
		while (x <= texture->getWidth() - 8)
		{
			glBegin(GL_LINES);
			glVertex2i(x, 0);
			glVertex2i(x, texture->getHeight());
			glEnd();

			x += 8;
		}
	}
}

/* CTextureCanvas::drawOffsetLines
 * Draws the offset center lines
 *******************************************************************/
void CTextureCanvas::drawOffsetLines()
{
	if (view_type == 1)
	{
		OpenGL::setColour(COL_BLACK);

		glBegin(GL_LINES);
		glVertex2d(-9999, 0);
		glVertex2d(9999, 0);
		glVertex2d(0, -9999);
		glVertex2d(0, 9999);
		glEnd();
	}
	else if (view_type == 2)
	{
		glPushMatrix();
		glEnable(GL_LINE_SMOOTH);
		Drawing::drawHud();
		glDisable(GL_LINE_SMOOTH);
		glPopMatrix();
	}
}

/* CTextureCanvas::redraw
 * Redraws the texture, updating it if [update_texture] is true
 *******************************************************************/
void CTextureCanvas::redraw(bool update_texture)
{
	if (update_texture)
		updateTexturePreview();

	Refresh();
}

/* CTextureCanvas::screenToTextPosition
 * Convert from [x,y] from the top left of the canvas to coordinates
 * relative to the top left of the texture
 *******************************************************************/
point2_t CTextureCanvas::screenToTexPosition(int x, int y)
{
	// Check a texture is open
	if (!texture)
		return point2_t(0, 0);

	// Get texture scale
	double scalex = 1;
	double scaley = 1;
	if (tex_scale)
	{
		scalex = texture->getScaleX();
		if (scalex == 0) scalex = 1;
		scaley = texture->getScaleY();
		if (scaley == 0) scaley = 1;
	}

	// Get top-left of texture in screen coordinates (ie relative to the top-left of the canvas)
	int left = GetSize().x * 0.5 + offset.x;
	int top = GetSize().y * 0.5 + offset.y;

	// Adjust for view type
	if (view_type == 0)
	{
		// None (centered)
		left -= ((double)texture->getWidth() / scalex) * 0.5 * scale;
		top -= ((double)texture->getHeight() / scaley) * 0.5 * scale;
	}
	if (view_type >= 1)
	{
		// Sprite
		left -= ((double)texture->getOffsetX() / scalex) * scale;
		top -= ((double)texture->getOffsetY() / scaley) * scale;
	}
	if (view_type == 2)
	{
		// HUD
		left -= 160 * scale;
		top -= 100 * scale;
	}

	return point2_t(double(x - left) / scale * scalex, double(y - top) / scale * scaley);
}

/* CTextureCanvas::patchAt
 * Returns the index of the patch at [x,y] on the texture, or -1
 * if no patch is at that position
 *******************************************************************/
int CTextureCanvas::patchAt(int x, int y)
{
	// Check a texture is open
	if (!texture)
		return -1;

	// Go through texture patches backwards (ie from frontmost to back)
	for (int a = texture->nPatches() - 1; a >= 0; a--)
	{
		// Check if x,y is within patch bounds
		CTPatch* patch = texture->getPatch(a);
		if (x >= patch->xOffset() && x < patch->xOffset() + (int)patch_textures[a]->getWidth() &&
		        y >= patch->yOffset() && y < patch->yOffset() + (int)patch_textures[a]->getHeight())
		{
			return a;
		}
	}

	// No patch at x,y
	return -1;
}

/* CTextureCanvas::swapPatches
 * Swaps patches at [p1] and [p2] in the texture. Returns false if
 * either index is invalid, true otherwise
 *******************************************************************/
bool CTextureCanvas::swapPatches(size_t p1, size_t p2)
{
	// Check a texture is open
	if (!texture)
		return false;

	// Check indices
	if (p1 >= texture->nPatches() || p2 >= texture->nPatches())
		return false;

	// Swap patch gl textures
	GLTexture* temp = patch_textures[p1];
	patch_textures[p1] = patch_textures[p2];
	patch_textures[p2] = temp;

	// Swap patches in the texture itself
	return texture->swapPatches(p1, p2);
}

/* CTextureCanvas::onAnnouncement
 * Called when the texture canvas recieves an announcement from the
 * texture being displayed
 *******************************************************************/
void CTextureCanvas::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// If the announcer isn't this canvas' texture, ignore it
	if (announcer != texture)
		return;

	// Patches modified
	if (event_name == "patches_modified")
	{
		// Reload patches
		selected_patches.clear();
		clearPatchTextures();
		hilight_patch = -1;
		for (uint32_t a = 0; a < texture->nPatches(); a++)
		{
			// Create GL texture
			patch_textures.push_back(new GLTexture());

			// Set selection
			selected_patches.push_back(false);
		}

		redraw(true);
	}
}

/* CTextureCanvas::onMouseEvent
 * Called when and mouse event is generated (movement/clicking/etc)
 *******************************************************************/
void CTextureCanvas::onMouseEvent(wxMouseEvent& e)
{
	bool refresh = false;

	// MOUSE MOVEMENT
	if (e.Moving() || e.Dragging())
	{
		dragging = false;

		// Pan if middle button is down
		if (e.MiddleIsDown())
		{
			offset = offset + point2_t(e.GetPosition().x - mouse_prev.x, e.GetPosition().y - mouse_prev.y);
			refresh = true;
			dragging = true;
		}
		else if (e.LeftIsDown())
			dragging = true;

		// Check if patch hilight changes
		point2_t pos = screenToTexPosition(e.GetX(), e.GetY());
		int patch = patchAt(pos.x, pos.y);
		if (hilight_patch != patch)
		{
			hilight_patch = patch;
			refresh = true;
		}
	}

	// LEFT BUTTON UP
	else if (e.LeftUp())
	{
		// If we were dragging, generate end drag event
		if (dragging)
		{
			dragging = false;
			updateTexturePreview();
			refresh = true;
			wxCommandEvent evt(EVT_DRAG_END, GetId());
			evt.SetInt(wxMOUSE_BTN_LEFT);
			ProcessWindowEvent(evt);
		}
	}

	// LEAVING
	if (e.Leaving())
	{
		// Set no hilighted patch
		hilight_patch = -1;
		refresh = true;
	}

	// Refresh is needed
	if (refresh)
		Refresh();

	// Update 'previous' mouse coordinates
	mouse_prev.set(e.GetPosition().x, e.GetPosition().y);
}
