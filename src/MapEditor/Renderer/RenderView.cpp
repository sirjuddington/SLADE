
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    RenderView.cpp
 * Description: RenderView class - handles the view and screen <->
 *              map coordinate translation for the map editor
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
#include "OpenGL/OpenGL.h"
#include "RenderView.h"
#include "Utility/MathStuff.h"

using namespace MapEditor;


/*******************************************************************
 * RENDERVIEW CLASS FUNCTIONS
 *******************************************************************/

/* RenderView::RenderView
 * RenderView class constructor
 *******************************************************************/
RenderView::RenderView() :
	scale_{ 0 },
	scale_inter_{ 0 },
	min_scale_{ 0.005 },
	max_scale_{ 10.0 }
{
}

/* RenderView::resetInter
 * Resets the interpolated view values to their non-interpolated
 * counterparts
 *******************************************************************/
void RenderView::resetInter(bool x, bool y, bool scale)
{
	if (x)
		offset_inter_.x = offset_.x;

	if (y)
		offset_inter_.y = offset_.y;

	if (scale)
		scale_inter_ = scale_;
}

/* RenderView::zoom
 * Zooms the view by [amount] towards the center of the view
 *******************************************************************/
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

/* RenderView::zoomTowards
 * Zooms the view by [amount] towards [point]
 *******************************************************************/
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

/* RenderView::fitTo
 * Zooms and offsets the view such that [bbox] fits within the
 * current view size
 *******************************************************************/
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

/* RenderView::interpolate
 * Updates the interpolated view values based on [mult]. If [towards]
 * is not nullptr, the scale interpolation will also interpolate
 * offsets towards [towards]
 *******************************************************************/
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

/* RenderView::apply
 * Applies the current (interpolated) view in OpenGL
 *******************************************************************/
void RenderView::apply() const
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

/* RenderView::setOverlayCoords
 * Sets/unsets the projection for rendering overlays (and text, etc.)
 *******************************************************************/
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

/* RenderView::updateMapBounds
 * Updates the map bounds member variable for the current view
 *******************************************************************/
void RenderView::updateMapBounds()
{
	map_bounds_.tl.x = mapX(0);
	map_bounds_.tl.y = mapY(size_.y);
	map_bounds_.br.x = mapX(size_.x);
	map_bounds_.br.y = mapY(0);
}
