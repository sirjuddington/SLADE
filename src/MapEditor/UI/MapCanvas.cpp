
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapCanvas.cpp
 * Description: MapCanvas class, the OpenGL canvas widget that the
 *              2d/3d map view is drawn on
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
#include "MapCanvas.h"
#include "Archive/ArchiveManager.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/UndoRedo.h"
#include "MainApp.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapEditorWindow.h"
#include "MapEditor/Renderer/MapRenderer2D.h"
#include "MapEditor/Renderer/MapRenderer3D.h"
#include "MapEditor/Renderer/MCAnimations.h"
#include "MapEditor/Renderer/Overlays/LineTextureOverlay.h"
#include "MapEditor/Renderer/Overlays/QuickTextureOverlay3d.h"
#include "MapEditor/Renderer/Overlays/SectorTextureOverlay.h"
#include "MapEditor/SectorBuilder.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ShowItemDialog.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapEditor/UI/PropsPanel/LinePropsPanel.h"
#include "MapEditor/UI/PropsPanel/MapObjectPropsPanel.h"
#include "MapEditor/UI/PropsPanel/SectorPropsPanel.h"
#include "MapEditor/UI/PropsPanel/ThingPropsPanel.h"
#include "ObjectEditPanel.h"
#include "OpenGL/Drawing.h"
#include "UI/SDialog.h"
#include "Utility/MathStuff.h"
#undef None


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, things_always, 2, CVAR_SAVE)
CVAR(Int, vertices_always, 0, CVAR_SAVE)
CVAR(Bool, line_tabs_always, 1, CVAR_SAVE)
CVAR(Bool, flat_fade, 1, CVAR_SAVE)
CVAR(Bool, line_fade, 0, CVAR_SAVE)
CVAR(Bool, grid_dashed, false, CVAR_SAVE)
CVAR(Bool, scroll_smooth, true, CVAR_SAVE)
CVAR(Int, flat_drawtype, 2, CVAR_SAVE)
CVAR(Bool, selection_clear_click, false, CVAR_SAVE)
CVAR(Bool, property_edit_dclick, true, CVAR_SAVE)
CVAR(Bool, map_showfps, false, CVAR_SAVE)
CVAR(Bool, camera_3d_gravity, true, CVAR_SAVE)
CVAR(Int, camera_3d_crosshair_size, 6, CVAR_SAVE)
CVAR(Bool, camera_3d_show_distance, false, CVAR_SAVE)
CVAR(Int, map_bg_ms, 15, CVAR_SAVE)
CVAR(Bool, info_overlay_3d, true, CVAR_SAVE)
CVAR(Bool, hilight_smooth, true, CVAR_SAVE)
CVAR(Bool, map_show_help, true, CVAR_SAVE)
CVAR(Int, map_crosshair, 0, CVAR_SAVE)
CVAR(Bool, map_show_selection_numbers, true, CVAR_SAVE)
CVAR(Int, map_max_selection_numbers, 1000, CVAR_SAVE)
CVAR(Bool, mlook_invert_y, false, CVAR_SAVE)
CVAR(Int, grid_64_style, 1, CVAR_SAVE)
CVAR(Float, camera_3d_sensitivity_x, 1.0f, CVAR_SAVE)
CVAR(Float, camera_3d_sensitivity_y, 1.0f, CVAR_SAVE)

// for testing
PolygonSplitter splitter;
SectorBuilder sbuilder;


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Int, render_3d_things)
EXTERN_CVAR(Int, render_3d_things_style)
EXTERN_CVAR(Int, render_3d_hilight)
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Float, render_3d_brightness)


/* MapCanvas::MapCanvas
 * MapCanvas class constructor
 *******************************************************************/
MapCanvas::MapCanvas(wxWindow* parent, int id, MapEditor* editor)
	: OGLCanvas(parent, id, false)
{
	// Init variables
	this->editor = editor;
	editor->setCanvas(this);
	view_xoff = 0;
	view_yoff = 0;
	view_scale = 1;
	last_hilight = -1;
	anim_flash_level = 0.5f;
	anim_flash_inc = true;
	anim_info_fade = 0.0f;
	anim_overlay_fade = 0.0f;
	fade_vertices = 1.0f;
	fade_lines = 1.0f;
	fade_flats = 1.0f;
	fade_things = 1.0f;
	mouse_state = MSTATE_NORMAL;
	mouse_downpos.set(-1, -1);
	fr_idle = 0;
	last_time = 0;
	renderer_2d = new MapRenderer2D(&editor->getMap());
	renderer_3d = new MapRenderer3D(&editor->getMap());
	view_xoff_inter = 0;
	view_yoff_inter = 0;
	view_scale_inter = 1;
	anim_view_speed = 0.05;
	zooming_cursor = false;
	mouse_selbegin = false;
	mouse_movebegin = false;
	overlay_current = NULL;
	mouse_locked = false;
	mouse_warp = false;
	edit_state = 0;
	edit_rotate = false;
	anim_help_fade = 0;
	panning = false;

#ifdef USE_SFML_RENDERWINDOW
	setVerticalSyncEnabled(false);
#endif

	// Bind Events
	Bind(wxEVT_SIZE, &MapCanvas::onSize, this);
	Bind(wxEVT_KEY_DOWN, &MapCanvas::onKeyDown, this);
	Bind(wxEVT_KEY_UP, &MapCanvas::onKeyUp, this);
	Bind(wxEVT_LEFT_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_RIGHT_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_MIDDLE_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_AUX1_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_AUX2_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_LEFT_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_RIGHT_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_MIDDLE_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_AUX1_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_AUX2_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_MOTION, &MapCanvas::onMouseMotion, this);
	Bind(wxEVT_MOUSEWHEEL, &MapCanvas::onMouseWheel, this);
	Bind(wxEVT_LEAVE_WINDOW, &MapCanvas::onMouseLeave, this);
	Bind(wxEVT_ENTER_WINDOW, &MapCanvas::onMouseEnter, this);
	Bind(wxEVT_SET_FOCUS, &MapCanvas::onFocus, this);
	Bind(wxEVT_KILL_FOCUS, &MapCanvas::onFocus, this);
	Bind(wxEVT_TIMER, &MapCanvas::onRTimer, this);
#ifdef USE_SFML_RENDERWINDOW
	Bind(wxEVT_IDLE, &MapCanvas::onIdle, this);
#endif

	timer.Start(10, true);
}

/* MapCanvas::~MapCanvas
 * MapCanvas class destructor
 *******************************************************************/
MapCanvas::~MapCanvas()
{
	delete renderer_2d;
	delete renderer_3d;
}

/* MapCanvas::overlayActive
 * Returns true if there is currently a full-screen overlay active
 *******************************************************************/
bool MapCanvas::overlayActive()
{
	if (!overlay_current)
		return false;
	else
		return overlay_current->isActive();
}

/* MapCanvas::helpActive
 * Returns true if feature help text should be shown currently
 *******************************************************************/
bool MapCanvas::helpActive()
{
	// Disable if no help
	if (feature_help_lines.empty())
		return false;

	// Enable depending on current state
	if (mouse_state == MSTATE_EDIT || mouse_state == MSTATE_LINE_DRAW || mouse_state == MSTATE_TAG_SECTORS)
		return true;

	return false;
}

/* MapCanvas::translateX
 * Translates an x position on the canvas to the corresponding x
 * position on the map itself
 *******************************************************************/
double MapCanvas::translateX(double x, bool inter)
{
	if (inter)
		return double(x / view_scale_inter) + view_xoff_inter - (double(GetSize().x * 0.5) / view_scale_inter);
	else
		return double(x / view_scale) + view_xoff - (double(GetSize().x * 0.5) / view_scale);
}

/* MapCanvas::translateY
 * Translates a y position on the canvas to the corresponding y
 * position on the map itself
 *******************************************************************/
double MapCanvas::translateY(double y, bool inter)
{
	if (inter)
		return double(-y / view_scale_inter) + view_yoff_inter + (double(GetSize().y * 0.5) / view_scale_inter);
	else
		return double(-y / view_scale) + view_yoff + (double(GetSize().y * 0.5) / view_scale);
}

/* MapCanvas::screenX
 * Translates [x] from map coordinates to canvas coordinates
 *******************************************************************/
int MapCanvas::screenX(double x)
{
	return MathStuff::round((GetSize().x * 0.5) + ((x - view_xoff_inter) * view_scale_inter));
}

/* MapCanvas::screenY
 * Translates [y] from map coordinates to canvas coordinates
 *******************************************************************/
int MapCanvas::screenY(double y)
{
	return MathStuff::round((GetSize().y * 0.5) - ((y - view_yoff_inter) * view_scale_inter));
}

/* MapCanvas::setTopY
 * Sets the view such that the map coordinate [y] is at the top
 * of the canvas (0)
 *******************************************************************/
void MapCanvas::setTopY(double y)
{
	double top_y = translateY(0);
	setView(view_xoff, view_yoff - (top_y - y));
	view_yoff_inter = view_yoff;
}

/* MapCanvas::setOverlayCoords
 * Sets/unsets the projection for rendering overlays (and text, etc.)
 *******************************************************************/
void MapCanvas::setOverlayCoords(bool set)
{
	if (set)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		if (OpenGL::accuracyTweak())
			glTranslatef(0.375f, 0.375f, 0);
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
}

/* MapCanvas::setView
 * Scrolls the view to be centered on map coordinates [x,y]
 *******************************************************************/
void MapCanvas::setView(double x, double y)
{
	// Set new view
	view_xoff = x;
	view_yoff = y;

	// Update screen limits
	view_tl.x = translateX(0);
	view_tl.y = translateY(GetSize().y);
	view_br.x = translateX(GetSize().x);
	view_br.y = translateY(0);

	// Update object visibility
	renderer_2d->updateVisibility(view_tl, view_br);
}

/* MapCanvas::pan
 * Scrolls the view relatively by [x,y]
 *******************************************************************/
void MapCanvas::pan(double x, double y)
{
	// Pan the view
	setView(view_xoff + x, view_yoff + y);
}

/* MapCanvas::zoom
 * Zooms the view by [amount]. If [towards_cursor] is true the view
 * will be zoomed towards the current mouse cursor position,
 * otherwise towards the center of the canvas
 *******************************************************************/
void MapCanvas::zoom(double amount, bool toward_cursor)
{
	// Get current mouse map coordinates
	double mx = translateX(mouse_pos.x);
	double my = translateY(mouse_pos.y);

	// Zoom view
	view_scale = view_scale * amount;

	// Check for zoom limits
	if (view_scale < 0.005)	// Min scale
		view_scale = 0.005;
	if (view_scale > 10.0) // Max scale
		view_scale = 10.0;

	// Do 'zoom towards cursor' stuff
	if (toward_cursor)
	{
		view_xoff += mx - translateX(mouse_pos.x);
		view_yoff += my - translateY(mouse_pos.y);
	}

	// Update screen limits
	view_tl.x = translateX(0);
	view_tl.y = translateY(GetSize().y);
	view_br.x = translateX(GetSize().x);
	view_br.y = translateY(0);

	// Update object visibility
	renderer_2d->setScale(view_scale_inter);
	renderer_2d->updateVisibility(view_tl, view_br);
}

/* MapCanvas::viewFitToMap
 * Centers the view to the center of the map, and zooms in or out so
 * that the entire map is showing
 *******************************************************************/
void MapCanvas::viewFitToMap(bool snap)
{
	// Get the map bbox
	bbox_t map_bbox = editor->getMap().getMapBBox();

	// Reset zoom and set offsets to the middle of the map
	view_scale = 2;
	view_xoff = map_bbox.min.x + ((map_bbox.max.x - map_bbox.min.x) * 0.5);
	view_yoff = map_bbox.min.y + ((map_bbox.max.y - map_bbox.min.y) * 0.5);

	// Now just keep zooming out until we fit the whole map in the view
	bool done = false;
	while (!done)
	{
		// Update screen limits
		view_tl.x = translateX(0);
		view_tl.y = translateY(GetSize().y);
		view_br.x = translateX(GetSize().x);
		view_br.y = translateY(0);

		if (map_bbox.min.x >= view_tl.x &&
		        map_bbox.max.x <= view_br.x &&
		        map_bbox.min.y >= view_tl.y &&
		        map_bbox.max.y <= view_br.y)
			done = true;
		else
			view_scale *= 0.8;
	}

	// Don't animate if specified
	if (snap)
	{
		view_xoff_inter = view_xoff;
		view_yoff_inter = view_yoff;
		view_scale_inter = view_scale;
	}

	// Update object visibility
	renderer_2d->setScale(view_scale_inter);
	renderer_2d->forceUpdate();
	renderer_2d->updateVisibility(view_tl, view_br);
}

/* MapCanvas::viewShowObject
 * Centers the view on the currently selected objects, and zooms so
 * that all objects are shown
 *******************************************************************/
void MapCanvas::viewShowObject()
{
	// Determine bbox of hilighted or selected object(s)
	bbox_t bbox;
	SLADEMap& map = editor->getMap();

	// Create list of objects
	vector<int> objects;
	vector<int>& selection = editor->getSelection();
	if (selection.size() > 0)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(selection[a]);
	}
	else if (editor->hilightItem() >= 0)
		objects.push_back(editor->hilightItem());
	else
		return;	// Nothing selected or hilighted

	// Generate bbox (depending on object type)
	if (editor->editMode() == MapEditor::MODE_VERTICES)
	{
		for (unsigned a = 0; a < objects.size(); a++)
			bbox.extend(map.getVertex(objects[a])->xPos(), map.getVertex(objects[a])->yPos());
	}
	else if (editor->editMode() == MapEditor::MODE_LINES)
	{
		MapLine* line;
		for (unsigned a = 0; a < objects.size(); a++)
		{
			line = map.getLine(objects[a]);
			bbox.extend(line->v1()->xPos(), line->v1()->yPos());
			bbox.extend(line->v2()->xPos(), line->v2()->yPos());
		}
	}
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
	{
		bbox = map.getSector(objects[0])->boundingBox();
		for (unsigned a = 1; a < objects.size(); a++)
		{
			bbox_t sbb = map.getSector(objects[a])->boundingBox();
			if (sbb.min.x < bbox.min.x)
				bbox.min.x = sbb.min.x;
			if (sbb.min.y < bbox.min.y)
				bbox.min.y = sbb.min.y;
			if (sbb.max.x > bbox.max.x)
				bbox.max.x = sbb.max.x;
			if (sbb.max.y > bbox.max.y)
				bbox.max.y = sbb.max.y;
		}
	}
	else if (editor->editMode() == MapEditor::MODE_THINGS)
	{
		for (unsigned a = 0; a < objects.size(); a++)
			bbox.extend(map.getThing(objects[a])->xPos(), map.getThing(objects[a])->yPos());
	}

	// Reset zoom and set offsets to the middle of the object(s)
	view_scale = 2;
	view_xoff = bbox.min.x + ((bbox.max.x - bbox.min.x) * 0.5);
	view_yoff = bbox.min.y + ((bbox.max.y - bbox.min.y) * 0.5);

	// Now just keep zooming out until we fit the whole map in the view
	bool done = false;
	while (!done)
	{
		// Update screen limits
		view_tl.x = translateX(0);
		view_tl.y = translateY(GetSize().y);
		view_br.x = translateX(GetSize().x);
		view_br.y = translateY(0);

		if (bbox.min.x >= view_tl.x &&
		        bbox.max.x <= view_br.x &&
		        bbox.min.y >= view_tl.y &&
		        bbox.max.y <= view_br.y)
			done = true;
		else
			view_scale *= 0.8;
	}

	// Update object visibility
	renderer_2d->updateVisibility(view_tl, view_br);
}

/* MapCanvas::viewMatchSpot
 * Sets the view so that [mx,my] in map coordinates matches up with
 * [sx,sy] in screen coordinates
 *******************************************************************/
void MapCanvas::viewMatchSpot(double mx, double my, double sx, double sy)
{
	setView(view_xoff_inter + mx - translateX(sx, true), view_yoff_inter + my - translateY(sy, true));
}

/* MapCanvas::set3dCameraThing
 * Sets the 3d mode camera to use the position/direction of [thing]
 *******************************************************************/
