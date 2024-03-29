// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BBox.cpp
// Description: A simple bounding box with related functions
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
#include "BBox.h"
#include "Rect.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// BBox Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// BBox Struct Constructor
// -----------------------------------------------------------------------------
BBox::BBox() : min{ 0, 0 }, max{ 0, 0 } {}

// -----------------------------------------------------------------------------
// Resets the bounding box to 0,0
// -----------------------------------------------------------------------------
void BBox::reset()
{
	min = { 0, 0 };
	max = { 0, 0 };
}

// -----------------------------------------------------------------------------
// Extends the bounding box so that it fits the point [x],[y] within
// -----------------------------------------------------------------------------
void BBox::extend(double x, double y)
{
	// Init bbox if it has been reset last
	if (min.x == 0 && min.y == 0 && max.x == 0 && max.y == 0)
	{
		min = { x, y };
		max = { x, y };
		return;
	}

	// Extend to fit the point [x,y]
	if (x < min.x)
		min.x = x;
	if (x > max.x)
		max.x = x;
	if (y < min.y)
		min.y = y;
	if (y > max.y)
		max.y = y;
}

// -----------------------------------------------------------------------------
// Extends the bounding box so that it fits [point] within
// -----------------------------------------------------------------------------
void BBox::extend(const Vec2d& point)
{
	extend(point.x, point.y);
}

// -----------------------------------------------------------------------------
// Extends the bounding box so that it fits another BBox within
// -----------------------------------------------------------------------------
void BBox::extend(const BBox& other)
{
	if (other.min.x < min.x)
		min.x = other.min.x;
	if (other.min.y < min.y)
		min.y = other.min.y;

	if (other.max.x > max.x)
		max.x = other.max.x;
	if (other.max.y > max.y)
		max.y = other.max.y;
}

// -----------------------------------------------------------------------------
// Returns true if [x],[y] is within the bounding box
// -----------------------------------------------------------------------------
bool BBox::pointWithin(double x, double y) const
{
	return (x >= min.x && x <= max.x && y >= min.y && y <= max.y);
}

// -----------------------------------------------------------------------------
// Returns true if [point] is within the bounding box
// -----------------------------------------------------------------------------
bool BBox::contains(const Vec2d& point) const
{
	return pointWithin(point.x, point.y);
}

// -----------------------------------------------------------------------------
// Returns true if this bounding box fits within [bmin]->[bmax]
// -----------------------------------------------------------------------------
bool BBox::isWithin(const Vec2d& bmin, const Vec2d& bmax) const
{
	return (min.x >= bmin.x && max.x <= bmax.x && min.y >= bmin.y && max.y <= bmax.y);
}

// -----------------------------------------------------------------------------
// Returns true if the bounding box is valid (min < max)
// -----------------------------------------------------------------------------
bool BBox::isValid() const
{
	return ((max.x - min.x > 0) && (max.y - min.y) > 0);
}

// -----------------------------------------------------------------------------
// Returns the size of the bounding box
// -----------------------------------------------------------------------------
Vec2d BBox::size() const
{
	return { max.x - min.x, max.y - min.y };
}

// -----------------------------------------------------------------------------
// Returns the width of the bounding box
// -----------------------------------------------------------------------------
double BBox::width() const
{
	return max.x - min.x;
}

// -----------------------------------------------------------------------------
// Returns the height of the bounding box
// -----------------------------------------------------------------------------
double BBox::height() const
{
	return max.y - min.y;
}

// -----------------------------------------------------------------------------
// Returns the mid point of the bounding box
// -----------------------------------------------------------------------------
Vec2d BBox::mid() const
{
	return { midX(), midY() };
}

// -----------------------------------------------------------------------------
// Returns the horizontal mid point of the bounding box
// -----------------------------------------------------------------------------
double BBox::midX() const
{
	return min.x + ((max.x - min.x) * 0.5);
}

// -----------------------------------------------------------------------------
// Returns the vertical mid point of the bounding box
// -----------------------------------------------------------------------------
double BBox::midY() const
{
	return min.y + ((max.y - min.y) * 0.5);
}

// -----------------------------------------------------------------------------
// Returns a line segment representing the left side of the bounding box
// -----------------------------------------------------------------------------
Seg2d BBox::leftSide() const
{
	return { min.x, min.y, min.x, max.y };
}

// -----------------------------------------------------------------------------
// Returns a line segment representing the right side of the bounding box
// -----------------------------------------------------------------------------
Seg2d BBox::rightSide() const
{
	return { max.x, min.y, max.x, max.y };
}

// -----------------------------------------------------------------------------
// Returns a line segment representing the bottom side of the bounding box
// -----------------------------------------------------------------------------
Seg2d BBox::bottomSide() const
{
	return { min.x, max.y, max.x, max.y };
}

// -----------------------------------------------------------------------------
// Returns a line segment representing the top side of the bounding box
// -----------------------------------------------------------------------------
Seg2d BBox::topSide() const
{
	return { min.x, min.y, max.x, min.y };
}
