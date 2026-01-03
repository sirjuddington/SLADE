// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Plane.cpp
// Description: A struct representing a 3d geometrical plane
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
#include "Plane.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Plane Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Plane default constructor
// -----------------------------------------------------------------------------
Plane::Plane() : a(0.0), b(0.0), c(0.0), d(0.0) {}

// -----------------------------------------------------------------------------
// Plane constructor
// -----------------------------------------------------------------------------
Plane::Plane(double a, double b, double c, double d) : a(a), b(b), c(c), d(d) {}

// -----------------------------------------------------------------------------
// Sets the plane's values
// -----------------------------------------------------------------------------
void Plane::set(double a, double b, double c, double d)
{
	this->a = a;
	this->b = b;
	this->c = c;
	this->d = d;
}

// -----------------------------------------------------------------------------
// Returns the plane's normal vector
// -----------------------------------------------------------------------------
Vec3d Plane::normal() const
{
	return glm::normalize(Vec3d{ a, b, c });
}

// -----------------------------------------------------------------------------
// Normalizes the plane's values
// -----------------------------------------------------------------------------
void Plane::normalize()
{
	Vec3d  vec(a, b, c);
	double mag = glm::length(vec);
	a          = a / mag;
	b          = b / mag;
	c          = c / mag;
	d          = d / mag;
}

// -----------------------------------------------------------------------------
// Returns the height (z value) on the plane at [point]
// -----------------------------------------------------------------------------
double Plane::heightAt(const Vec2d& point) const
{
	return heightAt(point.x, point.y);
}

// -----------------------------------------------------------------------------
// Returns the height (z value) on the plane at [x],[y]
// -----------------------------------------------------------------------------
double Plane::heightAt(double x, double y) const
{
	return ((-a * x) + (-b * y) + d) / c;
}

// -----------------------------------------------------------------------------
// Constructs a flat plane (perpendicular to the z axis) at the given height
// -----------------------------------------------------------------------------
Plane Plane::flat(float height)
{
	return { 0.0, 0.0, 1.0, height };
}
