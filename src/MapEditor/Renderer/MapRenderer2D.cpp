
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapRenderer2D.cpp
 * Description: MapRenderer2D class - handles all rendering related
 *              stuff for the map in 2d
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
#include "MapRenderer2D.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditorWindow.h"
#include "OpenGL/GLTexture.h"
#include "Utility/Polygon2D.h"
#include "MapEditor/ObjectEdit.h"
#include "OpenGL/OpenGL.h"
#include "OpenGL/Drawing.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, vertex_round, true, CVAR_SAVE)
CVAR(Int, vertex_size, 7, CVAR_SAVE)
CVAR(Float, line_width, 1.5f, CVAR_SAVE)
CVAR(Bool, line_smooth, true, CVAR_SAVE)
CVAR(Int, thing_drawtype, 1, CVAR_SAVE)
CVAR(Bool, thing_force_dir, false, CVAR_SAVE)
CVAR(Bool, thing_overlay_square, false, CVAR_SAVE)
CVAR(Float, flat_brightness, 0.8f, CVAR_SAVE)
CVAR(Bool, flat_ignore_light, false, CVAR_SAVE)
CVAR(Float, thing_shadow, 0.5f, CVAR_SAVE)
CVAR(Bool, sector_hilight_fill, true, CVAR_SAVE)
CVAR(Bool, sector_selected_fill, true, CVAR_SAVE)
CVAR(Bool, map_animate_hilight, true, CVAR_SAVE)
CVAR(Bool, map_animate_selection, false, CVAR_SAVE)
CVAR(Bool, map_animate_tagged, true, CVAR_SAVE)
CVAR(Float, arrow_alpha, 1.0f, CVAR_SAVE)
CVAR(Bool, arrow_colour, false, CVAR_SAVE)
CVAR(Bool, flats_use_vbo, true, CVAR_SAVE)
CVAR(Int, halo_width, 5, CVAR_SAVE)
CVAR(Float, arrowhead_angle, 0.7854f, CVAR_SAVE)
CVAR(Float, arrowhead_length, 25.f, CVAR_SAVE)
CVAR(Bool, action_lines, true, CVAR_SAVE)
CVAR(String, arrow_pathed_color, "#22FFFF", CVAR_SAVE)
CVAR(String, arrow_dragon_color, "#FF2222", CVAR_SAVE)

// Texture coordinates for rendering square things (since we can't just rotate these)
float sq_thing_tc[] = { 0.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f,
						1.0f, 1.0f
					  };

CVAR(Bool, test_ssplit, false, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, use_zeth_icons)


/*******************************************************************
 * MAPRENDERER2D CLASS FUNCTIONS
 *******************************************************************/

/* MapRenderer2D::MapRenderer2D
 * MapRenderer2D class constructor
 *******************************************************************/
MapRenderer2D::MapRenderer2D(SLADEMap* map)
{
	// Init variables
	this->map = map;
	this->vbo_vertices = 0;
	this->vbo_lines = 0;
	this->vbo_flats = 0;
	this->list_vertices = 0;
	this->list_lines = 0;
	this->lines_dirs = false;
	this->n_vertices = 0;
	this->n_lines = 0;
	this->n_things = 0;
}

/* MapRenderer2D::~MapRenderer2D
 * MapRenderer2D class destructor
 *******************************************************************/
MapRenderer2D::~MapRenderer2D()
{
	if (vbo_vertices > 0)		glDeleteBuffers(1, &vbo_vertices);
	if (vbo_lines > 0)			glDeleteBuffers(1, &vbo_lines);
	if (vbo_flats > 0)			glDeleteBuffers(1, &vbo_flats);
	if (list_vertices > 0)		glDeleteLists(list_vertices, 1);
	if (list_lines > 0)			glDeleteLists(list_lines, 1);
}

/* MapRenderer2D::setupVertexRendering
 * Sets up the renderer for vertices (point sprites, etc.). If
 * [overlay] is true, use the point sprite for hilight/selection/etc
 *******************************************************************/
bool MapRenderer2D::setupVertexRendering(float size_scale, bool overlay)
{
	// Setup rendering properties
	float vs = vertex_size*size_scale;
	if (view_scale < 1.0) vs *= view_scale;
	if (vs < 2.0) vs = 2.0;
	glPointSize(vs);

	// Setup point sprites if supported
	bool point = false;
	if (OpenGL::pointSpriteSupport())
	{
		// Get appropriate vertex texture
		GLTexture* tex;
		if (overlay)
		{
			if (vertex_round) tex = theMapEditor->textureManager().getEditorImage("vertex/hilight_r");
			else tex = theMapEditor->textureManager().getEditorImage("vertex/hilight_s");
		}
		else
		{
			if (vertex_round) tex = theMapEditor->textureManager().getEditorImage("vertex/round");
			else tex = theMapEditor->textureManager().getEditorImage("vertex/square");
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

	return point;
}

/* MapRenderer2D::renderVertices
 * Renders map vertices
 *******************************************************************/
void MapRenderer2D::renderVertices(float alpha)
{
	// Check there are any vertices to render
	if (map->nVertices() == 0)
		return;

	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	// Setup rendering properties
	bool point = setupVertexRendering(1.0f);

	// Set to vertex colour
	rgba_t col = ColourConfiguration::getColour("map_vertex");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(col.fr(), col.fg(), col.fb(), col.fa()*alpha);

	// Render the vertices depending on what features are supported
	if (OpenGL::vboSupport())
		renderVerticesVBO();
	else
		renderVerticesImmediate();

	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::renderVerticesImmediate
 * Renders vertices in immediate mode (slower, but required for old
 * video cards)
 *******************************************************************/
void MapRenderer2D::renderVerticesImmediate()
{
	if (list_vertices > 0 && map->nVertices() == n_vertices && map->geometryUpdated() <= vertices_updated)
		glCallList(list_vertices);
	else
	{
		list_vertices = glGenLists(1);
		glNewList(list_vertices, GL_COMPILE_AND_EXECUTE);

		// Draw all vertices
		glBegin(GL_POINTS);
		for (unsigned a = 0; a < map->nVertices(); a++)
			glVertex2d(map->getVertex(a)->xPos(), map->getVertex(a)->yPos());
		glEnd();

		glEndList();

		vertices_updated = theApp->runTimer();
	}
}

/* MapRenderer2D::renderVerticesVBO
 * Renders vertices using an OpenGL Vertex Buffer Object
 *******************************************************************/
void MapRenderer2D::renderVerticesVBO()
{
	// Do nothing if there are no vertices in the map
	if (map->nVertices() == 0)
		return;

	// Update vertices VBO if required
	if (vbo_vertices == 0 || map->nVertices() != n_vertices || map->geometryUpdated() > vertices_updated)
		updateVerticesVBO();

	// Set VBO arrays to use
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Setup VBO pointers
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glVertexPointer(2, GL_FLOAT, 0, 0);

	// Render the VBO
	glDrawArrays(GL_POINTS, 0, map->nVertices());

	// Cleanup state
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/* MapRenderer2D::renderVertexHilight
 * Renders the vertex hilight overlay for vertex [index]
 *******************************************************************/
void MapRenderer2D::renderVertexHilight(int index, float fade)
{
	// Check hilight
	if (!map->getVertex(index))
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_hilight");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	bool point = setupVertexRendering(1.8f + (0.6f * fade), true);

	// Draw vertex
	glBegin(GL_POINTS);
	glVertex2d(map->getVertex(index)->xPos(),
			   map->getVertex(index)->yPos());
	glEnd();

	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::renderVertexSelection
 * Renders the vertex selection overlay for vertex indices in
 * [selection]
 *******************************************************************/
void MapRenderer2D::renderVertexSelection(vector<int>& selection, float fade)
{
	// Check anything is selected
	if (selection.size() == 0)
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Set selection colour
	rgba_t col = ColourConfiguration::getColour("map_selection");
	col.a = 255;//*= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	bool point = setupVertexRendering(1.8f, true);

	// Draw selected vertices
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < selection.size(); a++)
		glVertex2d(map->getVertex(selection[a])->xPos(), map->getVertex(selection[a])->yPos());
	glEnd();

	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::lineColour
 * Returns the colour for [line]
 *******************************************************************/
rgba_t MapRenderer2D::lineColour(MapLine* line, bool ignore_filter)
{
	rgba_t col;

	if (line)
	{
		// Check for special line
		if (line->intProperty("special") > 0)
			col.set(ColourConfiguration::getColour("map_line_special"));
		else if (line->s1())
			col.set(ColourConfiguration::getColour("map_line_normal"));
		else
			col.set(ColourConfiguration::getColour("map_line_invalid"));

		// Check for two-sided line
		if (line->s2())
			col.a *= 0.5f;

		// Check if filtered
		if (line->isFiltered() && !ignore_filter)
			col.a *= 0.25f;
	}

	return col;
}

/* MapRenderer2D::renderLines
 * Renders map lines, with direction tabs if [show_direction] is true
 *******************************************************************/
void MapRenderer2D::renderLines(bool show_direction, float alpha)
{
	// Check there are any lines to render
	if (map->nLines() == 0)
		return;

	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	// Setup rendering properties
	glLineWidth(line_width);
	if (line_smooth)
		glEnable(GL_LINE_SMOOTH);
	else
		glDisable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render the lines depending on what features are supported
	if (OpenGL::vboSupport())
		renderLinesVBO(show_direction, alpha);
	else
		renderLinesImmediate(show_direction, alpha);
}

/* MapRenderer2D::renderLinesImmediate
 * Renders map lines in immediate mode
 *******************************************************************/
void MapRenderer2D::renderLinesImmediate(bool show_direction, float alpha)
{
	// Use display list if it's built
	if (list_lines > 0 &&
		show_direction == lines_dirs &&
		map->nLines() == n_lines &&
		map->geometryUpdated() <= lines_updated &&
		!map->modifiedSince(lines_updated, MOBJ_LINE))
	{
		glCallList(list_lines);
		return;
	}

	// Rebuild display list if we're switching from simple lines to directional lines
	if (list_lines > 0)
		glDeleteLists(list_lines, 1);

	// Create display list
	list_lines = glGenLists(1);
	glNewList(list_lines, GL_COMPILE_AND_EXECUTE);

	// Draw all lines
	rgba_t col;
	MapLine* line = NULL;
	double x1, y1, x2, y2;
	glBegin(GL_LINES);
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		// Get line info
		line = map->getLine(a);
		x1 = line->v1()->xPos();
		y1 = line->v1()->yPos();
		x2 = line->v2()->xPos();
		y2 = line->v2()->yPos();

		// Get line colour
		col = lineColour(line);

		// Set line colour
		glColor4f(col.fr(), col.fg(), col.fb(), alpha*col.fa());

		// Draw the line
		glVertex2d(x1, y1);
		glVertex2d(x2, y2);

		// Direction tab
		if (show_direction)
		{
			fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
			fpoint2_t tab = line->dirTabPoint();
			glVertex2d(mid.x, mid.y);
			glVertex2d(tab.x, tab.y);
		}
	}
	glEnd();

	glEndList();
	lines_dirs = show_direction;
	lines_updated = theApp->runTimer();
}

/* MapRenderer2D::renderLinesVBO
 * Renders map lines using an OpenGL Vertex Buffer Object
 *******************************************************************/
void MapRenderer2D::renderLinesVBO(bool show_direction, float alpha)
{
	// Do nothing if there are no lines in the map
	if (map->nLines() == 0)
		return;

	// Update lines VBO if required
	if (vbo_lines == 0 ||
		show_direction != lines_dirs ||
		map->nLines() != n_lines ||
		map->geometryUpdated() > lines_updated ||
		map->modifiedSince(lines_updated, MOBJ_LINE))
		updateLinesVBO(show_direction, alpha);

	// Disable any blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set VBO arrays to use
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Setup VBO pointers
	glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
	glVertexPointer(2, GL_FLOAT, 24, 0);

	glColorPointer(4, GL_FLOAT, 24, ((char*)NULL + 8));

	// Render the VBO
	if (show_direction)
		glDrawArrays(GL_LINES, 0, map->nLines()*4);
	else
		glDrawArrays(GL_LINES, 0, map->nLines()*2);

	// Clean state
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	lines_dirs = show_direction;
}

/* MapRenderer2D::renderLineHilight
 * Renders the line hilight overlay for line [index]
 *******************************************************************/
void MapRenderer2D::renderLineHilight(int index, float fade)
{
	// Check hilight
	if (!map->getLine(index))
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_hilight");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	glLineWidth(line_width*ColourConfiguration::getLineHilightWidth());

	// Render line
	MapLine* line = map->getLine(index);
	double x1 = line->v1()->xPos();
	double y1 = line->v1()->yPos();
	double x2 = line->v2()->xPos();
	double y2 = line->v2()->yPos();
	glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glEnd();

	// Direction tab
	fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
	fpoint2_t tab = line->dirTabPoint();
	glBegin(GL_LINES);
	glVertex2d(mid.x, mid.y);
	glVertex2d(tab.x, tab.y);
	glEnd();
}

/* MapRenderer2D::renderLineSelection
 * Renders the line selection overlay for line indices in [selection]
 *******************************************************************/
void MapRenderer2D::renderLineSelection(vector<int>& selection, float fade)
{
	// Check anything is selected
	if (selection.size() == 0)
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Set selection colour
	rgba_t col = ColourConfiguration::getColour("map_selection");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	glLineWidth(line_width*ColourConfiguration::getLineSelectionWidth());

	// Render selected lines
	MapLine* line;
	double x1, y1, x2, y2;
	glBegin(GL_LINES);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Get line properties
		line = map->getLine(selection[a]);
		x1 = line->v1()->xPos();
		y1 = line->v1()->yPos();
		x2 = line->v2()->xPos();
		y2 = line->v2()->yPos();

		// Draw line
		glVertex2d(x1, y1);
		glVertex2d(x2, y2);

		// Direction tab
		fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
		fpoint2_t tab = line->dirTabPoint();
		glVertex2d(mid.x, mid.y);
		glVertex2d(tab.x, tab.y);
	}
	glEnd();
}

/* MapRenderer2D::renderTaggedLines
 * Renders the tagged line overlay for lines in [lines]
 *******************************************************************/
void MapRenderer2D::renderTaggedLines(vector<MapLine*>& lines, float fade)
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_tagged");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	glLineWidth(line_width*ColourConfiguration::getLineHilightWidth());

	// Go through tagged lines
	double x1, y1, x2, y2;
	MapObject* object = theMapEditor->mapEditor().getHilightedObject();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Render line
		MapLine* line = lines[a];
		x1 = line->v1()->xPos();
		y1 = line->v1()->yPos();
		x2 = line->v2()->xPos();
		y2 = line->v2()->yPos();
		glBegin(GL_LINES);
		glVertex2d(x1, y1);
		glVertex2d(x2, y2);
		glEnd();

		// Direction tab
		fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
		fpoint2_t tab = line->dirTabPoint();
		glBegin(GL_LINES);
		glVertex2d(mid.x, mid.y);
		glVertex2d(tab.x, tab.y);
		glEnd();

		// Action lines
		if (object && action_lines)
		{
			glLineWidth(line_width*1.5f);
			Drawing::drawArrow(line->getPoint(MOBJ_POINT_WITHIN), object->getPoint(MOBJ_POINT_WITHIN), col, false, arrowhead_angle, arrowhead_length);
			glLineWidth(line_width*3);
		}
	}
}

/* MapRenderer2D::renderTaggingLines
 * Renders the tagging line overlay for lines in [lines]
 *******************************************************************/
void MapRenderer2D::renderTaggingLines(vector<MapLine*>& lines, float fade)
{
	// Reset fade if tagging animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_tagging");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup rendering properties
	glLineWidth(line_width*ColourConfiguration::getLineHilightWidth());

	// Go through tagging lines
	double x1, y1, x2, y2;
	MapObject* object = theMapEditor->mapEditor().getHilightedObject();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Render line
		MapLine* line = lines[a];
		x1 = line->v1()->xPos();
		y1 = line->v1()->yPos();
		x2 = line->v2()->xPos();
		y2 = line->v2()->yPos();
		glBegin(GL_LINES);
		glVertex2d(x1, y1);
		glVertex2d(x2, y2);
		glEnd();

		// Direction tab
		fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
		fpoint2_t tab = line->dirTabPoint();
		glBegin(GL_LINES);
		glVertex2d(mid.x, mid.y);
		glVertex2d(tab.x, tab.y);
		glEnd();

		// Action lines
		if (object && action_lines)
		{
			glLineWidth(line_width*1.5f);
			Drawing::drawArrow(object->getPoint(MOBJ_POINT_WITHIN), line->getPoint(MOBJ_POINT_WITHIN), col, false, arrowhead_angle, arrowhead_length);
			glLineWidth(line_width*5);
		}
	}
}

