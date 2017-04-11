
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MCAnimations.cpp
 * Description: MCAnimation specialisation classes - simple
 *              animations for the map editor that handle their
 *              own tracking/updating/drawing
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
#include "MCAnimations.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapVertex.h"
#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "OpenGL/OpenGL.h"
#include "MapEditor/MapTextureManager.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, thing_overlay_square)
EXTERN_CVAR(Int, thing_drawtype)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Float, line_width)
EXTERN_CVAR(Int, halo_width)
EXTERN_CVAR(Bool, sector_selected_fill)


/*******************************************************************
 * MCASELBOXFADER CLASS FUNCTIONS
 *******************************************************************
 * Fading out animation for selection box ending
 */

/* MCASelboxFader::MCASelboxFader
 * MCASelboxFader class constructor
 *******************************************************************/
MCASelboxFader::MCASelboxFader(long start, fpoint2_t tl, fpoint2_t br) : MCAnimation(start)
{
	// Init variables
	this->tl = tl;
	this->br = br;
	fade = 1.0f;
}

/* MCASelboxFader::~MCASelboxFader
 * MCASelboxFader class destructor
 *******************************************************************/
MCASelboxFader::~MCASelboxFader()
{
}

/* MCASelboxFader::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCASelboxFader::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.006f);

	// Check if animation is finished
	if (fade < 0.0f)
		return false;
	else
		return true;
}

/* MCASelboxFader::draw
 * Draws the animation
 *******************************************************************/
void MCASelboxFader::draw()
{
	glDisable(GL_TEXTURE_2D);

	rgba_t col;

	// Outline
	col.set(ColourConfiguration::getColour("map_selbox_outline"));
	col.a *= fade;
	OpenGL::setColour(col);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();

	// Fill
	col.set(ColourConfiguration::getColour("map_selbox_fill"));
	col.a *= fade;
	OpenGL::setColour(col);
	glBegin(GL_QUADS);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();
}


/*******************************************************************
 * MCATHINGSELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for things
 */

/* MCAThingSelection::MCAThingSelection
 * MCAThingSelection class constructor
 *******************************************************************/
MCAThingSelection::MCAThingSelection(long start, double x, double y, double radius, double scale_inv, bool select) : MCAnimation(start)
{
	// Init variables
	this->x = x;
	this->y = y;
	this->radius = radius;
	this->select = select;
	this->fade = 1.0f;

	// Adjust radius
	if (!thing_overlay_square)
		this->radius += 8;
	this->radius += halo_width * scale_inv;
}

/* MCAThingSelection::~MCAThingSelection
 * MCAThingSelection class destructor
 *******************************************************************/
MCAThingSelection::~MCAThingSelection()
{
}

/* MCAThingSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCAThingSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCAThingSelection::draw
 * Draws the animation
 *******************************************************************/
void MCAThingSelection::draw()
{
	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 255*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_selection");
		col.a *= fade;
	}
	OpenGL::setColour(col);

	// Get texture if needed
	if (!thing_overlay_square)
	{
		// Get thing selection texture
		GLTexture* tex = NULL;
		if (thing_drawtype == TDT_ROUND || thing_drawtype == TDT_SPRITE)
			tex = MapEditor::textureManager().getEditorImage("thing/hilight");
		else
			tex = MapEditor::textureManager().getEditorImage("thing/square/hilight");

		if (!tex)
			return;

		// Bind the texture
		glEnable(GL_TEXTURE_2D);
		tex->bind();
	}

	// Animate radius
	double r = radius;
	if (select) r += radius*0.2*fade;

	// Draw
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(x - r, y - r);
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(x - r, y + r);
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(x + r, y + r);
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(x + r, y - r);
	glEnd();
}


/*******************************************************************
 * MCALINESELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for lines
 */

/* MCALineSelection::MCALineSelection
 * MCALineSelection class constructor
 *******************************************************************/
MCALineSelection::MCALineSelection(long start, vector<MapLine*>& lines, bool select) : MCAnimation(start)
{
	// Init variables
	this->select = select;
	this->fade = 1.0f;

	// Go through list of lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (!lines[a]) continue;

		// Add line
		this->lines.push_back(frect_t(lines[a]->x1(), lines[a]->y1(), lines[a]->x2(), lines[a]->y2()));

		// Calculate line direction tab
		fpoint2_t mid = lines[a]->getPoint(MOBJ_POINT_MID);
		fpoint2_t tab = lines[a]->dirTabPoint();

		this->tabs.push_back(frect_t(mid.x, mid.y, tab.x, tab.y));
	}
}

/* MCALineSelection::~MCALineSelection
 * MCALineSelection class destructor
 *******************************************************************/
MCALineSelection::~MCALineSelection()
{
}

/* MCALineSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCALineSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCALineSelection::draw
 * Draws the animation
 *******************************************************************/
