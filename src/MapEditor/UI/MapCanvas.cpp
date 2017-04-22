
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
#include "App.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"	
#include "MapCanvas.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/Renderer/MapRenderer2D.h"
#include "MapEditor/Renderer/MCAnimations.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/SectorBuilder.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ShowItemDialog.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "ObjectEditPanel.h"
#include "OpenGL/Drawing.h"
#include "Utility/MathStuff.h"

#undef None

using MapEditor::Mode;
using MapEditor::SectorMode;
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
MapCanvas::MapCanvas(wxWindow* parent, int id, MapEditContext* editor)
	: OGLCanvas(parent, id, false)
{
	// Init variables
	this->editor = editor;
	editor->setCanvas(this);
	last_hilight = -1;
	anim_flash_level = 0.5f;
	anim_flash_inc = true;
	anim_info_fade = 0.0f;
	anim_overlay_fade = 0.0f;
	fade_vertices = 1.0f;
	fade_lines = 1.0f;
	fade_flats = 1.0f;
	fade_things = 1.0f;
	fr_idle = 0;
	last_time = 0;
	anim_view_speed = 0.05;
	mouse_selbegin = false;
	mouse_movebegin = false;
	mouse_locked = false;
	mouse_warp = false;
	anim_help_fade = 0;

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
}

/* MapCanvas::helpActive
 * Returns true if feature help text should be shown currently
 *******************************************************************/
