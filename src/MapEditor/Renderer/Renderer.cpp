
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Renderer.cpp
 * Description: Renderer class - handles rendering/drawing
 *              functionality for the map editor
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
#include "App.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/Edit/LineDraw.h"
#include "MapEditor/MapEditContext.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "Overlays/MCOverlay.h"
#include "Renderer.h"
#include "Utility/MathStuff.h"

using namespace MapEditor;


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, things_always, 2, CVAR_SAVE)
CVAR(Int, vertices_always, 0, CVAR_SAVE)
CVAR(Bool, line_tabs_always, 1, CVAR_SAVE)
CVAR(Bool, flat_fade, 1, CVAR_SAVE)
CVAR(Bool, line_fade, 0, CVAR_SAVE)
CVAR(Bool, grid_dashed, false, CVAR_SAVE)
CVAR(Int, grid_64_style, 1, CVAR_SAVE)
CVAR(Bool, scroll_smooth, true, CVAR_SAVE)
CVAR(Bool, map_showfps, false, CVAR_SAVE)
CVAR(Bool, camera_3d_gravity, true, CVAR_SAVE)
CVAR(Int, camera_3d_crosshair_size, 6, CVAR_SAVE)
CVAR(Bool, camera_3d_show_distance, false, CVAR_SAVE)
CVAR(Bool, map_show_help, true, CVAR_SAVE)
CVAR(Int, map_crosshair, 0, CVAR_SAVE)
CVAR(Bool, map_show_selection_numbers, true, CVAR_SAVE)
CVAR(Int, map_max_selection_numbers, 1000, CVAR_SAVE)
CVAR(Int, flat_drawtype, 2, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Int, vertex_size)


/*******************************************************************
 * RENDERER CLASS FUNCTIONS
 *******************************************************************/

/* Renderer::Renderer
 * Renderer class constructor
 *******************************************************************/
Renderer::Renderer(MapEditContext& context) :
	context_{context},
	renderer_2d_{&context.map()},
	renderer_3d_{&context.map()},
	animations_active_{false},
	anim_view_speed_{0.05},
	fade_vertices_{1},
	fade_things_{1},
	fade_flats_{1},
	fade_lines_{1},
	anim_flash_level_{0.5},
	anim_flash_inc_{true},
	anim_info_fade_{0},
	anim_overlay_fade_{0},
	anim_help_fade_{0}
{
}

/* Renderer::forceUpdate
 * Updates/refreshes the 2d and 3d renderers
 *******************************************************************/
void Renderer::forceUpdate()
{
	renderer_2d_.forceUpdate(fade_lines_);
	renderer_3d_.clearData();
}

/* Renderer::setView
 * Scrolls the view to be centered on map coordinates [x,y]
 *******************************************************************/
void Renderer::setView(double map_x, double map_y)
{
	// Set new view
	view_.setOffset(map_x, map_y);

	// Update object visibility
	renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);
}

/* Renderer::setViewSize
 * Sets the view size to [width,height]
 *******************************************************************/
void Renderer::setViewSize(int width, int height)
{
	// Set new size
	view_.setSize(width, height);

	// Update object visibility
	renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);
}

/* Renderer::setTopY
 * Sets the view such that the map coordinate [y] is at the top
 * of the canvas (0)
 *******************************************************************/
void Renderer::setTopY(double map_y)
{
	setView(view_.offset().x, view_.offset().y - (view_.mapY(0) - map_y));
	view_.resetInter(false, true, false);
}

/* Renderer::pan
 * Scrolls the view relatively by [x,y]. If [scale] is true, [x,y]
 * will be scaled by the current view scale
 *******************************************************************/
void Renderer::pan(double x, double y, bool scale)
{
	if (scale)
	{
		x /= view().scale();
		y /= view().scale();
	}

	setView(view_.offset().x + x, view_.offset().y + y);
}

/* MapCanvas::zoom
 * Zooms the view by [amount]. If [towards_cursor] is true the view
 * will be zoomed towards the current mouse cursor position,
 * otherwise towards the center of the screen
 *******************************************************************/
void Renderer::zoom(double amount, bool toward_cursor)
{
	// Zoom view
	if (toward_cursor)
		view_.zoomToward(amount, context_.input().mousePos());
	else
		view_.zoom(amount);

	// Update object visibility
	renderer_2d_.setScale(view_.scale(true));
	renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);
}

/* Renderer::viewFitToMap
 * Centers the view to the center of the map, and zooms in or out so
 * that the entire map is showing
 *******************************************************************/
void Renderer::viewFitToMap(bool snap)
{
	// Fit the view to the map bbox
	view_.fitTo(context_.map().getMapBBox());

	// Don't animate if specified
	if (snap)
		view_.resetInter(true, true, true);

	// Update object visibility
	renderer_2d_.setScale(view_.scale(true));
	renderer_2d_.forceUpdate();
	renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);
}

/* Renderer::viewFitToObjects
 * Centers the view to the center of [objects], and zooms in or out
 * so that all [objects] are showing
 *******************************************************************/
void Renderer::viewFitToObjects(const vector<MapObject*>& objects)
{
	// Determine bbox of all given object(s)
	bbox_t bbox;
	for (auto object : objects)
	{
		// Vertex
		if (object->getObjType() == MOBJ_VERTEX)
		{
			auto vertex = (MapVertex*)object;
			bbox.extend(vertex->xPos(), vertex->yPos());
		}

		// Line
		else if (object->getObjType() == MOBJ_LINE)
		{
			auto line = (MapLine*)object;
			bbox.extend(line->v1()->xPos(), line->v1()->yPos());
			bbox.extend(line->v2()->xPos(), line->v2()->yPos());
		}

		// Sector
		else if (object->getObjType() == MOBJ_SECTOR)
		{
			auto sbb = ((MapSector*)object)->boundingBox();
			if (sbb.min.x < bbox.min.x)
				bbox.min.x = sbb.min.x;
			if (sbb.min.y < bbox.min.y)
				bbox.min.y = sbb.min.y;
			if (sbb.max.x > bbox.max.x)
				bbox.max.x = sbb.max.x;
			if (sbb.max.y > bbox.max.y)
				bbox.max.y = sbb.max.y;
		}

		// Thing
		else if (object->getObjType() == MOBJ_THING)
		{
			auto thing = (MapThing*)object;
			bbox.extend(thing->xPos(), thing->yPos());
		}
	}

	// Fit the view to the bbox
	view_.fitTo(bbox);

	// Update object visibility
	renderer_2d_.setScale(view_.scale(true));
	renderer_2d_.forceUpdate();
	renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);
}