/* MapRenderer2D::setupThingOverlay
 * Sets up the renderer for thing overlays (point sprites, etc.)
 *******************************************************************/
bool MapRenderer2D::setupThingOverlay()
{
	// Get hilight texture
	GLTexture* tex = theMapEditor->textureManager().getEditorImage("thing/hilight");
	if (thing_drawtype == TDT_SQUARE || thing_drawtype == TDT_SQUARESPRITE || thing_drawtype == TDT_FRAMEDSPRITE)
		tex = theMapEditor->textureManager().getEditorImage("thing/square/hilight");

	// Nothing to do if thing_overlay_square is true and thing_drawtype is 1 or 2 (circles or sprites)
	// or if the hilight circle texture isn't found for some reason
	if (!tex || (thing_overlay_square && (thing_drawtype == TDT_ROUND || thing_drawtype == TDT_SPRITE)))
	{
		glDisable(GL_TEXTURE_2D);
		return false;
	}

	// Otherwise, we want the textured selection overlay
	glEnable(GL_TEXTURE_2D);
	tex->bind();

	// Setup point sprites if supported
	bool point = false;
	if (OpenGL::pointSpriteSupport())
	{
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		point = true;
	}

	return point;
}

/* MapRenderer2D::renderThingOverlay
 * Renders a thing overlay at [x,y] of size [radius]
 *******************************************************************/
void MapRenderer2D::renderThingOverlay(double x, double y, double radius, bool point)
{
	// Simplest case, thing_overlay_square is true and thing_drawtype is 1 or 2 (circles or sprites)
	if (thing_overlay_square && (thing_drawtype == TDT_ROUND || thing_drawtype == TDT_SPRITE))
	{
		// Draw square
		glBegin(GL_QUADS);
		glVertex2d(x - radius, y - radius);
		glVertex2d(x - radius, y + radius);
		glVertex2d(x + radius, y + radius);
		glVertex2d(x + radius, y - radius);
		glEnd();

		return;
	}

	// Otherwise, we want the circle selection overlay
	double ps = radius*2*view_scale;

	// Draw it
	if (point && ps <= OpenGL::maxPointSize())
	{
		// Point sprite
		glPointSize(ps);
		glBegin(GL_POINTS);
		glVertex2d(x, y);
		glEnd();
	}
	else
	{
		// Textured quad
		if (point) glDisable(GL_POINT_SPRITE);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex2d(x - radius, y - radius);
		glTexCoord2f(0.0f, 1.0f);	glVertex2d(x - radius, y + radius);
		glTexCoord2f(1.0f, 1.0f);	glVertex2d(x + radius, y + radius);
		glTexCoord2f(1.0f, 0.0f);	glVertex2d(x + radius, y - radius);
		glEnd();
		if (point) glEnable(GL_POINT_SPRITE);
	}
}

/* MapRenderer2D::renderRoundThing
 * Renders a round thing icon at [x,y]
 *******************************************************************/
void MapRenderer2D::renderRoundThing(double x, double y, double angle, ThingType* tt, float alpha, double radius_mult)
{
	// Ignore if no type given (shouldn't happen)
	if (!tt)
		return;

	// --- Determine texture to use ---
	GLTexture* tex = NULL;
	bool rotate = false;

	// Set colour
	glColor4f(tt->getColour().fr(), tt->getColour().fg(), tt->getColour().fb(), alpha);

	// Check for custom thing icon
	if (!tt->getIcon().IsEmpty() && !thing_force_dir && !things_angles)
	{
		if (use_zeth_icons && tt->getZeth() >= 0)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", tt->getZeth()));
		if (!tex)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", tt->getIcon()));
	}

	if (!tex)
	{
		// Otherwise, normal thing image

		// Check if we want an angle indicator
		if (tt->isAngled() || thing_force_dir || things_angles)
		{
			if (angle != 0) rotate = true;	// Also rotate to angle
			tex = theMapEditor->textureManager().getEditorImage("thing/normal_d");
		}
		else
			tex = theMapEditor->textureManager().getEditorImage("thing/normal_n");
	}

	// If for whatever reason the thing texture doesn't exist, just draw a basic, square thing
	if (!tex)
	{
		renderSimpleSquareThing(x, y, angle, tt, alpha);
		return;
	}

	// Bind texture
	if (tex_last != tex)
	{
		tex->bind();
		tex_last = tex;
	}

	// Rotate if needed
	if (rotate)
	{
		glPushMatrix();
		glTranslated(x, y, 0);
		glRotated(angle, 0, 0, 1);
		x = y = 0;
	}

	// Draw thing
	double radius = tt->getRadius() * radius_mult;
	if (tt->shrinkOnZoom()) radius = scaledRadius(radius);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(x-radius, y-radius);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(x-radius, y+radius);
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(x+radius, y+radius);
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(x+radius, y-radius);
	glEnd();

	// Restore previous matrix if rotated
	if (rotate)
		glPopMatrix();
}

