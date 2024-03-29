
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    View.cpp
// Description: View class - handles a 2d opengl 'view' that can be scrolled and
//              zoomed, with conversion between screen <-> canvas coordinates
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
#include "View.h"
#include "Geometry/BBox.h"
#include "OpenGL/OpenGL.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// View Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Resets the interpolated view values to their non-interpolated counterparts
// -----------------------------------------------------------------------------
void View::resetInter(bool x, bool y, bool scale)
{
	if (x)
		offset_inter_.x = offset_.x;

	if (y)
		offset_inter_.y = offset_.y;

	if (scale)
		scale_inter_ = scale_;
}

// -----------------------------------------------------------------------------
// Pans the view by [x,y]
// -----------------------------------------------------------------------------
void View::pan(double x, double y)
{
	offset_.x += x;
	offset_.y += y;

	updateVisibleRegion();
}

// -----------------------------------------------------------------------------
// Zooms the view by [amount] towards the center of the view
// -----------------------------------------------------------------------------
void View::zoom(double amount)
{
	// Zoom view
	scale_ *= amount;

	// Check for zoom limits
	if (scale_ < min_scale_)
		scale_ = min_scale_;
	if (scale_ > max_scale_)
		scale_ = max_scale_;

	// Update screen limits
	updateVisibleRegion();
}

// -----------------------------------------------------------------------------
// Zooms the view by [amount] towards [point] (in screen coords)
// -----------------------------------------------------------------------------
void View::zoomToward(double amount, const Vec2i& point)
{
	// Get current [point] in canvas coords before zooming
	auto orig_point = canvasPos(point);

	// Zoom view
	scale_ *= amount;

	// Check for zoom limits
	if (scale_ < min_scale_)
		scale_ = min_scale_;
	if (scale_ > max_scale_)
		scale_ = max_scale_;

	// Zoom towards [point]
	offset_.x += orig_point.x - canvasX(point.x);
	offset_.y += orig_point.y - canvasY(point.y);

	// Update screen limits
	updateVisibleRegion();
}

// -----------------------------------------------------------------------------
// Zooms and offsets the view such that [bbox] fits within the current view size
// -----------------------------------------------------------------------------
void View::fitTo(const BBox& bbox)
{
	// Reset zoom and set offsets to the middle of the canvas
	scale_    = 2;
	offset_.x = bbox.min.x + ((bbox.max.x - bbox.min.x) * 0.5);
	offset_.y = bbox.min.y + ((bbox.max.y - bbox.min.y) * 0.5);

	// Now just keep zooming out until we fit the whole canvas in the view
	bool done = false;
	while (!done)
	{
		// Update screen limits
		updateVisibleRegion();

		if (bbox.min.x >= visibleRegion().tl.x && bbox.max.x <= visibleRegion().br.x
			&& bbox.min.y >= visibleRegion().tl.y && bbox.max.y <= visibleRegion().br.y)
			done = true;
		else
			scale_ *= 0.8;
	}
}