/* Renderer::interpolateView
 * Interpolates the current 2d view if [smooth] is set, based on
 * [view_speed] and [mult]
 *******************************************************************/
double Renderer::interpolateView(bool smooth, double view_speed, double mult)
{
	auto anim_view_speed = view_speed;
	if (smooth)
	{
		auto mouse_pos = fpoint2_t{ context_.input().mousePos() };

		if (!view_.interpolate(mult * view_speed, &mouse_pos))
			anim_view_speed = 0.05;
		else
		{
			anim_view_speed += 0.05*mult;
			if (anim_view_speed > 0.4)
				anim_view_speed = 0.4;
		}
	}
	else
		view_.resetInter(true, true, true);

	return anim_view_speed;
}

/* Renderer::viewIsInterpolated
 * Returns true if the current view is interpolated
 *******************************************************************/
bool Renderer::viewIsInterpolated() const
{
	return (view_.scale(false) != view_.scale(true) ||
			view_.offset(false).x != view_.offset(true).x ||
			view_.offset(false).y != view_.offset(true).y);
}

/* Renderer::setCameraThing
 * Sets the 3d camera to match [thing]
 *******************************************************************/
void Renderer::setCameraThing(MapThing* thing)
{
	// Determine position
	fpoint3_t pos(thing->point(), 40);
	int sector = context_.map().sectorAt(thing->point());
	if (sector >= 0)
		pos.z += context_.map().getSector(sector)->getFloorHeight();

	// Set camera position & direction
	renderer_3d_.cameraSet(pos, MathStuff::vectorAngle(MathStuff::degToRad(thing->getAngle())));
}

/* Renderer::cameraPos2D
 * Returns the current 3d mode camera position in 2d
 *******************************************************************/
fpoint2_t Renderer::cameraPos2D()
{
	return { renderer_3d_.camPosition().x, renderer_3d_.camPosition().y };
}

/* Renderer::cameraDir2D
 * Returns the current 3d mode camera direction in 2d (no pitch)
 *******************************************************************/
fpoint2_t Renderer::cameraDir2D()
{
	return renderer_3d_.camDirection();
}

/* Renderer::drawGrid
 * Draws the grid
 *******************************************************************/
void Renderer::drawGrid() const
{
	// Get grid size
	int gridsize = context_.gridSize();

	// Disable line smoothing (not needed for straight, 1.0-sized lines)
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);

	// Enable dashed lines if needed
	if (grid_dashed)
	{
		glLineStipple(2, 0xAAAA);
		glEnable(GL_LINE_STIPPLE);
	}

	OpenGL::setColour(ColourConfiguration::getColour("map_grid"), false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Determine smallest grid size to bother drawing
	int grid_hidelevel = 2.0 / view_.scale();

	// Determine canvas edges in map coordinates
	int start_x = view_.mapX(0, true);
	int end_x = view_.mapX(view_.size().x, true);
	int start_y = view_.mapY(view_.size().y, true);
	int end_y = view_.mapY(0, true);

	// Draw regular grid if it's not too small
	if (gridsize > grid_hidelevel)
	{
		// Vertical
		int ofs = start_x % gridsize;
		for (int x = start_x - ofs; x <= end_x; x += gridsize)
		{
			glBegin(GL_LINES);
			glVertex2d(x, start_y);
			glVertex2d(x, end_y);
			glEnd();
		}

		// Horizontal
		ofs = start_y % gridsize;
		for (int y = start_y - ofs; y <= end_y; y += gridsize)
		{
			glBegin(GL_LINES);
			glVertex2d(start_x, y);
			glVertex2d(end_x, y);
			glEnd();
		}
	}

	// Disable dashed lines if 64 grid is set to crosses
	if (grid_64_style > 1)
		glDisable(GL_LINE_STIPPLE);

	// Draw 64 grid if it's not too small and we're not on a larger grid size
	if (64 > grid_hidelevel && gridsize < 64 && grid_64_style > 0)
	{
		int cross_size = 8;
		if (gridsize < cross_size)
			cross_size = gridsize;

		OpenGL::setColour(ColourConfiguration::getColour("map_64grid"));

		// Vertical
		int ofs = start_x % 64;
		for (int x = start_x - ofs; x <= end_x; x += 64)
		{
			glBegin(GL_LINES);

			if (grid_64_style > 1)
			{
				// Cross style
				int y = start_y - (start_y % 64);
				while (y < end_y)
				{
					glVertex2d(x, y - cross_size);
					glVertex2d(x, y + cross_size);
					y += 64;
				}
			}
			else
			{
				// Full style
				glVertex2d(x, start_y);
				glVertex2d(x, end_y);
			}

			glEnd();
		}

		// Horizontal
		ofs = start_y % 64;
		for (int y = start_y - ofs; y <= end_y; y += 64)
		{
			glBegin(GL_LINES);

			if (grid_64_style > 1)
			{
				// Cross style
				int x = start_x - (start_x % 64);
				while (x < end_x)
				{
					glVertex2d(x - cross_size, y);
					glVertex2d(x + cross_size, y);
					x += 64;
				}
			}
			else
			{
				// Full style
				glVertex2d(start_x, y);
				glVertex2d(end_x, y);
			}

			glEnd();
		}
	}

	glDisable(GL_LINE_STIPPLE);

	// Draw crosshair if needed
	if (map_crosshair > 0)
	{
		auto mouse_pos = context_.input().mousePos();
		double x = context_.snapToGrid(view_.mapX(mouse_pos.x), false);
		double y = context_.snapToGrid(view_.mapY(mouse_pos.y), false);
		auto col = ColourConfiguration::getColour("map_64grid");

		// Small
		glLineWidth(2.0f);
		if (map_crosshair == 1)
		{
			col = col.ampf(1.0f, 1.0f, 1.0f, 2.0f);
			auto col2 = col.ampf(1.0f, 1.0f, 1.0f, 0.0f);
			double size = context_.gridSize();
			double one = 1.0 / view_.scale(true);

			glBegin(GL_LINES);
			OpenGL::setColour(col, false);
			glVertex2d(x + one, y);
			OpenGL::setColour(col2, false);
			glVertex2d(x + size, y);

			OpenGL::setColour(col, false);
			glVertex2d(x - one, y);
			OpenGL::setColour(col2, false);
			glVertex2d(x - size, y);

			OpenGL::setColour(col, false);
			glVertex2d(x, y + one);
			OpenGL::setColour(col2, false);
			glVertex2d(x, y + size);

			OpenGL::setColour(col, false);
			glVertex2d(x, y - one);
			OpenGL::setColour(col2, false);
			glVertex2d(x, y - size);
			glEnd();
		}

		// Full
		else if (map_crosshair == 2)
		{
			OpenGL::setColour(col);

			glBegin(GL_LINES);
			glVertex2d(x, view_.mapBounds().tl.y);
			glVertex2d(x, view_.mapBounds().br.y);
			glVertex2d(view_.mapBounds().tl.x, y);
			glVertex2d(view_.mapBounds().br.x, y);
			glEnd();
		}
	}
}