/* MapRenderer2D::renderSpriteThing
 * Renders a sprite thing icon at [x,y]. If [fitradius] is true, the
 * sprite is drawn to fit within the thing's radius
 *******************************************************************/
bool MapRenderer2D::renderSpriteThing(double x, double y, double angle, ThingType* tt, unsigned index, float alpha, bool fitradius)
{
	// Ignore if no type given (shouldn't happen)
	if (!tt)
		return false;

	// Refresh sprites list if needed
	if (thing_sprites.size() != map->nThings())
	{
		thing_sprites.clear();
		for (unsigned a = 0; a < map->nThings(); a++)
			thing_sprites.push_back(NULL);
	}

	// --- Determine texture to use ---
	bool show_angle = false;
	GLTexture* tex = index < thing_sprites.size() ? thing_sprites[index] : NULL;

	// Attempt to get sprite texture
	if (!tex)
	{
		tex = theMapEditor->textureManager().getSprite(tt->getSprite(), tt->getTranslation(), tt->getPalette());

		if (index < thing_sprites.size())
		{
			thing_sprites[index] = tex;
			thing_sprites_updated = theApp->runTimer();
		}
	}

	// If sprite not found, just draw as a normal, round thing
	if (!tex)
	{
		if (thing_drawtype == TDT_FRAMEDSPRITE)
			renderRoundThing(x, y, angle, tt, alpha, 0.7);
		else
			renderRoundThing(x, y, angle, tt, alpha);
		return false;
	}

	// Check if we have to draw the angle arrow later
	if (tt->isAngled() || thing_force_dir || things_angles)
		show_angle = true;

	//// If for whatever reason the thing texture doesn't exist, just draw a basic, square thing
	//if (!tex)
	//{
	//	renderSquareThing(x, y, angle, tt, alpha);
	//	return false;
	//}

	// Bind texture
	if (tex_last != tex)
	{
		tex->bind();
		tex_last = tex;
	}

	// Draw thing
	double hw = tex->getWidth()*0.5;
	double hh = tex->getHeight()*0.5;

	// Fit to radius if needed
	if (fitradius)
	{
		double scale = ((double)tt->getRadius()*0.8) / max(hw, hh);
		hw *= scale;
		hh *= scale;
	}

	// Shadow if needed
	if (thing_shadow > 0.01f && alpha >= 0.9 && !fitradius)
	{
		double sz = (min(hw, hh))*0.1;
		if (sz < 1) sz = 1;
		glColor4f(0.0f, 0.0f, 0.0f, alpha*(thing_shadow*0.7));
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);	glVertex2d(x-hw-sz, y-hh-sz);
		glTexCoord2f(0.0f, 0.0f);	glVertex2d(x-hw-sz, y+hh+sz);
		glTexCoord2f(1.0f, 0.0f);	glVertex2d(x+hw+sz, y+hh+sz);
		glTexCoord2f(1.0f, 1.0f);	glVertex2d(x+hw+sz, y-hh-sz);
		glEnd();
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);	glVertex2d(x-hw-sz, y-hh-sz-sz);
		glTexCoord2f(0.0f, 0.0f);	glVertex2d(x-hw-sz, y+hh+sz);
		glTexCoord2f(1.0f, 0.0f);	glVertex2d(x+hw+sz+sz, y+hh+sz);
		glTexCoord2f(1.0f, 1.0f);	glVertex2d(x+hw+sz+sz, y-hh-sz-sz);
		glEnd();
	}
	// Draw thing
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(x-hw, y-hh);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(x-hw, y+hh);
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(x+hw, y+hh);
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(x+hw, y-hh);
	glEnd();


	return show_angle;
}

/* MapRenderer2D::renderSquareThing
 * Renders a square thing icon at [x,y]
 *******************************************************************/
bool MapRenderer2D::renderSquareThing(double x, double y, double angle, ThingType* tt, float alpha, bool showicon, bool framed)
{
	// Ignore if no type given (shouldn't happen)
	if (!tt)
		return false;

	// --- Determine texture to use ---
	GLTexture* tex = NULL;

	// Set colour
	glColor4f(tt->getColour().fr(), tt->getColour().fg(), tt->getColour().fb(), alpha);

	// Show icon anyway if no sprite set
	if (tt->getSprite().IsEmpty())
		showicon = true;

	// Check for custom thing icon
	if (!tt->getIcon().IsEmpty() && showicon && !thing_force_dir && !things_angles && !framed)
		tex = theMapEditor->textureManager().getEditorImage(S_FMT("thing/square/%s", tt->getIcon()));

	// Otherwise, no icon
	int tc_start = 0;
	if (!tex)
	{
		if (framed)
		{
			tex = theMapEditor->textureManager().getEditorImage("thing/square/frame");
		}
		else
		{
			tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_n");

			if ((tt->isAngled() && showicon) || thing_force_dir || things_angles)
			{
				tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_d1");

				// Setup variables depending on angle
				switch ((int)angle)
				{
				case 0:		// East: normal, texcoord 0
					break;
				case 45:	// Northeast: diagonal, texcoord 0
					tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_d2");
					break;
				case 90:	// North: normal, texcoord 2
					tc_start = 2;
					break;
				case 135:	// Northwest: diagonal, texcoord 2
					tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_d2");
					tc_start = 2;
					break;
				case 180:	// West: normal, texcoord 4
					tc_start = 4;
					break;
				case 225:	// Southwest: diagonal, texcoord 4
					tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_d2");
					tc_start = 4;
					break;
				case 270:	// South: normal, texcoord 6
					tc_start = 6;
					break;
				case 315:	// Southeast: diagonal, texcoord 6
					tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_d2");
					tc_start = 6;
					break;
				default:	// Unsupported angle, don't draw arrow
					tex = theMapEditor->textureManager().getEditorImage("thing/square/normal_n");
					break;
				};
			}
		}
	}

	// If for whatever reason the thing texture doesn't exist, just draw a basic, square thing
	if (!tex)
	{
		renderSimpleSquareThing(x, y, angle, tt, alpha);
		return false;
	}

	// Bind texture
	if (tex && tex_last != tex)
	{
		tex->bind();
		tex_last = tex;
	}

	// Draw thing
	double radius = tt->getRadius();
	if (tt->shrinkOnZoom()) radius = scaledRadius(radius);
	glBegin(GL_QUADS);
	int tc = tc_start;
	glTexCoord2f(sq_thing_tc[tc], sq_thing_tc[tc+1]);
	tc += 2;
	if (tc == 8) tc = 0;
	glVertex2d(x-radius, y-radius);
	glTexCoord2f(sq_thing_tc[tc], sq_thing_tc[tc+1]);
	tc += 2;
	if (tc == 8) tc = 0;
	glVertex2d(x-radius, y+radius);
	glTexCoord2f(sq_thing_tc[tc], sq_thing_tc[tc+1]);
	tc += 2;
	if (tc == 8) tc = 0;
	glVertex2d(x+radius, y+radius);
	glTexCoord2f(sq_thing_tc[tc], sq_thing_tc[tc+1]);
	glVertex2d(x+radius, y-radius);
	glEnd();

	return ((tt->isAngled() || thing_force_dir || things_angles) && !showicon);
}

/* MapRenderer2D::renderSimpleSquareThing
 * Renders a simple square thing icon at [x,y]
 *******************************************************************/
void MapRenderer2D::renderSimpleSquareThing(double x, double y, double angle, ThingType* tt, float alpha)
{
	// Ignore if no type given (shouldn't happen)
	if (!tt)
		return;

	// Get thing info
	double radius = tt->getRadius();
	if (tt->shrinkOnZoom()) radius = scaledRadius(radius);
	double radius2 = radius * 0.1;

	// Move to thing position
	glPushMatrix();
	glTranslated(x, y, 0);

	// Draw background
	glColor4f(0.0f, 0.0f, 0.0f, alpha);
	glBegin(GL_QUADS);
	glVertex2d(-radius, -radius);
	glVertex2d(-radius, radius);
	glVertex2d(radius, radius);
	glVertex2d(radius, -radius);
	glEnd();

	// Draw base
	//tt->getColour().set_gl(false);
	glColor4f(tt->getColour().fr(), tt->getColour().fg(), tt->getColour().fb(), alpha);
	glBegin(GL_QUADS);
	glVertex2d(-radius+radius2, -radius+radius2);
	glVertex2d(-radius+radius2, radius-radius2);
	glVertex2d(radius-radius2, radius-radius2);
	glVertex2d(radius-radius2, -radius+radius2);
	glEnd();

	// Draw angle indicator (if needed)
	if (tt->isAngled() || thing_force_dir)
	{
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glRotated(angle, 0, 0, 1);
		glBegin(GL_LINES);
		glVertex2d(0, 0);
		glVertex2d(radius, 0);
		glEnd();
	}

	// Restore previous matrix
	glPopMatrix();
}

/* MapRenderer2D::renderThings
 * Renders map things
 *******************************************************************/
void MapRenderer2D::renderThings(float alpha, bool force_dir)
{
	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	things_angles = force_dir;
	renderThingsImmediate(alpha);
}

/* MapRenderer2D::renderThingsImmediate
 * Renders map things in immediate mode
 *******************************************************************/
