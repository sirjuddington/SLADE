
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Renderer.cpp
// Description: Renderer class - handles rendering/drawing functionality for
//              the map editor
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Renderer.h"
#include "App.h"
#include "Camera.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/UI.h"
#include "MCAnimations.h"
#include "MapEditor/ClipboardItems.h"
#include "MapEditor/Edit/Input.h"
#include "MapEditor/Edit/LineDraw.h"
#include "MapEditor/Edit/MoveObjects.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/OpenGL.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "Overlays/MCOverlay.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"
#include <deque>
#include <numeric>

using namespace slade;
using namespace mapeditor;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, things_always, 2, CVar::Flag::Save)
CVAR(Int, vertices_always, 0, CVar::Flag::Save)
CVAR(Bool, line_tabs_always, 1, CVar::Flag::Save)
CVAR(Bool, flat_fade, 1, CVar::Flag::Save)
CVAR(Bool, line_fade, 0, CVar::Flag::Save)
CVAR(Bool, grid_dashed, false, CVar::Flag::Save)
CVAR(Int, grid_64_style, 1, CVar::Flag::Save)
CVAR(Bool, grid_show_origin, true, CVar::Flag::Save)
CVAR(Bool, scroll_smooth, true, CVar::Flag::Save)
CVAR(Bool, map_showfps, false, CVar::Flag::Save)
CVAR(Bool, camera_3d_gravity, true, CVar::Flag::Save)
CVAR(Int, camera_3d_crosshair_size, 6, CVar::Flag::Save)
CVAR(Bool, camera_3d_show_distance, false, CVar::Flag::Save)
CVAR(Bool, map_show_help, true, CVar::Flag::Save)
CVAR(Int, map_crosshair, 0, CVar::Flag::Save)
CVAR(Bool, map_show_selection_numbers, true, CVar::Flag::Save)
CVAR(Int, map_max_selection_numbers, 1000, CVar::Flag::Save)
CVAR(Int, flat_drawtype, 2, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Int, thing_shape)


// -----------------------------------------------------------------------------
//
// Renderer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Renderer class constructor
// -----------------------------------------------------------------------------
Renderer::Renderer(MapEditContext& context) : context_{ context }
{
	view_              = std::make_unique<View>(true, true);
	view_screen_       = std::make_unique<View>(false, false);
	renderer_2d_       = std::make_unique<MapRenderer2D>(&context.map(), view_.get());
	renderer_3d_       = std::make_unique<MapRenderer3D>(&context.map());
	vb_grid_           = std::make_unique<VertexBuffer2D>();
	lb_crosshair_      = std::make_unique<LineBuffer>();
	lb_objectedit_box_ = std::make_unique<LineBuffer>();
}

// -----------------------------------------------------------------------------
// Renderer class destructor
// -----------------------------------------------------------------------------
Renderer::~Renderer() = default;

// -----------------------------------------------------------------------------
// Updates/refreshes the 2d and/or 3d renderers
// -----------------------------------------------------------------------------
void Renderer::forceUpdate(bool update_2d, bool update_3d) const
{
	if (update_2d)
		renderer_2d_->forceUpdate(fade_lines_);
	if (update_3d)
		renderer_3d_->clearData();
}

// -----------------------------------------------------------------------------
// Clears the 2d renderer's texture cache
// -----------------------------------------------------------------------------
void Renderer::clearTextureCache() const
{
	renderer_2d_->clearTextureCache();
}

// -----------------------------------------------------------------------------
// Scrolls the view to be centered on map coordinates [x,y]
// -----------------------------------------------------------------------------
void Renderer::setView(double map_x, double map_y) const
{
	// Set new view
	view_->setOffset(map_x, map_y);
	view_->resetInter(true, true, false);

	// Update object visibility
	renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);
}

// -----------------------------------------------------------------------------
// Sets the view size to [width,height]
// -----------------------------------------------------------------------------
void Renderer::setViewSize(int width, int height) const
{
	// Set new size
	view_->setSize(width, height);
	view_screen_->setSize(width, height);

	// Update object visibility
	renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);
}

// -----------------------------------------------------------------------------
// Sets the view such that the map coordinate [y] is at the top of the canvas
// -----------------------------------------------------------------------------
void Renderer::setTopY(double map_y) const
{
	setView(view_->offset().x, view_->offset().y - (view_->canvasY(0) - map_y));
	view_->resetInter(false, true, false);
}

// -----------------------------------------------------------------------------
// Scrolls the view relatively by [x,y].
// If [scale] is true, [x,y] will be scaled by the current view scale
// -----------------------------------------------------------------------------
void Renderer::pan(double x, double y, bool scale) const
{
	if (scale)
	{
		x /= view().scale().x;
		y /= view().scale().y;
	}

	setView(view_->offset().x + x, view_->offset().y + y);
}

// -----------------------------------------------------------------------------
// Zooms the view by [amount]. If [towards_cursor] is true the view will be
// zoomed towards the current mouse cursor position, otherwise towards the
// center of the screen
// -----------------------------------------------------------------------------
void Renderer::zoom(double amount, bool toward_cursor)
{
	// Zoom view
	if (toward_cursor)
	{
		cursor_zoom_disabled_ = false;
		view_->zoomToward(amount, context_.input().mousePos());
	}
	else
	{
		cursor_zoom_disabled_ = true;
		view_->zoom(amount);
	}

	// Update object visibility
	renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);
}

// -----------------------------------------------------------------------------
// Centers the view to the center of the map, and zooms in or out so that the
// entire map is showing
// -----------------------------------------------------------------------------
void Renderer::viewFitToMap(bool snap)
{
	// Disable zooming towards cursor until zoom animation is done
	cursor_zoom_disabled_ = true;

	// Fit the view to the map bbox
	view_->fitTo(context_.map().bounds());

	// Don't animate if specified
	if (snap)
		view_->resetInter(true, true, true);

	// Update object visibility
	renderer_2d_->forceUpdate();
	renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);
}

// -----------------------------------------------------------------------------
// Centers the view to the center of [objects], and zooms in or out so that all
// [objects] are showing
// -----------------------------------------------------------------------------
void Renderer::viewFitToObjects(const vector<MapObject*>& objects)
{
	// Disable zooming towards cursor until zoom animation is done
	cursor_zoom_disabled_ = true;

	// Determine bbox of all given object(s)
	BBox bbox;
	for (auto object : objects)
	{
		// Vertex
		if (object->objType() == MapObject::Type::Vertex)
		{
			auto vertex = dynamic_cast<MapVertex*>(object);
			bbox.extend(vertex->xPos(), vertex->yPos());
		}

		// Line
		else if (object->objType() == MapObject::Type::Line)
		{
			auto line = dynamic_cast<MapLine*>(object);
			bbox.extend(line->v1()->xPos(), line->v1()->yPos());
			bbox.extend(line->v2()->xPos(), line->v2()->yPos());
		}

		// Sector
		else if (object->objType() == MapObject::Type::Sector)
		{
			auto sbb = dynamic_cast<MapSector*>(object)->boundingBox();
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
		else if (object->objType() == MapObject::Type::Thing)
		{
			auto thing = dynamic_cast<MapThing*>(object);
			bbox.extend(thing->xPos(), thing->yPos());
		}
	}

	// Fit the view to the bbox
	view_->fitTo(bbox);

	// Update object visibility
	renderer_2d_->forceUpdate();
	renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);
}