/* Renderer::drawEditorMessages
 * Draws any currently showing editor messages
 *******************************************************************/
void Renderer::drawEditorMessages() const
{
	int yoff = 0;
	if (map_showfps) yoff = 16;
	auto col_fg = ColourConfiguration::getColour("map_editor_message");
	auto col_bg = ColourConfiguration::getColour("map_editor_message_outline");
	Drawing::setTextState(true);
	Drawing::enableTextStateReset(false);

	// Go through editor messages
	for (unsigned a = 0; a < context_.numEditorMessages(); a++)
	{
		// Check message time
		long time = context_.editorMessageTime(a);
		if (time > 2000)
			continue;

		// Setup message colour
		auto col = col_fg;
		if (time < 200)
		{
			float flash = 1.0f - (time / 200.0f);
			col.r += (255 - col.r)*flash;
			col.g += (255 - col.g)*flash;
			col.b += (255 - col.b)*flash;
		}

		// Setup message alpha
		col_bg.a = 255;
		if (time > 1500)
		{
			col.a = 255 - (double((time - 1500) / 500.0) * 255);
			col_bg.a = col.a;
		}

		// Draw message
		Drawing::setTextOutline(1.0f, col_bg);
		Drawing::drawText(context_.editorMessage(a), 0, yoff, col, Drawing::FONT_BOLD);

		yoff += 16;
	}
	Drawing::setTextOutline(0);
	Drawing::setTextState(false);
	Drawing::enableTextStateReset(true);
}

/* Renderer::drawFeatureHelpText
 * Draws any feature help text currently showing
 *******************************************************************/
void Renderer::drawFeatureHelpText() const
{
	// Check if any text
	auto& help_lines = context_.featureHelpLines();
	if (help_lines.empty() || !map_show_help)
		return;

	// Draw title
	frect_t bounds;
	auto col = ColourConfiguration::getColour("map_editor_message");
	auto col_bg = ColourConfiguration::getColour("map_editor_message_outline");
	col.a = col.a * anim_help_fade_;
	col_bg.a = col_bg.a * anim_help_fade_;
	Drawing::setTextOutline(1.0f, col_bg);
	Drawing::drawText(help_lines[0], view_.size().x - 2, 2, col, Drawing::FONT_BOLD, Drawing::ALIGN_RIGHT, &bounds);

	// Draw underline
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
	OpenGL::setColour(col);
	glBegin(GL_LINES);
	glVertex2d(bounds.right() + 8, bounds.bottom() + 1);
	glVertex2d(bounds.left() + 16, bounds.bottom() + 1);
	glVertex2d(bounds.left() + 16, bounds.bottom() + 1);
	glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
	glVertex2d(bounds.left() - 48, bounds.bottom() + 1);
	glEnd();

	// Draw help text
	int yoff = 22;
	Drawing::setTextState(true);
	Drawing::enableTextStateReset(false);
	for (unsigned a = 1; a < help_lines.size(); a++)
	{
		Drawing::drawText(help_lines[a], view_.size().x - 2, yoff, col, Drawing::FONT_BOLD, Drawing::ALIGN_RIGHT);
		yoff += 16;
	}
	Drawing::setTextOutline(0);
	Drawing::setTextState(false);
	Drawing::enableTextStateReset(true);
}

/* Renderer::drawSelectionNumbers
 * Draws numbers for selected map objects
 *******************************************************************/
void Renderer::drawSelectionNumbers() const
{
	// Check if any selection exists
	auto selection = context_.selection().selectedObjects();
	if (selection.size() == 0)
		return;

	// Get editor message text colour
	auto col = ColourConfiguration::getColour("map_editor_message");

	// Go through selection
	string text;
	Drawing::enableTextStateReset(false);
	Drawing::setTextState(true);
	view_.setOverlayCoords(true);
#if USE_SFML_RENDERWINDOW && ((SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR >= 4) || SFML_VERSION_MAJOR > 2)
	Drawing::setTextOutline(1.0f, COL_BLACK);
#else
	if (context_.selection().size() <= map_max_selection_numbers * 0.5)
		Drawing::setTextOutline(1.0f, COL_BLACK);
#endif
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if ((int)a > map_max_selection_numbers)
			break;

		auto tp = selection[a]->getPoint(MOBJ_POINT_TEXT);
		tp.x = view_.screenX(tp.x);
		tp.y = view_.screenY(tp.y);

		text = S_FMT("%d", a + 1);
		auto ts = Drawing::textExtents(text, Drawing::FONT_BOLD);
		tp.x -= ts.x * 0.5;
		tp.y -= ts.y * 0.5;

		if (context_.editMode() == Mode::Vertices)
		{
			tp.x += 8;
			tp.y += 8;
		}

		// Draw text
		Drawing::drawText(S_FMT("%d", a + 1), tp.x, tp.y, col, Drawing::FONT_BOLD);
	}
	Drawing::setTextOutline(0);
	Drawing::enableTextStateReset();
	Drawing::setTextState(false);
	view_.setOverlayCoords(false);

	glDisable(GL_TEXTURE_2D);
}

/* Renderer::drawThingQuickAngleLines
 * Draws directional lines for thing quick angle selection
 *******************************************************************/