void MapRenderer2D::renderThingsImmediate(float alpha)
{
	// Display lists aren't really good for this, better to check for
	// visibility and just render things in immediate mode

	// Enable textures
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	tex_last = NULL;

	// Go through things
	MapThing* thing = NULL;
	double x, y, angle;
	vector<int> things_arrows;
	long last_update = thing_sprites_updated;

	// Draw thing shadows if needed
	if (thing_shadow > 0.01f && thing_drawtype != TDT_SPRITE)
	{
		glEnable(GL_TEXTURE_2D);
		GLTexture* tex_shadow = theMapEditor->textureManager().getEditorImage("thing/shadow");
		if (thing_drawtype == TDT_SQUARE || thing_drawtype == TDT_SQUARESPRITE || thing_drawtype == TDT_FRAMEDSPRITE)
			tex_shadow = theMapEditor->textureManager().getEditorImage("thing/square/shadow");
		if (tex_shadow)
		{
			tex_shadow->bind();
			glColor4f(0.0f, 0.0f, 0.0f, alpha*thing_shadow);

			// Setup point sprites if supported
			bool point = false;
			if (OpenGL::pointSpriteSupport())
			{
				glEnable(GL_POINT_SPRITE);
				glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
				point = true;
			}

			for (unsigned a = 0; a < map->nThings(); a++)
			{
				if (vis_t[a] > 0)
					continue;

				// No shadow if filtered
				thing = map->getThing(a);
				if (thing->isFiltered())
					continue;

				// Get thing info
				ThingType* tt = theGameConfiguration->thingType(thing->getType());
				double radius = (tt->getRadius()+1);
				if (tt->shrinkOnZoom()) radius = scaledRadius(radius);
				radius *= 1.3;
				x = thing->xPos();
				y = thing->yPos();

				// Draw shadow
				if (point && radius*2*view_scale <= OpenGL::maxPointSize())
				{
					// Point sprite
					glPointSize(radius*2*view_scale);
					glBegin(GL_POINTS);
					glVertex2d(x, y);
					glEnd();
				}
				else
				{
					// Textured quad
					if (point) glDisable(GL_POINT_SPRITE);
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 1.0f);	glVertex2d(x-radius, y-radius);
					glTexCoord2f(0.0f, 0.0f);	glVertex2d(x-radius, y+radius);
					glTexCoord2f(1.0f, 0.0f);	glVertex2d(x+radius, y+radius);
					glTexCoord2f(1.0f, 1.0f);	glVertex2d(x+radius, y-radius);
					glEnd();
					if (point) glEnable(GL_POINT_SPRITE);
				}
			}

			if (point)
				glDisable(GL_POINT_SPRITE);
		}
	}

	// Draw things
	double talpha;
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		if (vis_t[a] > 0)
			continue;

		// Get thing info
		thing = map->getThing(a);
		x = thing->xPos();
		y = thing->yPos();
		angle = thing->getAngle();

		// Set alpha
		if (thing->isFiltered())
			talpha = alpha*0.25;
		else
			talpha = alpha;

		// Get thing type properties from game configuration
		ThingType* tt = theGameConfiguration->thingType(thing->getType());

		// Reset thing sprite if modified
		if (thing->modifiedTime() > last_update && thing_sprites.size() > a)
			thing_sprites[a] = NULL;

		// Draw thing depending on 'things_drawtype' cvar
		if (thing_drawtype == TDT_SPRITE)  		// Drawtype 2: Sprites
		{
			// Check if we need to draw the direction arrow for this thing
			if (renderSpriteThing(x, y, angle, tt, a, talpha))
				things_arrows.push_back(a);
		}
		else if (thing_drawtype == TDT_ROUND)	// Drawtype 1: Round
			renderRoundThing(x, y, angle, tt, talpha);
		else  							// Drawtype 0 (or other): Square
		{
			if (renderSquareThing(x, y, angle, tt, talpha, (thing_drawtype < TDT_SQUARESPRITE), (thing_drawtype == TDT_FRAMEDSPRITE)))
				things_arrows.push_back(a);
		}
	}

	// Draw thing sprites within squares if that drawtype is set
	if (thing_drawtype > TDT_SPRITE)
	{
		glEnable(GL_TEXTURE_2D);

		for (unsigned a = 0; a < map->nThings(); a++)
		{
			if (vis_t[a] > 0)
				continue;

			// Get thing info
			thing = map->getThing(a);
			ThingType* tt = theGameConfiguration->thingType(thing->getType());
			x = thing->xPos();
			y = thing->yPos();

			if (thing_drawtype == TDT_SQUARESPRITE && tt->getSprite().IsEmpty())
				continue;

			// Set alpha
			if (thing->isFiltered())
				talpha = alpha*0.25;
			else
				talpha = alpha;

			renderSpriteThing(x, y, thing->getAngle(), tt, a, talpha, true);
		}
	}

	// Draw any thing direction arrows needed
	if (things_arrows.size() > 0)
	{
		rgba_t acol = COL_WHITE;
		acol.a = 255*alpha*arrow_alpha;
		OpenGL::setColour(acol);
		//glColor4f(1.0f, 1.0f, 1.0f, alpha * arrow_alpha);
		GLTexture* tex_arrow = theMapEditor->textureManager().getEditorImage("arrow");
		if (tex_arrow)
		{
			glEnable(GL_TEXTURE_2D);
			tex_arrow->bind();

			for (unsigned a = 0; a < things_arrows.size(); a++)
			{
				thing = map->getThing(things_arrows[a]);
				if (arrow_colour)
				{
					ThingType* tt = theGameConfiguration->thingType(thing->getType());
					if (tt)
					{
						acol.set(tt->getColour());
						acol.a = 255*alpha*arrow_alpha;
						OpenGL::setColour(acol, false);
						//glColor4f(tt->getColour().fr(), tt->getColour().fg(), tt->getColour().fb(), alpha * arrow_alpha);
					}
				}
				x = thing->xPos();
				y = thing->yPos();

				glPushMatrix();
				glTranslated(x, y, 0);
				glRotated(thing->getAngle(), 0, 0, 1);

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 1.0f);	glVertex2d(-32, -32);
				glTexCoord2f(0.0f, 0.0f);	glVertex2d(-32, 32);
				glTexCoord2f(1.0f, 0.0f);	glVertex2d(32, 32);
				glTexCoord2f(1.0f, 1.0f);	glVertex2d(32, -32);
				glEnd();

				glPopMatrix();
			}
		}
	}

	// Disable textures
	glDisable(GL_TEXTURE_2D);
}

/* MapRenderer2D::renderThingHilight
 * Renders the thing hilight overlay for thing [index]
 *******************************************************************/