void MapCanvas::set3dCameraThing(MapThing* thing)
{
	// Determine position
	fpoint3_t pos(thing->point(), 40);
	int sector = editor->getMap().sectorAt(thing->point());
	if (sector >= 0)
		pos.z += editor->getMap().getSector(sector)->getFloorHeight();

	// Determine direction
	fpoint2_t dir = MathStuff::vectorAngle(MathStuff::degToRad(thing->getAngle()));

	renderer_3d->cameraSet(pos, dir);
}

/* MapCanvas::get3dCameraPos
 * Returns the current 3d mode camera position
 *******************************************************************/
fpoint2_t MapCanvas::get3dCameraPos()
{
	return fpoint2_t(renderer_3d->camPosition().x, renderer_3d->camPosition().y);
}

/* MapCanvas::get3dCameraDir
 * Returns the current 3d mode camera direction
 *******************************************************************/
fpoint2_t MapCanvas::get3dCameraDir()
{
	return renderer_3d->camDirection();
}

/* MapCanvas::drawGrid
 * Draws the grid
 *******************************************************************/
void MapCanvas::drawGrid()
{
	// Get grid size
	int gridsize = editor->gridSize();

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
	int grid_hidelevel = 2.0 / view_scale;

	// Determine canvas edges in map coordinates
	int start_x = translateX(0, true);
	int end_x = translateX(GetSize().x, true);
	int start_y = translateY(GetSize().y, true);
	int end_y = translateY(0, true);

	// Draw regular grid if it's not too small
	if (gridsize > grid_hidelevel)
	{
		// Vertical
		int ofs = start_x % gridsize;
		for (int x = start_x-ofs; x <= end_x; x += gridsize)
		{
			glBegin(GL_LINES);
			glVertex2d(x, start_y);
			glVertex2d(x, end_y);
			glEnd();
		}

		// Horizontal
		ofs = start_y % gridsize;
		for (int y = start_y-ofs; y <= end_y; y += gridsize)
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
		for (int x = start_x-ofs; x <= end_x; x += 64)
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
		for (int y = start_y-ofs; y <= end_y; y += 64)
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
		double x = editor->snapToGrid(mouse_pos_m.x, false);
		double y = editor->snapToGrid(mouse_pos_m.y, false);
		rgba_t col = ColourConfiguration::getColour("map_64grid");

		// Small
		glLineWidth(2.0f);
		if (map_crosshair == 1)
		{
			col = col.ampf(1.0f, 1.0f, 1.0f, 2.0f);
			rgba_t col2 = col.ampf(1.0f, 1.0f, 1.0f, 0.0f);
			double size = editor->gridSize();
			double one = 1.0 / view_scale_inter;

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
			glVertex2d(x, view_tl.y);
			glVertex2d(x, view_br.y);
			glVertex2d(view_tl.x, y);
			glVertex2d(view_br.x, y);
			glEnd();
		}
	}
}

/* MapCanvas::drawEditorMessage
 * Draws any currently showing editor messages
 *******************************************************************/
void MapCanvas::drawEditorMessages()
{
	int yoff = 0;
	if (map_showfps) yoff = 16;
	rgba_t col_fg = ColourConfiguration::getColour("map_editor_message");
	rgba_t col_bg = ColourConfiguration::getColour("map_editor_message_outline");
	Drawing::setTextState(true);
	Drawing::enableTextStateReset(false);

	// Go through editor messages
	for (unsigned a = 0; a < editor->numEditorMessages(); a++)
	{
		// Check message time
		long time = editor->getEditorMessageTime(a);
		if (time > 2000)
			continue;

		// Setup message colour
		rgba_t col = col_fg;
		if (time < 200)
		{
			float flash = 1.0f - (time / 200.0f);
			col.r += (255-col.r)*flash;
			col.g += (255-col.g)*flash;
			col.b += (255-col.b)*flash;
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
		Drawing::drawText(editor->getEditorMessage(a), 0, yoff, col, Drawing::FONT_BOLD);
		
		yoff += 16;
	}
	Drawing::setTextOutline(0);
	Drawing::setTextState(false);
	Drawing::enableTextStateReset(true);
}

/* MapCanvas::drawFeatureHelpText
 * Draws any feature help text currently showing
 *******************************************************************/
void MapCanvas::drawFeatureHelpText()
{
	// Check if any text
	if (feature_help_lines.empty() || !map_show_help)
		return;

	// Draw title
	frect_t bounds;
	rgba_t col = ColourConfiguration::getColour("map_editor_message");
	rgba_t col_bg = ColourConfiguration::getColour("map_editor_message_outline");
	col.a = col.a * anim_help_fade;
	col_bg.a = col_bg.a * anim_help_fade;
	Drawing::setTextOutline(1.0f, col_bg);
	Drawing::drawText(feature_help_lines[0], GetSize().x - 2, 2, col, Drawing::FONT_BOLD, Drawing::ALIGN_RIGHT, &bounds);

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
	for (unsigned a = 1; a < feature_help_lines.size(); a++)
	{
		Drawing::drawText(feature_help_lines[a], GetSize().x - 2, yoff, col, Drawing::FONT_BOLD, Drawing::ALIGN_RIGHT);
		yoff += 16;
	}
	Drawing::setTextOutline(0);
	Drawing::setTextState(false);
	Drawing::enableTextStateReset(true);
}

/* MapCanvas::drawSelectionNumbers
 * Draws numbers for selected map objects
 *******************************************************************/
void MapCanvas::drawSelectionNumbers()
{
	// Check if any selection exists
	vector<MapObject*> selection;
	editor->getSelectedObjects(selection);
	if (selection.size() == 0)
		return;

	// Get editor message text colour
	rgba_t col = ColourConfiguration::getColour("map_editor_message");

	// Go through selection
	string text;
	Drawing::enableTextStateReset(false);
	Drawing::setTextState(true);
	setOverlayCoords();
#if USE_SFML_RENDERWINDOW && ((SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR >= 4) || SFML_VERSION_MAJOR > 2)
	Drawing::setTextOutline(1.0f, COL_BLACK);
#else
	if (editor->selectionSize() <= map_max_selection_numbers * 0.5)
		Drawing::setTextOutline(1.0f, COL_BLACK);
#endif
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if ((int)a > map_max_selection_numbers)
			break;

		fpoint2_t tp = selection[a]->getPoint(MOBJ_POINT_TEXT);
		tp.x = screenX(tp.x);
		tp.y = screenY(tp.y);

		text = S_FMT("%d", a+1);
		fpoint2_t ts = Drawing::textExtents(text, Drawing::FONT_BOLD);
		tp.x -= ts.x * 0.5;
		tp.y -= ts.y * 0.5;

		if (editor->editMode() == MapEditor::MODE_VERTICES)
		{
			tp.x += 8;
			tp.y += 8;
		}

		// Draw text
		Drawing::drawText(S_FMT("%d", a+1), tp.x, tp.y, col, Drawing::FONT_BOLD);
	}
	Drawing::setTextOutline(0);
	Drawing::enableTextStateReset();
	Drawing::setTextState(false);
	setOverlayCoords(false);

	glDisable(GL_TEXTURE_2D);
}

/* MapCanvas::drawThingQuickAngleLines
 * Draws directional lines for thing quick angle selection
 *******************************************************************/
void MapCanvas::drawThingQuickAngleLines()
{
	// Check if any selection exists
	vector<MapThing*> selection;
	editor->getSelectedThings(selection);
	if (selection.size() == 0)
		return;

	// Get moving colour
	rgba_t col = ColourConfiguration::getColour("map_moving");
	OpenGL::setColour(col);

	// Draw lines
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		glVertex2d(selection[a]->xPos(), selection[a]->yPos());
		glVertex2d(mouse_pos_m.x, mouse_pos_m.y);
	}
	glEnd();
}

/* MapCanvas::drawLineLength
 * Draws text showing the length from [p1] to [p2]
 *******************************************************************/
void MapCanvas::drawLineLength(fpoint2_t p1, fpoint2_t p2, rgba_t col)
{
	// Determine distance in screen scale
	double tdist = 20 / view_scale_inter;

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
	Drawing::drawText(length, screenX(tp.x), screenY(tp.y) - hh, col, Drawing::FONT_NORMAL, Drawing::ALIGN_CENTER);
	glDisable(GL_TEXTURE_2D);
}

/* MapCanvas::drawLineDrawLines
 * Draws current line drawing lines (best function name ever)
 *******************************************************************/
void MapCanvas::drawLineDrawLines()
{
	// Get line draw colour
	rgba_t col = ColourConfiguration::getColour("map_linedraw");
	OpenGL::setColour(col);

	// Determine end point
	fpoint2_t end = mouse_pos_m;
	if (modifiers_current & wxMOD_SHIFT)
	{
		// If shift is held down, snap to the nearest vertex (if any)
		int vertex = editor->getMap().nearestVertex(end);
		if (vertex >= 0)
		{
			end.x = editor->getMap().getVertex(vertex)->xPos();
			end.y = editor->getMap().getVertex(vertex)->yPos();
		}
		else if (editor->gridSnap())
		{
			// No nearest vertex, snap to grid if needed
			end.x = editor->snapToGrid(end.x);
			end.y = editor->snapToGrid(end.y);
		}
	}
	else if (editor->gridSnap())
	{
		// Otherwise, snap to grid if needed
		end.x = editor->snapToGrid(end.x);
		end.y = editor->snapToGrid(end.y);
	}

	// Draw lines
	int npoints = editor->nLineDrawPoints();
	glLineWidth(2.0f);
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
		{
			fpoint2_t p1 = editor->lineDrawPoint(a);
			fpoint2_t p2 = editor->lineDrawPoint(a+1);
			Drawing::drawLineTabbed(p1, p2);
		}
	}
	if (npoints > 0 && draw_state == DSTATE_LINE)
		Drawing::drawLineTabbed(editor->lineDrawPoint(npoints-1), end);

	// Draw line lengths
	setOverlayCoords(true);
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
			drawLineLength(editor->lineDrawPoint(a), editor->lineDrawPoint(a+1), col);
	}
	if (npoints > 0 && draw_state == DSTATE_LINE)
		drawLineLength(editor->lineDrawPoint(npoints-1), end, col);
	setOverlayCoords(false);

	// Draw points
	glPointSize(vertex_size);
	if (vertex_round) glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < editor->nLineDrawPoints(); a++)
	{
		fpoint2_t point = editor->lineDrawPoint(a);
		glVertex2d(point.x, point.y);
	}
	if (draw_state == DSTATE_LINE || draw_state == DSTATE_SHAPE_ORIGIN)
		glVertex2d(end.x, end.y);
	glEnd();
}

/* MapCanvas::drawPasteLines
 * Draws lines currently being pasted
 *******************************************************************/
void MapCanvas::drawPasteLines()
{
	// Get clipboard item
	MapArchClipboardItem* c = NULL;
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
	rgba_t col = ColourConfiguration::getColour("map_linedraw");
	OpenGL::setColour(col);

	// Draw
	fpoint2_t pos = editor->relativeSnapToGrid(c->getMidpoint(), mouse_pos_m);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		glVertex2d(pos.x + lines[a]->x1(), pos.y + lines[a]->y1());
		glVertex2d(pos.x + lines[a]->x2(), pos.y + lines[a]->y2());
	}
	glEnd();
}

/* MapCanvas::drawObjectEdit
 * Draws object edit objects, bounding box and text
 *******************************************************************/
void MapCanvas::drawObjectEdit()
{
	ObjectEditGroup* group = editor->getObjectEditGroup();
	rgba_t col = ColourConfiguration::getColour("map_object_edit");

	// Map objects
	renderer_2d->renderObjectEditGroup(group);

	// Bounding box
	OpenGL::setColour(COL_WHITE);
	glColor4f(col.fr(), col.fg(), col.fb(), 1.0f);
	bbox_t bbox = group->getBBox();
	bbox.min.x -= 4 / view_scale_inter;
	bbox.min.y -= 4 / view_scale_inter;
	bbox.max.x += 4 / view_scale_inter;
	bbox.max.y += 4 / view_scale_inter;

	if (edit_rotate)
	{
		// Rotate

		// Bbox
		fpoint2_t mid(bbox.min.x + bbox.width() * 0.5, bbox.min.y + bbox.height() * 0.5);
		fpoint2_t bl = MathStuff::rotatePoint(mid, bbox.min, group->getRotation());
		fpoint2_t tl = MathStuff::rotatePoint(mid, fpoint2_t(bbox.min.x, bbox.max.y), group->getRotation());
		fpoint2_t tr = MathStuff::rotatePoint(mid, bbox.max, group->getRotation());
		fpoint2_t br = MathStuff::rotatePoint(mid, fpoint2_t(bbox.max.x, bbox.min.y), group->getRotation());
		glLineWidth(2.0f);
		Drawing::drawLine(tl, bl);
		Drawing::drawLine(bl, br);
		Drawing::drawLine(br, tr);
		Drawing::drawLine(tr, tl);

		// Top Left
		double rad = 4 / view_scale_inter;
		glLineWidth(1.0f);
		if (edit_state == ESTATE_SIZE_TL)
			Drawing::drawFilledRect(tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad);
		else
			Drawing::drawRect(tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad);

		// Bottom Left
		if (edit_state == ESTATE_SIZE_BL)
			Drawing::drawFilledRect(bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad);
		else
			Drawing::drawRect(bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad);

		// Top Right
		if (edit_state == ESTATE_SIZE_TR)
			Drawing::drawFilledRect(tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad);
		else
			Drawing::drawRect(tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad);

		// Bottom Right
		if (edit_state == ESTATE_SIZE_BR)
			Drawing::drawFilledRect(br.x - rad, br.y - rad, br.x + rad, br.y + rad);
		else
			Drawing::drawRect(br.x - rad, br.y - rad, br.x + rad, br.y + rad);
	}
	else
	{
		// Move/scale

		// Left
		if (edit_state == ESTATE_MOVE || edit_state == ESTATE_SIZE_L || edit_state == ESTATE_SIZE_TL || edit_state == ESTATE_SIZE_BL)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.min.x, bbox.min.y, bbox.min.x, bbox.max.y);

		// Bottom
		if (edit_state == ESTATE_MOVE || edit_state == ESTATE_SIZE_B || edit_state == ESTATE_SIZE_BL || edit_state == ESTATE_SIZE_BR)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.min.x, bbox.min.y, bbox.max.x, bbox.min.y);

		// Right
		if (edit_state == ESTATE_MOVE || edit_state == ESTATE_SIZE_R || edit_state == ESTATE_SIZE_TR || edit_state == ESTATE_SIZE_BR)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.max.x, bbox.max.y, bbox.max.x, bbox.min.y);

		// Top
		if (edit_state == ESTATE_MOVE || edit_state == ESTATE_SIZE_T || edit_state == ESTATE_SIZE_TL || edit_state == ESTATE_SIZE_TR)
			glLineWidth(4.0f);
		else
			glLineWidth(2.0f);
		Drawing::drawLine(bbox.max.x, bbox.max.y, bbox.min.x, bbox.max.y);
	}

	// Line length
	fpoint2_t nl_v1, nl_v2;
	if (group->getNearestLine(mouse_pos_m, 128 / view_scale, nl_v1, nl_v2))
	{
		fpoint2_t mid(nl_v1.x + ((nl_v2.x - nl_v1.x) * 0.5), nl_v1.y + ((nl_v2.y - nl_v1.y) * 0.5));
		int length = MathStuff::distance(nl_v1, nl_v2);
		int x = screenX(mid.x);
		int y = screenY(mid.y) - 8;
		setOverlayCoords(true);
		Drawing::setTextOutline(1.0f, COL_BLACK);
		Drawing::drawText(S_FMT("%d", length), x, y, COL_WHITE, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
		Drawing::setTextOutline(0);
		setOverlayCoords(false);
		glDisable(GL_TEXTURE_2D);
	}
}

/* MapCanvas::drawMap2d
 * Draws the 2d map
 *******************************************************************/