void Renderer::drawThingQuickAngleLines() const
{
	// Check if any selection exists
	auto selection = context_.selection().selectedThings();
	if (selection.size() == 0)
		return;

	// Get moving colour
	auto col = ColourConfiguration::getColour("map_moving");
	OpenGL::setColour(col);

	// Draw lines
	auto mouse_pos_m = view_.mapPos(context_.input().mousePos(), true);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		glVertex2d(selection[a]->xPos(), selection[a]->yPos());
		glVertex2d(mouse_pos_m.x, mouse_pos_m.y);
	}
	glEnd();
}

/* Renderer::drawLineLength
 * Draws text showing the length from [p1] to [p2]
 *******************************************************************/
void Renderer::drawLineLength(fpoint2_t p1, fpoint2_t p2, rgba_t col) const
{
	// Determine distance in screen scale
	double tdist = 20 / view_.scale(true);

	// Determine line midpoint and front vector
	fpoint2_t mid(p1.x + (p2.x - p1.x) * 0.5, p1.y + (p2.y - p1.y) * 0.5);
	fpoint2_t vec(-(p2.y - p1.y), p2.x - p1.x);
	vec.normalize();

	// Determine point to place the text
	fpoint2_t tp(mid.x + (vec.x * tdist), mid.y + (vec.y * tdist));

	// Determine text half-height for vertical alignment
	string length = S_FMT("%d", MathStuff::round(MathStuff::distance(p1, p2)));
	double hh = Drawing::textExtents(length).y * 0.5;

	// Draw text
	Drawing::drawText(
		length,
		view_.screenX(tp.x),
		view_.screenY(tp.y) - hh,
		col,
		Drawing::FONT_NORMAL,
		Drawing::ALIGN_CENTER
	);
	glDisable(GL_TEXTURE_2D);
}

/* Renderer::drawLineDrawLines
 * Draws current line drawing lines (best function name ever)
 *******************************************************************/
void Renderer::drawLineDrawLines(bool snap_nearest_vertex) const
{
	// Get line draw colour
	auto col = ColourConfiguration::getColour("map_linedraw");
	OpenGL::setColour(col);

	// Determine end point
	auto end = view_.mapPos(context_.input().mousePos(), true);
	if (snap_nearest_vertex)
	{
		// If shift is held down, snap to the nearest vertex (if any)
		int vertex = context_.map().nearestVertex(end);
		if (vertex >= 0)
		{
			end.x = context_.map().getVertex(vertex)->xPos();
			end.y = context_.map().getVertex(vertex)->yPos();
		}
		else if (context_.gridSnap())
		{
			// No nearest vertex, snap to grid if needed
			end.x = context_.snapToGrid(end.x);
			end.y = context_.snapToGrid(end.y);
		}
	}
	else if (context_.gridSnap())
	{
		// Otherwise, snap to grid if needed
		end.x = context_.snapToGrid(end.x);
		end.y = context_.snapToGrid(end.y);
	}

	// Draw lines
	auto& line_draw = context_.lineDraw();
	int npoints = line_draw.nPoints();
	glLineWidth(2.0f);
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
			Drawing::drawLineTabbed(line_draw.point(a), line_draw.point(a + 1));
	}
	if (npoints > 0 && context_.lineDraw().state() == LineDraw::State::Line)
		Drawing::drawLineTabbed(line_draw.point(npoints - 1), end);

	// Draw line lengths
	view_.setOverlayCoords(true);
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
			drawLineLength(line_draw.point(a), line_draw.point(a + 1), col);
	}
	if (npoints > 0 && context_.lineDraw().state() == LineDraw::State::Line)
		drawLineLength(line_draw.point(npoints - 1), end, col);
	view_.setOverlayCoords(false);

	// Draw points
	glPointSize(vertex_size);
	if (vertex_round) glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
	for (auto& point : line_draw.points())
		glVertex2d(point.x, point.y);
	if (context_.lineDraw().state() == LineDraw::State::Line ||
		context_.lineDraw().state() == LineDraw::State::ShapeOrigin)
		glVertex2d(end.x, end.y);
	glEnd();
}

/* Renderer::drawPasteLines
 * Draws lines currently being pasted
 *******************************************************************/
void Renderer::drawPasteLines() const
{
	// Get clipboard item
	MapArchClipboardItem* c = nullptr;
	for (unsigned a = 0; a < theClipboard->nItems(); a++)
	{
		if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH)
		{
			c = (MapArchClipboardItem*)theClipboard->getItem(a);
			break;
		}
	}

	if (!c)
		return;

	// Get lines
	vector<MapLine*> lines;
	c->getLines(lines);

	// Get line draw colour
	auto col = ColourConfiguration::getColour("map_linedraw");
	OpenGL::setColour(col);

	// Draw
	auto pos = context_.relativeSnapToGrid(c->getMidpoint(), view_.mapPos(context_.input().mousePos(), true));
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		glVertex2d(pos.x + lines[a]->x1(), pos.y + lines[a]->y1());
		glVertex2d(pos.x + lines[a]->x2(), pos.y + lines[a]->y2());
	}
	glEnd();
}

/* Renderer::drawObjectEdit
 * Draws object edit objects, bounding box and text
 *******************************************************************/
