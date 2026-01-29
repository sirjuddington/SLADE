
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SlopeSpecials_Things.cpp.cpp
// Description:
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
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/SLADEMap.h"
#include "SlopeSpecials.h"
#include "Utility/MathStuff.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace map;


void SlopeSpecials::addLineSlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	LineSlopeThing lst{ surface_type };
	lst.thing = &thing;

	// Check line id
	const auto line_id = thing.arg(0);
	if (!line_id)
	{
		log::warning("Ignoring line slope thing {} with no lineid argument", thing.index());
		return;
	}

	// Get containing sector
	lst.containing_sector = map_->sectors().atPos(thing.position());
	if (!lst.containing_sector)
	{
		log::warning("Line slope thing {} is not within a sector", thing.index());
		return;
	}

	// Add LineSlopeThing for each tagged line
	for (const auto line : map_->lines().allWithId(line_id))
	{
		lst.line = line;

		// Line slope things only affect the sector on the side of the line that
		// faces the thing
		const double side = geometry::lineSide(thing.position(), line->seg());
		if (side < 0)
			lst.target = line->backSector();
		else if (side > 0)
			lst.target = line->frontSector();
		if (!lst.target)
			continue;

		line_slope_things_.push_back(lst);
		specials_updated_ = true;
	}

	sorted_slope_things_.clear();
}

void SlopeSpecials::applyLineSlopeThing(const LineSlopeThing& special)
{
	bool        floor        = special.surface_type == SectorSurfaceType::Floor;
	const auto& target_plane = floor ? special.target->plane<SectorSurfaceType::Floor>()
									 : special.target->plane<SectorSurfaceType::Ceiling>();

	// Need to know the containing sector's height to find the thing's true height
	auto thingz = floor
					  ? special.containing_sector->plane<SectorSurfaceType::Floor>().heightAt(special.thing->position())
					  : special.containing_sector->plane<SectorSurfaceType::Ceiling>().heightAt(
							special.thing->position());
	thingz += special.thing->zPos();

	// Three points: endpoints of the line, and the thing itself
	Vec3d p1(special.line->x1(), special.line->y1(), target_plane.heightAt(special.line->start()));
	Vec3d p2(special.line->x2(), special.line->y2(), target_plane.heightAt(special.line->end()));
	Vec3d p3(special.thing->xPos(), special.thing->yPos(), thingz);

	if (floor)
		special.target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		special.target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}

void SlopeSpecials::addSectorTiltThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto stt  = SectorTiltThing{ surface_type };
	stt.thing = &thing;

	// Get containing (target) sector
	stt.target = map_->sectors().atPos(thing.position());
	if (!stt.target)
	{
		log::warning("Sector Tilt slope thing {} is not within a sector", thing.index());
		return;
	}

	sector_tilt_things_.push_back(stt);
	sorted_slope_things_.clear();
	specials_updated_ = true;
}

void SlopeSpecials::applySectorTiltThing(const SectorTiltThing& special)
{
	constexpr double TAU = math::PI * 2; // Number of radians in the unit circle

	bool floor = special.surface_type == SectorSurfaceType::Floor;

	// First argument is the tilt angle, but starting with 0 as straight down;
	// subtracting 90 fixes that.
	int    raw_angle = special.thing->arg(0);
	double angle     = special.thing->angle() / 360.0 * TAU;
	double tilt      = (raw_angle - 90) / 360.0 * TAU;

	// Resulting plane goes through the position of the thing
	double z = (floor ? special.target->floor().height : special.target->ceiling().height) + special.thing->zPos();
	Vec3d  point(special.thing->xPos(), special.thing->yPos(), z);

	double cos_angle = cos(angle);
	double sin_angle = sin(angle);
	double cos_tilt  = cos(tilt);
	double sin_tilt  = sin(tilt);
	// Need to convert these angles into vectors on the plane, so we can take a
	// normal.
	// For the first: we know that the line perpendicular to the direction the
	// thing faces lies "flat", because this is the axis the tilt thing rotates
	// around.  "Rotate" the angle a quarter turn to get this vector -- switch
	// x and y, and negate one.
	Vec3d vec1(-sin_angle, cos_angle, 0.0);

	// For the second: the tilt angle makes a triangle between the floor plane
	// and the z axis.  sin gives us the distance along the z-axis, but cos
	// only gives us the distance away /from/ the z-axis.  Break that into x
	// and y by multiplying by cos and sin of the thing's facing angle.
	Vec3d vec2(cos_tilt * cos_angle, cos_tilt * sin_angle, sin_tilt);

	if (floor)
		special.target->setFloorPlane(geometry::planeFromTriangle(point, point + vec1, point + vec2));
	else
		special.target->setCeilingPlane(geometry::planeFromTriangle(point, point + vec1, point + vec2));
}

void SlopeSpecials::addVavoomSlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto vst  = VavoomSlopeThing{ surface_type };
	vst.thing = &thing;

	// Get containing (target) sector
	vst.target = map_->sectors().atPos(thing.position());
	if (!vst.target)
	{
		log::warning("Vavoom slope thing {} is not within a sector", thing.index());
		return;
	}

	// Find line in containing sector with first arg matching thing id
	const int        tid = thing.id();
	vector<MapLine*> lines;
	vst.target->putLines(lines);

	// TODO: unclear if this is the same order that ZDoom would go through the
	//       lines, which matters if two lines have the same first arg
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (tid != lines[a]->arg(0))
			continue;

		// Check thing position is valid
		if (geometry::distanceToLineFast(thing.position(), lines[a]->seg()) == 0)
		{
			log::warning("Vavoom slope thing {} lies directly on its target line {}", thing.index(), a);
			return;
		}

		vst.line = lines[a];
		vavoom_things_.push_back(vst);
		sorted_slope_things_.clear();
		specials_updated_ = true;

		return;
	}

	log::warning("Vavoom slope thing {} has no matching line with first arg {}", thing.index(), tid);
}