// -----------------------------------------------------------------------------
// Updates the interpolated view values based on [mult].
// If [towards] is not nullptr, the scale interpolation will also interpolate
// offsets towards [towards]
// -----------------------------------------------------------------------------
bool View::interpolate(double mult, const Vec2d* towards)
{
	bool interpolating = false;

	// Scale
	double diff_scale = scale_ - scale_inter_;
	if (diff_scale < -0.0000001 || diff_scale > 0.0000001)
	{
		// Get current mouse position in canvas coordinates (for zdooming towards [towards])
		double mx{}, my{};
		if (towards)
		{
			mx = canvasX(towards->x, true);
			my = canvasY(towards->y, true);
		}

		// Interpolate zoom
		scale_inter_ += diff_scale * mult;

		// Check for zoom finish
		if ((diff_scale < 0 && scale_inter_ < scale_) || (diff_scale > 0 && scale_inter_ > scale_))
			scale_inter_ = scale_;
		else
			interpolating = true;

		if (towards)
		{
			setOffset(
				offset_inter_.x + mx - canvasX(towards->x, true), offset_inter_.y + my - canvasY(towards->y, true));
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
		offset_inter_.x += diff_xoff * mult;

		// Check stuff
		if ((diff_xoff < 0 && offset_inter_.x < offset_.x) || (diff_xoff > 0 && offset_inter_.x > offset_.x))
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
		offset_inter_.y += diff_yoff * mult;

		if ((diff_yoff < 0 && offset_inter_.y < offset_.y) || (diff_yoff > 0 && offset_inter_.y > offset_.y))
			offset_inter_.y = offset_.y;
		else
			interpolating = true;
	}
	else
		offset_inter_.y = offset_.y;

	return interpolating;
}

// -----------------------------------------------------------------------------
// Translates an x position on the screen to the corresponding x position on the
// canvas itself
// -----------------------------------------------------------------------------
double View::canvasX(int screen_x, bool inter) const
{
	return inter ? screen_x / scale_inter_ + offset_inter_.x - (size_.x * 0.5 / scale_inter_) :
				   screen_x / scale_ + offset_.x - (size_.x * 0.5 / scale_);
}

// -----------------------------------------------------------------------------
// Translates a y position on the screen to the corresponding y position on the
// canvas itself
// -----------------------------------------------------------------------------
double View::canvasY(int screen_y, bool inter) const
{
	if (y_flipped_)
	{
		return inter ? -screen_y / scale_inter_ + offset_inter_.y + (size_.y * 0.5 / scale_inter_) :
					   -screen_y / scale_ + offset_.y + (size_.y * 0.5 / scale_);
	}
	else
	{
		return inter ? -screen_y / scale_inter_ + offset_inter_.y - (size_.y * 0.5 / scale_inter_) :
					   -screen_y / scale_ + offset_.y - (size_.y * 0.5 / scale_);
	}
}

// -----------------------------------------------------------------------------
// Translates a position on the screen to the corresponding position on the
// canvas itself
// -----------------------------------------------------------------------------
Vec2d View::canvasPos(const Vec2i& screen_pos, bool inter) const
{
	return { canvasX(screen_pos.x, inter), canvasY(screen_pos.y, inter) };
}

// -----------------------------------------------------------------------------
// Translates [x] from canvas coordinates to screen coordinates
// -----------------------------------------------------------------------------
int View::screenX(double canvas_x) const
{
	return math::round((size_.x * 0.5) + ((canvas_x - offset_inter_.x) * scale_inter_));
}

// -----------------------------------------------------------------------------
// Translates [y] from canvas coordinates to screen coordinates
// -----------------------------------------------------------------------------
int View::screenY(double canvas_y) const
{
	return y_flipped_ ? math::round((size_.y * 0.5) - ((canvas_y - offset_inter_.y) * scale_inter_)) :
						math::round((size_.y * 0.5) + ((canvas_y - offset_inter_.y) * scale_inter_));
}

// -----------------------------------------------------------------------------
// Applies the current (interpolated) view in OpenGL.
// If [init] is true, will also initialize the projection+view matrices
// -----------------------------------------------------------------------------
void View::apply(bool init) const
{
	if (init)
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
		if (gl::accuracyTweak())
			glTranslatef(0.375f, 0.375f, 0);
	}

	// Translate to middle of screen
	glTranslated(size_.x * 0.5, size_.y * 0.5, 0);

	// Zoom
	glScaled(scale_inter_, scale_inter_, 1);

	// Translate to offsets
	glTranslated(-offset_inter_.x, -offset_inter_.y, 0);
}

// -----------------------------------------------------------------------------
// Sets/unsets the projection for rendering overlays (and text, etc.)
// -----------------------------------------------------------------------------
void View::setOverlayCoords(bool set) const
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
		if (gl::accuracyTweak())
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

// -----------------------------------------------------------------------------
// Updates the canvas bounds member variable for the current view
// -----------------------------------------------------------------------------
void View::updateVisibleRegion()
{
	if (y_flipped_)
	{
		visible_region_.tl.x = canvasX(0);
		visible_region_.tl.y = canvasY(size_.y);
		visible_region_.br.x = canvasX(size_.x);
		visible_region_.br.y = canvasY(0);
	}
	else
	{
		visible_region_.tl.x = canvasX(0);
		visible_region_.tl.y = canvasY(0);
		visible_region_.br.x = canvasX(size_.x);
		visible_region_.br.y = canvasY(size_.y);
	}
}