void Renderer::drawObjectEdit()
{
	auto& group = context_.objectEdit().group();
	auto col = ColourConfiguration::getColour("map_object_edit");
	auto edit_state = context_.objectEdit().state();

	// Map objects
	renderer_2d_.renderObjectEditGroup(&group);

	// Bounding box
	OpenGL::setColour(COL_WHITE);
	glColor4f(col.fr(), col.fg(), col.fb(), 1.0f);
	auto bbox = group.getBBox();
	bbox.min.x -= 4 / view_.scale(true);
	bbox.min.y -= 4 / view_.scale(true);
	bbox.max.x += 4 / view_.scale(true);
	bbox.max.y += 4 / view_.scale(true);

	if (context_.objectEdit().rotating())
	{
		// Rotate

		// Bbox
		fpoint2_t mid(bbox.min.x + bbox.width() * 0.5, bbox.min.y + bbox.height() * 0.5);
		auto bl = MathStuff::rotatePoint(mid, bbox.min, group.getRotation());
		auto tl = MathStuff::rotatePoint(mid, fpoint2_t(bbox.min.x, bbox.max.y), group.getRotation());
		auto tr = MathStuff::rotatePoint(mid, bbox.max, group.getRotation());
		auto br = MathStuff::rotatePoint(mid, fpoint2_t(bbox.max.x, bbox.min.y), group.getRotation());
		glLineWidth(2.0f);
		Drawing::drawLine(tl, bl);
		Drawing::drawLine(bl, br);
		Drawing::drawLine(br, tr);
		Drawing::drawLine(tr, tl);

		// Top Left
		double rad = 4 / view_.scale(true);
		glLineWidth(1.0f);
		if (edit_state == ObjectEdit::State::TopLeft)
			Drawing::drawFilledRect(tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad);
		else
			Drawing::drawRect(tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad);

		// Bottom Left
		if (edit_state == ObjectEdit::State::BottomLeft)
			Drawing::drawFilledRect(bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad);
		else
			Drawing::drawRect(bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad);

		// Top Right
		if (edit_state == ObjectEdit::State::TopRight)
			Drawing::drawFilledRect(tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad);
		else
			Drawing::drawRect(tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad);

		// Bottom Right
		if (edit_state == ObjectEdit::State::BottomRight)
			Drawing::drawFilledRect(br.x - rad, br.y - rad, br.x + rad, br.y + rad);
		else
			Drawing::drawRect(br.x - rad, br.y - rad, br.x + rad, br.y + rad);
	}
	else
	{
		// Move/scale

		// Left
		if (edit_state == ObjectEdit::State::Move ||
			edit_state == ObjectEdit::State::Left ||
			edit_state == ObjectEdit::State::TopLeft ||
			edit_state == ObjectEdit::State::BottomLeft)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.min.x, bbox.min.y, bbox.min.x, bbox.max.y);

		// Bottom
		if (edit_state == ObjectEdit::State::Move ||
			edit_state == ObjectEdit::State::Bottom ||
			edit_state == ObjectEdit::State::BottomLeft ||
			edit_state == ObjectEdit::State::BottomRight)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.min.x, bbox.min.y, bbox.max.x, bbox.min.y);

		// Right
		if (edit_state == ObjectEdit::State::Move ||
			edit_state == ObjectEdit::State::Right ||
			edit_state == ObjectEdit::State::TopRight ||
			edit_state == ObjectEdit::State::BottomRight)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.max.x, bbox.max.y, bbox.max.x, bbox.min.y);

		// Top
		if (edit_state == ObjectEdit::State::Move ||
			edit_state == ObjectEdit::State::Top ||
			edit_state == ObjectEdit::State::TopLeft ||
			edit_state == ObjectEdit::State::TopRight)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.max.x, bbox.max.y, bbox.min.x, bbox.max.y);
	}

	// Line length
	fpoint2_t nl_v1, nl_v2;
	if (group.getNearestLine(
		view_.mapPos(context_.input().mousePos()),
		128 / view_.scale(),
		nl_v1,
		nl_v2))
	{
		fpoint2_t mid(nl_v1.x + ((nl_v2.x - nl_v1.x) * 0.5), nl_v1.y + ((nl_v2.y - nl_v1.y) * 0.5));
		int length = MathStuff::distance(nl_v1, nl_v2);
		int x = view_.mapX(mid.x);
		int y = view_.mapY(mid.y) - 8;
		view_.setOverlayCoords(true);
		Drawing::setTextOutline(1.0f, COL_BLACK);
		Drawing::drawText(S_FMT("%d", length), x, y, COL_WHITE, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
		Drawing::setTextOutline(0);
		view_.setOverlayCoords(false);
		glDisable(GL_TEXTURE_2D);
	}
}

/* Renderer::drawAnimations
 * Draws all MCAnimations for the current edit mode
 *******************************************************************/
void Renderer::drawAnimations() const
{
	auto mode = context_.editMode();
	for (auto& animation : animations_)
	{
		if ((mode == Mode::Visual && animation->mode3d()) ||
			(mode != Mode::Visual && !animation->mode3d()))
			animation->draw();
	}
}

/* Renderer::drawMap2d
 * Draws the 2d map
 *******************************************************************/
