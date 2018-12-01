
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapVertex.h"
#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "OpenGL/OpenGL.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, thing_overlay_square)
EXTERN_CVAR(Int, thing_drawtype)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Float, line_width)
EXTERN_CVAR(Int, halo_width)
EXTERN_CVAR(Bool, sector_selected_fill)


// -----------------------------------------------------------------------------
//
// MCASelboxFader Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCASelboxFader class constructor
// -----------------------------------------------------------------------------
MCASelboxFader::MCASelboxFader(long start, Vec2f tl, Vec2f br) : MCAnimation(start)
{
	// Init variables
	this->tl_ = tl;
	this->br_ = br;
	fade_     = 1.0f;
}

// -----------------------------------------------------------------------------
// MCASelboxFader class destructor
// -----------------------------------------------------------------------------
MCASelboxFader::~MCASelboxFader() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCASelboxFader::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	if (fade_ < 0.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCASelboxFader::draw()
{
	glDisable(GL_TEXTURE_2D);

	ColRGBA col;

	// Outline
	col.set(ColourConfiguration::colour("map_selbox_outline"));
	col.a *= fade_;
	OpenGL::setColour(col);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2d(tl_.x, tl_.y);
	glVertex2d(tl_.x, br_.y);
	glVertex2d(br_.x, br_.y);
	glVertex2d(br_.x, tl_.y);
	glEnd();

	// Fill
	col.set(ColourConfiguration::colour("map_selbox_fill"));
	col.a *= fade_;
	OpenGL::setColour(col);
	glBegin(GL_QUADS);
	glVertex2d(tl_.x, tl_.y);
	glVertex2d(tl_.x, br_.y);
	glVertex2d(br_.x, br_.y);
	glVertex2d(br_.x, tl_.y);
	glEnd();
}


// -----------------------------------------------------------------------------
//
// MCAThingSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAThingSelection class constructor
// -----------------------------------------------------------------------------
MCAThingSelection::MCAThingSelection(long start, double x, double y, double radius, double scale_inv, bool select) :
	MCAnimation(start)
{
	// Init variables
	this->x_      = x;
	this->y_      = y;
	this->radius_ = radius;
	this->select_ = select;
	this->fade_   = 1.0f;

	// Adjust radius
	if (!thing_overlay_square)
		this->radius_ += 8;
	this->radius_ += halo_width * scale_inv;
}

// -----------------------------------------------------------------------------
// MCAThingSelection class destructor
// -----------------------------------------------------------------------------
MCAThingSelection::~MCAThingSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAThingSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAThingSelection::draw()
{
	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 255 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_selection");
		col.a *= fade_;
	}
	OpenGL::setColour(col);

	// Get texture if needed
	if (!thing_overlay_square)
	{
		// Get thing selection texture
		GLTexture* tex = nullptr;
		if (thing_drawtype == MapRenderer2D::ThingDrawType::Round
			|| thing_drawtype == MapRenderer2D::ThingDrawType::Sprite)
			tex = MapEditor::textureManager().editorImage("thing/hilight");
		else
			tex = MapEditor::textureManager().editorImage("thing/square/hilight");

		if (!tex)
			return;

		// Bind the texture
		glEnable(GL_TEXTURE_2D);
		tex->bind();
	}

	// Animate radius
	double r = radius_;
	if (select_)
		r += radius_ * 0.2 * fade_;

	// Draw
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2d(x_ - r, y_ - r);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2d(x_ - r, y_ + r);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2d(x_ + r, y_ + r);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2d(x_ + r, y_ - r);
	glEnd();
}


// -----------------------------------------------------------------------------
//
// MCALineSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCALineSelection class constructor
// -----------------------------------------------------------------------------
MCALineSelection::MCALineSelection(long start, vector<MapLine*>& lines, bool select) : MCAnimation(start)
{
	// Init variables
	this->select_ = select;
	this->fade_   = 1.0f;

	// Go through list of lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (!lines[a])
			continue;

		// Add line
		this->lines_.push_back(Rectf(lines[a]->x1(), lines[a]->y1(), lines[a]->x2(), lines[a]->y2()));

		// Calculate line direction tab
		Vec2f mid = lines[a]->getPoint(MapObject::Point::Mid);
		Vec2f tab = lines[a]->dirTabPoint();

		this->tabs_.push_back(Rectf(mid.x, mid.y, tab.x, tab.y));
	}
}

