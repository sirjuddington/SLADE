
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MCAnimations.cpp
// Description: MCAnimation specialisation classes - simple animations for the
//              map editor that handle their own tracking/updating/drawing
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
#include "MCAnimations.h"
#include "App.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/Item.h"
#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/OpenGL.h"
#include "OpenGL/PointSpriteBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map2d_thing_overlay_square)
EXTERN_CVAR(Int, thing_drawtype)
EXTERN_CVAR(Int, map2d_thing_halo_width)
EXTERN_CVAR(Bool, map2d_sector_selected_fill)


// -----------------------------------------------------------------------------
//
// MCASelboxFader Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCASelboxFader class constructor
// -----------------------------------------------------------------------------
MCASelboxFader::MCASelboxFader(long start, const Vec2d& tl, const Vec2d& br) : MCAnimation(start), tl_{ tl }, br_{ br }
{
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCASelboxFader::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	return fade_ >= 0.0f;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCASelboxFader::draw(gl::draw2d::Context& dc)
{
	// Outline
	dc.texture        = 0;
	dc.line_thickness = 2.0f;
	dc.setColourFromConfig("map_selbox_outline", fade_);
	dc.drawRectOutline({ tl_, br_ });

	// Fill
	dc.setColourFromConfig("map_selbox_fill", fade_);
	dc.drawRect({ tl_, br_ });
}


// -----------------------------------------------------------------------------
//
// MCAThingSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAThingSelection class constructor
// -----------------------------------------------------------------------------
MCAThingSelection::MCAThingSelection(
	long                     start,
	const vector<MapThing*>& things,
	float                    view_scale,
	gl::PointSpriteType      ps_type,
	bool                     select) :
	MCAnimation(start),
	select_{ select },
	ps_type_{ ps_type }
{
	buffer_ = std::make_unique<gl::PointSpriteBuffer>();

	for (const auto* thing : things)
	{
		auto& tt = game::configuration().thingType(thing->type());

		float radius = tt.radius();
		if (tt.shrinkOnZoom())
			radius = scaledRadius(radius, view_scale);
		radius += 4.0f;

		buffer_->add(glm::vec2{ thing->xPos(), thing->yPos() }, radius);
	}
	buffer_->push();

	buffer_->setOutlineWidth(std::min(3.0f / view_scale, 4.0f));
	buffer_->setFillOpacity(0.25f);
}

// -----------------------------------------------------------------------------
// MCAThingSelection class destructor
// -----------------------------------------------------------------------------
slade::MCAThingSelection::~MCAThingSelection() = default;

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAThingSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAThingSelection::draw(gl::draw2d::Context& dc)
{
	// Colour
	if (select_)
	{
		buffer_->setColour({ 1.0f, 1.0f, 1.0f, fade_ * 0.4f });
		gl::setBlend(gl::Blend::Additive);
	}
	else
	{
		const auto& cdef = colourconfig::colDef("map_selection");
		buffer_->setColour(cdef.colour.ampf(1.0f, 1.0f, 1.0f, map2d_thing_overlay_square ? fade_ * 0.5f : fade_));
		gl::setBlend(cdef.blendMode());
	}

	// Animate radius
	if (select_ && !map2d_thing_overlay_square)
		buffer_->setPointRadius(1.0f + 0.2f * fade_);

	// No texture if square overlay
	if (map2d_thing_overlay_square)
	{
		gl::Texture::bind(gl::Texture::whiteTexture());
		buffer_->setPointRadius(0.8f);
	}

	// Draw
	buffer_->draw(map2d_thing_overlay_square ? gl::PointSpriteType::Textured : ps_type_, dc.view);
}

float MCAThingSelection::scaledRadius(float radius, float view_scale)
{
	if (radius > 16.0f)
		radius = 16.0f;

	if (view_scale > 1.0)
		return radius / view_scale;
	else
		return radius;
}


// -----------------------------------------------------------------------------
//
// MCALineSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCALineSelection class constructor
// -----------------------------------------------------------------------------
MCALineSelection::MCALineSelection(long start, const vector<MapLine*>& lines, bool select) :
	MCAnimation(start),
	select_{ select }
{
	// Go through list of lines
	for (auto& line : lines)
	{
		if (!line)
			continue;

		// Add line
		lines_.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());

		// Calculate line direction tab
		auto mid = line->getPoint(MapObject::Point::Mid);
		auto tab = line->dirTabPoint();

		tabs_.emplace_back(mid.x, mid.y, tab.x, tab.y);
	}
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCALineSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCALineSelection::draw(gl::draw2d::Context& dc)
{
	// Set colour
	if (select_)
	{
		dc.colour.set(255, 255, 255, 255 * fade_);
		dc.blend = gl::Blend::Additive;
	}
	else
		dc.setColourFromConfig("map_selection", fade_);

	// Draw lines
	dc.drawLines(lines_);
}


// -----------------------------------------------------------------------------
//
// MCAVertexSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAVertexSelection class constructor
// -----------------------------------------------------------------------------
MCAVertexSelection::MCAVertexSelection(long start, const vector<MapVertex*>& verts, float size, bool select) :
	MCAnimation(start),
	size_{ size },
	select_{ select }
{
	// Setup vertices list
	for (auto& vertex : verts)
	{
		if (!vertex)
			continue;
		vertices_.emplace_back(vertex->xPos(), vertex->yPos());
	}

	if (!select)
		size_ = size * 1.8f;
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAVertexSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAVertexSelection::draw(gl::draw2d::Context& dc)
{
	// Setup colour
	if (select_)
	{
		dc.colour.set(255, 255, 255, 255 * fade_);
		dc.blend = gl::Blend::Additive;
	}
	else
		dc.setColourFromConfig("map_selection", fade_);

	dc.texture            = MapRenderer2D::vertexTexture(!select_);
	dc.pointsprite_type   = gl::PointSpriteType::Textured;
	dc.pointsprite_radius = select_ ? size_ + (size_ * fade_) : size_;
	dc.drawPointSprites(vertices_);
}


// -----------------------------------------------------------------------------
//
// MCASectorSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCASectorSelection class constructor
// -----------------------------------------------------------------------------
MCASectorSelection::MCASectorSelection(long start, const vector<MapSector*>& sectors, bool select) :
	MCAnimation(start),
	select_{ select },
	blend_{ gl::Blend::Normal }
{
	// Build vertex buffer from sector polygon vertices
	vertex_buffer_ = std::make_unique<gl::VertexBuffer2D>();
	constexpr glm::vec4 white{ 1.0f };
	for (auto& sector : sectors)
		for (const auto& vertex : sector->polygonVertices())
			vertex_buffer_->add(vertex, white, glm::vec2{ 0.0f });

	// Set colour
	if (select_)
	{
		colour_ = glm::vec4{ 1.0f, 1.0f, 1.0f, 0.1f };
		blend_  = gl::Blend::Additive;
	}
	else
	{
		const auto& cdef = colourconfig::colDef("map_selection");
		colour_          = cdef.colour.ampf(1.0f, 1.0f, 1.0f, 0.2f);
		blend_           = cdef.blendMode();
	}
}

// -----------------------------------------------------------------------------
// MCASectorSelection class destructor
// -----------------------------------------------------------------------------
MCASectorSelection::~MCASectorSelection() = default;

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCASectorSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - (time - starttime_) * 0.004f;

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCASectorSelection::draw(gl::draw2d::Context& dc)
{
	// Don't draw if no fill
	if (!map2d_sector_selected_fill)
		return;

	// Setup shader
	const auto& shader = gl::draw2d::defaultShader(false);
	dc.view->setupShader(shader);
	gl::setBlend(blend_);
	auto colour = colour_;
	colour.a *= fade_;
	shader.setUniform("colour", colour);

	// Draw
	if (vertex_buffer_->buffer().empty())
		vertex_buffer_->push();
	vertex_buffer_->draw(gl::Primitive::Triangles);
}


// -----------------------------------------------------------------------------
//
// MCA3dWallSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCA3dWallSelection class constructor
// -----------------------------------------------------------------------------
MCA3dWallSelection::MCA3dWallSelection(long start, Vec3f points[4], bool select) :
	MCAnimation(start, true),
	points_{ points[0], points[1], points[2], points[3] },
	select_{ select }
{
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCA3dWallSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCA3dWallSelection::draw(MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view)
{
	//// Setup colour
	// if (select_)
	//	gl::setColour(255, 255, 255, 90 * fade_, gl::Blend::Additive);
	// else
	//	colourconfig::setGLColour("map_3d_selection", fade_);

	//// Draw quad outline
	// glLineWidth(2.0f);
	// glEnable(GL_LINE_SMOOTH);
	// glBegin(GL_LINE_LOOP);
	// for (auto& point : points_)
	//	glVertex3d(point.x, point.y, point.z);
	// glEnd();

	//// Draw quad fill
	// colourconfig::setGLColour("map_3d_selection", fade_ * 0.5f);
	// glBegin(GL_QUADS);
	// for (auto& point : points_)
	//	glVertex3d(point.x, point.y, point.z);
	// glEnd();
}


// -----------------------------------------------------------------------------
//
// MCA3dFlatSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCA3dFlatSelection class constructor
// -----------------------------------------------------------------------------
MCA3dFlatSelection::MCA3dFlatSelection(long start, MapSector* sector, const Plane& plane, bool select) :
	MCAnimation(start, true),
	sector_{ sector },
	plane_{ plane },
	select_{ select }
{
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCA3dFlatSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCA3dFlatSelection::draw(MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view)
{
	// if (!sector_)
	//	return;

	//// Setup colour
	// if (select_)
	//	gl::setColour(255, 255, 255, 60 * fade_, gl::Blend::Additive);
	// else
	//	colourconfig::setGLColour("map_3d_selection", fade_);
	// glDisable(GL_CULL_FACE);

	////// Set polygon to plane height
	////sector_->polygon()->setZ(plane_);

	////// Render flat
	////sector_->polygon()->render();

	////// Reset polygon height
	////sector_->polygon()->setZ(0);

	// glEnable(GL_CULL_FACE);
}





MCA3dSelection::MCA3dSelection(const vector<Item>& items, MapRenderer3D& renderer, bool select) :
	MCAnimation{ app::runTimer(), true },
	select_{ select }
{
	renderer.populateSelectionOverlay(overlay_, items);
	if (select)
	{
		additive_ = true;
		colour_   = glm::vec4{ 1.0f, 1.0f, 1.0f, 0.2f };
	}
	else
	{
		const auto& def = colourconfig::colDef("map_selection");
		additive_       = def.blendMode() == gl::Blend::Additive;
		colour_         = def.colour;
	}
}

bool MCA3dSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.008f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

void MCA3dSelection::draw(MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view)
{
	auto colour = colour_;
	colour.a *= fade_;
	renderer.renderSelectionOverlay(camera, view, overlay_, colour);
}






// -----------------------------------------------------------------------------
//
// MCAHilightFade Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAHilightFade class constructor
// -----------------------------------------------------------------------------
MCAHilightFade::MCAHilightFade(long start, MapObject* object, MapRenderer2D* renderer, float fade_init) :
	MCAnimation(start),
	object_{ object },
	fade_{ fade_init },
	init_fade_{ fade_init },
	renderer_{ renderer }
{
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAHilightFade::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = init_fade_ - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAHilightFade::draw(gl::draw2d::Context& dc)
{
	switch (object_->objType())
	{
	case MapObject::Type::Line:   renderer_->renderLineHilight(dc, object_->index(), fade_); break;
	case MapObject::Type::Sector: renderer_->renderFlatHilight(dc, object_->index(), fade_); break;
	case MapObject::Type::Thing:  renderer_->renderThingHilight(dc, object_->index(), fade_, false); break;
	case MapObject::Type::Vertex: renderer_->renderVertexHilight(dc, object_->index(), fade_); break;
	default:                      break;
	}
}


// -----------------------------------------------------------------------------
//
// MCAHilightFade3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAHilightFade3D class constructor
// -----------------------------------------------------------------------------
MCAHilightFade3D::MCAHilightFade3D(long start, const Item& item, float fade_init) :
	MCAnimation(start, true),
	item_{ item },
	fade_{ fade_init },
	init_fade_{ fade_init }
{
}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAHilightFade3D::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = init_fade_ - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	return !(fade_ < 0.0f || fade_ > 1.0f);
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAHilightFade3D::draw(MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view)
{
	renderer.renderHighlight(item_, camera, view, fade_);
}