void Renderer::drawMap2d()
{
	// Apply the current 2d view
	view_.apply();

	// Update visibility info if needed
	if (!renderer_2d_.visOK())
		renderer_2d_.updateVisibility(view_.mapBounds().tl, view_.mapBounds().br);

	// Draw flats if needed
	OpenGL::setColour(COL_WHITE);
	if (flat_drawtype > 0)
	{
		bool texture = false;
		if (flat_drawtype > 1)
			texture = true;

		// Adjust flat type depending on sector mode
		int drawtype = 0;
		if (context_.editMode() == Mode::Sectors)
		{
			if (context_.sectorEditMode() == SectorMode::Floor)
				drawtype = 1;
			else if (context_.sectorEditMode() == SectorMode::Ceiling)
				drawtype = 2;
		}

		renderer_2d_.renderFlats(drawtype, texture, fade_flats_);
	}

	// Draw grid
	drawGrid();

	// --- Draw map (depending on mode) ---
	auto mouse_state = context_.input().mouseState();
	OpenGL::resetBlend();
	if (context_.editMode() == Mode::Vertices)
	{
		// Vertices mode
		renderer_2d_.renderThings(fade_things_);					// Things
		renderer_2d_.renderLines(line_tabs_always, fade_lines_);	// Lines

		// Vertices
		if (mouse_state == Input::MouseState::Move)
			renderer_2d_.renderVertices(0.25f);
		else
			renderer_2d_.renderVertices(fade_vertices_);

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!context_.overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_.renderVertexSelection(context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_.renderVertexHilight(context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Lines)
	{
		// Lines mode
		renderer_2d_.renderThings(fade_things_);		// Things
		renderer_2d_.renderVertices(fade_vertices_);	// Vertices
		renderer_2d_.renderLines(true);					// Lines
		
		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!context_.overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_.renderLineSelection(context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_.renderLineHilight(context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Sectors)
	{
		// Sectors mode
		renderer_2d_.renderThings(fade_things_);					// Things
		renderer_2d_.renderVertices(fade_vertices_);				// Vertices
		renderer_2d_.renderLines(line_tabs_always, fade_lines_);	// Lines

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!context_.overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_.renderFlatSelection(context_.selection(), anim_flash_level_);

		//splitter.testRender();	// Testing

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_.renderFlatHilight(context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Things)
	{
		// Check if we should force thing angles visible
		bool force_dir = false;
		if (mouse_state == Input::MouseState::ThingAngle)
			force_dir = true;

		// Things mode
		renderer_2d_.renderVertices(fade_vertices_);				// Vertices
		renderer_2d_.renderLines(line_tabs_always, fade_lines_);	// Lines
		renderer_2d_.renderThings(fade_things_, force_dir);			// Things

		// Thing paths
		renderer_2d_.renderPathedThings(context_.pathedThings());

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!context_.overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_.renderThingSelection(context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_.renderThingHilight(context_.hilightItem().index, anim_flash_level_);
	}

	// Draw tagged sectors/lines/things if needed
	if (!context_.overlayActive() &&
		(mouse_state == Input::MouseState::Normal ||
		mouse_state == Input::MouseState::TagSectors ||
		mouse_state == Input::MouseState::TagThings))
	{
		if (context_.taggedSectors().size() > 0)
			renderer_2d_.renderTaggedFlats(context_.taggedSectors(), anim_flash_level_);
		if (context_.taggedLines().size() > 0)
			renderer_2d_.renderTaggedLines(context_.taggedLines(), anim_flash_level_);
		if (context_.taggedThings().size() > 0)
			renderer_2d_.renderTaggedThings(context_.taggedThings(), anim_flash_level_);
		if (context_.taggingLines().size() > 0)
			renderer_2d_.renderTaggingLines(context_.taggingLines(), anim_flash_level_);
		if (context_.taggingThings().size() > 0)
			renderer_2d_.renderTaggingThings(context_.taggingThings(), anim_flash_level_);
	}

	// Draw selection numbers if needed
	if (context_.selection().size() > 0 && mouse_state == Input::MouseState::Normal && map_show_selection_numbers)
		drawSelectionNumbers();

	// Draw thing quick angle lines if needed
	if (mouse_state == Input::MouseState::ThingAngle)
		drawThingQuickAngleLines();

	// Draw line drawing lines if needed
	if (mouse_state == Input::MouseState::LineDraw)
		drawLineDrawLines(context_.input().shiftDown());

	// Draw object edit objects if needed
	if (mouse_state == Input::MouseState::ObjectEdit)
		drawObjectEdit();

	// Draw sectorbuilder test stuff
	//sbuilder.drawResult();


	// Draw selection box if active
	auto mx = view_.mapX(context_.input().mousePos().x, true);
	auto my = view_.mapY(context_.input().mousePos().y, true);
	auto mdx = view_.mapX(context_.input().mouseDownPos().x, true);
	auto mdy = view_.mapY(context_.input().mouseDownPos().y, true);
	if (mouse_state == Input::MouseState::Selection)
	{
		// Outline
		OpenGL::setColour(ColourConfiguration::getColour("map_selbox_outline"));
		glLineWidth(2.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2d(mdx, mdy);
		glVertex2d(mdx, my);
		glVertex2d(mx, my);
		glVertex2d(mx, mdy);
		glEnd();

		// Fill
		OpenGL::setColour(ColourConfiguration::getColour("map_selbox_fill"));
		glBegin(GL_QUADS);
		glVertex2d(mdx, mdy);
		glVertex2d(mdx, my);
		glVertex2d(mx, my);
		glVertex2d(mx, mdy);
		glEnd();
	}

	// Draw animations
	drawAnimations();

	// Draw paste objects if needed
	if (mouse_state == Input::MouseState::Paste)
	{
		if (context_.editMode() == Mode::Things)
		{
			// Get clipboard item
			for (unsigned a = 0; a < theClipboard->nItems(); a++)
			{
				auto item = theClipboard->getItem(a);
				if (item->getType() == CLIPBOARD_MAP_THINGS)
				{
					vector<MapThing*> things;
					auto p = (MapThingsClipboardItem*)item;
					p->getThings(things);
					auto pos(context_.relativeSnapToGrid(p->getMidpoint(), { mx, my }));
					renderer_2d_.renderPasteThings(things, pos);
				}
			}
		}
		else
			drawPasteLines();
	}

	// Draw moving stuff if needed
	if (mouse_state == Input::MouseState::Move)
	{
		auto& items = context_.moveObjects().items();
		auto offset = context_.moveObjects().offset();
		switch (context_.editMode())
		{
		case Mode::Vertices:
			renderer_2d_.renderMovingVertices(items, offset); break;
		case Mode::Lines:
			renderer_2d_.renderMovingLines(items, offset); break;
		case Mode::Sectors:
			renderer_2d_.renderMovingSectors(items, offset); break;
		case Mode::Things:
			renderer_2d_.renderMovingThings(items, offset); break;
		default: break;
		};
	}
}

/* Renderer::drawMap3d
 * Draws the 3d map
 *******************************************************************/
void Renderer::drawMap3d()
{
	// Setup 3d renderer view
	renderer_3d_.setupView(view_.size().x, view_.size().y);

	// Render 3d map
	renderer_3d_.renderMap();

	// Draw selection if any
	auto selection = context_.selection();
	renderer_3d_.renderFlatSelection(selection);
	renderer_3d_.renderWallSelection(selection);
	renderer_3d_.renderThingSelection(selection);

	// Draw hilight if any
	if (context_.selection().hasHilight())
		renderer_3d_.renderHilight(context_.selection().hilight(), anim_flash_level_);

	// Draw animations
	drawAnimations();
}

/* Renderer::draw
 * Draws the current map editor state
 *******************************************************************/
void Renderer::draw()
{
	// Setup the viewport
	glViewport(0, 0, view_.size().x, view_.size().y);

	// Setup GL state
	rgba_t col_bg = ColourConfiguration::getColour("map_background");
	glClearColor(col_bg.fr(), col_bg.fg(), col_bg.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisable(GL_TEXTURE_2D);

	// Draw 2d or 3d map depending on mode
	if (context_.editMode() == Mode::Visual)
		drawMap3d();
	else
		drawMap2d();

	// Draw info overlay
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, view_.size().x, view_.size().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implemenataions)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw current info overlay
	glDisable(GL_TEXTURE_2D);
	context_.drawInfoOverlay(view_.size(), anim_info_fade_);

	// Draw current fullscreen overlay
	if (context_.currentOverlay())
		context_.currentOverlay()->draw(view_.size().x, view_.size().y, anim_overlay_fade_);

	// Draw crosshair if 3d mode
	if (context_.editMode() == Mode::Visual)
	{
		// Get crosshair colour
		rgba_t col = ColourConfiguration::getColour("map_3d_crosshair");
		OpenGL::setColour(col);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.5f);

		double midx = view_.size().x * 0.5;
		double midy = view_.size().y * 0.5;
		int size = camera_3d_crosshair_size;

		glBegin(GL_LINES);
		// Right
		OpenGL::setColour(col, false);
		glVertex2d(midx + 1, midy);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx + size, midy);

		// Left
		OpenGL::setColour(col, false);
		glVertex2d(midx - 1, midy);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx - size, midy);

		// Bottom
		OpenGL::setColour(col, false);
		glVertex2d(midx, midy + 1);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx, midy + size);

		// Top
		OpenGL::setColour(col, false);
		glVertex2d(midx, midy - 1);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx, midy - size);
		glEnd();

		// Draw item distance (if any)
		if (context_.renderer().renderer3D().itemDistance() >= 0 && camera_3d_show_distance)
		{
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(col);
			Drawing::drawText(
				S_FMT("%d", renderer_3d_.itemDistance()),
				midx + 5,
				midy + 5,
				rgba_t(255, 255, 255, 200),
				Drawing::FONT_SMALL
			);
		}
	}

	// FPS counter
	/*if (map_showfps)
	{
		glEnable(GL_TEXTURE_2D);
		if (frametime_last > 0)
		{
			int fps = MathStuff::round(1.0 / (frametime_last / 1000.0));
			fps_avg.push_back(fps);
			if (fps_avg.size() > 20) fps_avg.erase(fps_avg.begin());
		}
		int afps = 0;
		for (unsigned a = 0; a < fps_avg.size(); a++)
			afps += fps_avg[a];
		if (fps_avg.size() > 0) afps /= fps_avg.size();
		Drawing::drawText(S_FMT("FPS: %d", afps));
	}*/

	// test
	//Drawing::drawText(S_FMT("Render distance: %1.2f", (double)render_max_dist), 0, 100);

	// Editor messages
	drawEditorMessages();

	// Help text
	drawFeatureHelpText();
}

namespace
{
	// Helper function for updateAnimations, changes [fade_var] by [amount]
	// and returns true if [fade_var] did not hit [min] or [max]
	bool updateFade(float& fade_var, float amount, float min, float max)
	{
		fade_var += amount;
		if (fade_var > max)
		{
			fade_var = max;
			return false;
		}
		else if (fade_var < min)
		{
			fade_var = min;
			return false;
		}

		return true;
	}
}

/* Renderer::updateAnimations
 * Updates all currently active animations
 *******************************************************************/
void Renderer::updateAnimations(double mult)
{
	animations_active_ = false;

	// Update MCAnimations
	for (unsigned a = 0; a < animations_.size(); a++)
	{
		if (!animations_[a]->update(App::runTimer()))
		{
			// If animation is finished, remove it from the list
			animations_.erase(animations_.begin() + a);
			a--;
		}
		else
			animations_active_ = true;
	}

	// 2d mode animation
	if (context_.editMode() != Mode::Visual)
	{
		// Update 2d mode crossfade animation
		if (update2dModeCrossfade(mult))
			animations_active_ = true;

		// View pan/zoom animation
		anim_view_speed_ = interpolateView(scroll_smooth, anim_view_speed_, mult);
		if (viewIsInterpolated())
			animations_active_ = true;

		// Update renderer scale
		renderer_2d_.setScale(view_.scale(true));
	}

	// Flashing animation for hilight
	// Pulsates between 0.5-1.0f (multiplied with hilight alpha)
	if (anim_flash_inc_)
	{
		if (anim_flash_level_ < 0.5f)
			anim_flash_level_ += 0.053*mult;	// Initial fade in
		else
			anim_flash_level_ += 0.015f*mult;
		if (anim_flash_level_ >= 1.0f)
		{
			anim_flash_inc_ = false;
			anim_flash_level_ = 1.0f;
		}
	}
	else
	{
		anim_flash_level_ -= 0.015f*mult;
		if (anim_flash_level_ <= 0.5f)
		{
			anim_flash_inc_ = true;
			anim_flash_level_ = 0.6f;
		}
	}

	// Fader for info overlay
	if (context_.infoOverlayActive() && !context_.overlayActive())
	{
		if (updateFade(anim_info_fade_, 0.1f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_info_fade_, -0.04f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}

	// Fader for fullscreen overlay
	if (context_.overlayActive())
	{
		if (updateFade(anim_overlay_fade_, 0.1f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_overlay_fade_, -0.05f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}

	// Fader for help text
	if (!context_.featureHelpLines().empty())
	{
		if (updateFade(anim_help_fade_, 0.07f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_help_fade_, -0.05f*mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
}

/* Renderer::update2dModeCrossfade
 * Updates the 2d mode crossfade animations (when switching modes)
 *******************************************************************/
bool Renderer::update2dModeCrossfade(double mult)
{
	// --- Fade map objects depending on mode ---

	// Determine fade levels
	float fa_vertices, fa_lines, fa_flats, fa_things;

	// Vertices
	if (vertices_always == 0)		fa_vertices = 0.0f;
	else if (vertices_always == 1)	fa_vertices = 1.0f;
	else							fa_vertices = 0.5f;

	// Things
	if (things_always == 0)			fa_things = 0.0f;
	else if (things_always == 1)	fa_things = 1.0f;
	else							fa_things = 0.5f;

	// Lines
	if (line_fade)	fa_lines = 0.5f;
	else			fa_lines = 1.0f;

	// Flats
	if (flat_fade)	fa_flats = 0.7f;
	else			fa_flats = 1.0f;

	// Interpolate
	bool anim_mode_crossfade = false;
	float mcs_speed = 0.08f;
	if (context_.editMode() == Mode::Vertices)
	{
		if (fade_vertices_ < 1.0f) { fade_vertices_ += mcs_speed*(1.0f - fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines_ = fa_lines;
		if (fade_flats_ > fa_flats) { fade_flats_ -= mcs_speed*(1.0f - fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things_ > fa_things) { fade_things_ -= mcs_speed*(1.0f - fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (context_.editMode() == Mode::Lines)
	{
		if (fade_vertices_ > fa_vertices) { fade_vertices_ -= mcs_speed*(1.0f - fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines_ = 1.0f;
		if (fade_flats_ > fa_flats) { fade_flats_ -= mcs_speed*(1.0f - fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things_ > fa_things) { fade_things_ -= mcs_speed*(1.0f - fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (context_.editMode() == Mode::Sectors)
	{
		if (fade_vertices_ > fa_vertices) { fade_vertices_ -= mcs_speed*(1.0f - fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines_ = fa_lines;
		if (fade_flats_ < 1.0f) { fade_flats_ += mcs_speed*(1.0f - fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things_ > fa_things) { fade_things_ -= mcs_speed*(1.0f - fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (context_.editMode() == Mode::Things)
	{
		if (fade_vertices_ > fa_vertices) { fade_vertices_ -= mcs_speed*(1.0f - fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines_ = fa_lines;
		if (fade_flats_ > fa_flats) { fade_flats_ -= mcs_speed*(1.0f - fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things_ < 1.0f) { fade_things_ += mcs_speed*(1.0f - fa_things)*mult; anim_mode_crossfade = true; }
	}

	// Clamp
	if (fade_vertices_ < fa_vertices) fade_vertices_ = fa_vertices;
	if (fade_vertices_ > 1.0f) fade_vertices_ = 1.0f;
	if (fade_lines_ < fa_lines) fade_lines_ = fa_lines;
	if (fade_lines_ > 1.0f) fade_lines_ = 1.0f;
	if (fade_flats_ < fa_flats) fade_flats_ = fa_flats;
	if (fade_flats_ > 1.0f) fade_flats_ = 1.0f;
	if (fade_things_ < fa_things) fade_things_ = fa_things;
	if (fade_things_ > 1.0f) fade_things_ = 1.0f;

	return anim_mode_crossfade;
}

/* Renderer::animateSelectionChange
 * Animates the (de)selection of [item], depending on [selected]
 *******************************************************************/
void Renderer::animateSelectionChange(const MapEditor::Item& item, bool selected)
{
	using MapEditor::ItemType;

	// 3d mode wall
	if (MapEditor::baseItemType(item.type) == ItemType::Side)
	{
		// Get quad
		auto quad = renderer_3d_.getQuad(item);

		if (quad)
		{
			// Get quad points
			fpoint3_t points[4];
			for (unsigned a = 0; a < 4; a++)
				points[a].set(quad->points[a].x, quad->points[a].y, quad->points[a].z);

			// Start animation
			animations_.push_back(std::make_unique<MCA3dWallSelection>(App::runTimer(), points, selected));
		}
	}

	// 3d mode flat
	else if (item.type == ItemType::Ceiling || item.type == ItemType::Floor)
	{
		// Get flat
		auto flat = renderer_3d_.getFlat(item);

		// Start animation
		if (flat)
			animations_.push_back(
				std::make_unique<MCA3dFlatSelection>(
					App::runTimer(),
					flat->sector,
					flat->plane,
					selected
				)
			);
	}

	// 2d mode thing
	else if (item.type == ItemType::Thing)
	{
		// Get thing
		auto t = context_.map().getThing(item.index);
		if (!t) return;

		// Get thing type
		auto& tt = Game::configuration().thingType(t->getType());

		// Start animation
		double radius = tt.radius();
		if (tt.shrinkOnZoom()) radius = renderer_2d_.scaledRadius(radius);
		animations_.push_back(
			std::make_unique<MCAThingSelection>(
				App::runTimer(),
				t->xPos(),
				t->yPos(),
				radius,
				renderer_2d_.viewScaleInv(),
				selected
			)
		);
	}

	// 2d mode line
	else if (item.type == ItemType::Line)
	{
		// Get line
		vector<MapLine*> vec;
		vec.push_back(context_.map().getLine(item.index));

		// Start animation
		animations_.push_back(std::make_unique<MCALineSelection>(App::runTimer(), vec, selected));
	}

	// 2d mode vertex
	else if (item.type == ItemType::Vertex)
	{
		// Get vertex
		vector<MapVertex*> verts;
		verts.push_back(context_.map().getVertex(item.index));

		// Determine current vertex size
		float vs = vertex_size;
		if (view_.scale() < 1.0) vs *= view_.scale();
		if (vs < 2.0) vs = 2.0;

		// Start animation
		animations_.push_back(std::make_unique<MCAVertexSelection>(App::runTimer(), verts, vs, selected));
	}

	// 2d mode sector
	else if (item.type == ItemType::Sector)
	{
		// Get sector polygon
		vector<Polygon2D*> polys;
		polys.push_back(context_.map().getSector(item.index)->getPolygon());

		// Start animation
		animations_.push_back(std::make_unique<MCASectorSelection>(App::runTimer(), polys, selected));
	}
}

/* Renderer::animateSelectionChange
 * Animates the last selection change from [selection]
 *******************************************************************/
void Renderer::animateSelectionChange(const ItemSelection &selection)
{
	for (auto& change : selection.lastChange())
		animateSelectionChange(change.first, change.second);
}

/* Renderer::animateHilightChange
 * Animates a hilight change from [old_item] (3d mode) or
 * [old_object] (2d mode)
 *******************************************************************/
void Renderer::animateHilightChange(const MapEditor::Item& old_item, MapObject* old_object)
{
	if (old_object)
	{
		// 2d mode
		animations_.push_back(
			std::make_unique<MCAHilightFade>(
				App::runTimer(),
				old_object,
				&renderer_2d_,
				anim_flash_level_
				));
	}
	else
	{
		// 3d mode
		animations_.push_back(
			std::make_unique<MCAHilightFade3D>(
				App::runTimer(),
				old_item.index,
				old_item.type,
				&renderer_3d_,
				anim_flash_level_
				));
	}

	// Reset hilight flash
	anim_flash_inc_ = true;
	anim_flash_level_ = 0.f;
}

/* Renderer::addAnimation
 * Adds [animation] to the list of active animations
 *******************************************************************/
void Renderer::addAnimation(std::unique_ptr<MCAnimation> animation)
{
	animations_.push_back(std::move(animation));
}
