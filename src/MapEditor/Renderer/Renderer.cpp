
#include "Main.h"
#include "Application/App.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/Edit/LineDraw.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "Renderer.h"
#include "Utility/MathStuff.h"

using namespace MapEditor;

EXTERN_CVAR(Bool, grid_dashed)
EXTERN_CVAR(Bool, map_show_help)
EXTERN_CVAR(Bool, map_showfps)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Int, grid_64_style)
EXTERN_CVAR(Int, map_crosshair)
EXTERN_CVAR(Int, map_max_selection_numbers)
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Int, flat_drawtype)
EXTERN_CVAR(Bool, map_show_selection_numbers)
EXTERN_CVAR(Bool, line_tabs_always)
EXTERN_CVAR(Bool, hilight_smooth)
EXTERN_CVAR(Bool, vertices_always)
EXTERN_CVAR(Bool, things_always)
EXTERN_CVAR(Bool, flat_fade)
EXTERN_CVAR(Bool, line_fade)
EXTERN_CVAR(Bool, scroll_smooth)
EXTERN_CVAR(Bool, camera_3d_gravity)


RenderView::RenderView() :
	scale_{ 0 },
	scale_inter_{ 0 },
	min_scale_{ 0.005 },
	max_scale_{ 10.0 }
{
}

void RenderView::resetInter(bool x, bool y, bool scale)
{
	if (x)
		offset_inter_.x = offset_.x;
	
	if (y)
		offset_inter_.y = offset_.y;

	if (scale)
		scale_inter_ = scale_;
}

void RenderView::zoom(double amount)
{
	// Zoom view
	scale_ *= amount;

	// Check for zoom limits
	if (scale_ < min_scale_)
		scale_ = min_scale_;
	if (scale_ > max_scale_)
		scale_ = max_scale_;

	// Update screen limits
	updateMapBounds();
}

void RenderView::zoomToward(double amount, const fpoint2_t point)
{
	// Get current [point] in map coordinates
	auto orig_point = mapPos(point);
	
	// Zoom view
	scale_ *= amount;

	// Check for zoom limits
	if (scale_ < min_scale_)
		scale_ = min_scale_;
	if (scale_ > max_scale_)
		scale_ = max_scale_;

	// Zoom towards [point]
	offset_.x += orig_point.x - mapX(point.x);
	offset_.y += orig_point.y - mapY(point.y);

	// Update screen limits
	updateMapBounds();
}

void RenderView::fitTo(bbox_t bbox)
{
	// Reset zoom and set offsets to the middle of the map
	scale_ = 2;
	offset_.x = bbox.min.x + ((bbox.max.x - bbox.min.x) * 0.5);
	offset_.y = bbox.min.y + ((bbox.max.y - bbox.min.y) * 0.5);

	// Now just keep zooming out until we fit the whole map in the view
	bool done = false;
	while (!done)
	{
		// Update screen limits
		updateMapBounds();

		if (bbox.min.x >= mapBounds().tl.x &&
			bbox.max.x <= mapBounds().br.x &&
			bbox.min.y >= mapBounds().tl.y &&
			bbox.max.y <= mapBounds().br.y)
			done = true;
		else
			scale_ *= 0.8;
	}
}

bool RenderView::interpolate(double mult, const fpoint2_t* towards)
{
	bool interpolating = false;

	// Scale
	double diff_scale = scale_ - scale_inter_;
	if (diff_scale < -0.0000001 || diff_scale > 0.0000001)
	{
		// Get current mouse position in map coordinates (for zdooming towards [towards])
		double mx, my;
		if (towards)
		{
			mx = mapX(towards->x, true);
			my = mapY(towards->y, true);
		}

		// Interpolate zoom
		scale_inter_ += diff_scale*mult;

		// Check for zoom finish
		if ((diff_scale < 0 && scale_inter_ < scale_) ||
			(diff_scale > 0 && scale_inter_ > scale_))
			scale_inter_ = scale_;
		else
			interpolating = true;

		if (towards)
		{
			setOffset(
				offset_inter_.x + mx - mapX(towards->x, true),
				offset_inter_.y + my - mapY(towards->y, true)
			);
			offset_inter_ = offset_;
		}
	}
	else
		scale_inter_ = scale_;

	//if (!towards)
	{
		// X offset
		double diff_xoff = offset_.x - offset_inter_.x;
		if (diff_xoff < -0.05 || diff_xoff > 0.05)
		{
			// Interpolate offset
			offset_inter_.x += diff_xoff*mult;

			// Check stuff
			if ((diff_xoff < 0 && offset_inter_.x < offset_.x) ||
				(diff_xoff > 0 && offset_inter_.x > offset_.x))
				offset_inter_.x = offset_.x;
			else
				interpolating = true;
		}
		else
			offset_inter_.x = offset_.x;

		// Y offset
		double diff_yoff = offset_.y - offset_inter_.y;
		if (diff_yoff < -0.05 || diff_yoff > 0.05)
		{
			// Interpolate offset
			offset_inter_.y += diff_yoff*mult;

			if ((diff_yoff < 0 && offset_inter_.y < offset_.y) ||
				(diff_yoff > 0 && offset_inter_.y > offset_.y))
				offset_inter_.y = offset_.y;
			else
				interpolating = true;
		}
		else
			offset_inter_.y = offset_.y;
	}

	return interpolating;
}