bool MapCanvas::helpActive()
{
	// Disable if no help
	if (editor->featureHelpLines().empty())
		return false;

	// Enable depending on current state
	if (editor->input().mouseState() == Input::MouseState::ObjectEdit ||
		editor->input().mouseState() == Input::MouseState::LineDraw ||
		editor->input().mouseState() == Input::MouseState::TagSectors)
		return true;

	return false;
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
	glScaled(editor->renderer().viewScale(true), editor->renderer().viewScale(true), 1);

	// Translate to offsets
	glTranslated(-editor->renderer().viewXOff(true), -editor->renderer().viewYOff(true), 0);

	// Update visibility info if needed
	if (!editor->renderer().renderer2D().visOK())
		editor->renderer().renderer2D().updateVisibility(view_tl, view_br);


	// Draw flats if needed
	OpenGL::setColour(COL_WHITE);
	if (flat_drawtype > 0)
	{
		bool texture = false;
		if (flat_drawtype > 1)
			texture = true;

		// Adjust flat type depending on sector mode
		int drawtype = 0;
		if (editor->editMode() == Mode::Sectors)
		{
			if (editor->sectorEditMode() == SectorMode::Floor)
				drawtype = 1;
			else if (editor->sectorEditMode() == SectorMode::Ceiling)
				drawtype = 2;
		}

		editor->renderer().renderer2D().renderFlats(drawtype, texture, fade_flats);
	}

	// Draw grid
	editor->renderer().drawGrid();

	// --- Draw map (depending on mode) ---
	auto mouse_state = editor->input().mouseState();
	OpenGL::resetBlend();
	if (editor->editMode() == Mode::Vertices)
	{
		// Vertices mode
		editor->renderer().renderer2D().renderThings(fade_things);						// Things
		editor->renderer().renderer2D().renderLines(line_tabs_always, fade_lines);		// Lines

		// Vertices
		if (mouse_state == Input::MouseState::Move)
			editor->renderer().renderer2D().renderVertices(0.25f);
		else
			editor->renderer().renderer2D().renderVertices(fade_vertices);

		// Selection if needed
		if (mouse_state != Input::MouseState::Move && !editor->overlayActive() && mouse_state != Input::MouseState::ObjectEdit)
			editor->renderer().renderer2D().renderVertexSelection(editor->selection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !editor->overlayActive())
			editor->renderer().renderer2D().renderVertexHilight(editor->hilightItem().index, anim_flash_level);
	}
	else if (editor->editMode() == Mode::Lines)
	{
		// Lines mode
		editor->renderer().renderer2D().renderThings(fade_things);		// Things
		editor->renderer().renderer2D().renderVertices(fade_vertices);	// Vertices
		editor->renderer().renderer2D().renderLines(true);				// Lines

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!editor->overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			editor->renderer().renderer2D().renderLineSelection(editor->selection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !editor->overlayActive())
			editor->renderer().renderer2D().renderLineHilight(editor->hilightItem().index, anim_flash_level);
	}
	else if (editor->editMode() == Mode::Sectors)
	{
		// Sectors mode
		editor->renderer().renderer2D().renderThings(fade_things);					// Things
		editor->renderer().renderer2D().renderVertices(fade_vertices);				// Vertices
		editor->renderer().renderer2D().renderLines(line_tabs_always, fade_lines);	// Lines

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!editor->overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			editor->renderer().renderer2D().renderFlatSelection(editor->selection(), anim_flash_level);

		splitter.testRender();	// Testing

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !editor->overlayActive())
			editor->renderer().renderer2D().renderFlatHilight(editor->hilightItem().index, anim_flash_level);
	}
	else if (editor->editMode() == Mode::Things)
	{
		// Check if we should force thing angles visible
		bool force_dir = false;
		if (mouse_state == Input::MouseState::ThingAngle)
			force_dir = true;

		// Things mode
		editor->renderer().renderer2D().renderVertices(fade_vertices);				// Vertices
		editor->renderer().renderer2D().renderLines(line_tabs_always, fade_lines);	// Lines
		editor->renderer().renderer2D().renderThings(fade_things, force_dir);		// Things

		// Thing paths
		editor->renderer().renderer2D().renderPathedThings(editor->pathedThings());

		// Selection if needed
		if (mouse_state != Input::MouseState::Move &&
			!editor->overlayActive() &&
			mouse_state != Input::MouseState::ObjectEdit)
			editor->renderer().renderer2D().renderThingSelection(editor->selection(), anim_flash_level);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !editor->overlayActive())
			editor->renderer().renderer2D().renderThingHilight(editor->hilightItem().index, anim_flash_level);
	}


	// Draw tagged sectors/lines/things if needed
	if (!editor->overlayActive() &&
		(mouse_state == Input::MouseState::Normal ||
			mouse_state == Input::MouseState::TagSectors ||
			mouse_state == Input::MouseState::TagThings))
	{
		if (editor->taggedSectors().size() > 0)
			editor->renderer().renderer2D().renderTaggedFlats(editor->taggedSectors(), anim_flash_level);
		if (editor->taggedLines().size() > 0)
			editor->renderer().renderer2D().renderTaggedLines(editor->taggedLines(), anim_flash_level);
		if (editor->taggedThings().size() > 0)
			editor->renderer().renderer2D().renderTaggedThings(editor->taggedThings(), anim_flash_level);
		if (editor->taggingLines().size() > 0)
			editor->renderer().renderer2D().renderTaggingLines(editor->taggingLines(), anim_flash_level);
		if (editor->taggingThings().size() > 0)
			editor->renderer().renderer2D().renderTaggingThings(editor->taggingThings(), anim_flash_level);
	}

	// Draw selection numbers if needed
	if (editor->selection().size() > 0 && mouse_state == Input::MouseState::Normal && map_show_selection_numbers)
		editor->renderer().drawSelectionNumbers();

	// Draw thing quick angle lines if needed
	if (mouse_state == Input::MouseState::ThingAngle)
		editor->renderer().drawThingQuickAngleLines();

	// Draw line drawing lines if needed
	if (mouse_state == Input::MouseState::LineDraw)
		editor->renderer().drawLineDrawLines(editor->input().shiftDown());

	// Draw object edit objects if needed
	if (mouse_state == Input::MouseState::ObjectEdit)
		editor->renderer().drawObjectEdit();

	// Draw sectorbuilder test stuff
	//sbuilder.drawResult();


	// Draw selection box if active
	auto mx = editor->renderer().translateX(editor->input().mousePos().x, true);
	auto my = editor->renderer().translateY(editor->input().mousePos().y, true);
	auto mdx = editor->renderer().translateX(editor->input().mouseDownPos().x, true);
	auto mdy = editor->renderer().translateY(editor->input().mouseDownPos().y, true);
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
	for (unsigned a = 0; a < animations.size(); a++)
	{
		if (!animations[a]->mode3d())
			animations[a]->draw();
	}

	// Draw paste objects if needed
	if (mouse_state == Input::MouseState::Paste)
	{

		if (editor->editMode() == Mode::Things)
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
					fpoint2_t pos(editor->relativeSnapToGrid(p->getMidpoint(), { mx, my }));
					editor->renderer().renderer2D().renderPasteThings(things, pos);
				}
			}
		}
		else
			editor->renderer().drawPasteLines();
	}

	// Draw moving stuff if needed
	if (mouse_state == Input::MouseState::Move)
	{
		switch (editor->editMode())
		{
		case Mode::Vertices:
			editor->renderer().renderer2D().renderMovingVertices(editor->movingItems(), editor->moveVector()); break;
		case Mode::Lines:
			editor->renderer().renderer2D().renderMovingLines(editor->movingItems(), editor->moveVector()); break;
		case Mode::Sectors:
			editor->renderer().renderer2D().renderMovingSectors(editor->movingItems(), editor->moveVector()); break;
		case Mode::Things:
			editor->renderer().renderer2D().renderMovingThings(editor->movingItems(), editor->moveVector()); break;
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
	editor->renderer().renderer3D().setupView(GetSize().x, GetSize().y);

	// Render 3d map
	editor->renderer().renderer3D().renderMap();

	// Determine hilight
	MapEditor::Item hl{ -1, MapEditor::ItemType::Any };
	if (!editor->selection().hilightLocked())
	{
		auto old_hl = editor->selection().hilight();
		hl = editor->renderer().renderer3D().determineHilight();
		if (editor->selection().setHilight(hl))
		{
			// Update 3d info overlay
			if (info_overlay_3d && hl.index >= 0)
			{
				info_3d.update(hl.index, hl.type, &(editor->map()));
				anim_info_show = true;
			}
			else
				anim_info_show = false;

			// Animation
			animations.push_back(
				new MCAHilightFade3D(
					App::runTimer(),
					old_hl.index,
					old_hl.type,
					&editor->renderer().renderer3D(),
					anim_flash_level
				)
			);
			anim_flash_inc = true;
			anim_flash_level = 0.0f;
		}
	}

	// Draw selection if any
	auto selection = editor->selection();
	editor->renderer().renderer3D().renderFlatSelection(selection);
	editor->renderer().renderer3D().renderWallSelection(selection);
	editor->renderer().renderer3D().renderThingSelection(selection);

	// Draw hilight if any
	if (hl.index >= 0)
		editor->renderer().renderer3D().renderHilight(hl, anim_flash_level);

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
	if (editor->editMode() == Mode::Visual)
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
	if (editor->editMode() != Mode::Visual && editor->hilightItem().index != last_hilight)
	{
		// Update hilight index
		last_hilight = editor->hilightItem().index;
		anim_info_show = (last_hilight != -1);

		// Update info overlay depending on edit mode
		switch (editor->editMode())
		{
		case Mode::Vertices:	info_vertex.update(editor->selection().hilightedVertex()); break;
		case Mode::Lines:	info_line.update(editor->selection().hilightedLine()); break;
		case Mode::Sectors:	info_sector.update(editor->selection().hilightedSector()); break;
		case Mode::Things:	info_thing.update(editor->selection().hilightedThing()); break;
		}
	}

	// Draw current info overlay
	glDisable(GL_TEXTURE_2D);
	if (editor->editMode() == Mode::Vertices)
		info_vertex.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == Mode::Lines)
		info_line.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == Mode::Sectors)
		info_sector.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == Mode::Things)
		info_thing.draw(GetSize().y, GetSize().x, anim_info_fade);
	else if (editor->editMode() == Mode::Visual)
		info_3d.draw(GetSize().y, GetSize().x, GetSize().x * 0.5, anim_info_fade);

	// Draw current fullscreen overlay
	if (editor->currentOverlay())
		editor->currentOverlay()->draw(GetSize().x, GetSize().y, anim_overlay_fade);

	// Draw crosshair if 3d mode
	if (editor->editMode() == Mode::Visual)
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
		if (editor->renderer().renderer3D().itemDistance() >= 0 && camera_3d_show_distance)
		{
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(col);
			Drawing::drawText(S_FMT("%d", editor->renderer().renderer3D().itemDistance()), midx+5, midy+5, rgba_t(255, 255, 255, 200), Drawing::FONT_SMALL);
			//Drawing::drawText(S_FMT("%1.2f", editor->renderer().renderer3D().camPitch()), midx+5, midy+30, rgba_t(255, 255, 255, 200), Drawing::FONT_SMALL);
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
	editor->renderer().drawEditorMessages();

	// Help text
	editor->renderer().drawFeatureHelpText();

	SwapBuffers();

	glFinish();
}