// -----------------------------------------------------------------------------
// MCALineSelection class destructor
// -----------------------------------------------------------------------------
MCALineSelection::~MCALineSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCALineSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCALineSelection::draw()
{
	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 255 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_selection");
		col.a *= fade_;
	}
	OpenGL::setColour(col);

	// Draw lines
	glLineWidth(line_width * ColourConfiguration::lineSelectionWidth());
	glBegin(GL_LINES);
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		glVertex2d(lines_[a].tl.x, lines_[a].tl.y);
		glVertex2d(lines_[a].br.x, lines_[a].br.y);
		glVertex2d(tabs_[a].tl.x, tabs_[a].tl.y);
		glVertex2d(tabs_[a].br.x, tabs_[a].br.y);
	}
	glEnd();
}


// -----------------------------------------------------------------------------
//
// MCAVertexSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCAVertexSelection class constructor
// -----------------------------------------------------------------------------
MCAVertexSelection::MCAVertexSelection(long start, vector<MapVertex*>& verts, double size, bool select) :
	MCAnimation(start)
{
	// Init variables
	this->size_   = size;
	this->select_ = select;
	this->fade_   = 1.0f;

	// Setup vertices list
	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (!verts[a])
			continue;
		vertices_.push_back(Vec2f(verts[a]->xPos(), verts[a]->yPos()));
	}

	if (!select)
		this->size_ = size * 1.8f;
}

// -----------------------------------------------------------------------------
// MCAVertexSelection class destructor
// -----------------------------------------------------------------------------
MCAVertexSelection::~MCAVertexSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAVertexSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAVertexSelection::draw()
{
	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 255 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_selection");
		col.a *= fade_;
	}
	OpenGL::setColour(col);

	// Setup point sprites if supported
	bool point = false;
	if (OpenGL::pointSpriteSupport())
	{
		// Get appropriate vertex texture
		GLTexture* tex;
		// if (vertex_round) tex = MapEditor::textureManager().getEditorImage("vertex_r");
		// else tex = MapEditor::textureManager().getEditorImage("vertex_s");

		if (select_)
		{
			if (vertex_round)
				tex = MapEditor::textureManager().editorImage("vertex/round");
			else
				tex = MapEditor::textureManager().editorImage("vertex/square");
		}
		else
		{
			if (vertex_round)
				tex = MapEditor::textureManager().editorImage("vertex/hilight_r");
			else
				tex = MapEditor::textureManager().editorImage("vertex/hilight_s");
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
		if (vertex_round)
			glEnable(GL_POINT_SMOOTH);
		else
			glDisable(GL_POINT_SMOOTH);
	}

	// Draw points
	if (select_)
		glPointSize(size_ + (size_ * fade_));
	else
		glPointSize(size_);
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < vertices_.size(); a++)
		glVertex2d(vertices_[a].x, vertices_[a].y);
	glEnd();

	if (point)
	{
		glDisable(GL_POINT_SPRITE);
		glDisable(GL_TEXTURE_2D);
	}
}


// -----------------------------------------------------------------------------
//
// MCASectorSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCASectorSelection class constructor
// -----------------------------------------------------------------------------
MCASectorSelection::MCASectorSelection(long start, vector<Polygon2D*>& polys, bool select) : MCAnimation(start)
{
	// Init variables
	this->select_ = select;
	this->fade_   = 1.0f;

	// Copy polygon list
	for (unsigned a = 0; a < polys.size(); a++)
		this->polygons_.push_back(polys[a]);
}

// -----------------------------------------------------------------------------
// MCASectorSelection class destructor
// -----------------------------------------------------------------------------
MCASectorSelection::~MCASectorSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCASectorSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCASectorSelection::draw()
{
	// Don't draw if no fill
	if (!sector_selected_fill)
		return;

	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 180 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_selection");
		col.a *= fade_ * 0.75;
	}
	OpenGL::setColour(col);

	// Draw polygons
	for (unsigned a = 0; a < polygons_.size(); a++)
		polygons_[a]->render();
}


// -----------------------------------------------------------------------------
//
// MCA3dWallSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCA3dWallSelection class constructor
// -----------------------------------------------------------------------------
MCA3dWallSelection::MCA3dWallSelection(long start, Vec3f points[4], bool select) : MCAnimation(start, true)
{
	// Init variables
	this->select_    = select;
	this->fade_      = 1.0f;
	this->points_[0] = points[0];
	this->points_[1] = points[1];
	this->points_[2] = points[2];
	this->points_[3] = points[3];
}

