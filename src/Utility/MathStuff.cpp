
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MathStuff.cpp
// Description: Contains various useful math related functions
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
#include "MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// MathStuff Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clamps [val] to be between [min] and [max]
// -----------------------------------------------------------------------------
double math::clamp(double val, double min, double max)
{
	if (val < min)
		val = min;
	if (val > max)
		val = max;
	return val;
}

// -----------------------------------------------------------------------------
// Returns the integral floor of [val]
// -----------------------------------------------------------------------------
int math::floor(double val)
{
	if (val >= 0)
		return static_cast<int>(val);
	else
		return static_cast<int>(val) - 1;
}

// -----------------------------------------------------------------------------
// Returns the integral ceiling of [val]
// -----------------------------------------------------------------------------
int math::ceil(double val)
{
	if (val > 0)
		return static_cast<int>(val) + 1;
	else
		return static_cast<int>(val);
}

// -----------------------------------------------------------------------------
// Returns the closest integral value of [val]
// -----------------------------------------------------------------------------
int math::round(double val)
{
	int ret = static_cast<int>(val);
	if ((val - static_cast<double>(ret)) >= 0.5)
		ret++;
	return ret;
}