void MapCanvas::drawMap2d()
{
	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, GetSize().x, 0.0f, GetSize().y, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implemenataions)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Translate to middle of screen
	glTranslated(GetSize().x * 0.5, GetSize().y * 0.5, 0);

	// Zoom
	glScaled(view_scale_inter, view_scale_inter, 1);

	// Translate to offsets
	glTranslated(-view_xoff_inter, -view_yoff_inter, 0);

	// Update visibility info if needed
	if (!renderer_2d->visOK())
		renderer_2d->updateVisibility(view_tl, view_br);


	// Draw flats if needed
	OpenGL::setColour(COL_WHITE);
	if (flat_drawtype > 0)
	{
		bool texture = false;
		if (flat_drawtype > 1)
			texture = true;

		// Adjust flat type depending on sector mode
		int drawtype = 0;
		if (editor->editMode() == MapEditor::MODE_SECTORS)
		{
			if (editor->sectorEditMode() == MapEditor::SECTOR_FLOOR)
				drawtype = 1;
			else if (editor->sectorEditMode() == MapEditor::SECTOR_CEILING)
				drawtype = 2;
		}

		renderer_2d->renderFlats(drawtype, texture, fade_flats);
	}

	// Draw grid
	drawGrid();

	// --- Draw map (depending on mode) ---
	OpenGL::resetBlend();
	if (editor->editMode() == MapEditor::MODE_VERTICES)
	{
		// Vertices mode
		renderer_2d->renderThings(fade_things);						// Things
		renderer_2d->renderLines(line_tabs_always, fade_lines);		// Lines

		// Vertices
		if (mouse_state == MSTATE_MOVE)
			renderer_2d->renderVertices(0.25f);
		else
			renderer_2d->renderVertices(fade_vertices);

		// Selection if needed
		if (mouse_state != MSTATE_MOVE && !overlayActive() && mouse_state != MSTATE_EDIT)
			renderer_2d->renderVertexSelection(editor->getSelection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == MSTATE_NORMAL && !overlayActive())
			renderer_2d->renderVertexHilight(editor->hilightItem(), anim_flash_level);
	}
	else if (editor->editMode() == MapEditor::MODE_LINES)
	{
		// Lines mode
		renderer_2d->renderThings(fade_things);		// Things
		renderer_2d->renderVertices(fade_vertices);	// Vertices
		renderer_2d->renderLines(true);				// Lines

		// Selection if needed
		if (mouse_state != MSTATE_MOVE && !overlayActive() && mouse_state != MSTATE_EDIT)
			renderer_2d->renderLineSelection(editor->getSelection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == MSTATE_NORMAL && !overlayActive())
			renderer_2d->renderLineHilight(editor->hilightItem(), anim_flash_level);
	}
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
	{
		// Sectors mode
		renderer_2d->renderThings(fade_things);					// Things
		renderer_2d->renderVertices(fade_vertices);				// Vertices
		renderer_2d->renderLines(line_tabs_always, fade_lines);	// Lines

		// Selection if needed
		if (mouse_state != MSTATE_MOVE && !overlayActive() && mouse_state != MSTATE_EDIT)
			renderer_2d->renderFlatSelection(editor->getSelection(), anim_flash_level);

		splitter.testRender();	// Testing

		// Hilight if needed
		if (mouse_state == MSTATE_NORMAL && !overlayActive())
			renderer_2d->renderFlatHilight(editor->hilightItem(), anim_flash_level);
	}
	else if (editor->editMode() == MapEditor::MODE_THINGS)
	{
		// Check if we should force thing angles visible
		bool force_dir = false;
		if (mouse_state == MSTATE_THING_ANGLE)
			force_dir = true;

		// Things mode
		renderer_2d->renderVertices(fade_vertices);				// Vertices
		renderer_2d->renderLines(line_tabs_always, fade_lines);	// Lines
		renderer_2d->renderThings(fade_things, force_dir);		// Things

		// Thing paths
		renderer_2d->renderPathedThings(editor->pathedThings());

		// Selection if needed
		if (mouse_state != MSTATE_MOVE && !overlayActive() && mouse_state != MSTATE_EDIT)
			renderer_2d->renderThingSelection(editor->getSelection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == MSTATE_NORMAL && !overlayActive())
			renderer_2d->renderThingHilight(editor->hilightItem(), anim_flash_level);
	}


	// Draw tagged sectors/lines/things if needed
	if (!overlayActive() && (mouse_state == MSTATE_NORMAL || mouse_state == MSTATE_TAG_SECTORS || mouse_state == MSTATE_TAG_THINGS))
	{
		if (editor->taggedSectors().size() > 0)
			renderer_2d->renderTaggedFlats(editor->taggedSectors(), anim_flash_level);
		if (editor->taggedLines().size() > 0)
			renderer_2d->renderTaggedLines(editor->taggedLines(), anim_flash_level);
		if (editor->taggedThings().size() > 0)
			renderer_2d->renderTaggedThings(editor->taggedThings(), anim_flash_level);
		if (editor->taggingLines().size() > 0)
			renderer_2d->renderTaggingLines(editor->taggingLines(), anim_flash_level);
		if (editor->taggingThings().size() > 0)
			renderer_2d->renderTaggingThings(editor->taggingThings(), anim_flash_level);
	}

	// Draw selection numbers if needed
	if (editor->selectionSize() > 0 && mouse_state == MSTATE_NORMAL && map_show_selection_numbers)
		drawSelectionNumbers();

	// Draw thing quick angle lines if needed
	if (mouse_state == MSTATE_THING_ANGLE)
		drawThingQuickAngleLines();

	// Draw line drawing lines if needed
	if (mouse_state == MSTATE_LINE_DRAW)
		drawLineDrawLines();

	// Draw object edit objects if needed
	if (mouse_state == MSTATE_EDIT)
		drawObjectEdit();

	// Draw sectorbuilder test stuff
	//sbuilder.drawResult();


	// Draw selection box if active
	if (mouse_state == MSTATE_SELECTION)
	{
		// Outline
		OpenGL::setColour(ColourConfiguration::getColour("map_selbox_outline"));
		glLineWidth(2.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2d(mouse_downpos_m.x, mouse_downpos_m.y);
		glVertex2d(mouse_downpos_m.x, mouse_pos_m.y);
		glVertex2d(mouse_pos_m.x, mouse_pos_m.y);
		glVertex2d(mouse_pos_m.x, mouse_downpos_m.y);
		glEnd();

		// Fill
		OpenGL::setColour(ColourConfiguration::getColour("map_selbox_fill"));
		glBegin(GL_QUADS);
		glVertex2d(mouse_downpos_m.x, mouse_downpos_m.y);
		glVertex2d(mouse_downpos_m.x, mouse_pos_m.y);
		glVertex2d(mouse_pos_m.x, mouse_pos_m.y);
		glVertex2d(mouse_pos_m.x, mouse_downpos_m.y);
		glEnd();
	}

	// Draw animations
	for (unsigned a = 0; a < animations.size(); a++)
	{
		if (!animations[a]->mode3d())
			animations[a]->draw();
	}

	// Draw paste objects if needed
	if (mouse_state == MSTATE_PASTE)
	{

		if (editor->editMode() == MapEditor::MODE_THINGS)
		{
			// Get clipboard item
			for (unsigned a = 0; a < theClipboard->nItems(); a++)
			{
				ClipboardItem* item = theClipboard->getItem(a);
				if (item->getType() == CLIPBOARD_MAP_THINGS)
				{
					vector<MapThing*> things;
					MapThingsClipboardItem* p = (MapThingsClipboardItem*)item;
					p->getThings(things);
					fpoint2_t pos(editor->relativeSnapToGrid(p->getMidpoint(), mouse_pos_m));
					renderer_2d->renderPasteThings(things, pos);
				}
			}
		}
		else
			drawPasteLines();
	}

	// Draw moving stuff if needed
	if (mouse_state == MSTATE_MOVE)
	{
		switch (editor->editMode())
		{
		case MapEditor::MODE_VERTICES:
			renderer_2d->renderMovingVertices(editor->movingItems(), editor->moveVector()); break;
		case MapEditor::MODE_LINES:
			renderer_2d->renderMovingLines(editor->movingItems(), editor->moveVector()); break;
		case MapEditor::MODE_SECTORS:
			renderer_2d->renderMovingSectors(editor->movingItems(), editor->moveVector()); break;
		case MapEditor::MODE_THINGS:
			renderer_2d->renderMovingThings(editor->movingItems(), editor->moveVector()); break;
		default: break;
		};
	}
}

/* MapCanvas::drawMap3d
 * Draws the 3d map
 *******************************************************************/
void MapCanvas::drawMap3d()
{
	// Setup 3d renderer view
	renderer_3d->setupView(GetSize().x, GetSize().y);

	// Render 3d map
	renderer_3d->renderMap();

	// Determine hilight
	selection_3d_t hl;
	if (!editor->hilightLocked())
	{
		selection_3d_t old_hl = editor->hilightItem3d();
		hl = renderer_3d->determineHilight();
		if (editor->set3dHilight(hl))
		{
			// Update 3d info overlay
			if (info_overlay_3d && hl.index >= 0)
			{
				info_3d.update(hl.index, hl.type, &(editor->getMap()));
				anim_info_show = true;
			}
			else
				anim_info_show = false;

			// Animation
			animations.push_back(new MCAHilightFade3D(theApp->runTimer(), old_hl.index, old_hl.type, renderer_3d, anim_flash_level));
			anim_flash_inc = true;
			anim_flash_level = 0.0f;
		}
	}

	// Draw selection if any
	vector<selection_3d_t> selection = editor->get3dSelection();
	renderer_3d->renderFlatSelection(selection);
	renderer_3d->renderWallSelection(selection);
	renderer_3d->renderThingSelection(selection);

	// Draw hilight if any
	if (hl.index >= 0)
		renderer_3d->renderHilight(hl, anim_flash_level);

	// Draw animations
	for (unsigned a = 0; a < animations.size(); a++)
	{
		if (animations[a]->mode3d())
			animations[a]->draw();
	}
}

/* MapCanvas::draw
 * Draw the current map (2d or 3d) and any overlays etc
 *******************************************************************/
void MapCanvas::draw()
{
	if (!IsEnabled())
		return;

	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup GL state
	rgba_t col_bg = ColourConfiguration::getColour("map_background");
	glClearColor(col_bg.fr(), col_bg.fg(), col_bg.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisable(GL_TEXTURE_2D);

	// Draw 2d or 3d map depending on mode
	if (editor->editMode() == MapEditor::MODE_3D)
		drawMap3d();
	else
		drawMap2d();

	// Draw info overlay
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implemenataions)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Check if we have to update the info
	if (editor->editMode() != MapEditor::MODE_3D && editor->hilightItem() != last_hilight)
	{
		// Update hilight index
		last_hilight = editor->hilightItem();
		anim_info_show = (last_hilight != -1);

		// Update info overlay depending on edit mode
		switch (editor->editMode())
		{
		case MapEditor::MODE_VERTICES:	info_vertex.update(editor->getHilightedVertex()); break;
		case MapEditor::MODE_LINES:		info_line.update(editor->getHilightedLine()); break;
		case MapEditor::MODE_SECTORS:	info_sector.update(editor->getHilightedSector()); break;
		case MapEditor::MODE_THINGS:	info_thing.update(editor->getHilightedThing()); break;
		}
	}

	// Draw current info overlay
	glDisable(GL_TEXTURE_2D);
	if (editor->editMode() == MapEditor::MODE_VERTICES)
		info_vertex.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == MapEditor::MODE_LINES)
		info_line.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
		info_sector.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == MapEditor::MODE_THINGS)
		info_thing.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == MapEditor::MODE_3D)
		info_3d.draw(GetSize().y, GetSize().x, GetSize().x * 0.5, anim_info_fade);

	// Draw current fullscreen overlay
	if (overlay_current)
		overlay_current->draw(GetSize().x, GetSize().y, anim_overlay_fade);

	// Draw crosshair if 3d mode
	if (editor->editMode() == MapEditor::MODE_3D)
	{
		// Get crosshair colour
		rgba_t col = ColourConfiguration::getColour("map_3d_crosshair");
		OpenGL::setColour(col);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.5f);

		double midx = GetSize().x * 0.5;
		double midy = GetSize().y * 0.5;
		int size = camera_3d_crosshair_size;

		glBegin(GL_LINES);
		// Right
		OpenGL::setColour(col, false);
		glVertex2d(midx+1, midy);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx+size, midy);

		// Left
		OpenGL::setColour(col, false);
		glVertex2d(midx-1, midy);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx-size, midy);

		// Bottom
		OpenGL::setColour(col, false);
		glVertex2d(midx, midy+1);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx, midy+size);

		// Top
		OpenGL::setColour(col, false);
		glVertex2d(midx, midy-1);
		glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
		glVertex2d(midx, midy-size);
		glEnd();

		// Draw item distance (if any)
		if (renderer_3d->itemDistance() >= 0 && camera_3d_show_distance)
		{
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(col);
			Drawing::drawText(S_FMT("%d", renderer_3d->itemDistance()), midx+5, midy+5, rgba_t(255, 255, 255, 200), Drawing::FONT_SMALL);
			//Drawing::drawText(S_FMT("%1.2f", renderer_3d->camPitch()), midx+5, midy+30, rgba_t(255, 255, 255, 200), Drawing::FONT_SMALL);
		}
	}

	// FPS counter
	if (map_showfps)
	{
		glEnable(GL_TEXTURE_2D);
		if (frametime_last > 0)
		{
			int fps = MathStuff::round(1.0 / (frametime_last/1000.0));
			fps_avg.push_back(fps);
			if (fps_avg.size() > 20) fps_avg.erase(fps_avg.begin());
		}
		int afps = 0;
		for (unsigned a = 0; a < fps_avg.size(); a++)
			afps += fps_avg[a];
		if (fps_avg.size() > 0) afps /= fps_avg.size();
		Drawing::drawText(S_FMT("FPS: %d", afps));
	}

	// test
	//Drawing::drawText(S_FMT("Render distance: %1.2f", (double)render_max_dist), 0, 100);

	// Editor messages
	drawEditorMessages();

	// Help text
	drawFeatureHelpText();

	SwapBuffers();

	glFinish();
}

/* MapCanvas::update2d
 * Updates the current 2d map editor state (animations, hilight etc.)
 *******************************************************************/