// -----------------------------------------------------------------------------
// MCA3dWallSelection class destructor
// -----------------------------------------------------------------------------
MCA3dWallSelection::~MCA3dWallSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCA3dWallSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCA3dWallSelection::draw()
{
	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 90 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_3d_selection");
		col.a *= fade_ * 0.75;
	}
	OpenGL::setColour(col);

	// Draw quad outline
	glLineWidth(2.0f);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_LOOP);
	for (unsigned a = 0; a < 4; a++)
		glVertex3d(points_[a].x, points_[a].y, points_[a].z);
	glEnd();

	// Draw quad fill
	col.a *= 0.5;
	OpenGL::setColour(col, false);
	glBegin(GL_QUADS);
	for (unsigned a = 0; a < 4; a++)
		glVertex3d(points_[a].x, points_[a].y, points_[a].z);
	glEnd();
}


// -----------------------------------------------------------------------------
//
// MCA3dFlatSelection Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MCA3dFlatSelection class constructor
// -----------------------------------------------------------------------------
MCA3dFlatSelection::MCA3dFlatSelection(long start, MapSector* sector, Plane plane, bool select) :
	MCAnimation(start, true)
{
	// Init variables
	this->sector_ = sector;
	this->plane_  = plane;
	this->select_ = select;
	this->fade_   = 1.0f;
}

// -----------------------------------------------------------------------------
// MCA3dFlatSelection class destructor
// -----------------------------------------------------------------------------
MCA3dFlatSelection::~MCA3dFlatSelection() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCA3dFlatSelection::update(long time)
{
	// Determine fade amount (0.0-1.0 over 150ms)
	fade_ = 1.0f - ((time - starttime_) * 0.004f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCA3dFlatSelection::draw()
{
	if (!sector_)
		return;

	// Setup colour
	ColRGBA col;
	if (select_)
		col.set(255, 255, 255, 60 * fade_, 1);
	else
	{
		col = ColourConfiguration::colour("map_3d_selection");
		col.a *= fade_ * 0.75 * 0.5;
	}
	OpenGL::setColour(col);
	glDisable(GL_CULL_FACE);

	// Set polygon to plane height
	sector_->polygon()->setZ(plane_);

	// Render flat
	sector_->polygon()->render();

	// Reset polygon height
	sector_->polygon()->setZ(0);

	glEnable(GL_CULL_FACE);
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
	MCAnimation(start)
{
	this->object_    = object;
	this->renderer_  = renderer;
	this->init_fade_ = fade_init;
	this->fade_      = fade_init;
}

// -----------------------------------------------------------------------------
// MCAHilightFade class destructor
// -----------------------------------------------------------------------------
MCAHilightFade::~MCAHilightFade() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAHilightFade::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = init_fade_ - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAHilightFade::draw()
{
	switch (object_->objType())
	{
	case MapObject::Type::Line: renderer_->renderLineHilight(object_->index(), fade_); break;
	case MapObject::Type::Sector: renderer_->renderFlatHilight(object_->index(), fade_); break;
	case MapObject::Type::Thing: renderer_->renderThingHilight(object_->index(), fade_); break;
	case MapObject::Type::Vertex: renderer_->renderVertexHilight(object_->index(), fade_); break;
	default: break;
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
MCAHilightFade3D::MCAHilightFade3D(
	long                start,
	int                 item_index,
	MapEditor::ItemType item_type,
	MapRenderer3D*      renderer,
	float               fade_init) :
	MCAnimation(start, true),
	item_index_{ item_index },
	item_type_{ item_type },
	fade_{ fade_init },
	init_fade_{ fade_init },
	renderer_{ renderer }
{
}

// -----------------------------------------------------------------------------
// MCAHilightFade3D class destructor
// -----------------------------------------------------------------------------
MCAHilightFade3D::~MCAHilightFade3D() {}

// -----------------------------------------------------------------------------
// Updates the animation based on [time] elapsed in ms
// -----------------------------------------------------------------------------
bool MCAHilightFade3D::update(long time)
{
	// Determine fade amount (1.0-0.0 over 150ms)
	fade_ = init_fade_ - ((time - starttime_) * 0.006f);

	// Check if animation is finished
	if (fade_ < 0.0f || fade_ > 1.0f)
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Draws the animation
// -----------------------------------------------------------------------------
void MCAHilightFade3D::draw()
{
	renderer_->renderHilight({ item_index_, item_type_ }, fade_);
}
