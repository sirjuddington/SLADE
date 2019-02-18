
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SideList.cpp
// Description: A (non-owning) list of map sides. Includes std::vector-like API
//              for accessing items and some misc functions to get info about
//              the contained items.
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
#include "SideList.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// SideList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears the list (and texture usage)
// -----------------------------------------------------------------------------
void SideList::clear()
{
	usage_tex_.clear();
	MapObjectList::clear();
}

// -----------------------------------------------------------------------------
// Adds [side] to the list and updates texture usage
// -----------------------------------------------------------------------------
void SideList::add(MapSide* side)
{
	// Update texture counts
	usage_tex_[StrUtil::upper(side->tex_upper_)] += 1;
	usage_tex_[StrUtil::upper(side->tex_middle_)] += 1;
	usage_tex_[StrUtil::upper(side->tex_lower_)] += 1;

	MapObjectList::add(side);
}

// -----------------------------------------------------------------------------
// Removes [side] from the list and updates texture usage
// -----------------------------------------------------------------------------
void SideList::remove(unsigned index)
{
	if (index >= objects_.size())
		return;

	// Update texture counts
	usage_tex_[StrUtil::upper(objects_[index]->tex_upper_)] -= 1;
	usage_tex_[StrUtil::upper(objects_[index]->tex_middle_)] -= 1;
	usage_tex_[StrUtil::upper(objects_[index]->tex_lower_)] -= 1;

	MapObjectList::remove(index);
}

// -----------------------------------------------------------------------------
// Adjusts the usage count of [tex] by [adjust]
// -----------------------------------------------------------------------------
void SideList::updateTexUsage(std::string_view tex, int adjust) const
{
	usage_tex_[StrUtil::upper(tex)] += adjust;
}

// -----------------------------------------------------------------------------
// Returns the usage count of [tex]
// -----------------------------------------------------------------------------
int SideList::texUsageCount(std::string_view tex) const
{
	return usage_tex_[StrUtil::upper(tex)];
}