/* MapCanvas::update2d
 * Updates the current 2d map editor state (animations, hilight etc.)
 *******************************************************************/
bool MapCanvas::update2d(double mult)
{
	// Update hilight if needed
	if (editor->input().mouseState() == Input::MouseState::Normal && !mouse_movebegin)
	{
		auto old_hl = editor->selection().hilightedObject();
		if (editor->selection().updateHilight(editor->input().mousePosMap(), editor->renderer().viewScale()) && hilight_smooth)
		{
			// Hilight fade animation
			if (old_hl)
				animations.push_back(
					new MCAHilightFade(App::runTimer(), old_hl, &editor->renderer().renderer2D(), anim_flash_level)
				);

			// Reset hilight flash
			anim_flash_inc = true;
			anim_flash_level = 0.0f;
		}
	}

	// Do item moving if needed
	if (editor->input().mouseState() == Input::MouseState::Move)
		editor->doMove(editor->input().mousePosMap());

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
	if (editor->editMode() == Mode::Vertices)
	{
		if (fade_vertices < 1.0f)  { fade_vertices += mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = fa_lines;
		if (fade_flats > fa_flats) { fade_flats -= mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == Mode::Lines)
	{
		if (fade_vertices > fa_vertices) { fade_vertices -= mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = 1.0f;
		if (fade_flats > fa_flats) { fade_flats -= mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == Mode::Sectors)
	{
		if (fade_vertices > fa_vertices) { fade_vertices -= mcs_speed*(1.0f-fa_vertices)*mult; anim_mode_crossfade = true; }
		fade_lines = fa_lines;
		if (fade_flats < 1.0f) { fade_flats += mcs_speed*(1.0f-fa_flats)*mult; anim_mode_crossfade = true; }
		if (fade_things > fa_things) { fade_things -= mcs_speed*(1.0f-fa_things)*mult; anim_mode_crossfade = true; }
	}
	else if (editor->editMode() == Mode::Things)
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
	anim_view_speed = editor->renderer().interpolateView(scroll_smooth, anim_view_speed, mult);
	bool view_anim = editor->renderer().viewIsInterpolated();

	// Update renderer scale
	editor->renderer().renderer2D().setScale(editor->renderer().viewScale(true));

	// Check if framerate shouldn't be throttled
	if (editor->input().mouseState() == Input::MouseState::Selection ||
		editor->input().panning() ||
		view_anim ||
		anim_mode_crossfade)
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
	if (editor->overlayActive())
		return true;

	// --- Check for held-down keys ---
	bool moving = false;
	bool fast = editor->input().shiftDown();
	double speed = fast ? mult * 8 : mult * 4;

	// Camera forward
	if (KeyBind::isPressed("me3d_camera_forward"))
	{
		editor->renderer().renderer3D().cameraMove(speed, !camera_3d_gravity);
		moving = true;
	}

	// Camera backward
	if (KeyBind::isPressed("me3d_camera_back"))
	{
		editor->renderer().renderer3D().cameraMove(-speed, !camera_3d_gravity);
		moving = true;
	}

	// Camera left (strafe)
	if (KeyBind::isPressed("me3d_camera_left"))
	{
		editor->renderer().renderer3D().cameraStrafe(-speed);
		moving = true;
	}

	// Camera right (strafe)
	if (KeyBind::isPressed("me3d_camera_right"))
	{
		editor->renderer().renderer3D().cameraStrafe(speed);
		moving = true;
	}

	// Camera up
	if (KeyBind::isPressed("me3d_camera_up"))
	{
		editor->renderer().renderer3D().cameraMoveUp(speed);
		moving = true;
	}

	// Camera down
	if (KeyBind::isPressed("me3d_camera_down"))
	{
		editor->renderer().renderer3D().cameraMoveUp(-speed);
		moving = true;
	}

	// Camera turn left
	if (KeyBind::isPressed("me3d_camera_turn_left"))
	{
		editor->renderer().renderer3D().cameraTurn(fast ? mult*2 : mult);
		moving = true;
	}

	// Camera turn right
	if (KeyBind::isPressed("me3d_camera_turn_right"))
	{
		editor->renderer().renderer3D().cameraTurn(fast ? -mult*2 : -mult);
		moving = true;
	}

	// Apply gravity to camera if needed
	if (camera_3d_gravity)
		editor->renderer().renderer3D().cameraApplyGravity(mult);

	// Update status bar
	fpoint3_t pos = editor->renderer().renderer3D().camPosition();
	string status_text = S_FMT("Position: (%d, %d, %d)", (int)pos.x, (int)pos.y, (int)pos.z);
	MapEditor::window()->CallAfter(&MapEditorWindow::SetStatusText, status_text, 3);

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
	if (editor->editMode() == Mode::Visual)
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
	if (anim_info_show && !editor->overlayActive())
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
	if (editor->overlayActive())
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
	if (editor->overlayActive())
		editor->currentOverlay()->update(frametime);

	// Update animations
	bool anim_running = false;
	for (unsigned a = 0; a < animations.size(); a++)
	{
		if (!animations[a]->update(App::runTimer()))
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

/* MapCanvas::mouseLook3d
 * Handles 3d mode mouselook
 *******************************************************************/
void MapCanvas::mouseLook3d()
{
	// Check for 3d mode
	if (editor->editMode() == Mode::Visual && mouse_locked)
	{
		auto overlay_current = editor->currentOverlay();
		if (!overlay_current || !overlay_current->isActive() || (overlay_current && overlay_current->allow3dMlook()))
		{
			// Get relative mouse movement
			int xpos = wxGetMousePosition().x - GetScreenPosition().x;
			int ypos = wxGetMousePosition().y - GetScreenPosition().y;
			double xrel = xpos - int(GetSize().x * 0.5);
			double yrel = ypos - int(GetSize().y * 0.5);

			if (xrel != 0 || yrel != 0)
			{
				editor->renderer().renderer3D().cameraTurn(-xrel*0.1*camera_3d_sensitivity_x);
				if (mlook_invert_y)
					editor->renderer().renderer3D().cameraPitch(yrel*0.003*camera_3d_sensitivity_y);
				else
					editor->renderer().renderer3D().cameraPitch(-yrel*0.003*camera_3d_sensitivity_y);

				mouseToCenter();
				fr_idle = 0;
			}
		}
	}
}

/* MapCanvas::animateSelectionChange
 * Animates the (de)selection of [item], depending on [selected]
 *******************************************************************/
void MapCanvas::animateSelectionChange(const MapEditor::Item& item, bool selected)
{
	using MapEditor::ItemType;

	// 3d mode wall
	if (MapEditor::baseItemType(item.type) == ItemType::Side)
	{
		// Get quad
		auto quad = editor->renderer().renderer3D().getQuad(item);

		if (quad)
		{
			// Get quad points
			fpoint3_t points[4];
			for (unsigned a = 0; a < 4; a++)
				points[a].set(quad->points[a].x, quad->points[a].y, quad->points[a].z);

			// Start animation
			animations.push_back(new MCA3dWallSelection(App::runTimer(), points, selected));
		}
	}

	// 3d mode flat
	else if (item.type == ItemType::Ceiling || item.type == ItemType::Floor)
	{
		// Get flat
		auto flat = editor->renderer().renderer3D().getFlat(item);

		// Start animation
		if (flat)
			animations.push_back(
				new MCA3dFlatSelection(
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
		MapThing* t = editor->map().getThing(item.index);
		if (!t) return;

		// Get thing type
		ThingType* tt = theGameConfiguration->thingType(t->getType());

		// Start animation
		double radius = tt->getRadius();
		if (tt->shrinkOnZoom()) radius = editor->renderer().renderer2D().scaledRadius(radius);
		animations.push_back(
			new MCAThingSelection(
				App::runTimer(),
				t->xPos(),
				t->yPos(),
				radius,
				editor->renderer().renderer2D().viewScaleInv(),
				selected
			)
		);
	}

	// 2d mode line
	else if (item.type == ItemType::Line)
	{
		// Get line
		vector<MapLine*> vec;
		vec.push_back(editor->map().getLine(item.index));

		// Start animation
		animations.push_back(new MCALineSelection(App::runTimer(), vec, selected));
	}

	// 2d mode vertex
	else if (item.type == ItemType::Vertex)
	{
		// Get vertex
		vector<MapVertex*> verts;
		verts.push_back(editor->map().getVertex(item.index));

		// Determine current vertex size
		float vs = vertex_size;
		if (editor->renderer().viewScale() < 1.0) vs *= editor->renderer().viewScale();
		if (vs < 2.0) vs = 2.0;

		// Start animation
		animations.push_back(new MCAVertexSelection(App::runTimer(), verts, vs, selected));
	}

	// 2d mode sector
	else if (item.type == ItemType::Sector)
	{
		// Get sector polygon
		vector<Polygon2D*> polys;
		polys.push_back(editor->map().getSector(item.index)->getPolygon());

		// Start animation
		animations.push_back(new MCASectorSelection(App::runTimer(), polys, selected));
	}
}

/* MapCanvas::animateSelectionChange
 * Animates the last selection change from [selection]
 *******************************************************************/
void MapCanvas::animateSelectionChange(const ItemSelection &selection)
{
	for (auto& change : selection.lastChange())
		animateSelectionChange(change.first, change.second);
}

/* MapCanvas::updateInfoOverlay
 * Updates the current info overlay, depending on edit mode
 *******************************************************************/
void MapCanvas::updateInfoOverlay()
{
	// Update info overlay depending on edit mode
	switch (editor->editMode())
	{
	case Mode::Vertices:	info_vertex.update(editor->selection().hilightedVertex()); break;
	case Mode::Lines:	info_line.update(editor->selection().hilightedLine()); break;
	case Mode::Sectors:	info_sector.update(editor->selection().hilightedSector()); break;
	case Mode::Things:	info_thing.update(editor->selection().hilightedThing()); break;
	default: break;
	}
}

/* MapCanvas::forceRefreshRenderer
 * Forces a full refresh of the 2d/3d renderers
 *******************************************************************/
void MapCanvas::forceRefreshRenderer()
{
	// Update 3d mode info overlay if needed
	if (editor->editMode() == Mode::Visual)
	{
		auto hl = editor->renderer().renderer3D().determineHilight();
		info_3d.update(hl.index, hl.type, &(editor->map()));
	}

	if (!setActive())
		return;

	editor->renderer().renderer2D().forceUpdate();
	editor->renderer().renderer3D().clearData();
}

/* MapCanvas::onKeyBindPress
 * Called when the key bind [name] is pressed
 *******************************************************************/
void MapCanvas::onKeyBindPress(string name)
{
	// Screenshot
#ifdef USE_SFML_RENDERWINDOW
	if (name == "map_screenshot")
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
		string filename = App::path(S_FMT("sladeshot-%s.png", timestamp), App::Dir::User);
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

	// Handle keybinds depending on mode
	if (editor->editMode() == Mode::Visual)
	{
		// Release mouse cursor
		if (name == "me3d_release_mouse")
			lockMouse(false);
	}
}

/* MapCanvas::handleAction
 * Handles an SAction [id]. Returns true if the action was handled
 * here
 *******************************************************************/
bool MapCanvas::handleAction(string id)
{
	auto mouse_state = editor->input().mouseState();
	auto mouse_downpos_m = editor->input().mouseDownPosMap();

	// Skip if not shown
	if (!IsShown())
		return false;

	// Skip if overlay is active
	if (editor->overlayActive())
		return false;

	// Vertices mode
	if (id == "mapw_mode_vertices")
	{
		editor->setEditMode(Mode::Vertices);
		return true;
	}

	// Lines mode
	else if (id == "mapw_mode_lines")
	{
		editor->setEditMode(Mode::Lines);
		return true;
	}

	// Sectors mode
	else if (id == "mapw_mode_sectors")
	{
		editor->setEditMode(Mode::Sectors);
		return true;
	}

	// Things mode
	else if (id == "mapw_mode_things")
	{
		editor->setEditMode(Mode::Things);
		return true;
	}

	// 3d mode
	else if (id == "mapw_mode_3d")
	{
		SetFocusFromKbd();
		SetFocus();
		editor->setEditMode(Mode::Visual);
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
		editor->setSectorEditMode(SectorMode::Both);
		return true;
	}

	// Floors sector edit mode
	else if (id == "mapw_sectormode_floor")
	{
		editor->setSectorEditMode(SectorMode::Floor);
		return true;
	}

	// Ceilings sector edit mode
	else if (id == "mapw_sectormode_ceiling")
	{
		editor->setSectorEditMode(SectorMode::Ceiling);
		return true;
	}

	// Begin line drawing
	else if (id == "mapw_draw_lines" && mouse_state == Input::MouseState::Normal)
	{
		editor->lineDraw().begin();
		return true;
	}

	// Begin shape drawing
	else if (id == "mapw_draw_shape" && mouse_state == Input::MouseState::Normal)
	{
		editor->lineDraw().begin(true);
		return true;
	}
	
	// Begin object edit
	else if (id == "mapw_edit_objects" && mouse_state == Input::MouseState::Normal)
	{
		editor->objectEdit().begin();
		return true;
	}

	// Show full map
	else if (id == "mapw_show_fullmap")
	{
		editor->renderer().viewFitToMap();
		return true;
	}

	// Show item
	else if (id == "mapw_show_item")
	{
		// Setup dialog
		ShowItemDialog dlg(this);
		switch (editor->editMode())
		{
		case Mode::Vertices:
			dlg.setType(MOBJ_VERTEX); break;
		case Mode::Lines:
			dlg.setType(MOBJ_LINE); break;
		case Mode::Sectors:
			dlg.setType(MOBJ_SECTOR); break;
		case Mode::Things:
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
				editor->setEditMode(Mode::Vertices); break;
			case MOBJ_LINE:
				editor->setEditMode(Mode::Lines); break;
			case MOBJ_SIDE:
				editor->setEditMode(Mode::Lines); side = true; break;
			case MOBJ_SECTOR:
				editor->setEditMode(Mode::Sectors); break;
			case MOBJ_THING:
				editor->setEditMode(Mode::Things); break;
			default:
				break;
			}

			// If side, get its parent line
			if (side)
			{
				MapSide* s = editor->map().getSide(index);
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
		fpoint3_t pos(editor->input().mousePosMap());
		SLADEMap& map = editor->map();
		MapSector* sector = map.getSector(map.sectorAt(editor->input().mousePosMap()));
		if (sector)
			pos.z = sector->getFloorHeight() + 40;
		editor->renderer().renderer3D().cameraSetPosition(pos);
		return true;
	}

	// Edit item properties
	else if (id == "mapw_item_properties")
		editor->editObjectProperties();

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
		editor->openLineTextureOverlay();
		return true;
	}

	// Change line special
	else if (id == "mapw_line_changespecial")
	{
		// Get selection
		auto selection = editor->selection().selectedObjects();

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
				editor->renderer().renderer2D().forceUpdate();
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
			editor->input().setMouseState(Input::MouseState::TagSectors);

			// Setup help text
			string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
			string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
			editor->setFeatureHelp({
				"Tag Edit",
				S_FMT("%s = Accept", key_accept),
				S_FMT("%s = Cancel", key_cancel),
				"Left Click = Toggle tagged sector"
			});
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
		editor->changeThingType();
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
		editor->changeSectorTexture();
		return true;
	}

	// Change sector special
	else if (id == "mapw_sector_changespecial")
	{
		// Get selection
		auto selection = editor->selection().selectedSectors();

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
	editor->renderer().setViewSize(GetSize().x, GetSize().y);
	view_tl.x = editor->renderer().translateX(0);
	view_tl.y = editor->renderer().translateY(GetSize().y);
	view_br.x = editor->renderer().translateX(GetSize().x);
	view_br.y = editor->renderer().translateY(0);

	// Update map item visibility
	editor->renderer().renderer2D().updateVisibility(view_tl, view_br);

	e.Skip();
}

/* MapCanvas::onKeyDown
 * Called when a key is pressed within the canvas
 *******************************************************************/
void MapCanvas::onKeyDown(wxKeyEvent& e)
{
	// Set current modifiers
	editor->input().updateKeyModifiersWx(e.GetModifiers());

	// Send to overlay if active
	if (editor->overlayActive())
		editor->currentOverlay()->keyDown(KeyBind::keyName(e.GetKeyCode()));

	// Let keybind system handle it
	KeyBind::keyPressed(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Testing
	if (Global::debug)
	{
		if (e.GetKeyCode() == WXK_F6)
		{
			Polygon2D poly;
			sf::Clock clock;
			LOG_MESSAGE(1, "Generating polygons...");
			for (unsigned a = 0; a < editor->map().nSectors(); a++)
			{
				if (!poly.openSector(editor->map().getSector(a)))
					LOG_MESSAGE(1, "Splitting failed for sector %d", a);
			}
			//int ms = clock.GetElapsedTime() * 1000;
			//LOG_MESSAGE(1, "Polygon generation took %dms", ms);
		}
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			int nearest = editor->map().nearestLine(editor->input().mousePosMap(), 999999);
			MapLine* line = editor->map().getLine(nearest);
			if (line)
			{
				// Determine line side
				double side = MathStuff::lineSide(editor->input().mousePosMap(), line->seg());
				if (side >= 0)
					sbuilder.traceSector(&(editor->map()), line, true);
				else
					sbuilder.traceSector(&(editor->map()), line, false);
			}
		}
		if (e.GetKeyCode() == WXK_F5)
		{
			// Get nearest line
			int nearest = editor->map().nearestLine(editor->input().mousePosMap(), 999999);
			MapLine* line = editor->map().getLine(nearest);

			// Get sectors
			MapSector* sec1 = editor->map().getLineSideSector(line, true);
			MapSector* sec2 = editor->map().getLineSideSector(line, false);
			int i1 = -1;
			int i2 = -1;
			if (sec1) i1 = sec1->getIndex();
			if (sec2) i2 = sec2->getIndex();

			editor->addEditorMessage(S_FMT("Front %d Back %d", i1, i2));
		}
		if (e.GetKeyCode() == WXK_F5 && editor->editMode() == Mode::Sectors)
		{
			splitter.setVerbose(true);
			splitter.clear();
			splitter.openSector(editor->selection().hilightedSector());
			Polygon2D temp;
			splitter.doSplitting(&temp);
		}
	}

	// Update cursor in object edit mode
	//if (mouse_state == Input::MouseState::ObjectEdit)
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
	editor->input().updateKeyModifiersWx(e.GetModifiers());

	// Let keybind system handle it
	KeyBind::keyReleased(KeyBind::keyName(e.GetKeyCode()));

	e.Skip();
}

/* MapCanvas::onMouseDown
 * Called when a mouse button is pressed within the canvas
 *******************************************************************/
void MapCanvas::onMouseDown(wxMouseEvent& e)
{
	auto mouse_state = editor->input().mouseState();

	// Update hilight
	if (mouse_state == Input::MouseState::Normal)
		editor->selection().updateHilight(editor->input().mousePosMap(), editor->renderer().viewScale());

	// Update mouse variables
	editor->input().mouseDown();
	//mouse_downpos.set(e.GetX(), e.GetY());
	//mouse_downpos_m.set(editor->renderer().translateX(e.GetX()), editor->renderer().translateY(e.GetY()));

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		// Left click
		if (e.LeftDown())
			editor->currentOverlay()->mouseLeftClick();

		// Right click
		else if (e.RightDown())
			editor->currentOverlay()->mouseRightClick();

		return;
	}

	// Left button
	if (e.LeftDown() || e.LeftDClick())
	{
		// 3d mode
		if (editor->editMode() == Mode::Visual)
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
					editor->edit3d().selectAdjacent(editor->hilightItem());
				else	// Toggle selection
					editor->selection().toggleCurrent();
			}

			return;
		}

		// Line drawing state, add line draw point
		if (mouse_state == Input::MouseState::LineDraw)
		{
			// Snap point to nearest vertex if shift is held down
			bool nearest_vertex = false;
			if (e.GetModifiers() & wxMOD_SHIFT)
				nearest_vertex = true;

			// Line drawing
			if (editor->lineDraw().state() == LineDraw::State::Line)
			{
				if (editor->lineDraw().addPoint(editor->input().mouseDownPosMap(), nearest_vertex))
				{
					// If line drawing finished, revert to normal state
					editor->input().setMouseState(Input::MouseState::Normal);
				}
			}

			// Shape drawing
			else
			{
				if (editor->lineDraw().state() == LineDraw::State::ShapeOrigin)
				{
					// Set shape origin
					editor->lineDraw().setShapeOrigin(editor->input().mouseDownPosMap(), nearest_vertex);
					editor->lineDraw().setState(LineDraw::State::ShapeEdge);
				}
				else
				{
					// Finish shape draw
					editor->lineDraw().end(true);
					MapEditor::window()->showShapeDrawPanel(false);
					editor->input().setMouseState(Input::MouseState::Normal);
				}
			}
		}

		// Paste state, accept paste
		else if (mouse_state == Input::MouseState::Paste)
		{
			editor->paste(editor->input().mousePosMap());
			if (!e.ShiftDown())
				editor->input().setMouseState(Input::MouseState::Normal);
		}

		// Sector tagging state
		else if (mouse_state == Input::MouseState::TagSectors)
		{
			editor->tagSectorAt(editor->input().mousePosMap().x, editor->input().mousePosMap().y);
		}

		else if (mouse_state == Input::MouseState::Normal)
		{
			// Double click to edit the current selection
			if (e.LeftDClick() && property_edit_dclick)
			{
				editor->editObjectProperties();
				if (editor->selection().size() == 1)
					editor->selection().clear();
			}
			// Begin box selection if shift is held down, otherwise toggle selection on hilighted object
			else if (e.ShiftDown())
				editor->input().setMouseState(Input::MouseState::Selection);
			else
				mouse_selbegin = !editor->selection().toggleCurrent(selection_clear_click);
		}
	}

	// Right button
	else if (e.RightDown())
	{
		// 3d mode
		if (editor->editMode() == Mode::Visual)
		{
			// Get selection or hilight
			auto sel = editor->selection().selectionOrHilight();
			if (!sel.empty())
			{
				// Check type
				if (sel[0].type == MapEditor::ItemType::Thing)
					editor->changeThingType();
				else
					editor->edit3d().changeTexture();
			}
		}

		// Remove line draw point if in line drawing state
		if (mouse_state == Input::MouseState::LineDraw)
		{
			// Line drawing
			if (editor->lineDraw().state() == LineDraw::State::Line)
				editor->lineDraw().removePoint();

			// Shape drawing
			else if (editor->lineDraw().state() == LineDraw::State::ShapeEdge)
			{
				editor->lineDraw().end(false);
				editor->lineDraw().setState(LineDraw::State::ShapeOrigin);
			}
		}

		// Normal state
		else if (mouse_state == Input::MouseState::Normal)
		{
			// Begin move if something is selected/hilighted
			if (editor->selection().hasHilightOrSelection())
				mouse_movebegin = true;
			//else if (editor->editMode() == Mode::Vertices)
			//	editor->splitLine(editor->input().mousePosMap().x, editor->input().mousePosMap().y, 16/editor->renderer().viewScale());
		}
	}

	// Any other mouse button (let keybind system handle it)
	else// if (mouse_state == Input::MouseState::Normal)
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
	auto mouse_state = editor->input().mouseState();
	auto mouse_downpos_m = editor->input().mouseDownPosMap();

	// Clear mouse down position
	editor->input().mouseUp();
	//mouse_downpos.set(-1, -1);

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		return;
	}

	// Left button
	if (e.LeftUp())
	{
		mouse_selbegin = false;

		// If we're ending a box selection
		if (mouse_state == Input::MouseState::Selection)
		{
			// Reset mouse state
			editor->input().setMouseState(Input::MouseState::Normal);

			// Select
			editor->selection().selectWithin(
				frect_t(
					min(mouse_downpos_m.x, editor->input().mousePosMap().x),
					min(mouse_downpos_m.y, editor->input().mousePosMap().y),
					max(mouse_downpos_m.x, editor->input().mousePosMap().x),
					max(mouse_downpos_m.y, editor->input().mousePosMap().y)),
				e.ShiftDown()
			);

			// Begin selection box fade animation
			animations.push_back(new MCASelboxFader(App::runTimer(), mouse_downpos_m, editor->input().mousePosMap()));
		}

		// If we're in object edit mode
		if (mouse_state == Input::MouseState::ObjectEdit)
			editor->objectEdit().group().resetPositions();
	}

	// Right button
	else if (e.RightUp())
	{
		mouse_movebegin = false;

		if (mouse_state == Input::MouseState::Move)
		{
			editor->endMove();
			editor->input().setMouseState(Input::MouseState::Normal);
			editor->renderer().renderer2D().forceUpdate();
		}

		// Paste state, cancel paste
		else if (mouse_state == Input::MouseState::Paste)
		{
			editor->input().setMouseState(Input::MouseState::Normal);
		}

		else if (mouse_state == Input::MouseState::Normal)
		{
			// Context menu
			wxMenu menu_context;

			// Set 3d camera
			SAction::fromId("mapw_camera_set")->addToMenu(&menu_context, true);

			// Run from here
			SAction::fromId("mapw_run_map_here")->addToMenu(&menu_context, true);

			// Mode-specific
			bool object_selected = editor->selection().hasHilightOrSelection();
			if (editor->editMode() == Mode::Vertices)
			{
				menu_context.AppendSeparator();
				SAction::fromId("mapw_vertex_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == Mode::Lines)
			{
				if (object_selected)
				{
					menu_context.AppendSeparator();
					SAction::fromId("mapw_line_changetexture")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_changespecial")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_tagedit")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_flip")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_correctsectors")->addToMenu(&menu_context, true);
				}
			}
			else if (editor->editMode() == Mode::Things)
			{
				menu_context.AppendSeparator();

				if (object_selected)
					SAction::fromId("mapw_thing_changetype")->addToMenu(&menu_context, true);

				SAction::fromId("mapw_thing_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == Mode::Sectors)
			{
				if (object_selected)
				{
					SAction::fromId("mapw_sector_changetexture")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_sector_changespecial")->addToMenu(&menu_context, true);
					if (editor->selection().size() > 1)
					{
						SAction::fromId("mapw_sector_join")->addToMenu(&menu_context, true);
						SAction::fromId("mapw_sector_join_keep")->addToMenu(&menu_context, true);
					}
				}

				SAction::fromId("mapw_sector_create")->addToMenu(&menu_context, true);
			}

			if (object_selected)
			{
				// General edit
				menu_context.AppendSeparator();
				SAction::fromId("mapw_edit_objects")->addToMenu(&menu_context, true);
				SAction::fromId("mapw_mirror_x")->addToMenu(&menu_context, true);
				SAction::fromId("mapw_mirror_y")->addToMenu(&menu_context, true);

				// Properties
				menu_context.AppendSeparator();
				SAction::fromId("mapw_item_properties")->addToMenu(&menu_context, true);
			}

			PopupMenu(&menu_context);
		}
	}

	// Any other mouse button (let keybind system handle it)
	else if (mouse_state != Input::MouseState::Selection)
		KeyBind::keyReleased(KeyBind::mbName(e.GetButton()));

	e.Skip();
}

/* MapCanvas::onMouseMotion
 * Called when the mouse cursor is moved within the canvas
 *******************************************************************/
void MapCanvas::onMouseMotion(wxMouseEvent& e)
{
	auto mouse_pos = editor->input().mousePos();
	auto mouse_downpos = editor->input().mouseDownPos();
	auto mouse_downpos_m = editor->input().mouseDownPosMap();

	// Ignore if it was generated by a mouse pointer warp
	if (mouse_warp)
	{
		mouse_warp = false;
		e.Skip();
		return;
	}

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		editor->currentOverlay()->mouseMotion(e.GetX(), e.GetY());
		return;
	}

	// panning
	if (editor->input().panning())
		editor->renderer().pan((mouse_pos.x - e.GetX()) / editor->renderer().viewScale(), -((mouse_pos.y - e.GetY()) / editor->renderer().viewScale()));

	// Update mouse variables
	editor->input().mouseMove(e.GetX(), e.GetY());
	mouse_pos = editor->input().mousePos();
	//mouse_pos.set(e.GetX(), e.GetY());
	//editor->input().mousePosMap().set(editor->renderer().translateX(e.GetX()), editor->renderer().translateY(e.GetY()));

	// Update coordinates on status bar
	double mx = editor->input().mousePosMap().x;
	double my = editor->input().mousePosMap().y;
	if (editor->gridSnap())
	{
		mx = editor->snapToGrid(mx);
		my = editor->snapToGrid(my);
	}
	string status_text;
	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
		status_text = S_FMT("Position: (%1.3f, %1.3f)", mx, my);
	else
		status_text = S_FMT("Position: (%d, %d)", (int)mx, (int)my);
	MapEditor::window()->CallAfter(&MapEditorWindow::SetStatusText, status_text, 3);

	// Object edit
	auto edit_state = editor->objectEdit().state();
	if (editor->input().mouseState() == Input::MouseState::ObjectEdit)
	{
		// Do dragging if left mouse is down
		if (e.LeftIsDown() && edit_state != ObjectEdit::State::None)
		{
			if (editor->objectEdit().rotating())
			{
				// Rotate
				editor->objectEdit().group().doRotate(mouse_downpos_m, editor->input().mousePosMap(), !e.ShiftDown());
				MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group(), true);
			}
			else
			{
				// Get dragged offsets
				double xoff = editor->input().mousePosMap().x - mouse_downpos_m.x;
				double yoff = editor->input().mousePosMap().y - mouse_downpos_m.y;

				// Snap to grid if shift not held down
				if (!e.ShiftDown())
				{
					xoff = editor->snapToGrid(xoff);
					yoff = editor->snapToGrid(yoff);
				}

				if (edit_state == ObjectEdit::State::Move)
				{
					// Move objects
					editor->objectEdit().group().doMove(xoff, yoff);
					MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group());
				}
				else
				{
					// Scale objects
					editor->objectEdit().group().doScale(
						xoff,
						yoff,
						editor->objectEdit().stateLeft(false),
						editor->objectEdit().stateTop(false),
						editor->objectEdit().stateRight(false),
						editor->objectEdit().stateBottom(false)
					);
					MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group());
				}
			}
		}
		else
			editor->objectEdit().determineState();

		return;
	}

	// Check if we want to start a selection box
	if (mouse_selbegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 16)
		editor->input().setMouseState(Input::MouseState::Selection);

	// Check if we want to start moving
	if (mouse_movebegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 4)
	{
		mouse_movebegin = false;
		editor->beginMove(mouse_downpos_m);
		editor->input().setMouseState(Input::MouseState::Move);
		editor->renderer().renderer2D().forceUpdate();
	}

	// Check if we are in thing quick angle state
	if (editor->input().mouseState() == Input::MouseState::ThingAngle)
		editor->thingQuickAngle(editor->input().mousePosMap());

	// Update shape drawing if needed
	if (editor->input().mouseState() == Input::MouseState::LineDraw && editor->lineDraw().state() == LineDraw::State::ShapeEdge)
		editor->lineDraw().updateShape(editor->input().mousePosMap());

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

	editor->input().mouseWheel(e.GetWheelRotation() > 0, mwheel_rotation);
}

/* MapCanvas::onMouseLeave
 * Called when the mouse cursor leaves the canvas
 *******************************************************************/
void MapCanvas::onMouseLeave(wxMouseEvent& e)
{
	// Stop panning
	if (editor->input().panning())
	{
		editor->input().setPanning(false);
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
		if (MapEditor::window()->IsActive())
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
		if (editor->editMode() == Mode::Visual)
			lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		lockMouse(false);
}