/* RenderView::mapX
 * Translates an x position on the screen to the corresponding x
 * position on the map itself
 *******************************************************************/
double RenderView::mapX(double screen_x, bool inter) const
{
	return inter ?
		double(screen_x / scale_inter_) + offset_inter_.x - (double(size_.x * 0.5) / scale_inter_) :
		double(screen_x / scale_) + offset_.x - (double(size_.x * 0.5) / scale_);
}

/* RenderView::mapY
 * Translates a y position on the screen to the corresponding y
 * position on the map itself
 *******************************************************************/
double RenderView::mapY(double screen_y, bool inter) const
{
	return inter ?
		double(-screen_y / scale_inter_) + offset_inter_.y + (double(size_.y * 0.5) / scale_inter_) :
		double(-screen_y / scale_) + offset_.y + (double(size_.y * 0.5) / scale_);
}

/* RenderView::mapPos
 * Translates a position on the screen to the corresponding position
 * on the map itself
 *******************************************************************/
fpoint2_t RenderView::mapPos(const fpoint2_t& screen_pos, bool inter) const
{
	return { mapX(screen_pos.x, inter), mapY(screen_pos.y, inter) };
}

/* RenderView::screenX
 * Translates [x] from map coordinates to screen coordinates
 *******************************************************************/
int RenderView::screenX(double map_x) const
{
	return MathStuff::round((size_.x * 0.5) + ((map_x - offset_inter_.x) * scale_inter_));
}

/* RenderView::screenY
 * Translates [y] from map coordinates to screen coordinates
 *******************************************************************/
int RenderView::screenY(double map_y) const
{
	return MathStuff::round((size_.y * 0.5) - ((map_y - offset_inter_.y) * scale_inter_));
}

void RenderView::apply()
{
	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, size_.x, 0.0f, size_.y, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implemenataions)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Translate to middle of screen
	glTranslated(size_.x * 0.5, size_.y * 0.5, 0);

	// Zoom
	glScaled(scale_inter_, scale_inter_, 1);

	// Translate to offsets
	glTranslated(-offset_inter_.x, -offset_inter_.y, 0);
}

void RenderView::setOverlayCoords(bool set) const
{
	if (set)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, size_.x, size_.y, 0, -1, 1);
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

void RenderView::updateMapBounds()
{
	map_bounds_.tl.x = mapX(0);
	map_bounds_.tl.y = mapY(size_.y);
	map_bounds_.br.x = mapX(size_.x);
	map_bounds_.br.y = mapY(0);
}





Renderer::Renderer(MapEditContext& context) :
	context_{ context },
	renderer_2d_{ &context.map() },
	renderer_3d_{ &context.map() }
{
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
 * Scrolls the view relatively by [x,y]
 *******************************************************************/
void Renderer::pan(double x, double y)
{
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

bool Renderer::viewIsInterpolated() const
{
	return (view_.scale(false) != view_.scale(true) ||
			view_.offset(false).x != view_.offset(true).x ||
			view_.offset(false).y != view_.offset(true).y);
}

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

fpoint2_t Renderer::cameraPos2D()
{
	return { renderer_3d_.camPosition().x, renderer_3d_.camPosition().y };
}

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
		double y = context_.snapToGrid(view_.mapX(mouse_pos.y), false);
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
	//col.a = col.a * anim_help_fade_; // TODO: This
	//col_bg.a = col_bg.a * anim_help_fade_;
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
void Renderer::drawObjectEdit() const
{
	auto& group = context_.objectEdit().group();
	auto col = ColourConfiguration::getColour("map_object_edit");
	auto edit_state = context_.objectEdit().state();

	// Map objects
	context_.renderer().renderer2D().renderObjectEditGroup(&group);

	// Bounding box
	OpenGL::setColour(COL_WHITE);
	glColor4f(col.fr(), col.fg(), col.fb(), 1.0f);
	auto bbox = group.getBBox();
	bbox.min.x -= 4 / context_.renderer().viewScale(true);
	bbox.min.y -= 4 / context_.renderer().viewScale(true);
	bbox.max.x += 4 / context_.renderer().viewScale(true);
	bbox.max.y += 4 / context_.renderer().viewScale(true);

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
		double rad = 4 / context_.renderer().viewScale(true);
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
		128 / context_.renderer().viewScale(),
		nl_v1,
		nl_v2))
	{
		fpoint2_t mid(nl_v1.x + ((nl_v2.x - nl_v1.x) * 0.5), nl_v1.y + ((nl_v2.y - nl_v1.y) * 0.5));
		int length = MathStuff::distance(nl_v1, nl_v2);
		int x = context_.renderer().screenX(mid.x);
		int y = context_.renderer().screenY(mid.y) - 8;
		view_.setOverlayCoords(true);
		Drawing::setTextOutline(1.0f, COL_BLACK);
		Drawing::drawText(S_FMT("%d", length), x, y, COL_WHITE, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
		Drawing::setTextOutline(0);
		view_.setOverlayCoords(false);
		glDisable(GL_TEXTURE_2D);
	}
}