bool MapCanvas::update2d(double mult)
{
	// Update hilight if needed
	if (mouse_state == MSTATE_NORMAL && !mouse_movebegin)
	{
		MapObject* old_hl = editor->getHilightedObject();
		if (editor->updateHilight(mouse_pos_m, view_scale) && hilight_smooth)
		{
			// Hilight fade animation
			if (old_hl)
				animations.push_back(new MCAHilightFade(theApp->runTimer(), old_hl, renderer_2d, anim_flash_level));

			// Reset hilight flash
			anim_flash_inc = true;
			anim_flash_level = 0.0f;
		}
	}

	// Do item moving if needed
	if (mouse_state == MSTATE_MOVE)
		editor->doMove(mouse_pos_m);

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
	if (editor->editMode() == MapEditor::MODE_VERTICES)
	{
		if (fade_vertices < 1.0f)  { fade_vertices += mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = fa_lines;
		if (fade_flats > fa_flats) { fade_flats -= mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == MapEditor::MODE_LINES)
	{
		if (fade_vertices > fa_vertices) { fade_vertices -= mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = 1.0f;
		if (fade_flats > fa_flats) { fade_flats -= mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
	{
		if (fade_vertices > fa_vertices) { fade_vertices -= mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = fa_lines;
		if (fade_flats < 1.0f) { fade_flats += mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == MapEditor::MODE_THINGS)
	{
		if (fade_vertices > fa_vertices) { fade_vertices -= mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = fa_lines;
		if (fade_flats > fa_flats) { fade_flats -= mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things < 1.0f) { fade_things += mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}

	// Clamp
	if (fade_vertices < fa_vertices) fade_vertices = fa_vertices;
	if (fade_vertices > 1.0f) fade_vertices = 1.0f;
	if (fade_lines < fa_lines) fade_lines = fa_lines;
	if (fade_lines > 1.0f) fade_lines = 1.0f;
	if (fade_flats < fa_flats) fade_flats = fa_flats;
	if (fade_flats > 1.0f) fade_flats = 1.0f;
	if (fade_things < fa_things) fade_things = fa_things;
	if (fade_things > 1.0f) fade_things = 1.0f;

	// View pan/zoom animation
	bool view_anim = false;
	if (scroll_smooth)
	{
		// Scale
		double diff_scale = view_scale - view_scale_inter;
		if (diff_scale < -0.0000001 || diff_scale > 0.0000001)
		{
			// Get current mouse position in map coordinates (for zdooming towards cursor)
			double mx = translateX(mouse_pos.x, true);
			double my = translateY(mouse_pos.y, true);

			// Interpolate zoom
			view_scale_inter += diff_scale*anim_view_speed*mult;

			// Check for zoom finish
			if ((diff_scale < 0 && view_scale_inter < view_scale) ||
			        (diff_scale > 0 && view_scale_inter > view_scale))
				view_scale_inter = view_scale;
			else
				view_anim = true;

			if (zooming_cursor)
			{
				viewMatchSpot(mx, my, mouse_pos.x, mouse_pos.y);
				view_xoff_inter = view_xoff;
				view_yoff_inter = view_yoff;
			}
		}
		else
			view_scale_inter = view_scale;

		if (!zooming_cursor)
		{
			// X offset
			double diff_xoff = view_xoff - view_xoff_inter;
			if (diff_xoff < -0.05 || diff_xoff > 0.05)
			{
				// Interpolate offset
				view_xoff_inter += diff_xoff*anim_view_speed*mult;

				// Check stuff
				if ((diff_xoff < 0 && view_xoff_inter < view_xoff) ||
				        (diff_xoff > 0 && view_xoff_inter > view_xoff))
					view_xoff_inter = view_xoff;
				else
					view_anim = true;
			}
			else
				view_xoff_inter = view_xoff;

			// Y offset
			double diff_yoff = view_yoff - view_yoff_inter;
			if (diff_yoff < -0.05 || diff_yoff > 0.05)
			{
				// Interpolate offset
				view_yoff_inter += diff_yoff*anim_view_speed*mult;

				if ((diff_yoff < 0 && view_yoff_inter < view_yoff) ||
				        (diff_yoff > 0 && view_yoff_inter > view_yoff))
					view_yoff_inter = view_yoff;
				else
					view_anim = true;
			}
			else
				view_yoff_inter = view_yoff;
		}

		if (!view_anim)
			anim_view_speed = 0.05;
		else
		{
			anim_view_speed += 0.05*mult;
			if (anim_view_speed > 0.4)
				anim_view_speed = 0.4;
		}
	}
	else
	{
		view_xoff_inter = view_xoff;
		view_yoff_inter = view_yoff;
		view_scale_inter = view_scale;
	}

	if (!view_anim)
		zooming_cursor = false;

	// Update renderer scale
	renderer_2d->setScale(view_scale_inter);

	// Check if framerate shouldn't be throttled
	if (mouse_state == MSTATE_SELECTION || panning || view_anim || anim_mode_crossfade)
		return true;
	else
		return false;
}

/* MapCanvas::update3d
 * Updates the current 3d map editor state (animations, movement etc.)
 *******************************************************************/
bool MapCanvas::update3d(double mult)
{
	// Check if overlay active
	if (overlayActive())
		return true;

	// --- Check for held-down keys ---
	bool moving = false;
	bool fast = (modifiers_current & wxMOD_SHIFT) > 0;
	double speed = fast ? mult * 8 : mult * 4;

	// Camera forward
	if (KeyBind::isPressed("me3d_camera_forward"))
	{
		renderer_3d->cameraMove(speed, !camera_3d_gravity);
		moving = true;
	}

	// Camera backward
	if (KeyBind::isPressed("me3d_camera_back"))
	{
		renderer_3d->cameraMove(-speed, !camera_3d_gravity);
		moving = true;
	}

	// Camera left (strafe)
	if (KeyBind::isPressed("me3d_camera_left"))
	{
		renderer_3d->cameraStrafe(-speed);
		moving = true;
	}

	// Camera right (strafe)
	if (KeyBind::isPressed("me3d_camera_right"))
	{
		renderer_3d->cameraStrafe(speed);
		moving = true;
	}

	// Camera up
	if (KeyBind::isPressed("me3d_camera_up"))
	{
		renderer_3d->cameraMoveUp(speed);
		moving = true;
	}

	// Camera down
	if (KeyBind::isPressed("me3d_camera_down"))
	{
		renderer_3d->cameraMoveUp(-speed);
		moving = true;
	}

	// Camera turn left
	if (KeyBind::isPressed("me3d_camera_turn_left"))
	{
		renderer_3d->cameraTurn(fast ? mult*2 : mult);
		moving = true;
	}

	// Camera turn right
	if (KeyBind::isPressed("me3d_camera_turn_right"))
	{
		renderer_3d->cameraTurn(fast ? -mult*2 : -mult);
		moving = true;
	}

	// Apply gravity to camera if needed
	if (camera_3d_gravity)
		renderer_3d->cameraApplyGravity(mult);

	// Update status bar
	fpoint3_t pos = renderer_3d->camPosition();
	string status_text = S_FMT("Position: (%d, %d, %d)", (int)pos.x, (int)pos.y, (int)pos.z);
	theMapEditor->CallAfter(&MapEditorWindow::SetStatusText, status_text, 3);

	return moving;
}

/* MapCanvas::update
 * Updates the current map editor state (animations etc.), given time
 * since the last frame [frametime]
 *******************************************************************/
void MapCanvas::update(long frametime)
{
	// Get frame time multiplier
	float mult = (float)frametime / 10.0f;

	// Update stuff depending on (2d/3d) mode
	bool mode_anim = false;
	if (editor->editMode() == MapEditor::MODE_3D)
		mode_anim = update3d(mult);
	else
		mode_anim = update2d(mult);

	// Flashing animation for hilight
	// Pulsates between 0.5-1.0f (multiplied with hilight alpha)
	if (anim_flash_inc)
	{
		if (anim_flash_level < 0.5f)
			anim_flash_level += 0.053*mult;	// Initial fade in
		else
			anim_flash_level += 0.015f*mult;
		if (anim_flash_level >= 1.0f)
		{
			anim_flash_inc = false;
			anim_flash_level = 1.0f;
		}
	}
	else
	{
		anim_flash_level -= 0.015f*mult;
		if (anim_flash_level <= 0.5f)
		{
			anim_flash_inc = true;
			anim_flash_level = 0.6f;
		}
	}

	// Fader for info overlay
	bool fade_anim = true;
	if (anim_info_show && !overlayActive())
	{
		anim_info_fade += 0.1f*mult;
		if (anim_info_fade > 1.0f)
		{
			anim_info_fade = 1.0f;
			fade_anim = false;
		}
	}
	else
	{
		anim_info_fade -= 0.04f*mult;
		if (anim_info_fade < 0.0f)
		{
			anim_info_fade = 0.0f;
			fade_anim = false;
		}
	}

	// Fader for fullscreen overlay
	bool overlay_fade_anim = true;
	if (overlayActive())
	{
		anim_overlay_fade += 0.1f*mult;
		if (anim_overlay_fade > 1.0f)
		{
			anim_overlay_fade = 1.0f;
			overlay_fade_anim = false;
		}
	}
	else
	{
		anim_overlay_fade -= 0.05f*mult;
		if (anim_overlay_fade < 0.0f)
		{
			anim_overlay_fade = 0.0f;
			overlay_fade_anim = false;
		}
	}

	// Fader for help text
	bool help_fade_anim = true;
	if (helpActive())
	{
		anim_help_fade += 0.07f*mult;
		if (anim_help_fade > 1.0f)
		{
			anim_help_fade = 1.0f;
			help_fade_anim = false;
		}
	}
	else
	{
		anim_help_fade -= 0.05f*mult;
		if (anim_help_fade < 0.0f)
		{
			anim_help_fade = 0.0f;
			help_fade_anim = false;
		}
	}

	// Update overlay animation (if active)
	if (overlayActive())
		overlay_current->update(frametime);

	// Update animations
	bool anim_running = false;
	for (unsigned a = 0; a < animations.size(); a++)
	{
		if (!animations[a]->update(theApp->runTimer()))
		{
			// If animation is finished, delete and remove from the list
			delete animations[a];
			animations.erase(animations.begin() + a);
			a--;
		}
		else
			anim_running = true;
	}

	// Determine the framerate limit
#ifdef USE_SFML_RENDERWINDOW
	// SFML RenderWindow can handle high framerates better than wxGLCanvas, or something like that
	if (mode_anim || fade_anim || overlay_fade_anim || help_fade_anim || anim_running)
		fr_idle = 2;
	else	// No high-priority animations running, throttle framerate
		fr_idle = map_bg_ms;
#else
	//if (mode_anim || fade_anim || overlay_fade_anim || help_fade_anim || anim_running)
		fr_idle = 10;
	//else	// No high-priority animations running, throttle framerate
	//	fr_idle = map_bg_ms;
#endif

	frametime_last = frametime;
}

/* MapCanvas::mouseToCenter
 * Moves the mouse cursor to the center of the canvas
 *******************************************************************/
void MapCanvas::mouseToCenter()
{
	wxRect rect = GetScreenRect();
	mouse_warp = true;
	sf::Mouse::setPosition(sf::Vector2i(rect.x + int(rect.width*0.5), rect.y + int(rect.height*0.5)));
}

/* MapCanvas::lockMouse
 * Locks/unlocks the mouse cursor. A locked cursor is invisible and
 * will be moved to the center of the canvas every frame
 *******************************************************************/
void MapCanvas::lockMouse(bool lock)
{
	mouse_locked = lock;
	if (lock)
	{
		// Center mouse
		mouseToCenter();

		// Hide cursor
		wxImage img(32, 32, true);
		img.SetMask(true);
		img.SetMaskColour(0, 0, 0);
		SetCursor(wxCursor(img));
#ifdef USE_SFML_RENDERWINDOW
		setMouseCursorVisible(false);
#endif
	}
	else
	{
		// Show cursor
		SetCursor(wxNullCursor);
#ifdef USE_SFML_RENDERWINDOW
		setMouseCursorVisible(true);
#endif
	}
}

/* MapCanvas::determineObjectEditState
 * Determines the current object edit state depending on the mouse
 * cursor position relative to the object edit bounding box
 *******************************************************************/
void MapCanvas::determineObjectEditState()
{
	// Get object edit bbox
	bbox_t bbox = editor->getObjectEditGroup()->getBBox();
	int bbox_pad = 8;
	int left = screenX(bbox.min.x) - bbox_pad;
	int right = screenX(bbox.max.x) + bbox_pad;
	int top = screenY(bbox.max.y) - bbox_pad;
	int bottom = screenY(bbox.min.y) + bbox_pad;
	edit_rotate = ((modifiers_current & wxMOD_CONTROL) > 0);

	// Check mouse position relative to bbox
	if (mouse_pos.x < left || mouse_pos.x > right || mouse_pos.y < top || mouse_pos.y > bottom)
	{
		// Outside bbox
		this->SetCursor(wxNullCursor);
		edit_state = ESTATE_NONE;
	}
	else
	{
		// Inside bbox
		if (mouse_pos.x < left + bbox_pad && bbox.width() > 0)
		{
			// Left side
			if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
			{
				// Top left
				this->SetCursor(wxCursor(edit_rotate ? wxCURSOR_CROSS : wxCURSOR_SIZENWSE));
				edit_state = ESTATE_SIZE_TL;
			}
			else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
			{
				// Bottom left
				this->SetCursor(wxCursor(edit_rotate ? wxCURSOR_CROSS : wxCURSOR_SIZENESW));
				edit_state = ESTATE_SIZE_BL;
			}
			else if (!edit_rotate)
			{
				// Left
				this->SetCursor(wxCursor(wxCURSOR_SIZEWE));
				edit_state = ESTATE_SIZE_L;
			}
		}
		else if (mouse_pos.x > right - bbox_pad && bbox.width() > 0)
		{
			// Right side
			if (mouse_pos.y < top + bbox_pad && bbox.height() > 0)
			{
				// Top right
				this->SetCursor(wxCursor(edit_rotate ? wxCURSOR_CROSS : wxCURSOR_SIZENESW));
				edit_state = ESTATE_SIZE_TR;
			}
			else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0)
			{
				// Bottom right
				this->SetCursor(wxCursor(edit_rotate ? wxCURSOR_CROSS : wxCURSOR_SIZENWSE));
				edit_state = ESTATE_SIZE_BR;
			}
			else if (!edit_rotate)
			{
				// Right
				this->SetCursor(wxCursor(wxCURSOR_SIZEWE));
				edit_state = ESTATE_SIZE_R;
			}
		}
		else if (mouse_pos.y < top + bbox_pad && bbox.height() > 0 && !edit_rotate)
		{
			// Top
			this->SetCursor(wxCursor(wxCURSOR_SIZENS));
			edit_state = ESTATE_SIZE_T;
		}
		else if (mouse_pos.y > bottom - bbox_pad && bbox.height() > 0 && !edit_rotate)
		{
			// Bottom
			this->SetCursor(wxCursor(wxCURSOR_SIZENS));
			edit_state = ESTATE_SIZE_B;
		}
		else
		{
			// Middle
			this->SetCursor(edit_rotate ? wxNullCursor : wxCursor(wxCURSOR_SIZING));
			edit_state = edit_rotate ? ESTATE_NONE : ESTATE_MOVE;
		}
	}
}

/* MapCanvas::mouseLook3d
 * Handles 3d mode mouselook
 *******************************************************************/
void MapCanvas::mouseLook3d()
{
	// Check for 3d mode
	if (editor->editMode() == MapEditor::MODE_3D && mouse_locked)
	{
		if (!overlay_current || !overlayActive() || (overlay_current && overlay_current->allow3dMlook()))
		{
			// Get relative mouse movement
			int xpos = wxGetMousePosition().x - GetScreenPosition().x;
			int ypos = wxGetMousePosition().y - GetScreenPosition().y;
			double xrel = xpos - int(GetSize().x * 0.5);
			double yrel = ypos - int(GetSize().y * 0.5);

			if (xrel != 0 || yrel != 0)
			{
                renderer_3d->cameraTurn(-xrel*0.1*camera_3d_sensitivity_x);
				if (mlook_invert_y)
                    renderer_3d->cameraPitch(yrel*0.003*camera_3d_sensitivity_y);
				else
                    renderer_3d->cameraPitch(-yrel*0.003*camera_3d_sensitivity_y);

				mouseToCenter();
				fr_idle = 0;
			}
		}
	}
}

/* MapCanvas::itemSelected
 * Called when the item at [index] is selected or deselected (for
 * select/deselect animations)
 *******************************************************************/
void MapCanvas::itemSelected(int index, bool selected)
{
	// Things mode
	if (editor->editMode() == MapEditor::MODE_THINGS)
	{
		// Get thing
		MapThing* t = editor->getMap().getThing(index);
		if (!t) return;

		// Get thing type
		ThingType* tt = theGameConfiguration->thingType(t->getType());

		// Start animation
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = renderer_2d->scaledRadius(radius);
		animations.push_back(new MCAThingSelection(theApp->runTimer(), t->xPos(), t->yPos(), radius, renderer_2d->viewScaleInv(), selected));
	}

	// Lines mode
	else if (editor->editMode() == MapEditor::MODE_LINES)
	{
		// Get line
		vector<MapLine*> vec;
		vec.push_back(editor->getMap().getLine(index));

		// Start animation
		animations.push_back(new MCALineSelection(theApp->runTimer(), vec, selected));
	}

	// Vertices mode
	else if (editor->editMode() == MapEditor::MODE_VERTICES)
	{
		// Get vertex
		vector<MapVertex*> verts;
		verts.push_back(editor->getMap().getVertex(index));

		// Determine current vertex size
		float vs = vertex_size;
		if (view_scale < 1.0) vs *= view_scale;
		if (vs < 2.0) vs = 2.0;

		// Start animation
		animations.push_back(new MCAVertexSelection(theApp->runTimer(), verts, vs, selected));
	}

	// Sectors mode
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
	{
		// Get sector polygon
		vector<Polygon2D*> polys;
		polys.push_back(editor->getMap().getSector(index)->getPolygon());

		// Start animation
		animations.push_back(new MCASectorSelection(theApp->runTimer(), polys, selected));
	}
}

/* MapCanvas::itemSelected
 * Called when multiple [items] are selected or deselected (for
 * select/deselect animations)
 *******************************************************************/
void MapCanvas::itemsSelected(vector<int>& items, bool selected)
{
	// Things mode
	if (editor->editMode() == MapEditor::MODE_THINGS)
	{
		// Go through selection
		for (unsigned a = 0; a < items.size(); a++)
			itemSelected(items[a], selected);
	}

	// Lines mode
	else if (editor->editMode() == MapEditor::MODE_LINES)
	{
		vector<MapLine*> lines;
		for (unsigned a = 0; a < items.size(); a++)
			lines.push_back(editor->getMap().getLine(items[a]));

		// Start animation
		animations.push_back(new MCALineSelection(theApp->runTimer(), lines, selected));
	}

	// Vertices mode
	else if (editor->editMode() == MapEditor::MODE_VERTICES)
	{
		// Get list of vertices
		vector<MapVertex*> verts;
		for (unsigned a = 0; a < items.size(); a++)
			verts.push_back(editor->getMap().getVertex(items[a]));

		// Determine current vertex size
		float vs = vertex_size;
		if (view_scale < 1.0) vs *= view_scale;
		if (vs < 2.0) vs = 2.0;

		// Start animation
		animations.push_back(new MCAVertexSelection(theApp->runTimer(), verts, vs, selected));
	}

	// Sectors mode
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
	{
		// Get list of sector polygons
		vector<Polygon2D*> polys;
		for (unsigned a = 0; a < items.size(); a++)
			polys.push_back(editor->getMap().getSector(items[a])->getPolygon());

		// Start animation
		animations.push_back(new MCASectorSelection(theApp->runTimer(), polys, selected));
	}
}

/* MapCanvas::itemSelected3d
 * Called when 3d mode [item] is selected or deselected (for
 * select/deselect animations)
 *******************************************************************/
void MapCanvas::itemSelected3d(selection_3d_t item, bool selected)
{
	// Wall selected
	if (item.type == MapEditor::SEL_SIDE_BOTTOM ||
	        item.type == MapEditor::SEL_SIDE_TOP ||
	        item.type == MapEditor::SEL_SIDE_MIDDLE)
	{
		// Get quad
		MapRenderer3D::quad_3d_t* quad = renderer_3d->getQuad(item);

		if (quad)
		{
			// Get quad points
			fpoint3_t points[4];
			for (unsigned a = 0; a < 4; a++)
				points[a].set(quad->points[a].x, quad->points[a].y, quad->points[a].z);

			// Start animation
			animations.push_back(new MCA3dWallSelection(theApp->runTimer(), points, selected));
		}
	}

	// Flat selected
	if (item.type == MapEditor::SEL_CEILING || item.type == MapEditor::SEL_FLOOR)
	{
		// Get flat
		MapRenderer3D::flat_3d_t* flat = renderer_3d->getFlat(item);

		// Start animation
		if (flat)
			animations.push_back(new MCA3dFlatSelection(theApp->runTimer(), flat->sector, flat->plane, selected));
	}
}

/* MapCanvas::itemSelected3d
 * Called when multiple 3d mode [items] are selected or deselected
 * (for select/deselect animations)
 *******************************************************************/
void MapCanvas::itemsSelected3d(vector<selection_3d_t>& items, bool selected)
{
	// Just do one animation per item in 3d mode
	for (unsigned a = 0; a < items.size(); a++)
		itemSelected3d(items[a], selected);
}

/* MapCanvas::updateInfoOverlay
 * Updates the current info overlay, depending on edit mode
 *******************************************************************/
void MapCanvas::updateInfoOverlay()
{
	// Update info overlay depending on edit mode
	switch (editor->editMode())
	{
	case MapEditor::MODE_VERTICES:	info_vertex.update(editor->getHilightedVertex()); break;
	case MapEditor::MODE_LINES:		info_line.update(editor->getHilightedLine()); break;
	case MapEditor::MODE_SECTORS:	info_sector.update(editor->getHilightedSector()); break;
	case MapEditor::MODE_THINGS:	info_thing.update(editor->getHilightedThing()); break;
	}
}

/* MapCanvas::forceRefreshRenderer
 * Forces a full refresh of the 2d/3d renderers
 *******************************************************************/
void MapCanvas::forceRefreshRenderer()
{
	// Update 3d mode info overlay if needed
	if (editor->editMode() == MapEditor::MODE_3D)
	{
		selection_3d_t hl;
		hl = renderer_3d->determineHilight();
		info_3d.update(hl.index, hl.type, &(editor->getMap()));
	}

	if (!setActive())
		return;

	renderer_2d->forceUpdate();
	renderer_3d->clearData();
}

/* MapCanvas::changeEditMode
 * Changes the edit mode to [mode]
 *******************************************************************/
void MapCanvas::changeEditMode(int mode)
{
	// Set edit mode
	int mode_prev = editor->editMode();
	editor->setEditMode(mode);

	// Unlock mouse
	lockMouse(false);

	// Update toolbar
	if (mode != mode_prev) theMapEditor->removeAllCustomToolBars();
	if (mode == MapEditor::MODE_VERTICES)
		theApp->toggleAction("mapw_mode_vertices");
	else if (mode == MapEditor::MODE_LINES)
		theApp->toggleAction("mapw_mode_lines");
	else if (mode == MapEditor::MODE_SECTORS)
	{
		theApp->toggleAction("mapw_mode_sectors");

		// Sector mode toolbar
		if (mode_prev != MapEditor::MODE_SECTORS)
		{
			wxArrayString actions;
			actions.Add("mapw_sectormode_normal");
			actions.Add("mapw_sectormode_floor");
			actions.Add("mapw_sectormode_ceiling");
			theMapEditor->addCustomToolBar("Sector Mode", actions);
		}

		// Toggle current sector mode
		if (editor->sectorEditMode() == MapEditor::SECTOR_BOTH)
			theApp->toggleAction("mapw_sectormode_normal");
		else if (editor->sectorEditMode() == MapEditor::SECTOR_FLOOR)
			theApp->toggleAction("mapw_sectormode_floor");
		else if (editor->sectorEditMode() == MapEditor::SECTOR_CEILING)
			theApp->toggleAction("mapw_sectormode_ceiling");
	}
	else if (mode == MapEditor::MODE_THINGS)
		theApp->toggleAction("mapw_mode_things");
	else if (mode == MapEditor::MODE_3D)
	{
		theApp->toggleAction("mapw_mode_3d");
		KeyBind::releaseAll();
		lockMouse(true);
		renderer_3d->refresh();
	}
	theMapEditor->refreshToolBar();

	// Refresh
	//if (mode != MapEditor::MODE_LINES)
	//	renderer_2d->forceUpdate(fade_lines);
	//else
	//	renderer_2d->forceUpdate(1.0f);
}

/* MapCanvas::changeThingType
 * Opens the thing type browser for the currently selected thing(s)
 *******************************************************************/
void MapCanvas::changeThingType()
{
	// Determine the initial type
	int type = -1;
	vector<MapThing*> selection;
	editor->getSelectedThings(selection);
	if (selection.size() > 0)
		type = selection[0]->intProperty("type");
	else
		return;

	// Lock hilight in the editor
	bool hl_lock = editor->hilightLocked();
	editor->lockHilight();

	// Open type browser
	ThingTypeBrowser browser(theMapEditor, type);
	if (browser.ShowModal() == wxID_OK)
	{
		editor->changeThingType(browser.getSelectedType());
		editor->copyProperties(selection[0]);
	}

	// Unlock hilight if needed
	editor->lockHilight(hl_lock);
}

/* MapCanvas::changeSectorTexture
 * Depending on the current sector edit mode, either opens the
 * sector texture overlay (normal) or browses for the ceiling or
 * floor texture (ceiling/floor edit mode)
 *******************************************************************/
void MapCanvas::changeSectorTexture()
{
	// Determine the initial texture
	string texture = "";
	string browser_title;
	string undo_name;
	vector<MapSector*> selection;
	editor->getSelectedSectors(selection);
	if (selection.size() > 0)
	{
		// Check edit mode
		if (editor->sectorEditMode() == MapEditor::SECTOR_FLOOR)
		{
			texture = selection[0]->stringProperty("texturefloor");
			browser_title = "Browse Floor Texture";
			undo_name = "Change Floor Texture";
		}
		else if (editor->sectorEditMode() == MapEditor::SECTOR_CEILING)
		{
			texture = selection[0]->stringProperty("textureceiling");
			browser_title = "Browse Ceiling Texture";
			undo_name = "Change Ceiling Texture";
		}
		else
		{
			if (overlay_current) delete overlay_current;
			SectorTextureOverlay* sto = new SectorTextureOverlay();
			sto->openSectors(selection);
			overlay_current = sto;
			return;
		}
	}
	else
		return;

	// Lock hilight in the editor
	bool hl_lock = editor->hilightLocked();
	editor->lockHilight();

	// Open texture browser
	MapTextureBrowser browser(theMapEditor, 1, texture, &(theMapEditor->mapEditor().getMap()));
	browser.SetTitle(browser_title);
	if (browser.ShowModal() == wxID_OK)
	{
		// Set texture depending on edit mode
		editor->beginUndoRecord(undo_name, true, false, false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (editor->sectorEditMode() == MapEditor::SECTOR_FLOOR)
				selection[a]->setStringProperty("texturefloor", browser.getSelectedItem()->getName());
			else if (editor->sectorEditMode() == MapEditor::SECTOR_CEILING)
				selection[a]->setStringProperty("textureceiling", browser.getSelectedItem()->getName());
		}
		editor->endUndoRecord();
	}

	// Unlock hilight if needed
	editor->lockHilight(hl_lock);
	renderer_2d->clearTextureCache();
}

/* MapCanvas::changeThingType3d
 * Opens the thing type browser for the currently selected 3d mode
 * thing(s)
 *******************************************************************/
void MapCanvas::changeThingType3d(selection_3d_t first)
{
	// Get first selected thing
	MapThing* thing = editor->getMap().getThing(first.index);

	// Do nothing if no things selected/hilighted
	if (!thing)
		return;

	// Open type browser
	ThingTypeBrowser browser(theMapEditor, thing->getType());
	if (browser.ShowModal() == wxID_OK)
		editor->changeThingType(browser.getSelectedType());
}

/* MapCanvas::changeTexture3d
 * Opens the texture browser for the currently selected 3d mode walls
 * and/or floors
 *******************************************************************/
void MapCanvas::changeTexture3d(selection_3d_t first)
{
	// Check index
	if (first.index < 0)
		return;

	// Get initial texture
	string tex;
	int type = 0;
	if (first.type == MapEditor::SEL_FLOOR)
	{
		tex = editor->getMap().getSector(first.index)->getFloorTex();
		type = 1;
	}
	else if (first.type == MapEditor::SEL_CEILING)
	{
		tex = editor->getMap().getSector(first.index)->getCeilingTex();
		type = 1;
	}
	else if (first.type == MapEditor::SEL_SIDE_BOTTOM)
		tex = editor->getMap().getSide(first.index)->stringProperty("texturebottom");
	else if (first.type == MapEditor::SEL_SIDE_MIDDLE)
		tex = editor->getMap().getSide(first.index)->stringProperty("texturemiddle");
	else if (first.type == MapEditor::SEL_SIDE_TOP)
		tex = editor->getMap().getSide(first.index)->stringProperty("texturetop");

	// Open texture browser
	MapTextureBrowser browser(theMapEditor, type, tex, &(theMapEditor->mapEditor().getMap()));
	browser.SetTitle("Browse Texture");
	if (browser.ShowModal() == wxID_OK && browser.getSelectedItem() != NULL)
	{
		bool mix = theGameConfiguration->mixTexFlats();
		tex = browser.getSelectedItem()->getName();
		selection_3d_t hl = editor->hilightItem3d();

		// Begin undo level
		editor->beginUndoRecord("Change Texture", true, false, false);

		// Apply to flats
		vector<selection_3d_t>& selection = editor->get3dSelection();
		if (mix || type == 1)
		{
			// Selection
			if (selection.size() > 0)
			{
				for (unsigned a = 0; a < selection.size(); a++)
				{
					if (selection[a].type == MapEditor::SEL_FLOOR)
						editor->getMap().getSector(selection[a].index)->setStringProperty("texturefloor", tex);
					else if (selection[a].type == MapEditor::SEL_CEILING)
						editor->getMap().getSector(selection[a].index)->setStringProperty("textureceiling", tex);
				}
			}
			else if (hl.index >= 0)
			{
				// Hilight if no selection
				if (hl.type == MapEditor::SEL_FLOOR)
					editor->getMap().getSector(hl.index)->setStringProperty("texturefloor", tex);
				else if (hl.type == MapEditor::SEL_CEILING)
					editor->getMap().getSector(hl.index)->setStringProperty("textureceiling", tex);
			}
		}

		// Apply to walls
		if (mix || type == 0)
		{
			// Selection
			if (selection.size() > 0)
			{
				for (unsigned a = 0; a < selection.size(); a++)
				{
					if (selection[a].type == MapEditor::SEL_SIDE_BOTTOM)
						editor->getMap().getSide(selection[a].index)->setStringProperty("texturebottom", tex);
					else if (selection[a].type == MapEditor::SEL_SIDE_MIDDLE)
						editor->getMap().getSide(selection[a].index)->setStringProperty("texturemiddle", tex);
					else if (selection[a].type == MapEditor::SEL_SIDE_TOP)
						editor->getMap().getSide(selection[a].index)->setStringProperty("texturetop", tex);
				}
			}
			else if (hl.index >= 0)
			{
				// Hilight if no selection
				if (hl.type == MapEditor::SEL_SIDE_BOTTOM)
					editor->getMap().getSide(hl.index)->setStringProperty("texturebottom", tex);
				else if (hl.type == MapEditor::SEL_SIDE_MIDDLE)
					editor->getMap().getSide(hl.index)->setStringProperty("texturemiddle", tex);
				else if (hl.type == MapEditor::SEL_SIDE_TOP)
					editor->getMap().getSide(hl.index)->setStringProperty("texturetop", tex);
			}
		}

		// End undo level
		editor->endUndoRecord();
	}
}

/* MapCanvas::editObjectProperties
 * Opens a dialog containing a MapObjectPropsPanel to edit properties
 * for all objects in [list]
 *******************************************************************/
void MapCanvas::editObjectProperties(vector<MapObject*>& list)
{
	// Determine selection type
	string type = "Object";
	if (editor->editMode() == MapEditor::MODE_VERTICES)
		type = "Vertex";
	else if (editor->editMode() == MapEditor::MODE_LINES)
		type = "Line";
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
		type = "Sector";
	else if (editor->editMode() == MapEditor::MODE_THINGS)
		type = "Thing";

	// Begin recording undo level
	editor->beginUndoRecord(S_FMT("Property Edit (%s)", type));
	for (unsigned a = 0; a < list.size(); a++)
		editor->recordPropertyChangeUndoStep(list[a]);

	string selsize = "";
	if (list.size() == 1)
		type += S_FMT(" #%d", list[0]->getIndex());
	else if (list.size() > 1)
		selsize = S_FMT("(%lu selected)", list.size());

	// Create dialog for properties panel
	SDialog dlg(theMapEditor, S_FMT("%s Properties %s", type, selsize), S_FMT("mobjprops_%d", editor->editMode()), -1, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Create properties panel
	PropsPanelBase* panel_props = NULL;
	if (editor->editMode() == MapEditor::MODE_LINES)
		sizer->Add(panel_props = new LinePropsPanel(&dlg), 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	else if (editor->editMode() == MapEditor::MODE_SECTORS)
		sizer->Add(panel_props = new SectorPropsPanel(&dlg), 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	else if (editor->editMode() == MapEditor::MODE_THINGS)
		sizer->Add(panel_props = new ThingPropsPanel(&dlg), 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	else
		sizer->Add(panel_props = new MapObjectPropsPanel(&dlg, true), 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

	// Add dialog buttons
	sizer->AddSpacer(4);
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Open current selection
	panel_props->openObjects(list);

	// Open the dialog and apply changes if OK was clicked
	dlg.SetMinClientSize(sizer->GetMinSize());
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		panel_props->applyChanges();
		renderer_2d->forceUpdate(fade_lines);
		editor->updateDisplay();

		if (editor->editMode() == MapEditor::MODE_THINGS)
			editor->copyProperties(list[0]);
	}

	// End undo level
	editor->endUndoRecord(true);

	// Clear property grid to avoid crash (wxPropertyGrid is at fault there)
	//if (panel_props)
	//	panel_props->clearGrid();
}

/* MapCanvas::beginLineDraw
 * Sets up and begins line drawing
 *******************************************************************/
void MapCanvas::beginLineDraw()
{
	draw_state = DSTATE_LINE;
	mouse_state = MSTATE_LINE_DRAW;

	// Setup help text
	string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
	string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
	feature_help_lines.clear();
	feature_help_lines.push_back("Line Drawing");
	feature_help_lines.push_back(S_FMT("%s = Accept", key_accept));
	feature_help_lines.push_back(S_FMT("%s = Cancel", key_cancel));
	feature_help_lines.push_back("Left Click = Draw point");
	feature_help_lines.push_back("Right Click = Undo previous point");
	feature_help_lines.push_back("Shift = Snap to nearest vertex");
}

/* MapCanvas::beginShapeDraw
 * Sets up and begins shape drawing
 *******************************************************************/
void MapCanvas::beginShapeDraw()
{
	draw_state = DSTATE_SHAPE_ORIGIN;
	mouse_state = MSTATE_LINE_DRAW;
	theMapEditor->showShapeDrawPanel();

	// Setup help text
	string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
	string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
	feature_help_lines.clear();
	feature_help_lines.push_back("Shape Drawing");
	feature_help_lines.push_back(S_FMT("%s = Accept", key_accept));
	feature_help_lines.push_back(S_FMT("%s = Cancel", key_cancel));
	feature_help_lines.push_back("Left Click = Draw point");
	feature_help_lines.push_back("Right Click = Undo previous point");
}

/* MapCanvas::beginObjectEdit
 * Sets up and begins object edit
 *******************************************************************/
void MapCanvas::beginObjectEdit()
{
	if (editor->beginObjectEdit())
	{
		mouse_state = MSTATE_EDIT;
		renderer_2d->forceUpdate();

		// Setup help text
		string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
		string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
		string key_toggle = KeyBind::getBind("me2d_begin_object_edit").keysAsString();
		feature_help_lines.clear();
		feature_help_lines.push_back("Object Edit");
		feature_help_lines.push_back(S_FMT("%s = Accept", key_accept));
		feature_help_lines.push_back(S_FMT("%s or %s = Cancel", key_cancel, key_toggle));
		feature_help_lines.push_back("Shift = Disable grid snapping");
		feature_help_lines.push_back("Ctrl = Rotate");
	}
}

/* MapCanvas::onKeyBindPress
 * Called when the key bind [name] is pressed
 *******************************************************************/
void MapCanvas::onKeyBindPress(string name)
{
	// Check if an overlay is active
	if (overlayActive())
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			overlay_current->close();
			renderer_3d->enableHilight(true);
			renderer_3d->enableSelection(true);
		}

		// Cancel edit
		else if (name == "map_edit_cancel")
		{
			overlay_current->close(true);
			renderer_3d->enableHilight(true);
			renderer_3d->enableSelection(true);
		}

		// Nothing else
		return;
	}

	// Toggle 3d mode
	if (name == "map_toggle_3d")
	{
		if (editor->editMode() == MapEditor::MODE_3D)
			changeEditMode(mode_last);
		else
		{
			mode_last = editor->editMode();
			changeEditMode(MapEditor::MODE_3D);
		}
	}

	// Screenshot
#ifdef USE_SFML_RENDERWINDOW
	else if (name == "map_screenshot")
	{
		// Capture shot
		sf::Image shot = capture();

		// Remove alpha
		// sf::Image kinda sucks, shouldn't have to do it like this
		for (unsigned x = 0; x < shot.getSize().x; x++)
		{
			for (unsigned y = 0; y < shot.getSize().y; y++)
			{
				sf::Color col = shot.getPixel(x, y);
				shot.setPixel(x, y, sf::Color(col.r, col.g, col.b, 255));
			}
		}

		// Save to file
		wxDateTime date;
		date.SetToCurrent();
		string timestamp = date.FormatISOCombined('-');
		timestamp.Replace(":", "");
		string filename = appPath(S_FMT("sladeshot-%s.png", timestamp), DIR_USER);
		if (shot.saveToFile(UTF8(filename)))
		{
			// Editor message if the file is actually written, with full path
			editor->addEditorMessage(S_FMT("Screenshot taken (%s)", filename));
		}
		else
		{
			// Editor message also if the file couldn't be written
			editor->addEditorMessage(S_FMT("Screenshot failed (%s)", filename));
		}

	}
#endif

	// Send to editor first
	if (mouse_state == MSTATE_NORMAL)
	{
		if (editor->handleKeyBind(name, mouse_pos_m))
			return;
	}

	// Handle keybinds depending on mode
	if (editor->editMode() == MapEditor::MODE_3D)
		keyBinds3d(name);
	else
	{
		keyBinds2dView(name);
		keyBinds2d(name);
	}
}

/* MapCanvas::keyBinds2dView
 * Handles 2d mode view-related keybinds (can generally be used no
 * matter the current editor state)
 *******************************************************************/
void MapCanvas::keyBinds2dView(string name)
{
	// Pan left
	if (name == "me2d_left")
		pan(-128/view_scale, 0);

	// Pan right
	else if (name == "me2d_right")
		pan(128/view_scale, 0);

	// Pan up
	else if (name == "me2d_up")
		pan(0, 128/view_scale);

	// Pan down
	else if (name == "me2d_down")
		pan(0, -128/view_scale);

	// Zoom out
	else if (name == "me2d_zoom_out")
		zoom(0.8);

	// Zoom in
	else if (name == "me2d_zoom_in")
		zoom(1.25);

	// Zoom out (follow mouse)
	else if (name == "me2d_zoom_out_m")
	{
		zoom(1.0 - (0.2 * mwheel_rotation), true);
		zooming_cursor = true;
	}

	// Zoom in (follow mouse)
	else if (name == "me2d_zoom_in_m")
	{
		zoom(1.0 + (0.25 * mwheel_rotation), true);
		zooming_cursor = true;
	}

	// Zoom in (show object)
	else if (name == "me2d_show_object")
		viewShowObject();

	// Zoom out (full map)
	else if (name == "me2d_show_all")
		viewFitToMap();

	// Pan view
	else if (name == "me2d_pan_view")
	{
		mouse_downpos.set(mouse_pos);
		panning = true;
		if (mouse_state == MSTATE_NORMAL)
			editor->clearHilight();
		SetCursor(wxCURSOR_SIZING);
	}

	// Increment grid
	else if (name == "me2d_grid_inc")
		editor->incrementGrid();

	// Decrement grid
	else if (name == "me2d_grid_dec")
		editor->decrementGrid();
}

/* MapCanvas::keyBinds2d
 * Handles 2d mode key binds
 *******************************************************************/
void MapCanvas::keyBinds2d(string name)
{
	// --- Line Drawing ---
	if (mouse_state == MSTATE_LINE_DRAW)
	{
		// Accept line draw
		if (name == "map_edit_accept")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endLineDraw();
			theMapEditor->showShapeDrawPanel(false);
		}

		// Cancel line draw
		else if (name == "map_edit_cancel")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endLineDraw(false);
			theMapEditor->showShapeDrawPanel(false);
		}
	}

	// --- Paste ---
	else if (mouse_state == MSTATE_PASTE)
	{
		// Accept paste
		if (name == "map_edit_accept")
		{
			mouse_state = MSTATE_NORMAL;
			editor->paste(mouse_pos_m);
		}

		// Cancel paste
		else if (name == "map_edit_cancel")
			mouse_state = MSTATE_NORMAL;
	}

	// --- Tag edit ---
	else if (mouse_state == MSTATE_TAG_SECTORS)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endTagEdit(false);
		}
	}
	else if (mouse_state == MSTATE_TAG_THINGS)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state = MSTATE_NORMAL;
			editor->endTagEdit(false);
		}
	}

	// --- Moving ---
	else if (mouse_state == MSTATE_MOVE)
	{
		// Move toggle
		if (name == "me2d_move")
		{
			editor->endMove();
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
		}

		// Accept move
		else if (name == "map_edit_accept")
		{
			editor->endMove();
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
		}

		// Cancel move
		else if (name == "map_edit_cancel")
		{
			editor->endMove(false);
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
		}
	}

	// --- Object Edit ---
	else if (mouse_state == MSTATE_EDIT)
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			editor->endObjectEdit(true);
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
			this->SetCursor(wxNullCursor);
		}

		// Cancel edit
		else if (name == "map_edit_cancel" || name == "me2d_begin_object_edit")
		{
			editor->endObjectEdit(false);
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
			this->SetCursor(wxNullCursor);
		}
	}

	// --- Normal mouse state ---
	else if (mouse_state == MSTATE_NORMAL)
	{

		// --- All edit modes ---

		// Vertices mode
		if (name == "me2d_mode_vertices")
			changeEditMode(MapEditor::MODE_VERTICES);

		// Lines mode
		else if (name == "me2d_mode_lines")
			changeEditMode(MapEditor::MODE_LINES);

		// Sectors mode
		else if (name == "me2d_mode_sectors")
			changeEditMode(MapEditor::MODE_SECTORS);

		// Things mode
		else if (name == "me2d_mode_things")
			changeEditMode(MapEditor::MODE_THINGS);

		// 3d mode
		else if (name == "me2d_mode_3d")
			changeEditMode(MapEditor::MODE_3D);

		// Cycle flat type
		else if (name == "me2d_flat_type")
		{
			flat_drawtype = flat_drawtype + 1;
			if (flat_drawtype > 2)
				flat_drawtype = 0;

			// Editor message and toolbar update
			switch (flat_drawtype)
			{
			case 0: editor->addEditorMessage("Flats: None"); theApp->toggleAction("mapw_flat_none"); break;
			case 1: editor->addEditorMessage("Flats: Untextured"); theApp->toggleAction("mapw_flat_untextured"); break;
			case 2: editor->addEditorMessage("Flats: Textured"); theApp->toggleAction("mapw_flat_textured"); break;
			default: break;
			};

			theMapEditor->refreshToolBar();
		}

		// Move items (toggle)
		else if (name == "me2d_move")
		{
			if (editor->beginMove(mouse_pos_m))
			{
				mouse_state = MSTATE_MOVE;
				renderer_2d->forceUpdate();
			}
		}

		// Edit items
		else if (name == "me2d_begin_object_edit")
			beginObjectEdit();

		// Split line
		else if (name == "me2d_split_line")
			editor->splitLine(mouse_pos_m.x, mouse_pos_m.y, 16/view_scale);

		// Begin line drawing
		else if (name == "me2d_begin_linedraw")
			beginLineDraw();

		// Begin shape drawing
		else if (name == "me2d_begin_shapedraw")
			beginShapeDraw();

		// Create object
		else if (name == "me2d_create_object")
		{
			// If in lines mode, begin line drawing
			if (editor->editMode() == MapEditor::MODE_LINES)
			{
				draw_state = DSTATE_LINE;
				mouse_state = MSTATE_LINE_DRAW;
			}
			else
				editor->createObject(mouse_pos_m.x, mouse_pos_m.y);
		}

		// Delete object
		else if (name == "me2d_delete_object")
			editor->deleteObject();

		// Copy properties
		else if (name == "me2d_copy_properties")
			editor->copyProperties();

		// Paste properties
		else if (name == "me2d_paste_properties")
			editor->pasteProperties();

		// Paste object(s)
		else if (name == "paste")
		{
			// Check if any data is copied
			ClipboardItem* item = NULL;
			for (unsigned a = 0; a < theClipboard->nItems(); a++)
			{
				if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH ||
				        theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_THINGS)
				{
					item = theClipboard->getItem(a);
					break;
				}
			}

			// Begin paste if appropriate data exists
			if (item)
				mouse_state = MSTATE_PASTE;
		}

		// Toggle selection numbers
		else if (name == "me2d_toggle_selection_numbers")
		{
			map_show_selection_numbers = !map_show_selection_numbers;

			if (map_show_selection_numbers)
				editor->addEditorMessage("Selection numbers enabled");
			else
				editor->addEditorMessage("Selection numbers disabled");
		}

		// Mirror
		else if (name == "me2d_mirror_x")
			editor->mirror(true);
		else if (name == "me2d_mirror_y")
			editor->mirror(false);

		// Object Properties
		else if (name == "me2d_object_properties")
		{
			vector<MapObject*> objects;
			editor->getSelectedObjects(objects);
			if (!objects.empty())
				editObjectProperties(objects);
		}


		// --- Lines edit mode ---
		if (editor->editMode() == MapEditor::MODE_LINES)
		{
			// Change line texture
			if (name == "me2d_line_change_texture")
			{
				// Get selection
				vector<MapLine*> selection;
				editor->getSelectedLines(selection);

				// Open line texture overlay if anything is selected
				if (selection.size() > 0)
				{
					if (overlay_current) delete overlay_current;
					LineTextureOverlay* lto = new LineTextureOverlay();
					lto->openLines(selection);
					overlay_current = lto;
				}
			}

			// Flip line
			else if (name == "me2d_line_flip")
				editor->flipLines();

			// Flip line (no sides)
			else if (name == "me2d_line_flip_nosides")
				editor->flipLines(false);

			// Edit line tags
			else if (name == "me2d_line_tag_edit")
			{
				if (editor->beginTagEdit() > 0)
				{
					mouse_state = MSTATE_TAG_SECTORS;

					// Setup help text
					string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
					string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
					feature_help_lines.clear();
					feature_help_lines.push_back("Tag Edit");
					feature_help_lines.push_back(S_FMT("%s = Accept", key_accept));
					feature_help_lines.push_back(S_FMT("%s = Cancel", key_cancel));
					feature_help_lines.push_back("Left Click = Toggle tagged sector");
				}
			}
		}


		// --- Things edit mode ---
		else if (editor->editMode() == MapEditor::MODE_THINGS)
		{
			// Change type
			if (name == "me2d_thing_change_type")
				changeThingType();

			// Quick angle
			else if (name == "me2d_thing_quick_angle")
			{
				if (mouse_state == MSTATE_NORMAL)
				{
					if (editor->isHilightOrSelection())
						editor->beginUndoRecord("Thing Direction Change", true, false, false);

					mouse_state = MSTATE_THING_ANGLE;
				}
			}
		}


		// --- Sectors edit mode ---
		else if (editor->editMode() == MapEditor::MODE_SECTORS)
		{
			// Change sector texture
			if (name == "me2d_sector_change_texture")
				changeSectorTexture();
		}
	}
}

/* MapCanvas::keyBinds3d
 * Handles 3d mode key binds
 *******************************************************************/
void MapCanvas::keyBinds3d(string name)
{
	// Escape from 3D mode
	if (name == "map_edit_cancel")
		changeEditMode(mode_last);

	// Toggle fog
	else if (name == "me3d_toggle_fog")
	{
		bool fog = renderer_3d->fogEnabled();
		renderer_3d->enableFog(!fog);
		if (fog)
			editor->addEditorMessage("Fog disabled");
		else
			editor->addEditorMessage("Fog enabled");
	}

	// Toggle fullbright
	else if (name == "me3d_toggle_fullbright")
	{
		bool fb = renderer_3d->fullbrightEnabled();
		renderer_3d->enableFullbright(!fb);
		if (fb)
			editor->addEditorMessage("Fullbright disabled");
		else
			editor->addEditorMessage("Fullbright enabled");
	}

	// Adjust brightness
	else if (name == "me3d_adjust_brightness")
	{
		render_3d_brightness = render_3d_brightness + 0.1;
		if (render_3d_brightness > 2.0)
		{
			render_3d_brightness = 1.0;
		}
		editor->addEditorMessage(S_FMT("Brightness set to %1.1f", (double)render_3d_brightness));
	}

	// Toggle gravity
	else if (name == "me3d_toggle_gravity")
	{
		camera_3d_gravity = !camera_3d_gravity;
		if (!camera_3d_gravity)
			editor->addEditorMessage("Gravity disabled");
		else
			editor->addEditorMessage("Gravity enabled");
	}

	// Release mouse cursor
	else if (name == "me3d_release_mouse")
		lockMouse(false);

	// Toggle things
	else if (name == "me3d_toggle_things")
	{
		// Change thing display type
		render_3d_things = render_3d_things + 1;
		if (render_3d_things > 2)
			render_3d_things = 0;

		// Editor message
		if (render_3d_things == 0)
			editor->addEditorMessage("Things disabled");
		else if (render_3d_things == 1)
			editor->addEditorMessage("Things enabled: All");
		else
			editor->addEditorMessage("Things enabled: Decorations only");
	}

	// Change thing render style
	else if (name == "me3d_thing_style")
	{
		// Change thing display style
		render_3d_things_style = render_3d_things_style + 1;
		if (render_3d_things_style > 2)
			render_3d_things_style = 0;

		// Editor message
		if (render_3d_things_style == 0)
			editor->addEditorMessage("Thing render style: Sprites only");
		else if (render_3d_things_style == 1)
			editor->addEditorMessage("Thing render style: Sprites + Ground boxes");
		else
			editor->addEditorMessage("Thing render style: Sprites + Full boxes");
	}

	// Toggle hilight
	else if (name == "me3d_toggle_hilight")
	{
		// Change hilight type
		render_3d_hilight = render_3d_hilight + 1;
		if (render_3d_hilight > 2)
			render_3d_hilight = 0;

		// Editor message
		if (render_3d_hilight == 0)
			editor->addEditorMessage("Hilight disabled");
		else if (render_3d_hilight == 1)
			editor->addEditorMessage("Hilight enabled: Outline");
		else if (render_3d_hilight == 2)
			editor->addEditorMessage("Hilight enabled: Solid");
	}

	// Toggle info overlay
	else if (name == "me3d_toggle_info")
		info_overlay_3d = !info_overlay_3d;

	// Quick texture
	else if (name == "me3d_quick_texture")
	{
		vector<selection_3d_t> sel;
		editor->get3dSelectionOrHilight(sel);

		if (QuickTextureOverlay3d::ok(sel))
		{
			if (overlay_current) delete overlay_current;
			QuickTextureOverlay3d* qto = new QuickTextureOverlay3d(editor);
			overlay_current = qto;

			renderer_3d->enableHilight(false);
			renderer_3d->enableSelection(false);
			editor->lockHilight(true);
		}
	}

	// Send to map editor
	else
		editor->handleKeyBind(name, mouse_pos_m);
}

/* MapCanvas::onKeyBindRelease
 * Called when keybind [name] is released
 *******************************************************************/
void MapCanvas::onKeyBindRelease(string name)
{
	if (name == "me2d_pan_view" && panning)
	{
		panning = false;
		if (mouse_state == MSTATE_NORMAL)
			editor->updateHilight(mouse_pos_m, view_scale);
		SetCursor(wxNullCursor);
	}

	else if (name == "me2d_thing_quick_angle" && mouse_state == MSTATE_THING_ANGLE)
	{
		mouse_state = MSTATE_NORMAL;
		editor->endUndoRecord(true);
		editor->updateHilight(mouse_pos_m, view_scale);
	}
}

/* MapCanvas::handleAction
 * Handles an SAction [id]. Returns true if the action was handled
 * here
 *******************************************************************/
bool MapCanvas::handleAction(string id)
{
	// Skip if not shown
	if (!IsShown())
		return false;

	// Skip if overlay is active
	if (overlayActive())
		return false;

	// Vertices mode
	if (id == "mapw_mode_vertices")
	{
		changeEditMode(MapEditor::MODE_VERTICES);
		return true;
	}

	// Lines mode
	else if (id == "mapw_mode_lines")
	{
		changeEditMode(MapEditor::MODE_LINES);
		return true;
	}

	// Sectors mode
	else if (id == "mapw_mode_sectors")
	{
		changeEditMode(MapEditor::MODE_SECTORS);
		return true;
	}

	// Things mode
	else if (id == "mapw_mode_things")
	{
		changeEditMode(MapEditor::MODE_THINGS);
		return true;
	}

	// 3d mode
	else if (id == "mapw_mode_3d")
	{
		SetFocusFromKbd();
		SetFocus();
		changeEditMode(MapEditor::MODE_3D);
		return true;
	}

	// 'None' (wireframe) flat type
	else if (id == "mapw_flat_none")
	{
		flat_drawtype = 0;
		return true;
	}

	// 'Untextured' flat type
	else if (id == "mapw_flat_untextured")
	{
		flat_drawtype = 1;
		return true;
	}

	// 'Textured' flat type
	else if (id == "mapw_flat_textured")
	{
		flat_drawtype = 2;
		return true;
	}

	// Normal sector edit mode
	else if (id == "mapw_sectormode_normal")
	{
		editor->setSectorEditMode(MapEditor::SECTOR_BOTH);
		return true;
	}

	// Floors sector edit mode
	else if (id == "mapw_sectormode_floor")
	{
		editor->setSectorEditMode(MapEditor::SECTOR_FLOOR);
		return true;
	}

	// Ceilings sector edit mode
	else if (id == "mapw_sectormode_ceiling")
	{
		editor->setSectorEditMode(MapEditor::SECTOR_CEILING);
		return true;
	}

	// Begin line drawing
	else if (id == "mapw_draw_lines" && mouse_state == MSTATE_NORMAL)
	{
		beginLineDraw();
		return true;
	}

	// Begin shape drawing
	else if (id == "mapw_draw_shape" && mouse_state == MSTATE_NORMAL)
	{
		beginShapeDraw();
		return true;
	}

	// Begin object edit
	else if (id == "mapw_edit_objects" && mouse_state == MSTATE_NORMAL)
	{
		beginObjectEdit();
		return true;
	}

	// Show full map
	else if (id == "mapw_show_fullmap")
	{
		viewFitToMap();
		return true;
	}

	// Show item
	else if (id == "mapw_show_item")
	{
		// Setup dialog
		ShowItemDialog dlg(this);
		switch (editor->editMode())
		{
		case MapEditor::MODE_VERTICES:
			dlg.setType(MOBJ_VERTEX); break;
		case MapEditor::MODE_LINES:
			dlg.setType(MOBJ_LINE); break;
		case MapEditor::MODE_SECTORS:
			dlg.setType(MOBJ_SECTOR); break;
		case MapEditor::MODE_THINGS:
			dlg.setType(MOBJ_THING); break;
		default:
			return true;
		}

		// Show dialog
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get entered index
			int index = dlg.getIndex();
			if (index < 0)
				return true;

			// Set appropriate edit mode
			bool side = false;
			switch (dlg.getType())
			{
			case MOBJ_VERTEX:
				editor->setEditMode(MapEditor::MODE_VERTICES); break;
			case MOBJ_LINE:
				editor->setEditMode(MapEditor::MODE_LINES); break;
			case MOBJ_SIDE:
				editor->setEditMode(MapEditor::MODE_LINES); side = true; break;
			case MOBJ_SECTOR:
				editor->setEditMode(MapEditor::MODE_SECTORS); break;
			case MOBJ_THING:
				editor->setEditMode(MapEditor::MODE_THINGS); break;
			default:
				break;
			}

			// If side, get its parent line
			if (side)
			{
				MapSide* s = editor->getMap().getSide(index);
				if (s && s->getParentLine())
					index = s->getParentLine()->getIndex();
				else
					index = -1;
			}

			// Show the item
			if (index > -1)
				editor->showItem(index);
		}

		return true;
	}

	// Mirror Y
	else if (id == "mapw_mirror_y")
	{
		editor->mirror(false);
		return true;
	}

	// Mirror X
	else if (id == "mapw_mirror_x")
	{
		editor->mirror(true);
		return true;
	}


	// --- Context menu ---

	// Move 3d mode camera
	else if (id == "mapw_camera_set")
	{
		fpoint3_t pos(mouse_pos_m);
		SLADEMap& map = editor->getMap();
		MapSector* sector = map.getSector(map.sectorAt(mouse_pos_m));
		if (sector)
			pos.z = sector->getFloorHeight() + 40;
		renderer_3d->cameraSetPosition(pos);
		return true;
	}

	// Edit item properties
	else if (id == "mapw_item_properties")
	{
		// Get selection
		vector<MapObject*> list;
		editor->getSelectedObjects(list);

		editObjectProperties(list);

		return true;
	}

	// --- Vertex context menu ---

	// Create vertex
	else if (id == "mapw_vertex_create")
	{
		editor->createVertex(mouse_downpos_m.x, mouse_downpos_m.y);
		return true;
	}

	// --- Line context menu ---

	// Change line texture
	else if (id == "mapw_line_changetexture")
	{
		// Get selection
		vector<MapLine*> selection;
		editor->getSelectedLines(selection);

		// Open line texture overlay if anything is selected
		if (selection.size() > 0)
		{
			if (overlay_current) delete overlay_current;
			LineTextureOverlay* lto = new LineTextureOverlay();
			lto->openLines(selection);
			overlay_current = lto;
		}
		return true;
	}

	// Change line special
	else if (id == "mapw_line_changespecial")
	{
		// Get selection
		//vector<MapLine*> selection;
		//editor->getSelectedLines(selection);
		vector<MapObject*> selection;
		editor->getSelectedObjects(selection);

		// Open action special selection dialog
		if (selection.size() > 0)
		{
			int as = -1;
			ActionSpecialDialog dlg(this, true);
			dlg.openLines(selection);
			if (dlg.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Line Special", true, false, false);
				dlg.applyTo(selection, true);
				editor->endUndoRecord();
				renderer_2d->forceUpdate();
			}
		}

		return true;
	}

	// Tag to
	else if (id == "mapw_line_tagedit")
	{
		int type = editor->beginTagEdit();
		if (type > 0)
		{
			mouse_state = MSTATE_TAG_SECTORS;

			// Setup help text
			string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
			string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
			feature_help_lines.clear();
			feature_help_lines.push_back("Tag Edit");
			feature_help_lines.push_back(S_FMT("%s = Accept", key_accept));
			feature_help_lines.push_back(S_FMT("%s = Cancel", key_cancel));
			feature_help_lines.push_back("Left Click = Toggle tagged sector");
		}

		return true;
	}

	// Correct sectors
	else if (id == "mapw_line_correctsectors")
	{
		editor->correctLineSectors();
		return true;
	}

	// Flip
	else if (id == "mapw_line_flip")
	{
		editor->flipLines();
		return true;
	}

	// --- Thing context menu ---

	// Change thing type
	else if (id == "mapw_thing_changetype")
	{
		changeThingType();
		return true;
	}

	// Create thing
	else if (id == "mapw_thing_create")
	{
		editor->createThing(mouse_downpos_m.x, mouse_downpos_m.y);
		return true;
	}

	// --- Sector context menu ---

	// Change sector texture
	else if (id == "mapw_sector_changetexture")
	{
		changeSectorTexture();
		return true;
	}

	// Change sector special
	else if (id == "mapw_sector_changespecial")
	{
		// Get selection
		vector<MapSector*> selection;
		editor->getSelectedSectors(selection);

		// Open sector special selection dialog
		if (selection.size() > 0)
		{
			SectorSpecialDialog dlg(this);
			dlg.setup(selection[0]->intProperty("special"));
			if (dlg.ShowModal() == wxID_OK)
			{
				// Set specials of selected sectors
				int special = dlg.getSelectedSpecial();
				editor->beginUndoRecord("Change Sector Special", true, false, false);
				for (unsigned a = 0; a < selection.size(); a++)
					selection[a]->setIntProperty("special", special);
				editor->endUndoRecord();
			}
		}
	}

	// Create sector
	else if (id == "mapw_sector_create")
	{
		editor->createSector(mouse_downpos_m.x, mouse_downpos_m.y);
		return true;
	}

	// Merge sectors
	else if (id == "mapw_sector_join")
	{
		editor->joinSectors(false);
		return true;
	}

	// Join sectors
	else if (id == "mapw_sector_join_keep")
	{
		editor->joinSectors(true);
		return true;
	}

	// Not handled here
	return false;
}