void MapRenderer2D::renderThingHilight(int index, float fade)
{
	// Check hilight
	if (!map->getThing(index))
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_hilight");
	col.a *= fade;
	OpenGL::setColour(col);

	// Get thing info
	MapThing* thing = map->getThing(index);
	ThingType* tt = theGameConfiguration->thingType(thing->getType());
	double x = thing->xPos();
	double y = thing->yPos();

	// Get thing radius
	double radius = tt->getRadius();

	// Check if we want square overlays
	if (thing_overlay_square)
	{
		glDisable(GL_TEXTURE_2D);
		glLineWidth(3.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2d(x - radius, y - radius);
		glVertex2d(x - radius, y + radius);
		glVertex2d(x + radius, y + radius);
		glVertex2d(x + radius, y - radius);
		glEnd();
		col.a *= 0.5;
		OpenGL::setColour(col, false);
		glBegin(GL_QUADS);
		glVertex2d(x - radius, y - radius);
		glVertex2d(x - radius, y + radius);
		glVertex2d(x + radius, y + radius);
		glVertex2d(x + radius, y - radius);
		glEnd();

		return;
	}

	// Shrink if needed
	if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

	// Adjust radius
	if (thing_drawtype == TDT_SQUARE || thing_drawtype > TDT_SPRITE)
		radius += 6;
	else
		radius *= 1.1 + (0.2*fade);
	radius += halo_width * view_scale_inv;

	// Setup hilight thing texture
	GLTexture* tex = NULL;
	if (thing_drawtype == TDT_SQUARE || thing_drawtype == TDT_SQUARESPRITE || thing_drawtype == TDT_FRAMEDSPRITE)
		tex = theMapEditor->textureManager().getEditorImage("thing/square/hilight");
	else
		tex = theMapEditor->textureManager().getEditorImage("thing/hilight");
	if (tex)
	{
		glEnable(GL_TEXTURE_2D);
		tex->bind();
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(x - radius, y - radius);
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(x - radius, y + radius);
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(x + radius, y + radius);
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(x + radius, y - radius);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

/* MapRenderer2D::renderThingSelection
 * Renders the thing selection overlay for thing indices in
 * [selection]
 *******************************************************************/
void MapRenderer2D::renderThingSelection(vector<int>& selection, float fade)
{
	// Check anything is selected
	if (selection.size() == 0)
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Set selection colour
	rgba_t col = ColourConfiguration::getColour("map_selection");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup overlay rendering
	bool point = setupThingOverlay();

	// Draw all selection overlays
	for (unsigned a = 0; a < selection.size(); a++)
	{
		MapThing* thing = map->getThing(selection[a]);
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

		// Adjust radius if the overlay isn't square
		if (!thing_overlay_square)
			radius += 8;
		radius += halo_width * view_scale_inv;

		// Draw it
		renderThingOverlay(thing->xPos(), thing->yPos(), radius*(0.8+(0.2*fade)), point);
	}

	// Clean up gl state
	if (point)
		glDisable(GL_POINT_SPRITE);
	glDisable(GL_TEXTURE_2D);
}

/* MapRenderer2D::renderTaggedThings
 * Renders the tagged thing overlay for things in [things]
 *******************************************************************/
void MapRenderer2D::renderTaggedThings(vector<MapThing*>& things, float fade)
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_tagged");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup overlay rendering
	bool point = setupThingOverlay();

	// Draw all tagged overlays
	for (unsigned a = 0; a < things.size(); a++)
	{
		MapThing* thing = things[a];
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

		// Adjust radius if the overlay isn't square
		if (!thing_overlay_square)
			radius += 8;
		radius += halo_width * view_scale_inv;

		// Draw it
		renderThingOverlay(thing->xPos(), thing->yPos(), radius, point);
	}

	// Clean up gl state
	if (point)
		glDisable(GL_POINT_SPRITE);
	glDisable(GL_TEXTURE_2D);

	// Draw action lines
	// Because gl state is in texture mode above, we cannot merge the loops
	MapObject* object = theMapEditor->mapEditor().getHilightedObject();
	if (object && action_lines)
	{
		fpoint2_t dst = object->getPoint(MOBJ_POINT_WITHIN);
		glLineWidth(line_width*1.5f);
		for (unsigned a = 0; a < things.size(); a++)
		{
			MapThing* thing = things[a];
			Drawing::drawArrow(thing->getPoint(MOBJ_POINT_WITHIN), dst, col, false, arrowhead_angle, arrowhead_length);
		}
	}
}

/* MapRenderer2D::renderTaggingThings
 * Renders the tagging thing overlay for things in [things]
 *******************************************************************/
void MapRenderer2D::renderTaggingThings(vector<MapThing*>& things, float fade)
{
	// Reset fade if tagging animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_tagging");
	col.a *= fade;
	OpenGL::setColour(col);

	// Setup overlay rendering
	bool point = setupThingOverlay();

	// Draw all tagging overlays
	for (unsigned a = 0; a < things.size(); a++)
	{
		MapThing* thing = things[a];
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

		// Adjust radius if the overlay isn't square
		if (!thing_overlay_square)
			radius += 12;
		radius += halo_width * view_scale_inv;

		// Draw it
		renderThingOverlay(thing->xPos(), thing->yPos(), radius, point);
	}

	// Clean up gl state
	if (point)
		glDisable(GL_POINT_SPRITE);
	glDisable(GL_TEXTURE_2D);

	// Draw action lines
	// Because gl state is in texture mode above, we cannot merge the loops
	MapObject* object = theMapEditor->mapEditor().getHilightedObject();
	if (object && action_lines)
	{
		fpoint2_t src = object->getPoint(MOBJ_POINT_WITHIN);
		glLineWidth(line_width*1.5f);
		for (unsigned a = 0; a < things.size(); a++)
		{
			MapThing* thing = things[a];
			Drawing::drawArrow(src, thing->getPoint(MOBJ_POINT_WITHIN), col, false, arrowhead_angle, arrowhead_length);
		}
	}
}

/* MapRenderer2D::renderPathedThings
 * Renders thing pathing lines/arrows for [things]
 *******************************************************************/
void MapRenderer2D::renderPathedThings(vector<MapThing*>& things)
{
	// Skip if action lines are not desired, or if there's nothing to do
	if (!action_lines || things.size() == 0)
		return;

	// Check if paths need updating
	bool update = false;
	if (thing_paths.size() == 0)
		update = true;
	else if (map->thingsUpdated() > thing_paths_updated)
	{
		for (unsigned a = 0; a < things.size(); a++)
		{
			if (things[a]->modifiedTime() > thing_paths_updated)
			{
				update = true;
				break;
			}
		}
		if (!update)
			thing_paths_updated = theApp->runTimer();
	}

	// Get colours
	wxColour col(arrow_pathed_color);
	rgba_t pathedcol(col.Red(), col.Green(), col.Blue(), col.Alpha());
	col.Set(arrow_dragon_color);
	rgba_t dragoncol(col.Red(), col.Green(), col.Blue(), col.Alpha());

	if (update)
	{
		thing_paths.clear();

		// Find things that need to be pathed
		for (unsigned a = 0; a < things.size(); ++a)
		{
			MapThing* thing = things[a];
			tpath_t path;
			path.from_index = 0;
			path.to_index = 0;
			
			ThingType* tt = theGameConfiguration->thingType(thing->getType());

			// Dragon Path
			if (tt->getFlags() & THING_DRAGON)
			{
				MapThing* first = map->getFirstThingWithId(thing->intProperty("id"));
				if (first)
				{
					path.from_index = thing->getIndex();
					path.to_index = first->getIndex();
					path.type = PATH_DRAGON;
					thing_paths.push_back(path);

					vector<MapThing*> dragon_things;
					dragon_things.clear();
					map->getDragonTargets(first, dragon_things);
					for (unsigned d = 0; d < dragon_things.size(); ++d)
					{
						int id1 = dragon_things[d]->intProperty("id");
						int a11 = dragon_things[d]->intProperty("arg0");
						int a12 = dragon_things[d]->intProperty("arg1");
						int a13 = dragon_things[d]->intProperty("arg2");
						int a14 = dragon_things[d]->intProperty("arg3");
						int a15 = dragon_things[d]->intProperty("arg4");
						ThingType* tt1 = theGameConfiguration->thingType(dragon_things[d]->getType());
						for (unsigned e = d + 1; e < dragon_things.size(); ++e)
						{
							int id2 = dragon_things[e]->intProperty("id");
							int a21 = dragon_things[e]->intProperty("arg0");
							int a22 = dragon_things[e]->intProperty("arg1");
							int a23 = dragon_things[e]->intProperty("arg2");
							int a24 = dragon_things[e]->intProperty("arg3");
							int a25 = dragon_things[e]->intProperty("arg4");
							ThingType* tt2 = theGameConfiguration->thingType(dragon_things[e]->getType());
							bool l1to2 = ((a11 == id2) || (a12 == id2) || (a13 == id2) || (a14 == id2) || (a15 == id2));
							bool l2to1 = ((a21 == id1) || (a22 == id1) || (a23 == id1) || (a24 == id1) || (a25 == id1));
							if (!((tt1->getFlags()|tt2->getFlags()) & THING_DRAGON))
							{
								tpath_t dpath;
								if (l1to2)
								{
									dpath.from_index = dragon_things[e]->getIndex();
									dpath.to_index = dragon_things[d]->getIndex();
									dpath.type = l2to1 ? PATH_DRAGON_BOTH : PATH_DRAGON;
								}
								else if (l2to1)
								{
									dpath.from_index = dragon_things[d]->getIndex();
									dpath.to_index = dragon_things[e]->getIndex();
									dpath.type = PATH_DRAGON;
								}
								thing_paths.push_back(dpath);
							}
						}
					}
				}
				continue;
			}

			// Normal Path
			int tid = -1, tid2 = -1;
			int nexttype = tt->getNextType();
			int nextargs = tt->getNextArgs();
			if (nextargs)
			{
				int pos = nextargs % 10;
				string na = "arg_";
				na[3] = ('0' + pos - 1);
				tid = thing->intProperty(na);
			}
			if (nextargs >= 10)
			{
				int pos = nextargs / 10;
				string na = "arg_";
				na[3] = ('0' + pos - 1);
				tid += (256 * thing->intProperty(na));
			}
			for (unsigned b = a + 1; b < things.size(); ++b)
			{
				MapThing* thing2 = things[b];
				if (thing2->getType() == nexttype)
				{
					ThingType* tt2 = theGameConfiguration->thingType(thing2->getType());
					nextargs = tt2->getNextArgs();
					if (nextargs)
					{
						int pos = nextargs % 10;
						string na = "arg_";
						na[3] = ('0' + pos - 1);
						tid2 = thing2->intProperty(na);
					}
					if (nextargs >= 10)
					{
						int pos = nextargs / 10;
						string na = "arg_";
						na[3] = ('0' + pos - 1);
						tid2 += (256 * thing2->intProperty(na));
					}
					if (thing2->intProperty("id") == tid)
					{
						path.from_index = thing->getIndex();
						path.to_index = thing2->getIndex();
						path.type = (tid2 == thing->intProperty("id")) ? PATH_NORMAL_BOTH : PATH_NORMAL;
					}
					else if (thing->intProperty("id") == tid2)
					{
						path.from_index = thing2->getIndex();
						path.to_index = thing->getIndex();
						path.type = PATH_NORMAL;
					}
					thing_paths.push_back(path);
				}
			}

		}
		thing_paths_updated = theApp->runTimer();
	}

	// Setup GL stuff
	glLineWidth(line_width*1.5f);

	for (unsigned a = 0; a < thing_paths.size(); a++)
	{
		if (thing_paths[a].from_index == thing_paths[a].to_index)
			continue;

		MapThing *from = map->getThing(thing_paths[a].from_index);

		if (from && ((from->intProperty("arg3") | (from->intProperty("arg4") << 8)) > 0))
		{
			MapThing *to = map->getThing(thing_paths[a].to_index);
			if (!to)
				continue;
	
			Drawing::drawArrow(to->getPoint(MOBJ_POINT_MID),
				from->getPoint(MOBJ_POINT_MID),
				(thing_paths[a].type == PATH_DRAGON_BOTH || thing_paths[a].type == PATH_DRAGON) ? dragoncol : pathedcol, 
				(thing_paths[a].type == PATH_NORMAL_BOTH || thing_paths[a].type == PATH_DRAGON_BOTH), 
				arrowhead_angle, arrowhead_length);
		}
	}
}

/* MapRenderer2D::renderFlats
 * Renders map flats (sectors)
 *******************************************************************/
void MapRenderer2D::renderFlats(int type, bool texture, float alpha)
{
	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	if (OpenGL::vboSupport() && flats_use_vbo)
		renderFlatsVBO(type, texture, alpha);
	else
		renderFlatsImmediate(type, texture, alpha);

	flats_updated = theApp->runTimer();
}

/* MapRenderer2D::sortPolyByTex
 * Sorting function to sort polygons by their texture
 *******************************************************************/
bool sortPolyByTex(Polygon2D* left, Polygon2D* right)
{
	return left->getTexture()->glId() < right->getTexture()->glId();
}

/* MapRenderer2D::renderFlatsImmediate
 * Renders map flats in immediate mode
 *******************************************************************/
void MapRenderer2D::renderFlatsImmediate(int type, bool texture, float alpha)
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);

		// Apply flat alpha from theme
		alpha *= ColourConfiguration::getFlatAlpha();
	}

	if (flat_ignore_light)
		glColor4f(flat_brightness, flat_brightness, flat_brightness, alpha);

	// Re-init flats texture list if invalid
	if ((texture && tex_flats.size() < map->nSectors()) || last_flat_type != type)
	{
		tex_flats.clear();
		for (unsigned a = 0; a < map->nSectors(); a++)
			tex_flats.push_back(NULL);

		last_flat_type = type;
	}

	// Go through sectors
	GLTexture* tex_last = NULL;
	GLTexture* tex = NULL;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		MapSector* sector = map->getSector(a);

		// Skip if sector is out of view
		if (vis_s[a] > 0)
			continue;

		if (texture)
		{
			if (!tex_flats[a] || sector->modifiedTime() > flats_updated - 100)
			{
				// Get the sector texture
				if (type <= 1)
					tex = theMapEditor->textureManager().getFlat(sector->getFloorTex(), theGameConfiguration->mixTexFlats());
				else
					tex = theMapEditor->textureManager().getFlat(sector->getCeilingTex(), theGameConfiguration->mixTexFlats());

				tex_flats[a] = tex;
			}
			else
				tex = tex_flats[a];

			// Bind the texture if needed
			if (tex)
			{
				if (!tex_last)
					glEnable(GL_TEXTURE_2D);
				if (tex != tex_last)
					tex->bind();
			}
			else if (tex_last)
				glDisable(GL_TEXTURE_2D);

			tex_last = tex;
		}

		// Setup polygon texture info if needed
		Polygon2D* poly = sector->getPolygon();
		if (texture && poly->getTexture() != tex)
		{
			poly->setTexture(tex);

			// Get scaling/offset info
			double ox = 0;
			double oy = 0;
			double sx = tex->getScaleX();
			double sy = tex->getScaleY();
			double rot = 0;
			// Check for various UDMF extensions
			if (theMapEditor->currentMapDesc().format == MAP_UDMF)
			{
				// Floor
				if (type <= 1)
				{
					if (theGameConfiguration->udmfFlatPanning())
					{
						ox = sector->floatProperty("xpanningfloor");
						oy = sector->floatProperty("ypanningfloor");
					}
					if (theGameConfiguration->udmfFlatScaling())
					{
						sx *= (1.0 / sector->floatProperty("xscalefloor"));
						sy *= (1.0 / sector->floatProperty("yscalefloor"));
					}
					if (theGameConfiguration->udmfFlatRotation())
						rot = sector->floatProperty("rotationfloor");
				}
				// Ceiling
				else
				{
					if (theGameConfiguration->udmfFlatPanning())
					{
						ox = sector->floatProperty("xpanningceiling");
						oy = sector->floatProperty("ypanningceiling");
					}
					if (theGameConfiguration->udmfFlatScaling())
					{
						sx *= (1.0 / sector->floatProperty("xscaleceiling"));
						sy *= (1.0 / sector->floatProperty("yscaleceiling"));
					}
					if (theGameConfiguration->udmfFlatRotation())
						rot = sector->floatProperty("rotationceiling");
				}
			}

			poly->updateTextureCoords(sx, sy, ox, oy, rot);
		}

		// Render the polygon
		if (!flat_ignore_light)
		{
			rgba_t col = sector->getColour(type);
			col.ampf(flat_brightness, flat_brightness, flat_brightness, 1.0f);
			glColor4f(col.fr(), col.fg(), col.fb(), alpha);
		}
		poly->render();
	}

	if (texture)
		glDisable(GL_TEXTURE_2D);
}

