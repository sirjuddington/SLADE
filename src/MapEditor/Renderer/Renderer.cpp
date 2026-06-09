
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "2D/MapRenderer2D.h"
#include "3D/MapRenderer3D.h"
#include "App.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/Misc.h"
#include "Geometry/Geometry.h"
#include "MCAnimations.h"
#include "MapEditor/Edit/Input.h"
#include "MapEditor/Edit/LineDraw.h"
#include "MapEditor/Edit/MoveObjects.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapClipboardItems.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "OpenGL/Camera.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/OpenGL.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/VertexBuffer3D.h"
#include "OpenGL/View.h"
#include "Overlays/LoadingOverlay.h"
#include "Overlays/MCOverlay.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <deque>

using namespace slade;
using namespace mapeditor;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map_renderinfo, false, CVar::Flag::Save)
CVAR(Bool, map_show_help, true, CVar::Flag::Save)
CVAR(Bool, map_showfps, false, CVar::Flag::Save)
CVAR(Int, map2d_64grid_style, 1, CVar::Flag::Save)
CVAR(Int, map2d_crosshair, 0, CVar::Flag::Save)
CVAR(Bool, map2d_flat_fade, 1, CVar::Flag::Save)
CVAR(Bool, map2d_grid_dashed, false, CVar::Flag::Save)
CVAR(Bool, map2d_grid_show_origin, true, CVar::Flag::Save)
CVAR(Int, map2d_line_fade, 2, CVar::Flag::Save)
CVAR(Bool, map2d_line_tabs_always, 1, CVar::Flag::Save)
CVAR(Int, map2d_max_selection_numbers, 1000, CVar::Flag::Save)
CVAR(Bool, map2d_scroll_smooth, true, CVar::Flag::Save)
CVAR(Bool, map2d_show_selection_numbers, true, CVar::Flag::Save)
CVAR(Int, map2d_things_always, 2, CVar::Flag::Save)
CVAR(Int, map2d_vertices_always, 0, CVar::Flag::Save)
CVAR(Bool, map3d_crosshair_show_distance, false, CVar::Flag::Save)
CVAR(Int, map3d_fov, 90, CVar::Flag::Save)
CVAR(Bool, map3d_gravity, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map2d_vertex_round)
EXTERN_CVAR(Int, map2d_vertex_size)
EXTERN_CVAR(Int, map2d_thing_shape)
EXTERN_CVAR(Bool, map3d_mlook_always)



namespace
{
// -----------------------------------------------------------------------------
// FPSCounter class
//
// Simple utility class for calculating and tracking FPS
// -----------------------------------------------------------------------------
class FPSCounter
{
public:
	FPSCounter() = default;

	int currentFPS() const { return displayed_fps_; }

	void setUpdateInterval(unsigned ms) { fps_update_interval_us_ = ms * 1000; }

	void update()
	{
		// Add latest frame time
		frame_times_.push_back(frame_clock_.restart().asMicroseconds());
		if (frame_times_.size() > 10)
			frame_times_.pop_front();

		// Update displayed FPS if enough time has passed
		fps_update_acc_us_ += frame_times_.back();
		if (fps_update_acc_us_ >= fps_update_interval_us_)
		{
			fps_update_acc_us_ = 0;
			auto avg_frame_us  = std::accumulate(frame_times_.begin(), frame_times_.end(), static_cast<int64_t>(0))
								/ static_cast<int64_t>(frame_times_.size());
			displayed_fps_ = avg_frame_us > 0 ? static_cast<int>(1000000.0 / avg_frame_us) : 0;
		}
	}

private:
	sf::Clock       frame_clock_;
	std::deque<i64> frame_times_;
	int             displayed_fps_          = 0;
	i64             fps_update_acc_us_      = 0;
	i64             fps_update_interval_us_ = 250000;
};
} // namespace