// -----------------------------------------------------------------------------
// Interpolates the current 2d view if [smooth] is set, based on [view_speed]
// and [mult]
// -----------------------------------------------------------------------------
double Renderer::interpolateView(bool smooth, double view_speed, double mult)
{
	auto anim_view_speed = view_speed;
	if (smooth)
	{
		auto mouse_pos = Vec2d{ (double)context_.input().mousePos().x, (double)context_.input().mousePos().y };

		if (!view_->interpolate(mult * view_speed, cursor_zoom_disabled_ ? nullptr : &mouse_pos))
		{
			cursor_zoom_disabled_ = false;
			anim_view_speed       = 0.05;
		}
		else
		{
			anim_view_speed += 0.05 * mult;
			if (anim_view_speed > 0.4)
				anim_view_speed = 0.4;
		}
	}
	else
		view_->resetInter(true, true, true);

	return anim_view_speed;
}

// -----------------------------------------------------------------------------
// Returns true if the current view is interpolated
// -----------------------------------------------------------------------------
bool Renderer::viewIsInterpolated() const
{
	return view_->scale(false) != view_->scale(true) || view_->offset(false) != view_->offset(true);
}

// -----------------------------------------------------------------------------
// Sets the 3d camera to match [thing]
// -----------------------------------------------------------------------------
void Renderer::setCameraThing(const MapThing* thing) const
{
	// Determine position
	Vec3d pos(thing->position(), 40);
	auto  sector = context_.map().sectors().atPos(thing->position());
	if (sector)
		pos.z += sector->floor().plane.heightAt(pos.x, pos.y);

	// Set camera position & direction
	renderer_3d_->camera().set(pos, math::vectorAngle(math::degToRad(thing->angle())));
}

// -----------------------------------------------------------------------------
// Returns the current 3d mode camera position in 2d
// -----------------------------------------------------------------------------
Vec2d Renderer::cameraPos2D() const
{
	return renderer_3d_->camera().position().get2d();
}

// -----------------------------------------------------------------------------
// Returns the current 3d mode camera direction in 2d (no pitch)
// -----------------------------------------------------------------------------
Vec2d Renderer::cameraDir2D() const
{
	return renderer_3d_->camera().direction();
}

// -----------------------------------------------------------------------------
// Draws the grid
// -----------------------------------------------------------------------------
void Renderer::drawGrid(gl::draw2d::Context& dc) const
{
	// Determine smallest grid size to bother drawing
	int grid_hidelevel = 4.0 / view_->scale().x;

	// Determine canvas edges in map coordinates
	auto start_x = view_->canvasX(0);
	auto end_x   = view_->canvasX(view_->size().x);
	auto start_y = view_->canvasY(view_->size().y);
	auto end_y   = view_->canvasY(0);

	// Draw origin grid lines
	if (grid_show_origin)
	{
		dc.line_thickness = 3.0f;
		dc.setColourFromConfig("map_grid");
		dc.drawLines({ Rectf(0, start_y, 0, end_y), Rectf(start_x, 0, end_x, 0) });
	}

	// Setup regular grid if it's not too small
	auto shader   = grid_dashed ? &draw2d::lineStippleShader(0xAAAA, 2.0f) : &draw2d::defaultShader(false);
	auto gridsize = static_cast<int>(context_.gridSize());
	if (gridsize > grid_hidelevel)
	{
		auto col_grid = colourconfig::colour("map_grid").asVec4();

		// Vertical
		int ofs = static_cast<int>(start_x) % gridsize;
		for (int x = start_x - ofs; x <= end_x; x += gridsize)
		{
			vb_grid_->add({ x, start_y }, col_grid, {});
			vb_grid_->add({ x, end_y }, col_grid, {});
		}

		// Horizontal
		ofs = static_cast<int>(start_y) % gridsize;
		for (int y = start_y - ofs; y <= end_y; y += gridsize)
		{
			vb_grid_->add({ start_x, y }, col_grid, {});
			vb_grid_->add({ end_x, y }, col_grid, {});
		}

		vb_grid_->push();

		gl::setBlend(colourconfig::colDef("map_grid").blendMode());
		shader->setUniform("colour", glm::vec4{ 1.0f });
		vb_grid_->draw(Primitive::Lines, shader, view_.get());
	}

	// Setup 64 grid if it's not too small and we're not on a larger grid size
	if (64 > grid_hidelevel && gridsize < 64 && grid_64_style > 0)
	{
		auto col_64grid = colourconfig::colour("map_64grid").asVec4();
		int  cross_size = 8;
		if (gridsize < cross_size)
			cross_size = gridsize;

		// Disable stipple if style set to crosses
		if (grid_64_style > 1)
			shader = &draw2d::defaultShader(false);

		// Vertical
		int ofs = static_cast<int>(start_x) % 64;
		for (int x = start_x - ofs; x <= end_x; x += 64)
		{
			if (grid_64_style > 1)
			{
				// Cross style
				int y = start_y - (static_cast<int>(start_y) % 64);
				while (y < end_y)
				{
					vb_grid_->add({ x, y - cross_size }, col_64grid, {});
					vb_grid_->add({ x, y + cross_size }, col_64grid, {});
					y += 64;
				}
			}
			else
			{
				// Full style
				vb_grid_->add({ x, start_y }, col_64grid, {});
				vb_grid_->add({ x, end_y }, col_64grid, {});
			}
		}

		// Horizontal
		ofs = static_cast<int>(start_y) % 64;
		for (int y = start_y - ofs; y <= end_y; y += 64)
		{
			if (grid_64_style > 1)
			{
				// Cross style
				int x = start_x - (static_cast<int>(start_x) % 64);
				while (x < end_x)
				{
					vb_grid_->add({ x - cross_size, y }, col_64grid, {});
					vb_grid_->add({ x + cross_size, y }, col_64grid, {});
					x += 64;
				}
			}
			else
			{
				// Full style
				vb_grid_->add({ start_x, y }, col_64grid, {});
				vb_grid_->add({ end_x, y }, col_64grid, {});
			}
		}

		vb_grid_->push();

		gl::setBlend(colourconfig::colDef("map_64grid").blendMode());
		shader->setUniform("colour", glm::vec4{ 1.0f });
		vb_grid_->draw(Primitive::Lines, shader, view_.get());
	}

	// Draw crosshair if needed
	if (map_crosshair > 0)
	{
		auto   mouse_pos = context_.input().mousePos();
		double x         = context_.snapToGrid(view_->canvasX(mouse_pos.x), false);
		double y         = context_.snapToGrid(view_->canvasY(mouse_pos.y), false);
		auto&  def       = colourconfig::colDef("map_64grid");
		auto   col       = def.colour;

		// Small
		if (map_crosshair == 1)
		{
			auto           col1  = col.ampf(1.0f, 1.0f, 1.0f, 2.0f).asVec4();
			auto           col2  = col.ampf(1.0f, 1.0f, 1.0f, 0.0f).asVec4();
			double         size  = context_.gridSize();
			double         one   = 1.0 / view_->scale(true).x;
			constexpr auto width = 4.0f;

			lb_crosshair_->add(
				LineBuffer::Line{ { x + one, y, 0.0f, width }, col1, { x + size, y, 0.0f, width }, col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ { x - one, y, 0.0f, width }, col1, { x - size, y, 0.0f, width }, col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ { x, y + one, 0.0f, width }, col1, { x, y + size, 0.0f, width }, col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ { x, y - one, 0.0f, width }, col1, { x, y - size, 0.0f, width }, col2 });
		}

		// Full
		else if (map_crosshair == 2)
		{
			lb_crosshair_->add2d(x, start_y, x, end_y, col.asVec4(), 3.0f);
			lb_crosshair_->add2d(start_x, y, end_x, y, col.asVec4(), 3.0f);
		}

		gl::setBlend(def.blendMode());
		lb_crosshair_->push();
		lb_crosshair_->draw(view_.get());
	}
}