/*******************************************************************
 * MAPCANVAS CLASS EVENTS
 *******************************************************************/

/* MapCanvas::onSize
 * Called when the canvas is resized
 *******************************************************************/
void MapCanvas::onSize(wxSizeEvent& e)
{
	// Update screen limits
	view_tl.x = translateX(0);
	view_tl.y = translateY(GetSize().y);
	view_br.x = translateX(GetSize().x);
	view_br.y = translateY(0);

	// Update map item visibility
	renderer_2d->updateVisibility(view_tl, view_br);

	e.Skip();
}

/* MapCanvas::onKeyDown
 * Called when a key is pressed within the canvas
 *******************************************************************/
void MapCanvas::onKeyDown(wxKeyEvent& e)
{
	// Set current modifiers
	modifiers_current = e.GetModifiers();

	// Send to overlay if active
	if (overlayActive())
		overlay_current->keyDown(KeyBind::keyName(e.GetKeyCode()));

	// Let keybind system handle it
	KeyBind::keyPressed(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Testing
	if (Global::debug)
	{
		if (e.GetKeyCode() == WXK_F6)
		{
			Polygon2D poly;
			sf::Clock clock;
			wxLogMessage("Generating polygons...");
			for (unsigned a = 0; a < editor->getMap().nSectors(); a++)
			{
				if (!poly.openSector(editor->getMap().getSector(a)))
					wxLogMessage("Splitting failed for sector %d", a);
			}
			//int ms = clock.GetElapsedTime() * 1000;
			//wxLogMessage("Polygon generation took %dms", ms);
		}
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			int nearest = editor->getMap().nearestLine(mouse_pos_m, 999999);
			MapLine* line = editor->getMap().getLine(nearest);
			if (line)
			{
				// Determine line side
				double side = MathStuff::lineSide(mouse_pos_m, line->seg());
				if (side >= 0)
					sbuilder.traceSector(&(editor->getMap()), line, true);
				else
					sbuilder.traceSector(&(editor->getMap()), line, false);
			}
		}
		if (e.GetKeyCode() == WXK_F5)
		{
			// Get nearest line
			int nearest = editor->getMap().nearestLine(mouse_pos_m, 999999);
			MapLine* line = editor->getMap().getLine(nearest);

			// Get sectors
			MapSector* sec1 = editor->getMap().getLineSideSector(line, true);
			MapSector* sec2 = editor->getMap().getLineSideSector(line, false);
			int i1 = -1;
			int i2 = -1;
			if (sec1) i1 = sec1->getIndex();
			if (sec2) i2 = sec2->getIndex();

			editor->addEditorMessage(S_FMT("Front %d Back %d", i1, i2));
		}
		if (e.GetKeyCode() == WXK_F5 && editor->editMode() == MapEditor::MODE_SECTORS)
		{
			splitter.setVerbose(true);
			splitter.clear();
			splitter.openSector(editor->getHilightedSector());
			Polygon2D temp;
			splitter.doSplitting(&temp);
		}
	}

	// Update cursor in object edit mode
	//if (mouse_state == MSTATE_EDIT)
	//	determineObjectEditState();

#ifndef __WXMAC__
	// Skipping events on OS X doesn't do anything but causes
	// sound alert (a.k.a. error beep) on every key press
	if (e.GetKeyCode() != WXK_UP &&
		e.GetKeyCode() != WXK_DOWN &&
		e.GetKeyCode() != WXK_LEFT &&
		e.GetKeyCode() != WXK_RIGHT &&
		e.GetKeyCode() != WXK_NUMPAD_UP &&
		e.GetKeyCode() != WXK_NUMPAD_DOWN &&
		e.GetKeyCode() != WXK_NUMPAD_LEFT &&
		e.GetKeyCode() != WXK_NUMPAD_RIGHT)
		e.Skip();
#endif // !__WXMAC__
}