// -----------------------------------------------------------------------------
//
// Renderer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Renderer class constructor
// -----------------------------------------------------------------------------
Renderer::Renderer(MapEditContext& context) : context_{ &context }
{
	view_              = std::make_unique<View>(true, true);
	view_screen_       = std::make_unique<View>(false, false);
	renderer_2d_       = std::make_unique<MapRenderer2D>(&context.map(), view_.get());
	renderer_3d_       = std::make_unique<MapRenderer3D>(context_, this);
	vb_grid_           = std::make_unique<VertexBuffer2D>();
	lb_crosshair_      = std::make_unique<LineBuffer>();
	lb_objectedit_box_ = std::make_unique<LineBuffer>();
	camera_            = std::make_unique<Camera>(Vec3f{ 0, 0, 1 });
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
		renderer_2d_->forceUpdate(context_->sectorEditMode() == SectorMode::Ceiling);

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
// Clears the 2d and 3d renderers' data
// -----------------------------------------------------------------------------
void Renderer::clearData()
{
	renderer_2d_ = std::make_unique<MapRenderer2D>(&context_->map(), view_.get());
	renderer_3d_ = std::make_unique<MapRenderer3D>(context_, this);
}

// -----------------------------------------------------------------------------
// Scrolls the view to be centered on map coordinates [x,y]
// -----------------------------------------------------------------------------
void Renderer::setView(double map_x, double map_y) const
{
	// Set new view
	view_->setOffset(map_x, map_y);
	view_->resetInter(true, true, false);
}

// -----------------------------------------------------------------------------
// Sets the view size to [width,height]
// -----------------------------------------------------------------------------
void Renderer::setViewSize(int width, int height) const
{
	// Set new size
	view_->setSize(width, height);
	view_screen_->setSize(width, height);
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
		view_->zoomToward(amount, context_->input().mousePos());
	}
	else
	{
		cursor_zoom_disabled_ = true;
		view_->zoom(amount);
	}
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
	view_->fitTo(context_->map().bounds());

	// Don't animate if specified
	if (snap)
		view_->resetInter(true, true, true);
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
			auto sbb   = dynamic_cast<MapSector*>(object)->boundingBox();
			bbox.min.x = std::min(sbb.min.x, bbox.min.x);
			bbox.min.y = std::min(sbb.min.y, bbox.min.y);
			bbox.max.x = std::max(sbb.max.x, bbox.max.x);
			bbox.max.y = std::max(sbb.max.y, bbox.max.y);
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
		auto mouse_pos = Vec2d{ static_cast<double>(context_->input().mousePos().x),
								static_cast<double>(context_->input().mousePos().y) };

		if (!view_->interpolate(mult * view_speed, cursor_zoom_disabled_ ? nullptr : &mouse_pos))
		{
			cursor_zoom_disabled_ = false;
			anim_view_speed       = 0.05;
		}
		else
		{
			anim_view_speed += 0.05 * mult;
			anim_view_speed = std::min(anim_view_speed, 0.4);
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
	if (auto sector = context_->map().thingParentSector(*thing))
		pos.z += sector->floor().plane.heightAt(pos.x, pos.y);

	// Set camera position & direction
	camera_->set(pos, geometry::vectorAngle(geometry::degToRad(thing->angle())));
}

// -----------------------------------------------------------------------------
// Returns the current 3d mode camera position in 2d
// -----------------------------------------------------------------------------
Vec2d Renderer::cameraPos2D() const
{
	return camera_->position().xy();
}

// -----------------------------------------------------------------------------
// Returns the current 3d mode camera direction in 2d (no pitch)
// -----------------------------------------------------------------------------
Vec2d Renderer::cameraDir2D() const
{
	return camera_->direction();
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
	if (map2d_grid_show_origin)
	{
		dc.line_thickness = 3.0f;
		dc.setColourFromConfig("map_grid");
		dc.drawLines({ Rectf(0, start_y, 0, end_y), Rectf(start_x, 0, end_x, 0) });
	}

	// Setup regular grid if it's not too small
	auto shader   = map2d_grid_dashed ? &draw2d::lineStippleShader(0xAAAA, 2.0f) : &draw2d::defaultShader(false);
	auto gridsize = static_cast<int>(context_->gridSize());
	if (gridsize > grid_hidelevel)
	{
		glm::vec4 col_grid = colourconfig::colour("map_grid");

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
	if (64 > grid_hidelevel && gridsize < 64 && map2d_64grid_style > 0)
	{
		glm::vec4 col_64grid = colourconfig::colour("map_64grid");
		int       cross_size = 8;
		cross_size           = std::min(gridsize, cross_size);

		// Disable stipple if style set to crosses
		if (map2d_64grid_style > 1)
			shader = &draw2d::defaultShader(false);

		// Vertical
		int ofs = static_cast<int>(start_x) % 64;
		for (int x = start_x - ofs; x <= end_x; x += 64)
		{
			if (map2d_64grid_style > 1)
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
			if (map2d_64grid_style > 1)
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
	if (map2d_crosshair > 0)
	{
		auto   mouse_pos = context_->input().mousePos();
		double x         = context_->snapToGrid(view_->canvasX(mouse_pos.x), false);
		double y         = context_->snapToGrid(view_->canvasY(mouse_pos.y), false);
		auto&  def       = colourconfig::colDef("map_64grid");
		auto   col       = def.colour;

		// Small
		if (map2d_crosshair == 1)
		{
			glm::vec4      col1  = col.ampf(1.0f, 1.0f, 1.0f, 5.0f);
			glm::vec4      col2  = col.ampf(1.0f, 1.0f, 1.0f, 0.0f);
			double         size  = context_->gridSize();
			double         one   = 1.0 / view_->scale(true).x;
			constexpr auto width = 2.0f;

			lb_crosshair_->add(
				LineBuffer::Line{ .v1_pos_width = { x + one, y, 0.0f, width },
								  .v1_colour    = col1,
								  .v2_pos_width = { x + size, y, 0.0f, width },
								  .v2_colour    = col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ .v1_pos_width = { x - one, y, 0.0f, width },
								  .v1_colour    = col1,
								  .v2_pos_width = { x - size, y, 0.0f, width },
								  .v2_colour    = col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ .v1_pos_width = { x, y + one, 0.0f, width },
								  .v1_colour    = col1,
								  .v2_pos_width = { x, y + size, 0.0f, width },
								  .v2_colour    = col2 });
			lb_crosshair_->add(
				LineBuffer::Line{ .v1_pos_width = { x, y - one, 0.0f, width },
								  .v1_colour    = col1,
								  .v2_pos_width = { x, y - size, 0.0f, width },
								  .v2_colour    = col2 });
		}

		// Full
		else if (map2d_crosshair == 2)
		{
			lb_crosshair_->add2d(x, start_y, x, end_y, col, 3.0f);
			lb_crosshair_->add2d(start_x, y, end_x, y, col, 3.0f);
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
	dc.text_size      = 20 * ui_scale_;
	dc.text_style     = draw2d::TextStyle::Outline;
	dc.text_alignment = draw2d::Align::Left;

	// Go through editor messages
	for (unsigned a = 0; a < context_->numEditorMessages(); a++)
	{
		// Check message time
		long time = context_->editorMessageTime(a);
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
		dc.drawText(context_->editorMessage(a), { 4.0f, yoff });

		yoff += dc.textLineHeight();
	}
}

// -----------------------------------------------------------------------------
// Draws any feature help text currently showing
// -----------------------------------------------------------------------------
void Renderer::drawFeatureHelpText(draw2d::Context& dc) const
{
	// Check if any text
	auto& help_lines = context_->featureHelpLines();
	if (help_lines.empty() || !map_show_help)
		return;

	// Title
	dc.font           = draw2d::Font::Bold;
	dc.text_size      = 20 * ui_scale_;
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
	dc.text_size         = 18 * ui_scale_;
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
	auto selection = context_->selection().selectedObjects();
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
		if (map2d_max_selection_numbers > 0 && std::cmp_greater(a, map2d_max_selection_numbers.value))
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

		if (context_->editMode() == Mode::Vertices)
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
	auto selection = context_->selection().selectedThings();
	if (selection.empty())
		return;

	// Setup drawing options
	dc.setColourFromConfig("map_moving");
	dc.line_thickness = 2.0f;

	// Build lines list
	auto          mouse_pos_m = view_->canvasPos(context_->input().mousePos());
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
	vec = glm::normalize(vec);

	// Determine point to place the text
	Vec2d tp(mid.x + (vec.x * tdist), mid.y + (vec.y * tdist));

	// Determine text half-height for vertical alignment
	auto   length = fmt::format("{}", math::round(glm::distance(p1, p2)));
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
	auto end = view_->canvasPos(context_->input().mousePos());
	if (snap_nearest_vertex)
	{
		// If shift is held down, snap to the nearest vertex (if any)
		if (auto vertex = context_->map().vertices().nearest(end))
		{
			end.x = vertex->xPos();
			end.y = vertex->yPos();
		}
	}
	else if (context_->gridSnap())
	{
		// Otherwise, snap to grid if needed
		end.x = context_->snapToGrid(end.x);
		end.y = context_->snapToGrid(end.y);
	}

	// Draw lines
	auto& line_draw = context_->lineDraw();
	int   npoints   = line_draw.nPoints();
	dc.setColourFromConfig("map_linedraw");
	dc.line_thickness = 2.0f;
	vector<Rectf> lines;
	if (npoints > 1)
	{
		for (int a = 0; a < npoints - 1; a++)
		{
			lines.emplace_back(line_draw.point(a), line_draw.point(a + 1));
			lines.push_back(geometry::lineTab(lines.back()));
		}
	}
	if (npoints > 0 && context_->lineDraw().state() == LineDraw::State::Line)
	{
		lines.emplace_back(line_draw.point(npoints - 1), end);
		lines.push_back(geometry::lineTab(lines.back()));
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
	if (npoints > 0 && context_->lineDraw().state() == LineDraw::State::Line)
		drawLineLength(dc, line_draw.point(npoints - 1), end);
	dc.view = view_.get();

	// Draw points
	auto          ps_type = map2d_vertex_round ? PointSpriteType::Circle : PointSpriteType::Textured;
	vector<Vec2f> points;
	for (auto& point : line_draw.points())
		points.emplace_back(point);
	if (context_->lineDraw().state() == LineDraw::State::Line
		|| context_->lineDraw().state() == LineDraw::State::ShapeOrigin)
		points.emplace_back(end);
	dc.pointsprite_type   = ps_type;
	dc.pointsprite_radius = map2d_vertex_size / view_->scale(true).x;
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
	auto          pos = context_->relativeSnapToGrid(c->midpoint(), view_->canvasPos(context_->input().mousePos()));
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

	auto& group      = context_->objectEdit().group();
	auto  edit_state = context_->objectEdit().state();

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

	if (context_->objectEdit().rotating())
	{
		// Rotate

		// Bbox
		Vec2d mid(bbox.min.x + bbox.width() * 0.5, bbox.min.y + bbox.height() * 0.5);
		auto  bl = geometry::rotatePoint(mid, bbox.min, group.rotation());
		auto  tl = geometry::rotatePoint(mid, Vec2d(bbox.min.x, bbox.max.y), group.rotation());
		auto  tr = geometry::rotatePoint(mid, bbox.max, group.rotation());
		auto  br = geometry::rotatePoint(mid, Vec2d(bbox.max.x, bbox.min.y), group.rotation());
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
		if (edit_state == State::Move
			|| edit_state == State::Left
			|| edit_state == State::TopLeft
			|| edit_state == State::BottomLeft)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.min.x, bbox.min.y, bbox.min.x, bbox.max.y, glm::vec4{ 1.0f }, width);

		// Bottom
		if (edit_state == State::Move
			|| edit_state == State::Bottom
			|| edit_state == State::BottomLeft
			|| edit_state == State::BottomRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.min.x, bbox.min.y, bbox.max.x, bbox.min.y, glm::vec4{ 1.0f }, width);

		// Right
		if (edit_state == State::Move
			|| edit_state == State::Right
			|| edit_state == State::TopRight
			|| edit_state == State::BottomRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.max.x, bbox.max.y, bbox.max.x, bbox.min.y, glm::vec4{ 1.0f }, width);

		// Top
		if (edit_state == State::Move
			|| edit_state == State::Top
			|| edit_state == State::TopLeft
			|| edit_state == State::TopRight)
			width = 4.0f;
		else
			width = 2.0f;
		lb_objectedit_box_->add2d(bbox.max.x, bbox.max.y, bbox.min.x, bbox.max.y, glm::vec4{ 1.0f }, width);

		lb_objectedit_box_->push();
		lb_objectedit_box_->draw(dc.view, dc.colour);
	}

	// Line length
	Vec2d nl_v1, nl_v2;
	if (group.nearestLineEndpoints(
			view_->canvasPos(context_->input().mousePos()), 128 / view_->scale().x, nl_v1, nl_v2))
	{
		Vec2d mid(nl_v1.x + ((nl_v2.x - nl_v1.x) * 0.5), nl_v1.y + ((nl_v2.y - nl_v1.y) * 0.5));
		int   length = glm::distance(nl_v1, nl_v2);
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
// Draws the 2d map
// -----------------------------------------------------------------------------
void Renderer::drawMap2d(draw2d::Context& dc, bool focused) const
{
	if (!context_->map().isOpen())
	{
		// Map isn't open, just draw the grid
		drawGrid(dc);
		return;
	}

	auto mouse_state = context_->input().mouseState();

	// Draw flats if needed
	renderer_2d_->renderFlats(context_->sectorEditMode() == SectorMode::Ceiling, fade_flats_);

	// Draw point light previews if enabled
	renderer_2d_->renderPointLightPreviews(dc, fade_things_);

	// Draw grid
	drawGrid(dc);

	// --- Draw map (depending on mode) ---
	gl::resetBlend();
	if (context_->editMode() == Mode::Vertices)
	{
		// Vertices mode
		renderer_2d_->renderThings(fade_things_);                       // Things
		renderer_2d_->renderLines(map2d_line_tabs_always, fade_lines_); // Lines

		// Vertices
		if (mouse_state == Input::MouseState::Move)
			renderer_2d_->renderVertices(0.25f);
		else
			renderer_2d_->renderVertices(fade_vertices_);

		// Selection if needed
		if (mouse_state != Input::MouseState::Move
			&& !context_->overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderVertexSelection(dc, context_->selection(), anim_flash_level_);

		// Hilight if needed
		if (focused && mouse_state == Input::MouseState::Normal && !context_->overlayActive())
			renderer_2d_->renderVertexHilight(dc, context_->hilightItem().index, anim_flash_level_);
	}
	else if (context_->editMode() == Mode::Lines)
	{
		// Lines mode
		renderer_2d_->renderThings(fade_things_);     // Things
		renderer_2d_->renderVertices(fade_vertices_); // Vertices
		renderer_2d_->renderLines(true);              // Lines

		// Selection if needed
		if (mouse_state != Input::MouseState::Move
			&& !context_->overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderLineSelection(dc, context_->selection(), anim_flash_level_);

		// Hilight if needed
		if (focused && mouse_state == Input::MouseState::Normal && !context_->overlayActive())
			renderer_2d_->renderLineHilight(dc, context_->hilightItem().index, anim_flash_level_);
	}
	else if (context_->editMode() == Mode::Sectors)
	{
		// Sectors mode
		renderer_2d_->renderThings(fade_things_);                       // Things
		renderer_2d_->renderLines(map2d_line_tabs_always, fade_lines_); // Lines
		renderer_2d_->renderVertices(fade_vertices_);                   // Vertices

		// Selection if needed
		if (mouse_state != Input::MouseState::Move
			&& !context_->overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderFlatSelection(dc, context_->selection(), anim_flash_level_);

		// splitter.testRender();	// Testing

		// Hilight if needed
		if (focused && mouse_state == Input::MouseState::Normal && !context_->overlayActive())
			renderer_2d_->renderFlatHilight(dc, context_->hilightItem().index, anim_flash_level_);
	}
	else if (context_->editMode() == Mode::Things)
	{
		// Check if we should force thing angles visible
		bool force_dir = false;
		if (mouse_state == Input::MouseState::ThingAngle)
			force_dir = true;

		// Things mode
		renderer_2d_->renderLines(map2d_line_tabs_always, fade_lines_); // Lines
		renderer_2d_->renderVertices(fade_vertices_);                   // Vertices
		renderer_2d_->renderThings(fade_things_, force_dir);            // Things

		// Thing paths
		renderer_2d_->renderPathedThings(dc, context_->pathedThings());

		// Selection if needed
		if (mouse_state != Input::MouseState::Move
			&& !context_->overlayActive()
			&& mouse_state != Input::MouseState::ObjectEdit)
			renderer_2d_->renderThingSelection(dc, context_->selection(), anim_flash_level_);

		// Hilight if needed
		if (focused && mouse_state == Input::MouseState::Normal && !context_->overlayActive())
			renderer_2d_->renderThingHilight(dc, context_->hilightItem().index, anim_flash_level_);
	}

	// Draw tagged sectors/lines/things if needed
	if (focused
		&& !context_->overlayActive()
		&& (mouse_state == Input::MouseState::Normal
			|| mouse_state == Input::MouseState::TagSectors
			|| mouse_state == Input::MouseState::TagThings))
	{
		if (!context_->taggedSectors().empty())
			renderer_2d_->renderTaggedFlats(dc, context_->taggedSectors(), anim_flash_level_);
		if (!context_->taggedLines().empty())
			renderer_2d_->renderTaggedLines(dc, context_->taggedLines(), anim_flash_level_);
		if (!context_->taggedThings().empty())
			renderer_2d_->renderTaggedThings(dc, context_->taggedThings(), anim_flash_level_);
		if (!context_->taggingLines().empty())
			renderer_2d_->renderTaggingLines(dc, context_->taggingLines(), anim_flash_level_);
		if (!context_->taggingThings().empty())
			renderer_2d_->renderTaggingThings(dc, context_->taggingThings(), anim_flash_level_);
	}

	// Draw selection numbers if needed
	if (!context_->selection().empty() && mouse_state == Input::MouseState::Normal && map2d_show_selection_numbers)
		drawSelectionNumbers(dc);

	// Draw thing quick angle lines if needed
	if (mouse_state == Input::MouseState::ThingAngle)
		drawThingQuickAngleLines(dc);

	// Draw line drawing lines if needed
	if (mouse_state == Input::MouseState::LineDraw)
		drawLineDrawLines(dc, context_->input().shiftDown());

	// Draw object edit objects if needed
	if (mouse_state == Input::MouseState::ObjectEdit)
		drawObjectEdit(dc);

	// Draw sectorbuilder test stuff
	// sbuilder.drawResult();


	// Draw selection box if active
	Rectf sel_rect(
		view_->canvasX(context_->input().mousePos().x),
		view_->canvasY(context_->input().mousePos().y),
		view_->canvasX(context_->input().mouseDownPos().x),
		view_->canvasY(context_->input().mouseDownPos().y));
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
	for (auto& animation : animations_)
		if (!animation->mode3d())
			animation->draw(dc);

	// Draw paste objects if needed
	if (mouse_state == Input::MouseState::Paste)
	{
		if (context_->editMode() == Mode::Things)
		{
			Vec2d mpos{ view_->canvasX(context_->input().mousePos().x),
						view_->canvasY(context_->input().mousePos().y) };

			// Get clipboard item
			for (unsigned a = 0; a < app::clipboard().size(); a++)
			{
				auto item = app::clipboard().item(a);
				if (item->type() == ClipboardItem::Type::MapThings)
				{
					vector<MapThing*> things;
					auto              p = dynamic_cast<MapThingsClipboardItem*>(item);
					p->putThings(things);
					auto pos(context_->relativeSnapToGrid(p->midpoint(), mpos));
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
		auto& items  = context_->moveObjects().items();
		auto  offset = context_->moveObjects().offset();
		switch (context_->editMode())
		{
		case Mode::Vertices: renderer_2d_->renderMovingVertices(dc, items, offset); break;
		case Mode::Lines:    renderer_2d_->renderMovingLines(dc, items, offset); break;
		case Mode::Sectors:  renderer_2d_->renderMovingSectors(dc, items, offset); break;
		case Mode::Things:   renderer_2d_->renderMovingThings(dc, items, offset); break;
		default:             break;
		}
	}
}

// -----------------------------------------------------------------------------
// Draws the 3d map
// -----------------------------------------------------------------------------
void Renderer::drawMap3d(bool focused) const
{
	// Setup camera
	camera_->setProjection(view_->size().x, view_->size().y, 2.0f, 40000.0f, map3d_fov);

	// Render map
	renderer_3d_->render(*camera_, *view_);

	// Render highlight
	if (focused
		&& (context_->input().mouseState() == Input::MouseState::Normal
			|| (context_->input().mouseState() == Input::MouseState::MouseLook && map3d_mlook_always)))
		renderer_3d_->renderHighlight(context_->hilightItem(), *camera_, *view_, anim_flash_level_);

	// Render selection
	renderer_3d_->renderSelection(*camera_, *view_);

	// Draw animations
	for (auto& animation : animations_)
		if (animation->mode3d())
			animation->draw(*renderer_3d_, *camera_, *view_);
}

// -----------------------------------------------------------------------------
// Draws the current map editor state
// -----------------------------------------------------------------------------
void Renderer::draw(bool focused) const
{
	static sf::Clock           clock;
	static std::deque<int64_t> render_times;
	static unsigned            draw_calls;
	static FPSCounter          fps_counter;

	draw2d::Context dc{ view_.get() };

	fps_counter.update();
	clock.restart();
	gl::resetDrawCallCount();

	// Draw 2d or 3d map depending on mode
	if (context_->editMode() == Mode::Visual)
		drawMap3d(focused);
	else
		drawMap2d(dc, focused);

	// Disable depth testing for 2d overlays
	glDisable(GL_DEPTH_TEST);

	render_times.push_back(clock.getElapsedTime().asMicroseconds());
	if (render_times.size() > 50)
		render_times.pop_front();
	draw_calls = gl::drawCallCount();

	// Set view for overlays
	dc.view = view_screen_.get();

	// Draw crosshair if in 3d mode + locked mouselook
	if (focused && context_->editMode() == Mode::Visual && map3d_mlook_always)
	{
		dc.setColourFromConfig("map_3d_crosshair");
		dc.line_thickness = 2.0f;
		auto center       = Vec2f(view_->size().x * 0.5f, view_->size().y * 0.5f);
		auto inner        = 3.0f * ui_scale_;
		auto outer        = 10.0f * ui_scale_;
		dc.drawLines(
			{ { Vec2f(center.x - inner, center.y), Vec2f(center.x - outer, center.y) },
			  { Vec2f(center.x + inner, center.y), Vec2f(center.x + outer, center.y) },
			  { Vec2f(center.x, center.y - inner), Vec2f(center.x, center.y - outer) },
			  { Vec2f(center.x, center.y + inner), Vec2f(center.x, center.y + outer) } },
			true);
	}

	// Draw info overlay
	if (focused)
	{
	dc.font            = draw2d::Font::Condensed;
	dc.text_size       = 16 * ui_scale_;
	dc.text_style      = draw2d::TextStyle::Normal;
	dc.text_dropshadow = false;
	dc.blend           = Blend::Normal;
	context_->drawInfoOverlay(dc, anim_info_fade_);
	}

	// Draw current fullscreen overlay
	if (context_->currentOverlay() && anim_overlay_fade_ > 0.01f)
		context_->currentOverlay()->draw(dc, anim_overlay_fade_);

	// Draw loading overlay if active
	if (context_->loadingOverlayActive())
		context_->loadingOverlay().draw(dc, 1.0f);

	// FPS counter
	if (map_showfps)
	{
		dc.font           = draw2d::Font::MonospaceBold;
		dc.text_size      = 20 * ui_scale_;
		dc.text_style     = draw2d::TextStyle::Outline;
		dc.text_alignment = draw2d::Align::Left;
		dc.colour         = colourconfig::colour("map_editor_message");
		dc.outline_colour = colourconfig::colour("map_editor_message_outline");
		dc.drawText(fmt::format("{} FPS", fps_counter.currentFPS()), { 4.0f, 4.0f });
	}

	// Editor messages
	drawEditorMessages(dc);

	// Help text
	drawFeatureHelpText(dc);

	// RenderInfo
	if (map_renderinfo)
	{
		// Render performance info
		auto avg_frame = std::accumulate(render_times.begin(), render_times.end(), static_cast<int64_t>(0))
						 / render_times.size();
		auto frame_ms     = static_cast<double>(avg_frame) / 1000.0;
		auto fps          = static_cast<int>(1000.0 / frame_ms);
		dc.text_alignment = draw2d::Align::Left;
		dc.text_size      = 18;
		dc.text_style     = draw2d::TextStyle::Normal;
		dc.font           = draw2d::Font::Monospace;
		dc.colour         = ColRGBA::WHITE;
		dc.drawText(fmt::format("{:1.2f}ms ({}fps) - {} draw calls", frame_ms, fps, draw_calls), { 0.0f, 0.0f });

		// 3d mode misc info
		if (context_->editMode() == Mode::Visual)
		{
			auto y            = 0.0f;
			dc.text_alignment = draw2d::Align::Right;
			dc.drawText(
				fmt::format(
					"Position: {:1.2f},{:1.2f},{:1.2f} | Pitch: {:1.2f} | Direction: {:1.2f},{:1.2f},{:1.2f} | Up: "
					"{:1.2f},{:1.2f},{:1.2f}",
					camera_->position().x,
					camera_->position().y,
					camera_->position().z,
					camera_->pitch(),
					camera_->directionVector().x,
					camera_->directionVector().y,
					camera_->directionVector().z,
					camera_->upVector().x,
					camera_->upVector().y,
					camera_->upVector().z),
				{ view_->size().x, y });

			y += dc.textLineHeight();
			dc.drawText(
				fmt::format("Flats vertex buffer: {}", misc::sizeAsString(renderer_3d_->flatsBufferSize())),
				{ view_->size().x, y });

			y += dc.textLineHeight();
			dc.drawText(
				fmt::format("Quads vertex buffer: {}", misc::sizeAsString(renderer_3d_->quadsBufferSize())),
				{ view_->size().x, y });
		}
	}

	// Fade screen if not focused
	if (!focused)
	{
		dc.setColourFromConfig("map_background");
		dc.colour.a *= 0.5f;
		dc.drawRect({ 0.0f, 0.0f, static_cast<float>(view_->size().x), static_cast<float>(view_->size().y) });
	}
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
	if (context_->editMode() != Mode::Visual)
	{
		// Update 2d mode crossfade animation
		if (update2dModeCrossfade(mult))
			animations_active_ = true;

		// View pan/zoom animation
		anim_view_speed_ = interpolateView(map2d_scroll_smooth, anim_view_speed_, mult);
		if (viewIsInterpolated())
			animations_active_ = true;
	}

	// Flashing animation for hilight
	// Pulsates between 0.5-1.0f (multiplied with hilight alpha) using a sine wave
	constexpr float flash_min   = 0.5f;
	constexpr float flash_max   = 1.0f;
	constexpr float flash_mid   = (flash_min + flash_max) * 0.5f;
	constexpr float flash_amp   = (flash_max - flash_min) * 0.5f;
	constexpr float flash_speed = 4.0f;
	if (anim_flash_inc_)
	{
		if (anim_flash_level_ < flash_min)
			anim_flash_level_ = std::min(
				anim_flash_level_ + 0.053f * static_cast<float>(mult), flash_min); // Initial fade in
		else
		{
			// Transition to sine: align anim_flash_time_ so sine starts at current level
			const float asin_arg = std::clamp((anim_flash_level_ - flash_mid) / flash_amp, -1.0f, 1.0f);
			anim_flash_time_     = std::asin(asin_arg);
			anim_flash_inc_      = false;
		}
	}
	else
	{
		anim_flash_time_ += flash_speed * (mult / 60.0f);
		anim_flash_level_ = flash_mid + flash_amp * std::sin(anim_flash_time_);
	}

	// Fader for info overlay
	if (context_->infoOverlayActive() && !context_->overlayActive())
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
	if (context_->overlayActive() || context_->loadingOverlayActive())
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
	if (!context_->featureHelpLines().empty())
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
	if (map2d_vertices_always == 0)
		fa_vertices = 0.0f;
	else if (map2d_vertices_always == 1)
		fa_vertices = 1.0f;
	else
		fa_vertices = 0.5f;

	// Things
	if (map2d_things_always == 0)
		fa_things = 0.0f;
	else if (map2d_things_always == 1)
		fa_things = 1.0f;
	else
		fa_things = 0.5f;

	// Lines
	if (map2d_line_fade == 1 || context_->editMode() == Mode::Vertices && map2d_line_fade == 2)
		fa_lines = 0.6f;
	else
		fa_lines = 1.0f;

	// Flats
	if (map2d_flat_fade)
		fa_flats = 0.7f;
	else
		fa_flats = 1.0f;

	// Interpolate
	bool  anim_mode_crossfade = false;
	float mcs_speed           = 0.08f;
	if (context_->editMode() == Mode::Vertices)
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
	else if (context_->editMode() == Mode::Lines)
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
	else if (context_->editMode() == Mode::Sectors)
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
	else if (context_->editMode() == Mode::Things)
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
	fade_vertices_ = std::clamp(fade_vertices_, fa_vertices, 1.0f);
	fade_lines_    = std::clamp(fade_lines_, fa_lines, 1.0f);
	fade_flats_    = std::clamp(fade_flats_, fa_flats, 1.0f);
	fade_things_   = std::clamp(fade_things_, fa_things, 1.0f);

	return anim_mode_crossfade;
}

// -----------------------------------------------------------------------------
// Animates the (de)selection of [item], depending on [selected]
// -----------------------------------------------------------------------------
void Renderer::animateSelectionChange(const Item& item, bool selected)
{
	using mapeditor::ItemType;

	// 3d mode wall
	if (baseItemType(item.type) == ItemType::Side)
		animations_.push_back(std::make_unique<MCA3dSelection>(vector{ item }, *renderer_3d_, selected));

	// 3d mode flat
	else if (item.type == ItemType::Ceiling || item.type == ItemType::Floor)
		animations_.push_back(std::make_unique<MCA3dSelection>(vector{ item }, *renderer_3d_, selected));

	// 2d mode thing
	else if (item.type == ItemType::Thing)
	{
		// Get thing
		auto t = item.asThing(context_->map());
		if (!t)
			return;

		// Start animation
		animations_.push_back(
			std::make_unique<MCAThingSelection>(
				app::runTimer(),
				vector{ t },
				1.0 / view_->scale(true).x,
				map2d_thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
				selected));
	}

	// 2d mode line
	else if (item.type == ItemType::Line)
	{
		// Get line
		auto line = item.asLine(context_->map());
		if (!line)
			return;

		// Start animation
		animations_.push_back(std::make_unique<MCALineSelection>(app::runTimer(), vector{ line }, selected));
	}

	// 2d mode vertex
	else if (item.type == ItemType::Vertex)
	{
		// Get vertex
		auto vertex = item.asVertex(context_->map());
		if (!vertex)
			return;

		// Start animation
		animations_.push_back(
			std::make_unique<MCAVertexSelection>(
				app::runTimer(), vector{ vertex }, renderer_2d_->vertexRadius(), selected));
	}

	// 2d mode sector
	else if (item.type == ItemType::Sector)
	{
		// Get sector polygon
		auto sector = item.asSector(context_->map());
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
	const auto&        map = context_->map();
	vector<MapVertex*> vertices_selected;
	vector<MapVertex*> vertices_deselected;
	vector<MapLine*>   lines_selected;
	vector<MapLine*>   lines_deselected;
	vector<MapSector*> sectors_selected;
	vector<MapSector*> sectors_deselected;
	vector<MapThing*>  things_selected;
	vector<MapThing*>  things_deselected;
	vector<Item>       other_selected;
	vector<Item>       other_deselected;

	for (auto& change : selection.lastChange())
	{
		// 2d mode vertex
		if (auto vertex = change.first.asVertex(map))
		{
			if (change.second)
				vertices_selected.push_back(vertex);
			else
				vertices_deselected.push_back(vertex);
		}

		// 2d mode line
		else if (auto line = change.first.asLine(map))
		{
			if (change.second)
				lines_selected.push_back(line);
			else
				lines_deselected.push_back(line);
		}

		// 2d mode sector
		else if (auto sector = change.first.asSector(map))
		{
			if (change.second)
				sectors_selected.push_back(sector);
			else
				sectors_deselected.push_back(sector);
		}

		// 2d mode thing
		else if (auto thing = change.first.asThing(map); thing && context_->editMode() != Mode::Visual)
		{
			if (change.second)
				things_selected.push_back(thing);
			else
				things_deselected.push_back(thing);
		}

		// Other (3d mode)
		else
		{
			if (change.second)
				other_selected.push_back(change.first);
			else
				other_deselected.push_back(change.first);
		}
	}

	// Animate selected vertices
	if (!vertices_selected.empty())
		animations_.push_back(
			std::make_unique<MCAVertexSelection>(
				app::runTimer(), vertices_selected, renderer_2d_->vertexRadius(), true));

	// Animate deselected vertices
	if (!vertices_deselected.empty())
		animations_.push_back(
			std::make_unique<MCAVertexSelection>(
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
		animations_.push_back(
			std::make_unique<MCAThingSelection>(
				app::runTimer(),
				things_selected,
				view_->scale(true).x,
				map2d_thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
				true));

	// Animate deselected things
	if (!things_deselected.empty())
		animations_.push_back(
			std::make_unique<MCAThingSelection>(
				app::runTimer(),
				things_deselected,
				view_->scale(true).x,
				map2d_thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline,
				false));

	// Animate other selected items (3d mode)
	if (!other_selected.empty())
		animations_.push_back(std::make_unique<MCA3dSelection>(other_selected, *renderer_3d_, true));
	if (!other_deselected.empty())
		animations_.push_back(std::make_unique<MCA3dSelection>(other_deselected, *renderer_3d_, false));
}

// -----------------------------------------------------------------------------
// Animates a hilight change from [old_item] (3d mode) or [old_object] (2d mode)
// -----------------------------------------------------------------------------
void Renderer::animateHilightChange(const mapeditor::Item& old_item, MapObject* old_object)
{
	auto fade = std::isfinite(anim_flash_level_) ? std::clamp(anim_flash_level_, 0.0f, 1.0f) : 0.3f;

	if (old_object)
	{
		// 2d mode
		animations_.push_back(std::make_unique<MCAHilightFade>(app::runTimer(), old_object, renderer_2d_.get(), fade));
	}
	else
	{
		// 3d mode
		animations_.push_back(std::make_unique<MCAHilightFade3D>(app::runTimer(), old_item, fade));
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

// -----------------------------------------------------------------------------
// Clears all active animations
// -----------------------------------------------------------------------------
void Renderer::clearAnimations()
{
	animations_.clear();
}