void SlopeSpecials::applyVavoomSlopeThing(const VavoomSlopeThing& special)
{
	bool floor = special.surface_type == SectorSurfaceType::Floor;

	// Vavoom things use the plane defined by the thing and its two
	// endpoints, based on the sector's original (flat) plane and treating
	// the thing's height as absolute
	short height = floor ? special.target->floor().height : special.target->ceiling().height;
	Vec3d p1(special.thing->xPos(), special.thing->yPos(), special.thing->zPos());
	Vec3d p2(special.line->x1(), special.line->y1(), height);
	Vec3d p3(special.line->x2(), special.line->y2(), height);

	if (floor)
		special.target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		special.target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}

void SlopeSpecials::removeSlopeThing(const MapThing& thing)
{
	// Remove from combined slope things list
	unsigned i = 0;
	while (i < sorted_slope_things_.size())
	{
		if (sorted_slope_things_[i]->thing == &thing)
		{
			vectorAddUnique(sectors_to_update_, sorted_slope_things_[i]->target);
			sorted_slope_things_.erase(sorted_slope_things_.begin() + i);
			specials_updated_ = true;
			continue;
		}
		i++;
	}

	// Remove from individual slope thing lists
	static auto remove_slope_thing = [this](const MapThing& thing, auto& vec)
	{
		unsigned i = 0;
		while (i < vec.size())
		{
			if (vec[i].thing == &thing)
			{
				vectorAddUnique(sectors_to_update_, vec[i].target);
				vec.erase(vec.begin() + i);
				specials_updated_ = true;
				continue;
			}
			i++;
		}
	};
	remove_slope_thing(thing, line_slope_things_);
	remove_slope_thing(thing, sector_tilt_things_);
	remove_slope_thing(thing, vavoom_things_);
}

void SlopeSpecials::applySlopeThingSpecials(const MapSector& sector)
{
	// Rebuild sorted list of slope thing specials if needed
	if (sorted_slope_things_.empty())
	{
		// Combine all slope thing specials into one list
		sorted_slope_things_.reserve(line_slope_things_.size() + sector_tilt_things_.size() + vavoom_things_.size());
		for (auto& lst : line_slope_things_)
			sorted_slope_things_.push_back(&lst);
		for (auto& stt : sector_tilt_things_)
			sorted_slope_things_.push_back(&stt);
		for (auto& vst : vavoom_things_)
			sorted_slope_things_.push_back(&vst);

		// Sort by thing index
		std::ranges::sort(
			sorted_slope_things_, [](const auto& a, const auto& b) { return a->thing->index() < b->thing->index(); });
	}

	// Apply each SlopeThingSpecial in order
	for (auto st : sorted_slope_things_)
	{
		if (st->target == &sector)
		{
			switch (st->type)
			{
			case SlopeSpecial::Type::LineThing:       applyLineSlopeThing(static_cast<LineSlopeThing&>(*st)); break;
			case SlopeSpecial::Type::SectorTiltThing: applySectorTiltThing(static_cast<SectorTiltThing&>(*st)); break;
			case SlopeSpecial::Type::VavoomThing:     applyVavoomSlopeThing(static_cast<VavoomSlopeThing&>(*st)); break;
			default:                                  break;
			}
		}
	}
}

void SlopeSpecials::addCopySlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto cst  = CopySlopeThing{ surface_type };
	cst.thing = &thing;

	// Get containing (target) sector
	cst.target = map_->sectors().atPos(thing.position());
	if (!cst.target)
	{
		log::warning("Copy slope thing {} is not within a sector", thing.index());
		return;
	}

	// First argument is the tag of a sector whose slope should be copied
	const int tag = thing.arg(0);
	if (!tag)
	{
		log::warning("Ignoring slope copy thing {} in sector {} with no argument", thing.index(), cst.target->index());
		return;
	}

	// Model sector to copy is the first with the tag
	cst.model = map_->sectors().firstWithId(tag);
	if (!cst.model)
	{
		log::warning(
			"Ignoring slope copy thing {} in sector {}; no sectors have target tag {}",
			thing.index(),
			cst.target->index(),
			tag);
		return;
	}

	copy_slope_things_.push_back(cst);
	copy_slope_things_sorted_ = false;
	specials_updated_         = true;
}

void SlopeSpecials::removeCopySlopeThing(const MapThing& thing)
{
	unsigned i = 0;
	while (i < copy_slope_things_.size())
	{
		if (copy_slope_things_[i].thing == &thing)
		{
			vectorAddUnique(sectors_to_update_, copy_slope_things_[i].target);
			copy_slope_things_.erase(copy_slope_things_.begin() + i);
			specials_updated_ = true;
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applyCopySlopeThing(const CopySlopeThing& special)
{
	// Copy the plane from the model sector
	if (special.surface_type == SectorSurfaceType::Floor)
		special.target->setFloorPlane(special.model->floor().plane);
	else
		special.target->setCeilingPlane(special.model->ceiling().plane);
}

void SlopeSpecials::applyCopySlopeThingSpecials(const MapSector& sector)
{
	// Sort by thing index if needed
	if (!copy_slope_things_sorted_)
	{
		std::sort(
			copy_slope_things_.begin(),
			copy_slope_things_.end(),
			[](const auto& a, const auto& b) { return a.thing->index() < b.thing->index(); });

		copy_slope_things_sorted_ = true;
	}

	// Apply each CopySlopeThing in order
	for (const auto& cst : copy_slope_things_)
	{
		if (cst.target == &sector)
			applyCopySlopeThing(cst);
	}
}