// -----------------------------------------------------------------------------
// Draws any currently showing editor messages
// -----------------------------------------------------------------------------
void Renderer::drawEditorMessages(draw2d::Context& dc) const
{
	float yoff = 4.0f;
	if (map_showfps)
		yoff = 32.0f;
	auto col_fg = colourconfig::colour("map_editor_message");
	auto col_bg = colourconfig::colour("map_editor_message_outline");

	dc.font           = draw2d::Font::Bold;
	dc.text_size      = 20 * ui::scaleFactor();
	dc.text_style     = draw2d::TextStyle::Outline;
	dc.text_alignment = draw2d::Align::Left;

	// Go through editor messages
	for (unsigned a = 0; a < context_.numEditorMessages(); a++)
	{
		// Check message time
		long time = context_.editorMessageTime(a);
		if (time > 2000)
			continue;

		// Setup message colour
		dc.colour = col_fg;
		if (time < 200)
		{
			float flash = 1.0f - (time / 200.0f);
			dc.colour.r += (255 - dc.colour.r) * flash;
			dc.colour.g += (255 - dc.colour.g) * flash;
			dc.colour.b += (255 - dc.colour.b) * flash;
		}

		// Setup message alpha
		col_bg.a = 255;
		if (time > 1500)
		{
			dc.colour.a = 255 - ((time - 1500) / 500.0 * 255);
			col_bg.a    = dc.colour.a;
		}

		// Draw message
		dc.outline_colour = col_bg;
		dc.drawText(context_.editorMessage(a), { 4.0f, yoff });

		yoff += dc.textLineHeight();
	}
}

// -----------------------------------------------------------------------------
// Draws any feature help text currently showing
// -----------------------------------------------------------------------------
void Renderer::drawFeatureHelpText(draw2d::Context& dc) const
{
	// Check if any text
	auto& help_lines = context_.featureHelpLines();
	if (help_lines.empty() || !map_show_help)
		return;

	// Title
	dc.font           = draw2d::Font::Bold;
	dc.text_size      = 20 * ui::scaleFactor();
	dc.text_style     = draw2d::TextStyle::Outline;
	dc.text_alignment = draw2d::Align::Right;
	dc.colour         = colourconfig::colour("map_editor_message").ampf(1.0f, 1.0f, 1.0f, anim_help_fade_);
	dc.outline_colour = colourconfig::colour("map_editor_message_outline").ampf(1.0f, 1.0f, 1.0f, anim_help_fade_);
	auto x            = static_cast<float>(view_->size().x) - 4.0f;
	auto underline_y  = 6.0f + dc.textLineHeight();
	dc.drawText(help_lines[0], { x, 4.0f });

	// Help Text
	auto yoff            = 4.0f + dc.textLineHeight() + 8.0f;
	auto underline_width = dc.textExtents(help_lines[0]).x;
	dc.text_size         = 18 * ui::scaleFactor();
	for (unsigned a = 1; a < help_lines.size(); a++)
	{
		dc.drawText(help_lines[a], { x, yoff });
		yoff += dc.textLineHeight();

		if (auto te = dc.textExtents(help_lines[a]); te.x > underline_width)
			underline_width = te.x;
	}

	// Title Underline
	dc.drawLines({ Rectf{ x - underline_width, underline_y, x + 4.0f, underline_y } });
}

// -----------------------------------------------------------------------------
// Draws numbers for selected map objects
// -----------------------------------------------------------------------------
void Renderer::drawSelectionNumbers(draw2d::Context& dc) const
{
	// Check if any selection exists
	auto selection = context_.selection().selectedObjects();
	if (selection.empty())
		return;

	// Setup drawing options
	dc.font       = draw2d::Font::Bold;
	dc.text_style = draw2d::TextStyle::Outline;
	dc.colour     = colourconfig::colour("map_editor_message");
	dc.view       = view_screen_.get();

	// Go through selection
	string text;
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (map_max_selection_numbers > 0 && (int)a > map_max_selection_numbers)
			break;
		if (!selection[a])
			continue;

		auto tp = selection[a]->getPoint(MapObject::Point::Text);
		tp.x    = view_->screenX(tp.x);
		tp.y    = view_->screenY(tp.y);

		text    = fmt::format("{}", a + 1);
		auto ts = draw2d::textExtents(text, draw2d::Font::Bold);
		tp.x -= ts.x * 0.5;
		tp.y -= ts.y * 0.5;

		if (context_.editMode() == Mode::Vertices)
		{
			tp.x += 8;
			tp.y += 8;
		}

		// Draw text
		dc.drawText(text, Vec2f(tp.x, tp.y));
	}

	// Reset to map view
	dc.view = view_.get();
}