/* MapRenderer2D::renderFlatsVBO
 * Renders map flats using an OpenGL Vertex Buffer Object
 *******************************************************************/
void MapRenderer2D::renderFlatsVBO(int type, bool texture, float alpha)
{
	bool vbo_updated = false;

	if (flat_ignore_light)
		glColor4f(flat_brightness, flat_brightness, flat_brightness, alpha);

	if (!glGenBuffers)
		return;

	// Apply flat alpha from theme
	if (texture)
		alpha *= ColourConfiguration::getFlatAlpha();

	// Re-init flats texture list if invalid
	if ((texture && tex_flats.size() != map->nSectors()) || last_flat_type != type)
	{
		tex_flats.clear();
		for (unsigned a = 0; a < map->nSectors(); a++)
			tex_flats.push_back(NULL);

		last_flat_type = type;
	}

	// First, check if any polygon vertex data has changed (in this case we need to refresh the entire vbo)
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		Polygon2D* poly = map->getSector(a)->getPolygon();
		if (poly && poly->vboUpdate() > 1)
		{
			//wxLogMessage("Updating sector %d polygon vbo data", a);
			updateFlatsVBO();
			vbo_updated = true;
		}
	}

	// Create VBO if necessary
	if (!vbo_updated && vbo_flats == 0)
	{
		updateFlatsVBO();
		vbo_updated = true;
	}

	//if (vbo_updated)
	//	wxLogMessage("Updated vbo");

	// Setup opengl state
	if (texture) glEnable(GL_TEXTURE_2D);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_flats);

	// Setup VBO pointers
	Polygon2D::setupVBOPointers();

	// Go through sectors
	GLTexture* tex_last = NULL;
	GLTexture* tex = NULL;
	bool first = true;
	unsigned update = 0;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		MapSector* sector = map->getSector(a);

		// Skip if sector is out of view
		if (vis_s[a] > 0)
			continue;

		first = false;
		if (texture)
		{
			if (!tex_flats[a] || sector->modifiedTime() > flats_updated - 100)
			{
				// Get the sector texture
				if (type <= 1)
					tex = theMapEditor->textureManager().getFlat(sector->getFloorTex(), theGameConfiguration->mixTexFlats());
				else
					tex = theMapEditor->textureManager().getFlat(sector->getCeilingTex(), theGameConfiguration->mixTexFlats());

				tex_flats[a] = tex;
			}
			else
				tex = tex_flats[a];
		}

		// Setup polygon texture info if needed
		Polygon2D* poly = sector->getPolygon();
		if (texture && poly->getTexture() != tex)
		{
			poly->setTexture(tex);			// Set polygon texture

			// Get scaling/offset info
			double ox = 0;
			double oy = 0;
			double sx = tex->getScaleX();
			double sy = tex->getScaleY();
			double rot = 0;
			// Check for various UDMF extensions
			if (theMapEditor->currentMapDesc().format == MAP_UDMF)
			{
				// Floor
				if (type <= 1)
				{
					if (theGameConfiguration->udmfFlatPanning())
					{
						ox = sector->floatProperty("xpanningfloor");
						oy = sector->floatProperty("ypanningfloor");
					}
					if (theGameConfiguration->udmfFlatScaling())
					{
						sx *= (1.0 / sector->floatProperty("xscalefloor"));
						sy *= (1.0 / sector->floatProperty("yscalefloor"));
					}
					if (theGameConfiguration->udmfFlatRotation())
						rot = sector->floatProperty("rotationfloor");
				}
				// Ceiling
				else
				{
					if (theGameConfiguration->udmfFlatPanning())
					{
						ox = sector->floatProperty("xpanningceiling");
						oy = sector->floatProperty("ypanningceiling");
					}
					if (theGameConfiguration->udmfFlatScaling())
					{
						sx *= (1.0 / sector->floatProperty("xscaleceiling"));
						sy *= (1.0 / sector->floatProperty("yscaleceiling"));
					}
					if (theGameConfiguration->udmfFlatRotation())
						rot = sector->floatProperty("rotationceiling");
				}
			}
			// Scaling applies to offsets as well.
			// Note for posterity: worldpanning only applies to textures, not flats
			ox /= sx;
			oy /= sy;

			poly->updateTextureCoords(sx, sy, ox, oy, rot);
		}

		// Update polygon VBO data if needed
		if (poly->vboUpdate() > 0)
		{
			poly->updateVBOData();
			update++;
			if (update > 200)
				break;
		}

		// Bind the texture if needed
		if (tex)
		{
			if (!tex_last || first)
				glEnable(GL_TEXTURE_2D);
			if (tex != tex_last)
				tex->bind();
		}
		else if (!tex_last || first)
			glDisable(GL_TEXTURE_2D);
		tex_last = tex;

		// Render the polygon
		if (!flat_ignore_light)
		{
			rgba_t col = sector->getColour(type);
			col.ampf(flat_brightness, flat_brightness, flat_brightness, 1.0f);
			glColor4f(col.fr(), col.fg(), col.fb(), alpha);
		}
		poly->renderVBO(false);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Clean up opengl state
	if (texture) glDisable(GL_TEXTURE_2D);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/* MapRenderer2D::renderFlatHilight
 * Renders the flat hilight overlay for sector [index]
 *******************************************************************/
void MapRenderer2D::renderFlatHilight(int index, float fade)
{
	// Check hilight
	if (!map->getSector(index))
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set hilight colour
	rgba_t col = ColourConfiguration::getColour("map_hilight");
	col.a *= fade;
	OpenGL::setColour(col);

	// Fill if cvar is set
	if (sector_hilight_fill)
	{
		glColor4f(col.fr(), col.fg(), col.fb(), col.fa()*0.5f);
		map->getSector(index)->getPolygon()->render();
		glLineWidth(line_width * ColourConfiguration::getLineHilightWidth());
	}
	else
		glLineWidth(line_width * ColourConfiguration::getLineHilightWidth());

	// Get all lines belonging to the hilighted sector
	vector<MapLine*> lines;
	map->getSector(index)->getLines(lines);

	// Draw hilight
	MapLine* line = NULL;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		line = lines[a];
		if (!line) continue;

		// Draw line
		glBegin(GL_LINES);
		glVertex2d(line->v1()->xPos(), line->v1()->yPos());
		glVertex2d(line->v2()->xPos(), line->v2()->yPos());
		glEnd();
	}

	// Draw sector split lines
	if (test_ssplit)
	{
		glColor4f(col.fr(), col.fg(), col.fb(), col.fa() * 0.5f);
		glLineWidth(1.0f);
		map->getSector(index)->getPolygon()->renderWireframe();
	}

	//// TEST draw text point
	//glPointSize(8.0f);
	//glBegin(GL_POINTS);
	//glVertex2d(map->getSector(index)->getPoint(MOBJ_POINT_WITHIN).x, map->getSector(index)->getPoint(MOBJ_POINT_WITHIN).y);
	//glEnd();
}

/* MapRenderer2D::renderFlatSelection
 * Renders the flat selection overlay for sector indices in
 * [selection]
 *******************************************************************/
void MapRenderer2D::renderFlatSelection(vector<int>& selection, float fade)
{
	// Check anything is selected
	if (selection.size() == 0)
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Set selection colour
	rgba_t col = ColourConfiguration::getColour("map_selection");
	col.a *= fade;
	OpenGL::setColour(col);

	// Draw selection
	glColor4f(col.fr(), col.fg(), col.fb(), col.fa() * 0.75f);
	vector<MapSide*> sides_selected;
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Don't draw if outside screen (but still draw if it's small)
		if (vis_s[selection[a]] > 0 && vis_s[selection[a]] != VIS_SMALL)
			continue;

		// Get the sector's polygon
		Polygon2D* poly = map->getSector(selection[a])->getPolygon();
		vector<MapSide*>& sides = map->getSector(selection[a])->connectedSides();

		if (poly->hasPolygon())
		{
			if (sector_selected_fill)
				map->getSector(selection[a])->getPolygon()->render();

			for (unsigned s = 0; s < sides.size(); s++)
				sides_selected.push_back(sides[s]);
		}
		else
		{
			// Something went wrong with the polygon, just draw sector outline instead
			glColor4f(col.fr(), col.fg(), col.fb(), col.fa());
			glBegin(GL_LINES);
			for (unsigned s = 0; s < sides.size(); s++)
			{
				MapLine* line = sides[s]->getParentLine();
				glVertex2d(line->v1()->xPos(), line->v1()->yPos());
				glVertex2d(line->v2()->xPos(), line->v2()->yPos());
			}
			glEnd();

			glColor4f(col.fr(), col.fg(), col.fb(), col.fa() * 0.6f);
		}
	}

	// Draw selection outline
	glColor4f(col.fr(), col.fg(), col.fb(), col.fa());
	glLineWidth(line_width * 2);
	bool* lines_drawn = new bool[map->nLines()];
	memset(lines_drawn, 0, map->nLines());
	glBegin(GL_LINES);
	for (unsigned a = 0; a < sides_selected.size(); a++)
	{
		MapLine* line = sides_selected[a]->getParentLine();
		if (lines_drawn[line->getIndex()])
			continue;

		glVertex2d(line->v1()->xPos(), line->v1()->yPos());
		glVertex2d(line->v2()->xPos(), line->v2()->yPos());
		lines_drawn[line->getIndex()] = true;
	}
	glEnd();
	delete[] lines_drawn;
}

/* MapRenderer2D::renderTaggedFlats
 * Renders the tagged flat overlay for sectors in [sectors]
 *******************************************************************/
void MapRenderer2D::renderTaggedFlats(vector<MapSector*>& sectors, float fade)
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Set colour
	rgba_t col = ColourConfiguration::getColour("map_tagged");
	col.a *= fade;
	OpenGL::setColour(col);

	// Render each sector polygon
	glDisable(GL_TEXTURE_2D);
	MapObject* object = theMapEditor->mapEditor().getHilightedObject();
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		sectors[a]->getPolygon()->render();

		// Get all lines belonging to the tagged sector
		vector<MapLine*> lines;
		sectors[a]->getLines(lines);

		// Draw hilight
		MapLine* line = NULL;
		for (unsigned b = 0; b < lines.size(); b++)
		{
			line = lines[b];
			if (!line) continue;

			// Draw line
			glBegin(GL_LINES);
			glVertex2d(line->v1()->xPos(), line->v1()->yPos());
			glVertex2d(line->v2()->xPos(), line->v2()->yPos());
			glEnd();
		}

		// Action lines
		if (object && action_lines)
		{
			// Skip if the tagged sector is adjacent
			if (object->getObjType() == MOBJ_LINE)
			{
				MapLine* line = (MapLine*)object;
				if (line->frontSector() == sectors[a] || line->backSector() == sectors[a])
					continue;
			}

			glLineWidth(line_width*1.5f);
			Drawing::drawArrow(sectors[a]->getPoint(MOBJ_POINT_WITHIN), object->getPoint(MOBJ_POINT_WITHIN), col, false, arrowhead_angle, arrowhead_length);
		}
	}
}