void MCALineSelection::draw()
{
	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 255*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_selection");
		col.a *= fade;
	}
	OpenGL::setColour(col);

	// Draw lines
	glLineWidth(line_width*ColourConfiguration::getLineSelectionWidth());
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		glVertex2d(lines[a].tl.x, lines[a].tl.y);
		glVertex2d(lines[a].br.x, lines[a].br.y);
		glVertex2d(tabs[a].tl.x, tabs[a].tl.y);
		glVertex2d(tabs[a].br.x, tabs[a].br.y);
	}
	glEnd();
}


/*******************************************************************
 * MCAVERTEXSELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for vertices
 */

/* MCAVertexSelection::MCAVertexSelection
 * MCAVertexSelection class constructor
 *******************************************************************/
MCAVertexSelection::MCAVertexSelection(long start, vector<MapVertex*>& verts, double size, bool select) : MCAnimation(start)
{
	// Init variables
	this->size = size;
	this->select = select;
	this->fade = 1.0f;

	// Setup vertices list
	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (!verts[a]) continue;
		vertices.push_back(fpoint2_t(verts[a]->xPos(), verts[a]->yPos()));
	}

	if (!select)
		this->size = size * 1.8f;
}

/* MCAVertexSelection::~MCAVertexSelection
 * MCAVertexSelection class destructor
 *******************************************************************/
MCAVertexSelection::~MCAVertexSelection()
{
}

/* MCAVertexSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCAVertexSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCAVertexSelection::draw
 * Draws the animation
 *******************************************************************/
void MCAVertexSelection::draw()
{
	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 255*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_selection");
		col.a *= fade;
	}
	OpenGL::setColour(col);

	// Setup point sprites if supported
	bool point = false;
	if (OpenGL::pointSpriteSupport())
	{
		// Get appropriate vertex texture
		GLTexture* tex;
		//if (vertex_round) tex = MapEditor::textureManager().getEditorImage("vertex_r");
		//else tex = MapEditor::textureManager().getEditorImage("vertex_s");

		if (select)
		{
			if (vertex_round) tex = MapEditor::textureManager().getEditorImage("vertex/round");
			else tex = MapEditor::textureManager().getEditorImage("vertex/square");
		}
		else
		{
			if (vertex_round) tex = MapEditor::textureManager().getEditorImage("vertex/hilight_r");
			else tex = MapEditor::textureManager().getEditorImage("vertex/hilight_s");
		}

		// If it was found, enable point sprites
		if (tex)
		{
			glEnable(GL_TEXTURE_2D);
			tex->bind();
			glEnable(GL_POINT_SPRITE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
			point = true;
		}
	}

	// No point sprites, use regular points
	if (!point)
	{
		if (vertex_round)	glEnable(GL_POINT_SMOOTH);
		else				glDisable(GL_POINT_SMOOTH);
	}

	// Draw points
	if (select)
		glPointSize(size+(size*fade));
	else
		glPointSize(size);
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < vertices.size(); a++)
		glVertex2d(vertices[a].x, vertices[a].y);
	glEnd();

	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}


/*******************************************************************
 * MCASECTORSELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for sectors
 */

/* MCASectorSelection::MCASectorSelection
 * MCASectorSelection class constructor
 *******************************************************************/
MCASectorSelection::MCASectorSelection(long start, vector<Polygon2D*>& polys, bool select) : MCAnimation(start)
{
	// Init variables
	this->select = select;
	this->fade = 1.0f;

	// Copy polygon list
	for (unsigned a = 0; a < polys.size(); a++)
		this->polygons.push_back(polys[a]);
}

/* MCASectorSelection::~MCASectorSelection
 * MCASectorSelection class destructor
 *******************************************************************/
MCASectorSelection::~MCASectorSelection()
{
}

/* MCASectorSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCASectorSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCASectorSelection::draw
 * Draws the animation
 *******************************************************************/
void MCASectorSelection::draw()
{
	// Don't draw if no fill
	if (!sector_selected_fill)
		return;

	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 180*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_selection");
		col.a *= fade*0.75;
	}
	OpenGL::setColour(col);

	// Draw polygons
	for (unsigned a = 0; a < polygons.size(); a++)
		polygons[a]->render();
}


/*******************************************************************
 * MCA3DWALLSELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for 3d mode walls
 */

/* MCA3dWallSelection::MCA3dWallSelection
 * MCA3dWallSelection class constructor
 *******************************************************************/
MCA3dWallSelection::MCA3dWallSelection(long start, fpoint3_t points[4], bool select) : MCAnimation(start, true)
{
	// Init variables
	this->select = select;
	this->fade = 1.0f;
	this->points[0] = points[0];
	this->points[1] = points[1];
	this->points[2] = points[2];
	this->points[3] = points[3];
}

/* MCA3dWallSelection::~MCA3dWallSelection
 * MCA3dWallSelection class destructor
 *******************************************************************/
MCA3dWallSelection::~MCA3dWallSelection()
{
}

/* MCA3dWallSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCA3dWallSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCA3dWallSelection::draw
 * Draws the animation
 *******************************************************************/