/* MapCanvas::onKeyUp
 * Called when a key is released within the canvas
 *******************************************************************/
void MapCanvas::onKeyUp(wxKeyEvent& e)
{
	// Set current modifiers
	modifiers_current = e.GetModifiers();

	// Let keybind system handle it
	KeyBind::keyReleased(KeyBind::keyName(e.GetKeyCode()));

	// Update cursor in object edit mode
	//if (mouse_state == MSTATE_EDIT)
	//	determineObjectEditState();

	e.Skip();
}

/* MapCanvas::onMouseDown
 * Called when a mouse button is pressed within the canvas
 *******************************************************************/
void MapCanvas::onMouseDown(wxMouseEvent& e)
{
	// Update hilight
	if (mouse_state == MSTATE_NORMAL)
		editor->updateHilight(mouse_pos_m, view_scale);

	// Update mouse variables
	mouse_downpos.set(e.GetX(), e.GetY());
	mouse_downpos_m.set(translateX(e.GetX()), translateY(e.GetY()));

	// Check if a full screen overlay is active
	if (overlayActive())
	{
		// Left click
		if (e.LeftDown())
			overlay_current->mouseLeftClick();

		// Right click
		else if (e.RightDown())
			overlay_current->mouseRightClick();

		return;
	}

	// Left button
	if (e.LeftDown() || e.LeftDClick())
	{
		// 3d mode
		if (editor->editMode() == MapEditor::MODE_3D)
		{
			// If the mouse is unlocked, lock the mouse
			if (!mouse_locked)
			{
				mouseToCenter();
				lockMouse(true);
			}
			else
			{
				if (e.ShiftDown())	// Shift down, select all matching adjacent structures
					editor->selectAdjacent3d(editor->hilightItem3d());
				else	// Toggle selection
					editor->selectCurrent();
			}

			return;
		}

		// Line drawing state, add line draw point
		if (mouse_state == MSTATE_LINE_DRAW)
		{
			// Snap point to nearest vertex if shift is held down
			bool nearest_vertex = false;
			if (e.GetModifiers() & wxMOD_SHIFT)
				nearest_vertex = true;

			// Line drawing
			if (draw_state == DSTATE_LINE)
			{
				if (editor->addLineDrawPoint(mouse_downpos_m, nearest_vertex))
				{
					// If line drawing finished, revert to normal state
					mouse_state = MSTATE_NORMAL;
				}
			}

			// Shape drawing
			else
			{
				if (draw_state == DSTATE_SHAPE_ORIGIN)
				{
					// Set shape origin
					editor->setShapeDrawOrigin(mouse_downpos_m, nearest_vertex);
					draw_state = DSTATE_SHAPE_EDGE;
				}
				else
				{
					// Finish shape draw
					editor->endLineDraw(true);
					theMapEditor->showShapeDrawPanel(false);
					mouse_state = MSTATE_NORMAL;
				}
			}
		}

		// Paste state, accept paste
		else if (mouse_state == MSTATE_PASTE)
		{
			editor->paste(mouse_pos_m);
			if (!e.ShiftDown())
				mouse_state = MSTATE_NORMAL;
		}

		// Sector tagging state
		else if (mouse_state == MSTATE_TAG_SECTORS)
		{
			editor->tagSectorAt(mouse_pos_m.x, mouse_pos_m.y);
		}

		else if (mouse_state == MSTATE_NORMAL)
		{
			// Double click to edit the current selection
			if (e.LeftDClick() && property_edit_dclick)
			{
				vector<MapObject*> objects;
				editor->getSelectedObjects(objects);
				if (!objects.empty())
				{
					editObjectProperties(objects);
					if (objects.size() == 1)
						editor->clearSelection();
				}
			}
			// Begin box selection if shift is held down, otherwise toggle selection on hilighted object
			else if (e.ShiftDown())
				mouse_state = MSTATE_SELECTION;
			else
				mouse_selbegin = !editor->selectCurrent(selection_clear_click);
		}
	}

	// Right button
	else if (e.RightDown())
	{
		// 3d mode
		if (editor->editMode() == MapEditor::MODE_3D)
		{
			// Get first selected item
			selection_3d_t first = editor->hilightItem3d();
			if (editor->get3dSelection().size() > 0)
				first = editor->get3dSelection()[0];

			// Check type
			if (first.type == MapEditor::SEL_THING)
				changeThingType3d(first);
			else
				changeTexture3d(first);
		}

		// Remove line draw point if in line drawing state
		if (mouse_state == MSTATE_LINE_DRAW)
		{
			// Line drawing
			if (draw_state == DSTATE_LINE)
				editor->removeLineDrawPoint();

			// Shape drawing
			else if (draw_state == DSTATE_SHAPE_EDGE)
			{
				editor->endLineDraw(false);
				draw_state = DSTATE_SHAPE_ORIGIN;
			}
		}

		// Normal state
		else if (mouse_state == MSTATE_NORMAL)
		{
			// Begin move if something is selected/hilighted
			if (editor->isHilightOrSelection())
				mouse_movebegin = true;
			//else if (editor->editMode() == MapEditor::MODE_VERTICES)
			//	editor->splitLine(mouse_pos_m.x, mouse_pos_m.y, 16/view_scale);
		}
	}

	// Any other mouse button (let keybind system handle it)
	else// if (mouse_state == MSTATE_NORMAL)
		KeyBind::keyPressed(keypress_t(KeyBind::mbName(e.GetButton()), e.AltDown(), e.CmdDown(), e.ShiftDown()));

	// Set focus
	SetFocus();

	e.Skip();
}