/* MapRenderer2D::renderMovingVertices
 * Renders the moving overlay for vertex indices in [vertices], to
 * show movement by [move_vec]
 *******************************************************************/
void MapRenderer2D::renderMovingVertices(vector<int>& vertices, fpoint2_t move_vec)
{
	uint8_t* lines_drawn = new uint8_t[map->nLines()];
	memset(lines_drawn, 0, map->nLines());

	// Determine what lines need drawing (and which of their vertices are being moved)
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		MapVertex* v = map->getVertex(vertices[a]);
		for (unsigned l = 0; l < v->nConnectedLines(); l++)
		{
			MapLine* line = v->connectedLine(l);

			if (line->v1() == v) lines_drawn[line->getIndex()] |= 1;
			if (line->v2() == v) lines_drawn[line->getIndex()] |= 2;
		}
	}

	// Draw any lines attached to the moving vertices
	glLineWidth(line_width);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		uint8_t drawn = lines_drawn[line->getIndex()];

		// Skip if not attached to any moving vertices
		if (drawn == 0)
			continue;

		// Set line colour
		OpenGL::setColour(lineColour(line, true), false);

		// First vertex
		if (drawn & 1)
			glVertex2d(line->x1() + move_vec.x, line->y1() + move_vec.y);
		else
			glVertex2d(line->x1(), line->y1());

		// Second vertex
		if (drawn & 2)
			glVertex2d(line->x2() + move_vec.x, line->y2() + move_vec.y);
		else
			glVertex2d(line->x2(), line->y2());
	}
	glEnd();

	// Set 'moving' colour
	OpenGL::setColour(ColourConfiguration::getColour("map_moving"));

	// Draw moving vertex overlays
	bool point = setupVertexRendering(1.5f);
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		glVertex2d(map->getVertex(vertices[a])->xPos() + move_vec.x,
				   map->getVertex(vertices[a])->yPos() + move_vec.y);
	}
	glEnd();

	// Clean up
	delete[] lines_drawn;
	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::renderMovingLines
 * Renders the moving overlay for line indices in [lines], to show
 * movement by [move_vec]
 *******************************************************************/
void MapRenderer2D::renderMovingLines(vector<int>& lines, fpoint2_t move_vec)
{
	uint8_t* lines_drawn = new uint8_t[map->nLines()];
	memset(lines_drawn, 0, map->nLines());

	// Determine what lines need drawing (and which of their vertices are being moved)
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Check first vertex
		MapVertex* v = map->getLine(lines[a])->v1();
		for (unsigned l = 0; l < v->nConnectedLines(); l++)
		{
			MapLine* line = v->connectedLine(l);

			if (line->v1() == v) lines_drawn[line->getIndex()] |= 1;
			if (line->v2() == v) lines_drawn[line->getIndex()] |= 2;
		}

		// Check second vertex
		v = map->getLine(lines[a])->v2();
		for (unsigned l = 0; l < v->nConnectedLines(); l++)
		{
			MapLine* line = v->connectedLine(l);

			if (line->v1() == v) lines_drawn[line->getIndex()] |= 1;
			if (line->v2() == v) lines_drawn[line->getIndex()] |= 2;
		}
	}

	// Draw any lines attached to the moving vertices
	glLineWidth(line_width);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		uint8_t drawn = lines_drawn[line->getIndex()];

		// Skip if not attached to any moving vertices
		if (drawn == 0)
			continue;

		// Set line colour
		OpenGL::setColour(lineColour(line, true), false);

		// First vertex
		if (drawn & 1)
			glVertex2d(line->x1() + move_vec.x, line->y1() + move_vec.y);
		else
			glVertex2d(line->x1(), line->y1());

		// Second vertex
		if (drawn & 2)
			glVertex2d(line->x2() + move_vec.x, line->y2() + move_vec.y);
		else
			glVertex2d(line->x2(), line->y2());
	}
	glEnd();

	// Set 'moving' colour
	OpenGL::setColour(ColourConfiguration::getColour("map_moving"));

	// Draw moving line overlays
	glLineWidth(line_width*3);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		MapLine* line = map->getLine(lines[a]);
		glVertex2d(line->x1() + move_vec.x, line->y1() + move_vec.y);
		glVertex2d(line->x2() + move_vec.x, line->y2() + move_vec.y);
	}
	glEnd();

	// Clean up
	delete[] lines_drawn;
}

/* MapRenderer2D::renderMovingSectors
 * Renders the moving overlay for sector indices in [sectors], to
 * show movement by [move_vec]
 *******************************************************************/
void MapRenderer2D::renderMovingSectors(vector<int>& sectors, fpoint2_t move_vec)
{
	// Determine what lines are being moved
	uint8_t* lines_moved = new uint8_t[map->nLines()];
	memset(lines_moved, 0, map->nLines());
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		// Go through connected sides
		vector<MapSide*>& sides = map->getSector(sectors[a])->connectedSides();
		for (unsigned s = 0; s < sides.size(); s++)
			lines_moved[sides[s]->getParentLine()->getIndex()] = 1;	// Mark parent line as moved
	}

	// Build list of moving lines
	vector<int> lines;
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		if (lines_moved[a] > 0)
			lines.push_back(a);
	}

	// Draw moving lines
	renderMovingLines(lines, move_vec);

	// Clean up
	delete[] lines_moved;
}

/* MapRenderer2D::renderMovingThings
 * Renders the moving overlay for thing indices in [things], to
 * show movement by [move_vec]
 *******************************************************************/
void MapRenderer2D::renderMovingThings(vector<int>& things, fpoint2_t move_vec)
{
	// Enable textures
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	tex_last = NULL;

	// Draw things
	MapThing* thing = NULL;
	double x, y, angle;
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Get thing info
		thing = map->getThing(things[a]);
		x = thing->xPos() + move_vec.x;
		y = thing->yPos() + move_vec.y;
		angle = thing->getAngle();

		// Get thing type properties from game configuration
		ThingType* tt = theGameConfiguration->thingType(thing->getType());

		// Draw thing depending on 'things_drawtype' cvar
		if (thing_drawtype == TDT_SPRITE)		// Drawtype 2: Sprites
			renderSpriteThing(x, y, angle, tt, a, 1.0f);
		else if (thing_drawtype == TDT_ROUND)	// Drawtype 1: Round
			renderRoundThing(x, y, angle, tt, 1.0f);
		else							// Drawtype 0 (or other): Square
			renderSquareThing(x, y, angle, tt, 1.0f, thing_drawtype < TDT_SQUARESPRITE, thing_drawtype == TDT_FRAMEDSPRITE);
	}

	// Draw thing sprites within squares if that drawtype is set
	if (thing_drawtype > TDT_SPRITE)
	{
		glEnable(GL_TEXTURE_2D);

		for (unsigned a = 0; a < things.size(); a++)
		{
			// Get thing info
			thing = map->getThing(things[a]);
			ThingType* tt = theGameConfiguration->thingType(thing->getType());
			x = thing->xPos() + move_vec.x;
			y = thing->yPos() + move_vec.y;
			angle = thing->getAngle();

			renderSpriteThing(x, y, angle, tt, things[a], 1.0f, true);
		}
	}

	// Set 'moving' colour
	OpenGL::setColour(ColourConfiguration::getColour("map_moving"));

	// Draw moving thing overlays
	bool point = setupThingOverlay();
	for (unsigned a = 0; a < things.size(); a++)
	{
		thing = map->getThing(things[a]);
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

		// Adjust radius if the overlay isn't square
		if (!thing_overlay_square)
			radius += 8;

		renderThingOverlay(thing->xPos() + move_vec.x, thing->yPos() + move_vec.y, radius, point);
	}

	// Clean up gl state
	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::renderPasteThings
 * Renders pasting overlay for [things] at [pos]
 *******************************************************************/
void MapRenderer2D::renderPasteThings(vector<MapThing*>& things, fpoint2_t pos)
{
	// Enable textures
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	tex_last = NULL;

	// Draw things
	MapThing* thing = NULL;
	double x, y, angle;
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Get thing info
		thing = things[a];
		x = thing->xPos() + pos.x;
		y = thing->yPos() + pos.y;
		angle = thing->getAngle();

		// Get thing type properties from game configuration
		ThingType* tt = theGameConfiguration->thingType(thing->getType());

		// Draw thing depending on 'things_drawtype' cvar
		if (thing_drawtype == TDT_SPRITE)		// Drawtype 2: Sprites
			renderSpriteThing(x, y, angle, tt, wxUINT32_MAX, 1.0f);
		else if (thing_drawtype == TDT_ROUND)	// Drawtype 1: Round
			renderRoundThing(x, y, angle, tt, 1.0f);
		else							// Drawtype 0 (or other): Square
			renderSquareThing(x, y, angle, tt, 1.0f, thing_drawtype < TDT_SQUARESPRITE, thing_drawtype == TDT_FRAMEDSPRITE);
	}

	// Draw thing sprites within squares if that drawtype is set
	if (thing_drawtype > TDT_SPRITE)
	{
		glEnable(GL_TEXTURE_2D);

		for (unsigned a = 0; a < things.size(); a++)
		{
			// Get thing info
			thing = things[a];
			ThingType* tt = theGameConfiguration->thingType(thing->getType());
			x = thing->xPos() + pos.x;
			y = thing->yPos() + pos.y;
			angle = thing->getAngle();

			renderSpriteThing(x, y, angle, tt, wxUINT32_MAX, 1.0f, true);
		}
	}

	// Set 'drawing' colour
	OpenGL::setColour(ColourConfiguration::getColour("map_linedraw"));

	// Draw moving thing overlays
	bool point = setupThingOverlay();
	for (unsigned a = 0; a < things.size(); a++)
	{
		thing = things[a];
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

		// Adjust radius if the overlay isn't square
		if (!thing_overlay_square)
			radius += 8;

		renderThingOverlay(thing->xPos() + pos.x, thing->yPos() + pos.y, radius, point);
	}

	// Clean up gl state
	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapRenderer2D::renderObjectEditGroup
 * Renders object edit group overlay for [group]
 *******************************************************************/