// -----------------------------------------------------------------------------
// Draws directional lines for thing quick angle selection
// -----------------------------------------------------------------------------
void Renderer::drawThingQuickAngleLines(draw2d::Context& dc) const
{
	// Check if any selection exists
	auto selection = context_.selection().selectedThings();
	if (selection.empty())
		return;

	// Setup drawing options
	dc.setColourFromConfig("map_moving");
	dc.line_thickness = 2.0f;

	// Build lines list
	auto          mouse_pos_m = view_->canvasPos(context_.input().mousePos());
	vector<Rectf> lines;
	for (auto& thing : selection)
		lines.emplace_back(thing->xPos(), thing->yPos(), mouse_pos_m.x, mouse_pos_m.y);

	// Draw
	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws text showing the length from [p1] to [p2]
// -----------------------------------------------------------------------------
void Renderer::drawLineLength(draw2d::Context& dc, const Vec2d& p1, const Vec2d& p2) const
{
	// Determine distance in screen scale
	double tdist = 20 / view_->scale(true).x;

	// Determine line midpoint and front vector
	Vec2d mid(p1.x + (p2.x - p1.x) * 0.5, p1.y + (p2.y - p1.y) * 0.5);
	Vec2d vec(-(p2.y - p1.y), p2.x - p1.x);
	vec.normalize();

	// Determine point to place the text
	Vec2d tp(mid.x + (vec.x * tdist), mid.y + (vec.y * tdist));

	// Determine text half-height for vertical alignment
	auto   length = fmt::format("{}", math::round(math::distance(p1, p2)));
	double hh     = draw2d::textExtents(length, dc.font).y * 0.5;

	// Draw text
	dc.text_alignment = draw2d::Align::Center;
	dc.drawText(length, Vec2f(view_->screenX(tp.x), view_->screenY(tp.y) - hh));
}

// -----------------------------------------------------------------------------
// Draws current line drawing lines (best function name ever)
// -----------------------------------------------------------------------------
void Renderer::drawLineDrawLines(draw2d::Context& dc, bool snap_nearest_vertex) const
{
	// Determine end point
	auto end = view_->canvasPos(context_.input().mousePos());
	if (snap_nearest_vertex)
	{
		// If shift is held down, snap to the nearest vertex (if any)
		if (auto vertex = context_.map().vertices().nearest(end))
		{
			end.x = vertex->xPos();
			end.y = vertex->yPos();
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
	int   npoints   = line_draw.nPoints();
	dc.setColourFromConfig("map_linedraw");
	dc.line_thickness = 2.0f;
	vector<Rectf> lines;
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
		{
			lines.emplace_back(line_draw.point(a), line_draw.point(a + 1));
			lines.push_back(math::lineTab(lines.back()));
		}
	}
	if (npoints > 0 && context_.lineDraw().state() == LineDraw::State::Line)
	{
		lines.emplace_back(line_draw.point(npoints - 1), end);
		lines.push_back(math::lineTab(lines.back()));
	}
	dc.drawLines(lines);

	// Draw line lengths
	dc.view = view_screen_.get();
	dc.font = draw2d::Font::Bold;
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
			drawLineLength(dc, line_draw.point(a), line_draw.point(a + 1));
	}
	if (npoints > 0 && context_.lineDraw().state() == LineDraw::State::Line)
		drawLineLength(dc, line_draw.point(npoints - 1), end);
	dc.view = view_.get();

	// Draw points
	auto          ps_type = vertex_round ? PointSpriteType::Circle : PointSpriteType::Textured;
	vector<Vec2f> points;
	for (auto& point : line_draw.points())
		points.emplace_back(point);
	if (context_.lineDraw().state() == LineDraw::State::Line
		|| context_.lineDraw().state() == LineDraw::State::ShapeOrigin)
		points.emplace_back(end);
	dc.pointsprite_type   = ps_type;
	dc.pointsprite_radius = vertex_size / view_->scale(true).x;
	dc.drawPointSprites(points);
}

// -----------------------------------------------------------------------------
// Draws lines currently being pasted
// -----------------------------------------------------------------------------
void Renderer::drawPasteLines(draw2d::Context& dc) const
{
	// Get clipboard item
	MapArchClipboardItem* c = nullptr;
	for (unsigned a = 0; a < app::clipboard().size(); a++)
	{
		if (app::clipboard().item(a)->type() == ClipboardItem::Type::MapArchitecture)
		{
			c = dynamic_cast<MapArchClipboardItem*>(app::clipboard().item(a));
			break;
		}
	}

	if (!c)
		return;

	// Get lines
	vector<MapLine*> map_lines;
	c->putLines(map_lines);
	auto          pos = context_.relativeSnapToGrid(c->midpoint(), view_->canvasPos(context_.input().mousePos()));
	vector<Rectf> lines;
	for (const auto& line : map_lines)
		lines.emplace_back(pos.x + line->x1(), pos.y + line->y1(), pos.x + line->x2(), pos.y + line->y2());

	// Draw
	dc.setColourFromConfig("map_linedraw");
	dc.line_thickness = 2.0f;
	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws object edit objects, bounding box and text
// -----------------------------------------------------------------------------
void Renderer::drawObjectEdit(draw2d::Context& dc) const
{
	using State = ObjectEdit::State;

	auto& group      = context_.objectEdit().group();
	auto  edit_state = context_.objectEdit().state();

	// Map objects
	renderer_2d_->renderObjectEditGroup(dc, &group);

	// Bounding box
	auto bbox = group.bbox();
	bbox.min.x -= 4 / view_->scale(true).x;
	bbox.min.y -= 4 / view_->scale(true).x;
	bbox.max.x += 4 / view_->scale(true).x;
	bbox.max.y += 4 / view_->scale(true).x;

	// Setup draw options
	dc.colour         = colourconfig::colour("map_object_edit");
	dc.colour.a       = 255;
	dc.blend          = Blend::Normal;
	dc.line_thickness = 2.0f;
	gl::setBlend(dc.blend);

	if (context_.objectEdit().rotating())
	{
		// Rotate

		// Bbox
		Vec2d mid(bbox.min.x + bbox.width() * 0.5, bbox.min.y + bbox.height() * 0.5);
		auto  bl = math::rotatePoint(mid, bbox.min, group.rotation());
		auto  tl = math::rotatePoint(mid, Vec2d(bbox.min.x, bbox.max.y), group.rotation());
		auto  tr = math::rotatePoint(mid, bbox.max, group.rotation());
		auto  br = math::rotatePoint(mid, Vec2d(bbox.max.x, bbox.min.y), group.rotation());
		dc.drawLines({ { tl, bl }, { bl, br }, { br, tr }, { tr, tl } });

		// Top Left
		float rad         = 4.0f / view_->scale(true).x;
		dc.line_thickness = 1.0f;
		if (edit_state == State::TopLeft)
			dc.drawRect({ tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad });
		else
			dc.drawRectOutline({ tl.x - rad, tl.y - rad, tl.x + rad, tl.y + rad });

		// Bottom Left
		if (edit_state == State::BottomLeft)
			dc.drawRect({ bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad });
		else
			dc.drawRectOutline({ bl.x - rad, bl.y - rad, bl.x + rad, bl.y + rad });

		// Top Right
		if (edit_state == State::TopRight)
			dc.drawRect({ tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad });
		else
			dc.drawRectOutline({ tr.x - rad, tr.y - rad, tr.x + rad, tr.y + rad });

		// Bottom Right
		if (edit_state == State::BottomRight)
			dc.drawRect({ br.x - rad, br.y - rad, br.x + rad, br.y + rad });
		else
			dc.drawRectOutline({ br.x - rad, br.y - rad, br.x + rad, br.y + rad });
	}
	else
	{
		// Move/scale
		float width;

		// Left
		if (edit_state == State::Move || edit_state == State::Left || edit_state == State::TopLeft
			|| edit_state == State::BottomLeft)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.min.x, bbox.min.y, bbox.min.x, bbox.max.y, glm::vec4{ 1.0f }, width);

		// Bottom
		if (edit_state == State::Move || edit_state == State::Bottom || edit_state == State::BottomLeft
			|| edit_state == State::BottomRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.min.x, bbox.min.y, bbox.max.x, bbox.min.y, glm::vec4{ 1.0f }, width);

		// Right
		if (edit_state == State::Move || edit_state == State::Right || edit_state == State::TopRight
			|| edit_state == State::BottomRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.max.x, bbox.max.y, bbox.max.x, bbox.min.y, glm::vec4{ 1.0f }, width);

		// Top
		if (edit_state == State::Move || edit_state == State::Top || edit_state == State::TopLeft
			|| edit_state == State::TopRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.max.x, bbox.max.y, bbox.min.x, bbox.max.y, glm::vec4{ 1.0f }, width);

		lb_objectedit_box_->push();
		lb_objectedit_box_->draw(dc.view, dc.colour.asVec4());
	}

	// Line length
	Vec2d nl_v1, nl_v2;
	if (group.nearestLineEndpoints(view_->canvasPos(context_.input().mousePos()), 128 / view_->scale().x, nl_v1, nl_v2))
	{
		Vec2d mid(nl_v1.x + ((nl_v2.x - nl_v1.x) * 0.5), nl_v1.y + ((nl_v2.y - nl_v1.y) * 0.5));
		int   length = math::distance(nl_v1, nl_v2);
		auto  pos    = view_->screenPos(mid.x, mid.y);
		pos.y -= 8.0f;

		dc.view           = view_screen_.get();
		dc.text_style     = draw2d::TextStyle::Outline;
		dc.colour         = ColRGBA::WHITE;
		dc.outline_colour = ColRGBA::BLACK;
		dc.font           = draw2d::Font::Bold;
		dc.text_alignment = draw2d::Align::Center;
		dc.drawText(fmt::format("{}", length), pos);
	}
}

// -----------------------------------------------------------------------------
// Draws all MCAnimations for the current edit mode
// -----------------------------------------------------------------------------
void Renderer::drawAnimations(gl::draw2d::Context& dc) const
{
	auto mode = context_.editMode();
	for (auto& animation : animations_)
	{
		if ((mode == Mode::Visual && animation->mode3d()) || (mode != Mode::Visual && !animation->mode3d()))
			animation->draw(dc);
	}
}

// -----------------------------------------------------------------------------
// Draws the 2d map
// -----------------------------------------------------------------------------
void Renderer::drawMap2d(draw2d::Context& dc) const
{
	auto mouse_state = context_.input().mouseState();

	// Update visibility info if needed
	if (!renderer_2d_->visOK())
		renderer_2d_->updateVisibility(view_->visibleRegion().tl, view_->visibleRegion().br);

	// Draw flats if needed
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

		renderer_2d_->renderFlats(drawtype, texture, fade_flats_);
	}

	// Draw grid
	drawGrid(dc);

	// --- Draw map (depending on mode) ---
	gl::resetBlend();
	if (context_.editMode() == Mode::Vertices)
	{
		// Vertices mode
		renderer_2d_->renderThings(fade_things_);                 // Things
		renderer_2d_->renderLines(line_tabs_always, fade_lines_); // Lines

		// Vertices
		if (mouse_state == Input::MouseState::Move)
			renderer_2d_->renderVertices(0.25f);
		else
			renderer_2d_->renderVertices(fade_vertices_);

		// Selection if needed
		if (mouse_state != Input::MouseState::Move && !context_.overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderVertexSelection(dc, context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_->renderVertexHilight(context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Lines)
	{
		// Lines mode
		renderer_2d_->renderThings(fade_things_);     // Things
		renderer_2d_->renderVertices(fade_vertices_); // Vertices
		renderer_2d_->renderLines(true);              // Lines

		// Selection if needed
		if (mouse_state != Input::MouseState::Move && !context_.overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderLineSelection(dc, context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_->renderLineHilight(dc, context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Sectors)
	{
		// Sectors mode
		renderer_2d_->renderThings(fade_things_);                 // Things
		renderer_2d_->renderLines(line_tabs_always, fade_lines_); // Lines
		renderer_2d_->renderVertices(fade_vertices_);             // Vertices

		// Selection if needed
		if (mouse_state != Input::MouseState::Move && !context_.overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderFlatSelection(dc, context_.selection(), anim_flash_level_);

		// splitter.testRender();	// Testing

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_->renderFlatHilight(dc, context_.hilightItem().index, anim_flash_level_);
	}
	else if (context_.editMode() == Mode::Things)
	{
		// Check if we should force thing angles visible
		bool force_dir = false;
		if (mouse_state == Input::MouseState::ThingAngle)
			force_dir = true;

		// Things mode
		auto hl_index = context_.hilightItem().index;
		renderer_2d_->renderLines(line_tabs_always, fade_lines_);           // Lines
		renderer_2d_->renderVertices(fade_vertices_);                       // Vertices
		renderer_2d_->renderPointLightPreviews(dc, fade_things_, hl_index); // Point light previews
		renderer_2d_->renderThings(fade_things_, force_dir);                // Things

		// Thing paths
		renderer_2d_->renderPathedThings(dc, context_.pathedThings());

		// Selection if needed
		if (mouse_state != Input::MouseState::Move && !context_.overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderThingSelection(dc, context_.selection(), anim_flash_level_);

		// Hilight if needed
		if (mouse_state == Input::MouseState::Normal && !context_.overlayActive())
			renderer_2d_->renderThingHilight(dc, hl_index, anim_flash_level_);
	}

	// Draw tagged sectors/lines/things if needed
	if (!context_.overlayActive()
		&& (mouse_state == Input::MouseState::Normal || mouse_state == Input::MouseState::TagSectors
			|| mouse_state == Input::MouseState::TagThings))
	{
		if (!context_.taggedSectors().empty())
			renderer_2d_->renderTaggedFlats(dc, context_.taggedSectors(), anim_flash_level_);
		if (!context_.taggedLines().empty())
			renderer_2d_->renderTaggedLines(dc, context_.taggedLines(), anim_flash_level_);
		if (!context_.taggedThings().empty())
			renderer_2d_->renderTaggedThings(dc, context_.taggedThings(), anim_flash_level_);
		if (!context_.taggingLines().empty())
			renderer_2d_->renderTaggingLines(dc, context_.taggingLines(), anim_flash_level_);
		if (!context_.taggingThings().empty())
			renderer_2d_->renderTaggingThings(dc, context_.taggingThings(), anim_flash_level_);
	}

	// Draw selection numbers if needed
	if (!context_.selection().empty() && mouse_state == Input::MouseState::Normal && map_show_selection_numbers)
		drawSelectionNumbers(dc);

	// Draw thing quick angle lines if needed
	if (mouse_state == Input::MouseState::ThingAngle)
		drawThingQuickAngleLines(dc);

	// Draw line drawing lines if needed
	if (mouse_state == Input::MouseState::LineDraw)
		drawLineDrawLines(dc, context_.input().shiftDown());

	// Draw object edit objects if needed
	if (mouse_state == Input::MouseState::ObjectEdit)
		drawObjectEdit(dc);

	// Draw sectorbuilder test stuff
	// sbuilder.drawResult();


	// Draw selection box if active
	Rectf sel_rect(
		view_->canvasX(context_.input().mousePos().x),
		view_->canvasY(context_.input().mousePos().y),
		view_->canvasX(context_.input().mouseDownPos().x),
		view_->canvasY(context_.input().mouseDownPos().y));
	if (mouse_state == Input::MouseState::Selection)
	{
		// Outline
		dc.setColourFromConfig("map_selbox_outline");
		dc.texture        = 0;
		dc.line_thickness = 2.0f;
		dc.drawRectOutline(sel_rect);

		// Fill
		dc.setColourFromConfig("map_selbox_fill");
		dc.drawRect(sel_rect);
	}

	// Draw animations
	drawAnimations(dc);

	// Draw paste objects if needed
	if (mouse_state == Input::MouseState::Paste)
	{
		if (context_.editMode() == Mode::Things)
		{
			Vec2d mpos{ view_->canvasX(context_.input().mousePos().x), view_->canvasY(context_.input().mousePos().y) };

			// Get clipboard item
			for (unsigned a = 0; a < app::clipboard().size(); a++)
			{
				auto item = app::clipboard().item(a);
				if (item->type() == ClipboardItem::Type::MapThings)
				{
					vector<MapThing*> things;
					auto              p = dynamic_cast<MapThingsClipboardItem*>(item);
					p->putThings(things);
					auto pos(context_.relativeSnapToGrid(p->midpoint(), mpos));
					renderer_2d_->renderPasteThings(dc, things, pos);
				}
			}
		}
		else
			drawPasteLines(dc);
	}

	// Draw moving stuff if needed
	if (mouse_state == Input::MouseState::Move)
	{
		auto& items  = context_.moveObjects().items();
		auto  offset = context_.moveObjects().offset();
		switch (context_.editMode())
		{
		case Mode::Vertices: renderer_2d_->renderMovingVertices(dc, items, offset); break;
		case Mode::Lines: renderer_2d_->renderMovingLines(dc, items, offset); break;
		case Mode::Sectors: renderer_2d_->renderMovingSectors(dc, items, offset); break;
		case Mode::Things: renderer_2d_->renderMovingThings(dc, items, offset); break;
		default: break;
		}
	}
}

// -----------------------------------------------------------------------------
// Draws the 3d map
// -----------------------------------------------------------------------------
void Renderer::drawMap3d() const
{
	// Setup 3d renderer view
	renderer_3d_->setupView(view_->size().x, view_->size().y);

	// Render 3d map
	renderer_3d_->renderMap();

	// Draw selection if any
	auto selection = context_.selection();
	renderer_3d_->renderFlatSelection(selection);
	renderer_3d_->renderWallSelection(selection);
	renderer_3d_->renderThingSelection(selection);

	// Draw hilight if any
	if (context_.selection().hasHilight())
		renderer_3d_->renderHilight(context_.selection().hilight(), anim_flash_level_);

	// Draw animations
	// drawAnimations();
}

// -----------------------------------------------------------------------------
// Draws the current map editor state
// -----------------------------------------------------------------------------
void Renderer::draw() const
{
	static sf::Clock           clock;
	static std::deque<int64_t> render_times;
	static unsigned            draw_calls;

	draw2d::Context dc{ view_.get() };

	clock.restart();
	gl::resetDrawCallCount();

	// Draw 2d or 3d map depending on mode
	if (context_.editMode() == Mode::Visual)
		drawMap2d(dc); // drawMap3d();
	else
		drawMap2d(dc);

	render_times.push_back(clock.getElapsedTime().asMicroseconds());
	if (render_times.size() > 50)
		render_times.pop_front();
	draw_calls = gl::drawCallCount();

	// Set view for overlays
	dc.view = view_screen_.get();

	// Draw info overlay
	dc.font       = draw2d::Font::Condensed;
	dc.text_size  = 16 * ui::scaleFactor();
	dc.text_style = draw2d::TextStyle::Normal;
	dc.blend      = Blend::Normal;
	context_.drawInfoOverlay(dc, anim_info_fade_);

	// Draw current fullscreen overlay
	if (context_.currentOverlay() && anim_overlay_fade_ > 0.01f)
		context_.currentOverlay()->draw(dc, anim_overlay_fade_);

	//// Draw crosshair if 3d mode
	// if (context_.editMode() == Mode::Visual)
	//{
	//	// Get crosshair colour
	//	auto& def = colourconfig::colDef("map_3d_crosshair");
	//	auto  col = def.colour;
	//	gl::setColour(col, def.blendMode());

	//	glDisable(GL_TEXTURE_2D);
	//	glEnable(GL_LINE_SMOOTH);
	//	glLineWidth(1.5f);

	//	double midx = view_->size().x * 0.5;
	//	double midy = view_->size().y * 0.5;
	//	int    size = camera_3d_crosshair_size;

	//	glBegin(GL_LINES);
	//	// Right
	//	gl::setColour(col);
	//	glVertex2d(midx + 1, midy);
	//	glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
	//	glVertex2d(midx + size, midy);

	//	// Left
	//	gl::setColour(col);
	//	glVertex2d(midx - 1, midy);
	//	glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
	//	glVertex2d(midx - size, midy);

	//	// Bottom
	//	gl::setColour(col);
	//	glVertex2d(midx, midy + 1);
	//	glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
	//	glVertex2d(midx, midy + size);

	//	// Top
	//	gl::setColour(col);
	//	glVertex2d(midx, midy - 1);
	//	glColor4f(col.fr(), col.fg(), col.fb(), 0.0f);
	//	glVertex2d(midx, midy - size);
	//	glEnd();

	//	// Draw item distance (if any)
	//	if (context_.renderer().renderer3D().itemDistance() >= 0 && camera_3d_show_distance)
	//	{
	//		glEnable(GL_TEXTURE_2D);
	//		gl::setColour(col);
	//		drawing::drawText(
	//			fmt::format("{}", renderer_3d_->itemDistance()),
	//			midx + 5,
	//			midy + 5,
	//			ColRGBA(255, 255, 255, 200),
	//			drawing::Font::Small);
	//	}
	//}

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
		Drawing::drawText(fmt::format("FPS: {}", afps));
	}*/

	// test
	// Drawing::drawText(fmt::format("Render distance: {:1.2f}", (double)render_max_dist), 0, 100);

	// Editor messages
	drawEditorMessages(dc);

	// Help text
	drawFeatureHelpText(dc);

	// TESTING: Render performance info
	auto avg_frame    = std::accumulate(render_times.begin(), render_times.end(), (int64_t)0) / render_times.size();
	dc.text_alignment = draw2d::Align::Left;
	dc.text_size      = 18;
	dc.text_style     = draw2d::TextStyle::Normal;
	dc.font           = draw2d::Font::Monospace;
	dc.colour         = ColRGBA::WHITE;
	dc.drawText(
		fmt::format("{:1.2f}ms - {} draw calls", static_cast<double>(avg_frame) / 1000.0, draw_calls), { 0.0f, 0.0f });
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
} // namespace

// -----------------------------------------------------------------------------
// Updates all currently active animations
// -----------------------------------------------------------------------------
void Renderer::updateAnimations(double mult)
{
	animations_active_ = false;

	// Update MCAnimations
	for (unsigned a = 0; a < animations_.size(); a++)
	{
		if (!animations_[a]->update(app::runTimer()))
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
	}

	// Flashing animation for hilight
	// Pulsates between 0.5-1.0f (multiplied with hilight alpha)
	constexpr float flash_min = 0.5f;
	constexpr float flash_max = 1.0f;
	if (anim_flash_inc_)
	{
		if (anim_flash_level_ < flash_min)
			anim_flash_level_ += 0.053 * mult; // Initial fade in
		else
			anim_flash_level_ += 0.015f * mult;
		if (anim_flash_level_ >= flash_max)
		{
			anim_flash_inc_   = false;
			anim_flash_level_ = flash_max;
		}
	}
	else
	{
		anim_flash_level_ -= 0.015f * mult;
		if (anim_flash_level_ <= flash_min)
		{
			anim_flash_inc_   = true;
			anim_flash_level_ = flash_min;
		}
	}

	// Fader for info overlay
	if (context_.infoOverlayActive() && !context_.overlayActive())
	{
		if (updateFade(anim_info_fade_, 0.1f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_info_fade_, -0.04f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}

	// Fader for fullscreen overlay
	if (context_.overlayActive())
	{
		if (updateFade(anim_overlay_fade_, 0.1f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_overlay_fade_, -0.05f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}

	// Fader for help text
	if (!context_.featureHelpLines().empty())
	{
		if (updateFade(anim_help_fade_, 0.07f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
	else
	{
		if (updateFade(anim_help_fade_, -0.05f * mult, 0.0f, 1.0f))
			animations_active_ = true;
	}
}

// -----------------------------------------------------------------------------
// Updates the 2d mode crossfade animations (when switching modes)
// -----------------------------------------------------------------------------
bool Renderer::update2dModeCrossfade(double mult)
{
	// --- Fade map objects depending on mode ---

	// Determine fade levels
	float fa_vertices, fa_lines, fa_flats, fa_things;

	// Vertices
	if (vertices_always == 0)
		fa_vertices = 0.0f;
	else if (vertices_always == 1)
		fa_vertices = 1.0f;
	else
		fa_vertices = 0.5f;

	// Things
	if (things_always == 0)
		fa_things = 0.0f;
	else if (things_always == 1)
		fa_things = 1.0f;
	else
		fa_things = 0.5f;

	// Lines
	if (line_fade)
		fa_lines = 0.5f;
	else
		fa_lines = 1.0f;

	// Flats
	if (flat_fade)
		fa_flats = 0.7f;
	else
		fa_flats = 1.0f;

	// Interpolate
	bool  anim_mode_crossfade = false;
	float mcs_speed           = 0.08f;
	if (context_.editMode() == Mode::Vertices)
	{
		if (fade_vertices_ < 1.0f)
		{
			fade_vertices_ += mcs_speed * (1.0f - fa_vertices) * mult;
			anim_mode_crossfade = true;
		}
		fade_lines_ = fa_lines;
		if (fade_flats_ > fa_flats)
		{
			fade_flats_ -= mcs_speed * (1.0f - fa_flats) * mult;
			anim_mode_crossfade = true;
		}
		if (fade_things_ > fa_things)
		{
			fade_things_ -= mcs_speed * (1.0f - fa_things) * mult;
			anim_mode_crossfade = true;
		}
	}
	else if (context_.editMode() == Mode::Lines)
	{
		if (fade_vertices_ > fa_vertices)
		{
			fade_vertices_ -= mcs_speed * (1.0f - fa_vertices) * mult;
			anim_mode_crossfade = true;
		}
		fade_lines_ = 1.0f;
		if (fade_flats_ > fa_flats)
		{
			fade_flats_ -= mcs_speed * (1.0f - fa_flats) * mult;
			anim_mode_crossfade = true;
		}
		if (fade_things_ > fa_things)
		{
			fade_things_ -= mcs_speed * (1.0f - fa_things) * mult;
			anim_mode_crossfade = true;
		}
	}
	else if (context_.editMode() == Mode::Sectors)
	{
		if (fade_vertices_ > fa_vertices)
		{
			fade_vertices_ -= mcs_speed * (1.0f - fa_vertices) * mult;
			anim_mode_crossfade = true;
		}
		fade_lines_ = fa_lines;
		if (fade_flats_ < 1.0f)
		{
			fade_flats_ += mcs_speed * (1.0f - fa_flats) * mult;
			anim_mode_crossfade = true;
		}
		if (fade_things_ > fa_things)
		{
			fade_things_ -= mcs_speed * (1.0f - fa_things) * mult;
			anim_mode_crossfade = true;
		}
	}
	else if (context_.editMode() == Mode::Things)
	{
		if (fade_vertices_ > fa_vertices)
		{
			fade_vertices_ -= mcs_speed * (1.0f - fa_vertices) * mult;
			anim_mode_crossfade = true;
		}
		fade_lines_ = fa_lines;
		if (fade_flats_ > fa_flats)
		{
			fade_flats_ -= mcs_speed * (1.0f - fa_flats) * mult;
			anim_mode_crossfade = true;
		}
		if (fade_things_ < 1.0f)
		{
			fade_things_ += mcs_speed * (1.0f - fa_things) * mult;
			anim_mode_crossfade = true;
		}
	}

	// Clamp
	if (fade_vertices_ < fa_vertices)
		fade_vertices_ = fa_vertices;
	if (fade_vertices_ > 1.0f)
		fade_vertices_ = 1.0f;
	if (fade_lines_ < fa_lines)
		fade_lines_ = fa_lines;
	if (fade_lines_ > 1.0f)
		fade_lines_ = 1.0f;
	if (fade_flats_ < fa_flats)
		fade_flats_ = fa_flats;
	if (fade_flats_ > 1.0f)
		fade_flats_ = 1.0f;
	if (fade_things_ < fa_things)
		fade_things_ = fa_things;
	if (fade_things_ > 1.0f)
		fade_things_ = 1.0f;

	return anim_mode_crossfade;
}

// -----------------------------------------------------------------------------
// Animates the (de)selection of [item], depending on [selected]
// -----------------------------------------------------------------------------
void Renderer::animateSelectionChange(const mapeditor::Item& item, bool selected)
{
	using mapeditor::ItemType;

	// 3d mode wall
	if (mapeditor::baseItemType(item.type) == ItemType::Side)
	{
		// Get quad
		auto quad = renderer_3d_->getQuad(item);

		if (quad)
		{
			// Get quad points
			Vec3f points[4];
			for (unsigned a = 0; a < 4; a++)
				points[a].set(quad->points[a].x, quad->points[a].y, quad->points[a].z);

			// Start animation
			animations_.push_back(std::make_unique<MCA3dWallSelection>(app::runTimer(), points, selected));
		}
	}

	// 3d mode flat
	else if (item.type == ItemType::Ceiling || item.type == ItemType::Floor)
	{
		// Get flat
		auto flat = renderer_3d_->getFlat(item);

		// Start animation
		if (flat)
			animations_.push_back(
				std::make_unique<MCA3dFlatSelection>(app::runTimer(), flat->sector, flat->plane, selected));
	}

	// 2d mode thing
	else if (item.type == ItemType::Thing)
	{
		// Get thing
		auto t = item.asThing(context_.map());
		if (!t)
			return;

		// Start animation
		animations_.push_back(std::make_unique<MCAThingSelection>(
			app::runTimer(),
			vector{ t },
			1.0 / view_->scale(true).x,
			thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
			selected));
	}

	// 2d mode line
	else if (item.type == ItemType::Line)
	{
		// Get line
		auto line = item.asLine(context_.map());
		if (!line)
			return;

		// Start animation
		animations_.push_back(std::make_unique<MCALineSelection>(app::runTimer(), vector{ line }, selected));
	}

	// 2d mode vertex
	else if (item.type == ItemType::Vertex)
	{
		// Get vertex
		auto vertex = item.asVertex(context_.map());
		if (!vertex)
			return;

		// Start animation
		animations_.push_back(std::make_unique<MCAVertexSelection>(
			app::runTimer(), vector{ vertex }, renderer_2d_->vertexRadius(), selected));
	}

	// 2d mode sector
	else if (item.type == ItemType::Sector)
	{
		// Get sector polygon
		auto sector = item.asSector(context_.map());
		if (!sector)
			return;

		// Start animation
		animations_.push_back(std::make_unique<MCASectorSelection>(app::runTimer(), vector{ sector }, selected));
	}
}

// -----------------------------------------------------------------------------
// Animates the last selection change from [selection]
// -----------------------------------------------------------------------------
void Renderer::animateSelectionChange(const ItemSelection& selection)
{
	const auto&        map = context_.map();
	vector<MapVertex*> vertices_selected;
	vector<MapVertex*> vertices_deselected;
	vector<MapLine*>   lines_selected;
	vector<MapLine*>   lines_deselected;
	vector<MapSector*> sectors_selected;
	vector<MapSector*> sectors_deselected;
	vector<MapThing*>  things_selected;
	vector<MapThing*>  things_deselected;

	for (auto& change : selection.lastChange())
	{
		// 2d mode vertex
		if (change.first.type == ItemType::Vertex)
		{
			if (change.second)
				vertices_selected.push_back(change.first.asVertex(map));
			else
				vertices_deselected.push_back(change.first.asVertex(map));
		}

		// 2d mode line
		else if (change.first.type == ItemType::Line)
		{
			if (change.second)
				lines_selected.push_back(change.first.asLine(map));
			else
				lines_deselected.push_back(change.first.asLine(map));
		}

		// 2d mode sector
		else if (change.first.type == ItemType::Sector)
		{
			if (change.second)
				sectors_selected.push_back(change.first.asSector(map));
			else
				sectors_deselected.push_back(change.first.asSector(map));
		}

		// 2d mode thing
		else if (change.first.type == ItemType::Thing)
		{
			if (change.second)
				things_selected.push_back(change.first.asThing(map));
			else
				things_deselected.push_back(change.first.asThing(map));
		}

		// Other (animate individual items)
		else
			animateSelectionChange(change.first, change.second);
	}

	// Animate selected vertices
	if (!vertices_selected.empty())
		animations_.push_back(std::make_unique<MCAVertexSelection>(
			app::runTimer(), vertices_selected, renderer_2d_->vertexRadius(), true));

	// Animate deselected vertices
	if (!vertices_deselected.empty())
		animations_.push_back(std::make_unique<MCAVertexSelection>(
			app::runTimer(), vertices_deselected, renderer_2d_->vertexRadius(), false));

	// Animate selected lines
	if (!lines_selected.empty())
		animations_.push_back(std::make_unique<MCALineSelection>(app::runTimer(), lines_selected, true));

	// Animate deselected lines
	if (!lines_deselected.empty())
		animations_.push_back(std::make_unique<MCALineSelection>(app::runTimer(), lines_deselected, false));

	// Animate selected sectors
	if (!sectors_selected.empty())
		animations_.push_back(std::make_unique<MCASectorSelection>(app::runTimer(), sectors_selected, true));

	// Animate deselected sectors
	if (!sectors_deselected.empty())
		animations_.push_back(std::make_unique<MCASectorSelection>(app::runTimer(), sectors_deselected, false));

	// Animate selected things
	if (!things_selected.empty())
		animations_.push_back(std::make_unique<MCAThingSelection>(
			app::runTimer(),
			things_selected,
			view_->scale(true).x,
			thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
			true));

	// Animate deselected things
	if (!things_deselected.empty())
		animations_.push_back(std::make_unique<MCAThingSelection>(
			app::runTimer(),
			things_deselected,
			view_->scale(true).x,
			thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
			false));
}

// -----------------------------------------------------------------------------
// Animates a hilight change from [old_item] (3d mode) or [old_object] (2d mode)
// -----------------------------------------------------------------------------
void Renderer::animateHilightChange(const mapeditor::Item& old_item, MapObject* old_object)
{
	if (old_object)
	{
		// 2d mode
		animations_.push_back(
			std::make_unique<MCAHilightFade>(app::runTimer(), old_object, renderer_2d_.get(), anim_flash_level_));
	}
	else
	{
		// 3d mode
		animations_.push_back(std::make_unique<MCAHilightFade3D>(
			app::runTimer(), old_item.index, old_item.type, renderer_3d_.get(), anim_flash_level_));
	}

	// Reset hilight flash
	anim_flash_inc_   = true;
	anim_flash_level_ = 0.3f;
}

// -----------------------------------------------------------------------------
// Adds [animation] to the list of active animations
// -----------------------------------------------------------------------------
void Renderer::addAnimation(unique_ptr<MCAnimation> animation)
{
	animations_.push_back(std::move(animation));
}
