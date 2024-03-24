
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingList.cpp
// Description: A (non-owning) list of map things. Includes std::vector-like API
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
#include "ThingList.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/Game.h"
#include "Game/ThingType.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ThingList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the thing closest to the point, or null if none found.
// Igonres any thing further away than [min]
// -----------------------------------------------------------------------------
MapThing* ThingList::nearest(const Vec2d& point, double min) const
{
	// Go through things
	double    dist;
	double    min_dist = 999999999;
	MapThing* nearest  = nullptr;
	for (const auto& thing : objects_)
	{
		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicabDistanceTo(thing->position());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			nearest  = thing;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest thing,
	// to check for minimum hilight distance
	if (nearest)
	{
		double rdist = math::distance(nearest->position(), point);
		if (rdist > min)
			return nullptr;
	}

	return nearest;
}

// -----------------------------------------------------------------------------
// Same as 'nearest', but returns a list of things for the case where there are
// multiple things at the same point
// -----------------------------------------------------------------------------
vector<MapThing*> ThingList::multiNearest(const Vec2d& point) const
{
	vector<MapThing*> ret;

	// Go through things
	double min_dist = 999999999;
	double dist     = 0;
	for (const auto& thing : objects_)
	{
		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicabDistanceTo(thing->position());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			ret.clear();
			ret.push_back(thing);
			min_dist = dist;
		}
		else if (dist == min_dist)
			ret.push_back(thing);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a bounding box for all things's positions
// -----------------------------------------------------------------------------
BBox ThingList::allThingBounds() const
{
	if (count_ == 0)
		return {};

	BBox bbox;
	for (const auto& thing : objects_)
		bbox.extend(thing->position());

	return bbox;
}

// -----------------------------------------------------------------------------
// Adds all things with TID [id] to [list].
// If [type] is not 0, only checks things of that type
// -----------------------------------------------------------------------------
void ThingList::putAllWithId(int id, vector<MapThing*>& list, unsigned start, int type) const
{
	for (unsigned i = start; i < count_; ++i)
		if (objects_[i]->id() == id && (type == 0 || objects_[i]->type() == type))
			list.push_back(objects_[i]);
}

// -----------------------------------------------------------------------------
// Returns a list of all things with TID [id].
// If [type] is not 0, only checks things of that type
// -----------------------------------------------------------------------------
vector<MapThing*> ThingList::allWithId(int id, unsigned start, int type) const
{
	vector<MapThing*> list;
	putAllWithId(id, list, start, type);
	return list;
}

// -----------------------------------------------------------------------------
// Returns the first thing found with TID [id], or null if none were found.
// If [type] is not 0, only checks things of that type
// -----------------------------------------------------------------------------
MapThing* ThingList::firstWithId(int id, unsigned start, int type, bool ignore_dragon) const
{
	for (unsigned i = start; i < count_; ++i)
		if (objects_[i]->id() == id && (type == 0 || objects_[i]->type() == type))
		{
			if (ignore_dragon)
			{
				auto& tt = game::configuration().thingType(objects_[i]->type());
				if (tt.flags() & game::ThingType::Flags::Dragon)
					continue;
			}

			return objects_[i];
		}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all things with a 'pathed' type to [list]
// -----------------------------------------------------------------------------
void ThingList::putAllPathed(vector<MapThing*>& list) const
{
	// Find things that need to be pathed
	for (const auto& thing : objects_)
	{
		auto& tt = game::configuration().thingType(thing->type());
		if (tt.flags() & (game::ThingType::Flags::Pathed | game::ThingType::Flags::Dragon))
			list.push_back(thing);
	}
}

// -----------------------------------------------------------------------------
// Adds all things with special affecting matching id to [list]
// -----------------------------------------------------------------------------
#define IDEQ(x) (((x) != 0) && ((x) == id))
void ThingList::putAllTaggingWithId(int id, int type, vector<MapThing*>& list, int ttype) const
{
	using game::TagType;

	// Find things with special affecting matching id
	int tag, arg2, arg3, arg4, arg5, tid;
	for (auto& thing : objects_)
	{
		auto& tt        = game::configuration().thingType(thing->type());
		auto  needs_tag = tt.needsTag();
		if (needs_tag != TagType::None || (thing->special() && !(tt.flags() & game::ThingType::Flags::Script)))
		{
			if (needs_tag == TagType::None)
				needs_tag = game::configuration().actionSpecial(thing->special()).needsTag();
			tag       = thing->arg(0);
			bool fits = false;
			int  path_type;
			switch (needs_tag)
			{
			case TagType::Sector:
			case TagType::SectorOrBack:
			case TagType::SectorAndBack: fits = (IDEQ(tag) && type == SLADEMap::SECTORS); break;
			case TagType::LineNegative:  tag = abs(tag);
			case TagType::Line:          fits = (IDEQ(tag) && type == SLADEMap::LINEDEFS); break;
			case TagType::Thing:         fits = (IDEQ(tag) && type == SLADEMap::THINGS); break;
			case TagType::Thing1Sector2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SLADEMap::SECTORS));
				break;
			case TagType::Thing1Sector3:
				arg3 = thing->arg(2);
				fits = (type == SLADEMap::THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SLADEMap::SECTORS));
				break;
			case TagType::Thing1Thing2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Thing1Thing4:
				arg4 = thing->arg(3);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case TagType::Thing1Thing2Thing3:
				arg2 = thing->arg(1);
				arg3 = thing->arg(2);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case TagType::Sector1Thing2Thing3Thing5:
				arg2 = thing->arg(1);
				arg3 = thing->arg(2);
				arg5 = thing->arg(4);
				fits =
					(type == SLADEMap::SECTORS ?
						 (IDEQ(tag)) :
						 (type == SLADEMap::THINGS && (IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case TagType::LineId1Line2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::LINEDEFS && IDEQ(arg2));
				break;
			case TagType::Thing4:
				arg4 = thing->arg(3);
				fits = (type == SLADEMap::THINGS && IDEQ(arg4));
				break;
			case TagType::Thing5:
				arg5 = thing->arg(4);
				fits = (type == SLADEMap::THINGS && IDEQ(arg5));
				break;
			case TagType::Line1Sector2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SLADEMap::SECTORS));
				break;
			case TagType::Sector1Sector2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Sector1Sector2Sector3Sector4:
				arg2 = thing->arg(1);
				arg3 = thing->arg(2);
				arg4 = thing->arg(3);
				fits = (type == SLADEMap::SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case TagType::Sector2Is3Line:
				arg2 = thing->arg(1);
				fits = (IDEQ(tag) && (arg2 == 3 ? type == SLADEMap::LINEDEFS : type == SLADEMap::SECTORS));
				break;
			case TagType::Sector1Thing2:
				arg2 = thing->arg(1);
				fits = (type == SLADEMap::SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SLADEMap::THINGS));
				break;
			case TagType::Patrol: path_type = 9047;
			case TagType::Interpolation:
			{
				path_type = 9075;

				tid  = thing->id();
				fits = ((path_type == ttype) && (IDEQ(tid)) && (tt.needsTag() == needs_tag));
			}
			break;
			default: break;
			}
			if (fits)
				list.push_back(thing);
		}
	}
}
#undef IDEQ

// -----------------------------------------------------------------------------
// Returns the lowest unused thing id
// -----------------------------------------------------------------------------
int ThingList::firstFreeId() const
{
	int id = 1;
	for (unsigned i = 0; i < count_; ++i)
	{
		if (objects_[i]->id() == id)
		{
			id++;
			i = 0;
		}
	}

	return id;
}