void MCA3dWallSelection::draw()
{
	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 90*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_3d_selection");
		col.a *= fade*0.75;
	}
	OpenGL::setColour(col);

	// Draw quad outline
	glLineWidth(2.0f);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_LOOP);
	for (unsigned a = 0; a < 4; a++)
		glVertex3d(points[a].x, points[a].y, points[a].z);
	glEnd();

	// Draw quad fill
	col.a *= 0.5;
	OpenGL::setColour(col, false);
	glBegin(GL_QUADS);
	for (unsigned a = 0; a < 4; a++)
		glVertex3d(points[a].x, points[a].y, points[a].z);
	glEnd();
}


/*******************************************************************
 * MCA3DFLATSELECTION CLASS FUNCTIONS
 *******************************************************************
 * Selection/deselection animation for 3d mode flats
 */

/* MCA3dFlatSelection::MCA3dFlatSelection
 * MCA3dFlatSelection class constructor
 *******************************************************************/
MCA3dFlatSelection::MCA3dFlatSelection(long start, MapSector* sector, plane_t plane, bool select) : MCAnimation(start, true)
{
	// Init variables
	this->sector = sector;
	this->plane = plane;
	this->select = select;
	this->fade = 1.0f;
}

/* MCA3dFlatSelection::~MCA3dFlatSelection
 * MCA3dFlatSelection class destructor
 *******************************************************************/
MCA3dFlatSelection::~MCA3dFlatSelection()
{
}

/* MCA3dFlatSelection::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCA3dFlatSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade = 1.0f - ((time - starttime) * 0.004f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCA3dFlatSelection::draw
 * Draws the animation
 *******************************************************************/
void MCA3dFlatSelection::draw()
{
	if (!sector)
		return;

	// Setup colour
	rgba_t col;
	if (select)
		col.set(255, 255, 255, 60*fade, 1);
	else
	{
		col = ColourConfiguration::getColour("map_3d_selection");
		col.a *= fade*0.75*0.5;
	}
	OpenGL::setColour(col);
	glDisable(GL_CULL_FACE);

	// Set polygon to plane height
	sector->getPolygon()->setZ(plane);

	// Render flat
	sector->getPolygon()->render();

	// Reset polygon height
	sector->getPolygon()->setZ(0);

	glEnable(GL_CULL_FACE);
}


/*******************************************************************
 * MCAHILIGHTFADE CLASS FUNCTIONS
 *******************************************************************
 * Fading out animation for object hilights
 */

/* MCAHilightFade::MCAHilightFade
 * MCAHilightFade class constructor
 *******************************************************************/
MCAHilightFade::MCAHilightFade(long start, MapObject* object, MapRenderer2D* renderer, float fade_init) : MCAnimation(start)
{
	this->object = object;
	this->renderer = renderer;
	this->init_fade = fade_init;
	this->fade = fade_init;
}

/* MCAHilightFade::~MCAHilightFade
 * MCAHilightFade class destructor
 *******************************************************************/
MCAHilightFade::~MCAHilightFade()
{
}

/* MCAHilightFade::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCAHilightFade::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade = init_fade - ((time - starttime) * 0.006f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCAHilightFade::draw
 * Draws the animation
 *******************************************************************/
void MCAHilightFade::draw()
{
	switch (object->getObjType())
	{
	case MOBJ_LINE:
		renderer->renderLineHilight(object->getIndex(), fade); break;
	case MOBJ_SECTOR:
		renderer->renderFlatHilight(object->getIndex(), fade); break;
	case MOBJ_THING:
		renderer->renderThingHilight(object->getIndex(), fade); break;
	case MOBJ_VERTEX:
		renderer->renderVertexHilight(object->getIndex(), fade); break;
	default:
		break;
	}
}


/*******************************************************************
 * MCAHILIGHTFADE3D CLASS FUNCTIONS
 *******************************************************************
 * Fading out animation for 3d mode wall/flat/thing hilights
 */

/* MCAHilightFade3D::MCAHilightFade3D
 * MCAHilightFade3D class constructor
 *******************************************************************/
MCAHilightFade3D::MCAHilightFade3D(
	long start,
	int item_index,
	MapEditor::ItemType item_type,
	MapRenderer3D* renderer,
	float fade_init
) :
	MCAnimation(start, true),
	item_index{ item_index },
	item_type{ item_type },
	fade{ fade_init },
	init_fade{ fade_init },
	renderer{ renderer }
{
}

/* MCAHilightFade3D::~MCAHilightFade3D
 * MCAHilightFade3D class destructor
 *******************************************************************/
MCAHilightFade3D::~MCAHilightFade3D()
{
}

/* MCAHilightFade3D::update
 * Updates the animation based on [time] elapsed in ms
 *******************************************************************/
bool MCAHilightFade3D::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade = init_fade - ((time - starttime) * 0.006f);

	// Check if animation is finished
	if (fade < 0.0f || fade > 1.0f)
		return false;
	else
		return true;
}

/* MCAHilightFade3D::draw
 * Draws the animation
 *******************************************************************/
void MCAHilightFade3D::draw()
{
	renderer->renderHilight({ item_index, item_type }, fade);
}
