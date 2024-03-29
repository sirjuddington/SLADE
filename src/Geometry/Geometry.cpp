
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Geometry.cpp
// Description: Contains various useful 2D/3D geometry related functions
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
#include "Geometry.h"

using namespace slade;
using namespace geometry;


// -----------------------------------------------------------------------------
//
// Geometry Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the taxicab aka "Manhattan" distance between [p1] and [p2].
// It's just the sum of the vertical and horizontal distance, for an upper bound
// on the true distance
// -----------------------------------------------------------------------------
double geometry::taxicabDistance(const Point2d& p1, const Point2d& p2)
{
	double dist;

	if (p2.x < p1.x)
		dist = p1.x - p2.x;
	else
		dist = p2.x - p1.x;
	if (p2.y < p1.y)
		dist += p1.y - p2.y;
	else
		dist += p2.y - p1.y;

	return dist;
}