void MapRenderer2D::renderObjectEditGroup(ObjectEditGroup* group)
{
	// Simple test
	vector<fpoint2_t> vertex_points;
	group->getVerticesToDraw(vertex_points);
	vector<ObjectEditGroup::line_t> lines;
	group->getLinesToDraw(lines);

	// Set 'drawing' colour
	OpenGL::setColour(ColourConfiguration::getColour("map_linedraw"));

	// --- Lines ---

	// Lines
	glBegin(GL_LINES);
	glLineWidth(line_width);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		OpenGL::setColour(lineColour(lines[a].map_line, true), false);
		glVertex2d(lines[a].v1->position.x, lines[a].v1->position.y);
		glVertex2d(lines[a].v2->position.x, lines[a].v2->position.y);
	}
	glEnd();

	// Edit overlay
	OpenGL::setColour(ColourConfiguration::getColour("map_object_edit"));
	glLineWidth(line_width*3);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a].isExtra())
			continue;

		glVertex2d(lines[a].v1->position.x, lines[a].v1->position.y);
		glVertex2d(lines[a].v2->position.x, lines[a].v2->position.y);
	}
	glEnd();

	// --- Vertices ---

	// Setup rendering properties
	bool point = setupVertexRendering(1.0f);
	OpenGL::setColour(COL_WHITE);
	OpenGL::setColour(ColourConfiguration::getColour("map_object_edit"), false);

	// Render vertices
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < vertex_points.size(); a++)
		glVertex2d(vertex_points[a].x, vertex_points[a].y);
	glEnd();

	// Clean up
	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}

	// --- Things ---

	// Get things to draw
	vector<ObjectEditGroup::thing_t> things;
	group->getThingsToDraw(things);

	if (!things.empty())
	{
		// Enable textures
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		tex_last = NULL;

		// Draw things
		MapThing* thing = NULL;
		double x, y, angle;
		for (unsigned a = 0; a < things.size(); a++)
		{
			// Get thing info
			thing = things[a].map_thing;
			x = things[a].position.x;
			y = things[a].position.y;
			angle = thing->getAngle();

			// Get thing type properties from game configuration
			ThingType* tt = theGameConfiguration->thingType(thing->getType());

			// Draw thing depending on 'things_drawtype' cvar
			if (thing_drawtype == TDT_SPRITE)		// Drawtype 2: Sprites
				renderSpriteThing(x, y, angle, tt, thing->getIndex(), 1.0f);
			else if (thing_drawtype == TDT_ROUND)	// Drawtype 1: Round
				renderRoundThing(x, y, angle, tt, 1.0f);
			else							// Drawtype 0 (or other): Square
				renderSquareThing(x, y, angle, tt, 1.0f, thing_drawtype < TDT_SQUARESPRITE, thing_drawtype == TDT_FRAMEDSPRITE);
		}

		// Draw thing sprites within squares if that drawtype is set
		if (thing_drawtype > TDT_SPRITE)
		{
			glEnable(GL_TEXTURE_2D);

			for (unsigned a = 0; a < things.size(); a++)
			{
				// Get thing info
				thing = things[a].map_thing;
				ThingType* tt = theGameConfiguration->thingType(thing->getType());
				x = things[a].position.x;
				y = things[a].position.y;
				angle = thing->getAngle();

				renderSpriteThing(x, y, angle, tt, thing->getIndex(), 1.0f, true);
			}
		}

		// Set 'moving' colour
		OpenGL::setColour(ColourConfiguration::getColour("map_object_edit"));

		// Draw moving thing overlays
		bool point = setupThingOverlay();
		for (unsigned a = 0; a < things.size(); a++)
		{
			thing = things[a].map_thing;
			ThingType* tt = theGameConfiguration->thingType(thing->getType());
			double radius = tt->getRadius();
			if (tt->shrinkOnZoom()) radius = scaledRadius(radius);

			// Adjust radius if the overlay isn't square
			if (!thing_overlay_square)
				radius += 8;

			renderThingOverlay(things[a].position.x, things[a].position.y, radius, point);
		}

		// Clean up gl state
		if (point)
		{
			glDisable(GL_POINT_SPRITE);
			glDisable(GL_TEXTURE_2D);
		}
	}
}

/* MapRenderer2D::updateVerticesVBO
 * (Re)builds the map vertices VBO
 *******************************************************************/
void MapRenderer2D::updateVerticesVBO()
{
	// Create VBO if needed
	if (vbo_vertices == 0)
		glGenBuffers(1, &vbo_vertices);

	// Fill vertices VBO
	int nfloats = map->nVertices()*2;
	GLfloat* verts = new GLfloat[nfloats];
	unsigned i = 0;
	for (unsigned a = 0; a < map->nVertices(); a++)
	{
		verts[i++] = map->getVertex(a)->xPos();
		verts[i++] = map->getVertex(a)->yPos();
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*nfloats, verts, GL_STATIC_DRAW);

	// Clean up
	delete[] verts;
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	n_vertices = map->nVertices();
	vertices_updated = theApp->runTimer();
}

/* MapRenderer2D::updateLinesVBO
 * (Re)builds the map lines VBO
 *******************************************************************/
void MapRenderer2D::updateLinesVBO(bool show_direction, float base_alpha)
{
	LOG_MESSAGE(3, "Updating lines VBO");

	// Create VBO if needed
	if (vbo_lines == 0)
		glGenBuffers(1, &vbo_lines);

	// Determine the number of vertices per line
	int vpl = 2;
	if (show_direction) vpl = 4;

	// Fill lines VBO
	int nverts = map->nLines()*vpl;
	glvert_t* lines = new glvert_t[nverts];
	unsigned v = 0;
	rgba_t col;
	float alpha;
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);

		// Get line colour
		col = lineColour(line);
		alpha = base_alpha*col.fa();

		// Set line vertices
		lines[v].x = line->v1()->xPos();
		lines[v].y = line->v1()->yPos();
		lines[v+1].x = line->v2()->xPos();
		lines[v+1].y = line->v2()->yPos();

		// Set line colour(s)
		lines[v].r = lines[v+1].r = col.fr();
		lines[v].g = lines[v+1].g = col.fg();
		lines[v].b = lines[v+1].b = col.fb();
		lines[v].a = lines[v+1].a = alpha;

		// Direction tab if needed
		if (show_direction)
		{
			fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
			fpoint2_t tab = line->dirTabPoint();
			lines[v+2].x = mid.x;
			lines[v+2].y = mid.y;
			lines[v+3].x = tab.x;
			lines[v+3].y = tab.y;

			// Colours
			lines[v+2].r = lines[v+3].r = col.fr();
			lines[v+2].g = lines[v+3].g = col.fg();
			lines[v+2].b = lines[v+3].b = col.fb();
			lines[v+2].a = lines[v+3].a = alpha*0.6f;
		}

		// Next line
		v += vpl;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glvert_t)*nverts, lines, GL_STATIC_DRAW);

	// Clean up
	delete[] lines;
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	n_lines = map->nLines();
	lines_updated = theApp->runTimer();
}

/* MapRenderer2D::updateFlatsVBO
 * (Re)builds the map flats VBO
 *******************************************************************/
void MapRenderer2D::updateFlatsVBO()
{
	if (!flats_use_vbo)
		return;

	// Create VBO if needed
	if (vbo_flats == 0)
		glGenBuffers(1, &vbo_flats);

	// Get total size needed
	unsigned totalsize = 0;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		Polygon2D* poly = map->getSector(a)->getPolygon();
		totalsize += poly->vboDataSize();
	}

	// Allocate buffer data
	glBindBuffer(GL_ARRAY_BUFFER, vbo_flats);
	glBufferData(GL_ARRAY_BUFFER, totalsize, NULL, GL_STATIC_DRAW);

	// Write polygon data to VBO
	unsigned offset = 0;
	unsigned index = 0;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		Polygon2D* poly = map->getSector(a)->getPolygon();
		offset = poly->writeToVBO(offset, index);
		index += poly->totalVertices();
	}

	// Clean up
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	flats_updated = theApp->runTimer();
}

/* MapRenderer2D::updateVisibility
 * Updates map object visibility info depending on the current view
 *******************************************************************/
void MapRenderer2D::updateVisibility(fpoint2_t view_tl, fpoint2_t view_br)
{
	// Sector visibility
	if (map->nSectors() != vis_s.size())
	{
		// Number of sectors changed, reset array
		vis_s.clear();
		for (unsigned a = 0; a < map->nSectors(); a++)
			vis_s.push_back(0);
	}
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		// Check against sector bounding box
		bbox_t bbox = map->getSector(a)->boundingBox();
		vis_s[a] = 0;
		if (bbox.max.x < view_tl.x) vis_s[a] = VIS_LEFT;
		if (bbox.max.y < view_tl.y) vis_s[a] = VIS_ABOVE;
		if (bbox.min.x > view_br.x) vis_s[a] = VIS_RIGHT;
		if (bbox.min.y > view_br.y) vis_s[a] = VIS_BELOW;

		// Check if the sector is worth drawing
		if ((bbox.max.x - bbox.min.x) * view_scale < 4 ||
				(bbox.max.y - bbox.min.y) * view_scale < 4)
			vis_s[a] = VIS_SMALL;
	}

	// Thing visibility
	if (map->nThings() != vis_t.size())
	{
		// Number of things changed, reset array
		vis_t.clear();
		for (unsigned a = 0; a < map->nThings(); a++)
			vis_t.push_back(0);
	}
	double x, y;
	double radius;
	for (unsigned a = 0; a < vis_t.size(); a++)
	{
		vis_t[a] = 0;
		x = map->getThing(a)->xPos();
		y = map->getThing(a)->yPos();

		// Get thing type properties from game configuration
		ThingType* tt = theGameConfiguration->thingType(map->getThing(a)->getType());
		radius = tt->getRadius() * 1.3;

		// Ignore if outside of screen
		if (x+radius < view_tl.x || x-radius > view_br.x || y+radius < view_tl.y || y-radius > view_br.y)
			vis_t[a] = 1;

		// Check if the thing is worth drawing
		else if (radius*view_scale < 2)
			vis_t[a] = VIS_SMALL;
	}
}

/* MapRenderer2D::forceUpdate
 * Updates all VBOs and other cached data
 *******************************************************************/
void MapRenderer2D::forceUpdate(float line_alpha)
{
	// Update variables
	this->view_scale_inv = 1.0 / view_scale;
	tex_flats.clear();
	thing_sprites.clear();
	thing_paths.clear();

	if (OpenGL::vboSupport())
	{
		updateVerticesVBO();
		updateLinesVBO(lines_dirs, line_alpha);
	}
	else
	{
		if (list_lines > 0)
		{
			glDeleteLists(list_lines, 1);
			list_lines = 0;
		}
		if (list_vertices > 0)
		{
			glDeleteLists(list_vertices, 1);
			list_vertices = 0;
		}
	}
	renderVertices(view_scale);
	renderLines(lines_dirs);
}

/* MapRenderer2D::scaledRadius
 * Returns [radius] scaled such that it stays the same size on screen
 * at all zoom levels
 *******************************************************************/
double MapRenderer2D::scaledRadius(int radius)
{
	if (radius > 16)
		radius = 16;

	if (view_scale > 1.0)
		return radius * view_scale_inv;
	else
		return (double)radius;
}

/* MapRenderer2D::visOK
 * Returns true if the current visibility info is valid
 *******************************************************************/
bool MapRenderer2D::visOK()
{
	if (map->nSectors() != vis_s.size() ||
			map->nThings() != vis_t.size())
		return false;
	else
		return true;
}