/* MapCanvas::onMouseUp
 * Called when a mouse button is released within the canvas
 *******************************************************************/
void MapCanvas::onMouseUp(wxMouseEvent& e)
{
	// Clear mouse down position
	mouse_downpos.set(-1, -1);

	// Check if a full screen overlay is active
	if (overlayActive())
	{
		return;
	}

	// Left button
	if (e.LeftUp())
	{
		mouse_selbegin = false;

		// If we're ending a box selection
		if (mouse_state == MSTATE_SELECTION)
		{
			// Reset mouse state
			mouse_state = MSTATE_NORMAL;

			// Select
			editor->selectWithin(min(mouse_downpos_m.x, mouse_pos_m.x), min(mouse_downpos_m.y, mouse_pos_m.y),
			                     max(mouse_downpos_m.x, mouse_pos_m.x), max(mouse_downpos_m.y, mouse_pos_m.y), e.ShiftDown());

			// Begin selection box fade animation
			animations.push_back(new MCASelboxFader(theApp->runTimer(), mouse_downpos_m, mouse_pos_m));
		}

		// If we're in object edit mode
		if (mouse_state == MSTATE_EDIT)
			editor->getObjectEditGroup()->resetPositions();
	}

	// Right button
	else if (e.RightUp())
	{
		mouse_movebegin = false;

		if (mouse_state == MSTATE_MOVE)
		{
			editor->endMove();
			mouse_state = MSTATE_NORMAL;
			renderer_2d->forceUpdate();
		}

		// Paste state, cancel paste
		else if (mouse_state == MSTATE_PASTE)
		{
			mouse_state = MSTATE_NORMAL;
		}

		else if (mouse_state == MSTATE_NORMAL)
		{
			// Context menu
			wxMenu menu_context;

			// Set 3d camera
			theApp->getAction("mapw_camera_set")->addToMenu(&menu_context, true);

			// Run from here
			theApp->getAction("mapw_run_map_here")->addToMenu(&menu_context, true);

			// Mode-specific
			bool object_selected = (editor->selectionSize() > 0 || editor->hilightItem() >= 0);
			if (editor->editMode() == MapEditor::MODE_VERTICES)
			{
				menu_context.AppendSeparator();
				theApp->getAction("mapw_vertex_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == MapEditor::MODE_LINES)
			{
				if (object_selected)
				{
					menu_context.AppendSeparator();
					theApp->getAction("mapw_line_changetexture")->addToMenu(&menu_context, true);
					theApp->getAction("mapw_line_changespecial")->addToMenu(&menu_context, true);
					theApp->getAction("mapw_line_tagedit")->addToMenu(&menu_context, true);
					theApp->getAction("mapw_line_flip")->addToMenu(&menu_context, true);
					theApp->getAction("mapw_line_correctsectors")->addToMenu(&menu_context, true);
				}
			}
			else if (editor->editMode() == MapEditor::MODE_THINGS)
			{
				menu_context.AppendSeparator();

				if (object_selected)
					theApp->getAction("mapw_thing_changetype")->addToMenu(&menu_context, true);

				theApp->getAction("mapw_thing_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == MapEditor::MODE_SECTORS)
			{
				if (object_selected)
				{
					theApp->getAction("mapw_sector_changetexture")->addToMenu(&menu_context, true);
					theApp->getAction("mapw_sector_changespecial")->addToMenu(&menu_context, true);
					if (editor->getSelection().size() > 1)
					{
						theApp->getAction("mapw_sector_join")->addToMenu(&menu_context, true);
						theApp->getAction("mapw_sector_join_keep")->addToMenu(&menu_context, true);
					}
				}

				theApp->getAction("mapw_sector_create")->addToMenu(&menu_context, true);
			}

			if (object_selected)
			{
				// General edit
				menu_context.AppendSeparator();
				theApp->getAction("mapw_edit_objects")->addToMenu(&menu_context, true);
				theApp->getAction("mapw_mirror_x")->addToMenu(&menu_context, true);
				theApp->getAction("mapw_mirror_y")->addToMenu(&menu_context, true);

				// Properties
				menu_context.AppendSeparator();
				theApp->getAction("mapw_item_properties")->addToMenu(&menu_context, true);
			}

			PopupMenu(&menu_context);
		}
	}

	// Any other mouse button (let keybind system handle it)
	else if (mouse_state != MSTATE_SELECTION)
		KeyBind::keyReleased(KeyBind::mbName(e.GetButton()));

	e.Skip();
}

/* MapCanvas::onMouseMotion
 * Called when the mouse cursor is moved within the canvas
 *******************************************************************/
void MapCanvas::onMouseMotion(wxMouseEvent& e)
{
	// Ignore if it was generated by a mouse pointer warp
	if (mouse_warp)
	{
		mouse_warp = false;
		e.Skip();
		return;
	}

	// Check if a full screen overlay is active
	if (overlayActive())
	{
		overlay_current->mouseMotion(e.GetX(), e.GetY());
		return;
	}

	// Panning
	if (panning)
		pan((mouse_pos.x - e.GetX()) / view_scale, -((mouse_pos.y - e.GetY()) / view_scale));

	// Update mouse variables
	mouse_pos.set(e.GetX(), e.GetY());
	mouse_pos_m.set(translateX(e.GetX()), translateY(e.GetY()));

	// Update coordinates on status bar
	double mx = mouse_pos_m.x;
	double my = mouse_pos_m.y;
	if (editor->gridSnap())
	{
		mx = editor->snapToGrid(mx);
		my = editor->snapToGrid(my);
	}
	string status_text;
	if (theMapEditor->currentMapDesc().format == MAP_UDMF)
		status_text = S_FMT("Position: (%1.3f, %1.3f)", mx, my);
	else
		status_text = S_FMT("Position: (%d, %d)", (int)mx, (int)my);
	theMapEditor->CallAfter(&MapEditorWindow::SetStatusText, status_text, 3);

	// Object edit
	if (mouse_state == MSTATE_EDIT)
	{
		// Do dragging if left mouse is down
		if (e.LeftIsDown() && edit_state != ESTATE_NONE)
		{
			if (edit_rotate)
			{
				// Rotate
				editor->getObjectEditGroup()->doRotate(mouse_downpos_m, mouse_pos_m, !e.ShiftDown());
				theMapEditor->objectEditPanel()->update(editor->getObjectEditGroup(), true);
			}
			else
			{
				// Get dragged offsets
				double xoff = mouse_pos_m.x - mouse_downpos_m.x;
				double yoff = mouse_pos_m.y - mouse_downpos_m.y;

				// Snap to grid if shift not held down
				if (!e.ShiftDown())
				{
					xoff = editor->snapToGrid(xoff);
					yoff = editor->snapToGrid(yoff);
				}

				if (edit_state == ESTATE_MOVE)
				{
					// Move objects
					editor->getObjectEditGroup()->doMove(xoff, yoff);
					theMapEditor->objectEditPanel()->update(editor->getObjectEditGroup());
				}
				else
				{
					// Scale objects
					editor->getObjectEditGroup()->doScale(xoff, yoff,
						(edit_state == ESTATE_SIZE_L || edit_state == ESTATE_SIZE_TL || edit_state == ESTATE_SIZE_BL),	// Left?
						(edit_state == ESTATE_SIZE_T || edit_state == ESTATE_SIZE_TL || edit_state == ESTATE_SIZE_TR),	// Top?
						(edit_state == ESTATE_SIZE_R || edit_state == ESTATE_SIZE_TR || edit_state == ESTATE_SIZE_BR),	// Right?
						(edit_state == ESTATE_SIZE_B || edit_state == ESTATE_SIZE_BL || edit_state == ESTATE_SIZE_BR));	// Bottom?
					theMapEditor->objectEditPanel()->update(editor->getObjectEditGroup());
				}
			}
		}
		else
		{
			determineObjectEditState();
		}

		return;
	}

	// Check if we want to start a selection box
	if (mouse_selbegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 16)
		mouse_state = MSTATE_SELECTION;

	// Check if we want to start moving
	if (mouse_movebegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 4)
	{
		mouse_movebegin = false;
		editor->beginMove(mouse_downpos_m);
		mouse_state = MSTATE_MOVE;
		renderer_2d->forceUpdate();
	}

	// Check if we are in thing quick angle state
	if (mouse_state == MSTATE_THING_ANGLE)
		editor->thingQuickAngle(mouse_pos_m);

	// Update shape drawing if needed
	if (mouse_state == MSTATE_LINE_DRAW && draw_state == DSTATE_SHAPE_EDGE)
		editor->updateShapeDraw(mouse_pos_m);

	e.Skip();
}

/* MapCanvas::onMouseWheel
 * Called when the mouse wheel is moved
 *******************************************************************/
void MapCanvas::onMouseWheel(wxMouseEvent& e)
{
#ifdef __WXOSX__
	mwheel_rotation = (double)e.GetWheelRotation() / (double)e.GetWheelDelta();
	if (mwheel_rotation < 0)
		mwheel_rotation = 0 - mwheel_rotation;
#else
	mwheel_rotation = 1;
#endif

	if (mwheel_rotation < 0.001)
		return;

	if (e.GetWheelRotation() > 0)
	{
		KeyBind::keyPressed(keypress_t("mwheelup", e.AltDown(), e.ControlDown(), e.ShiftDown()));

		// Send to overlay if active
		if (overlayActive())
			overlay_current->keyDown("mwheelup");

		KeyBind::keyReleased("mwheelup");
	}
	else if (e.GetWheelRotation() < 0)
	{
		KeyBind::keyPressed(keypress_t("mwheeldown", e.AltDown(), e.ControlDown(), e.ShiftDown()));

		// Send to overlay if active
		if (overlayActive())
			overlay_current->keyDown("mwheeldown");

		KeyBind::keyReleased("mwheeldown");
	}
}

/* MapCanvas::onMouseLeave
 * Called when the mouse cursor leaves the canvas
 *******************************************************************/
void MapCanvas::onMouseLeave(wxMouseEvent& e)
{
	// Stop panning
	if (panning)
	{
		panning = false;
		SetCursor(wxNullCursor);
	}

	e.Skip();
}

/* MapCanvas::onMouseEnter
 * Called when the mouse cursor enters the canvas
 *******************************************************************/
void MapCanvas::onMouseEnter(wxMouseEvent& e)
{
	// Set focus
	//SetFocus();

	e.Skip();
}

/* MapCanvas::onIdle
 * Called when the canvas is idle
 *******************************************************************/
void MapCanvas::onIdle(wxIdleEvent& e)
{
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sfclock.getElapsedTime().asMilliseconds()) - last_time;

	if (frametime < fr_idle)
		return;

	last_time = (sfclock.getElapsedTime().asMilliseconds());
	update(frametime);
	Refresh();
}

/* MapCanvas::onRTimer
 * Called when the canvas timer is triggered
 *******************************************************************/
void MapCanvas::onRTimer(wxTimerEvent& e)
{
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sfclock.getElapsedTime().asMilliseconds()) - last_time;

	if (frametime > fr_idle)
	{
		last_time = (sfclock.getElapsedTime().asMilliseconds());
		if (theMapEditor->IsActive())
		{
			update(frametime);
			Refresh();
		}
	}

	timer.Start(-1, true);
}

/* MapCanvas::onFocus
 * Called when the canvas loses or gains focus
 *******************************************************************/
void MapCanvas::onFocus(wxFocusEvent& e)
{
	if (e.GetEventType() == wxEVT_SET_FOCUS)
	{
		if (editor->editMode() == MapEditor::MODE_3D)
			lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		lockMouse(false);
}
