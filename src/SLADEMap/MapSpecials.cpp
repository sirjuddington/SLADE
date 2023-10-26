
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSpecials.cpp
// Description: Various functions for processing map specials and scripts,
//              mostly for visual effects (transparency, colours, slopes, etc.)
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
#include "MapSpecials.h"
#include "Game/Configuration.h"
#include "SLADEMap.h"
#include "Utility/MathStuff.h"
#include "Utility/Tokenizer.h"

using namespace slade;
using SurfaceType = MapSector::SurfaceType;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
constexpr double TAU = math::PI * 2; // Number of radians in the unit circle
} // namespace


// -----------------------------------------------------------------------------
//
// MapSpecials Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clear out all internal state
// -----------------------------------------------------------------------------
void MapSpecials::reset()
{
	sector_colours_.clear();
	sector_fadecolours_.clear();
	translucent_lines_.clear();
}

// -----------------------------------------------------------------------------
// Process map specials, depending on the current game/port
// -----------------------------------------------------------------------------
void MapSpecials::processMapSpecials(SLADEMap* map)
{
	// ZDoom
	if (game::configuration().currentPort() == "zdoom")
		processZDoomMapSpecials(map);
	// Eternity, currently no need for processEternityMapSpecials
	else if (game::configuration().currentPort() == "eternity")
		processEternitySlopes(map);
	// Sonic Robo Blast 2
	else if (game::configuration().currentGame() == "srb2")
		processSRB2Slopes(map);
	// EDGE-Classic
	else if (game::configuration().currentPort() == "edge_classic")
		processEDGEClassicSlopes(map);
}

// -----------------------------------------------------------------------------
// Process a line's special, depending on the current game/port
// -----------------------------------------------------------------------------
void MapSpecials::processLineSpecial(MapLine* line)
{
	if (game::configuration().currentPort() == "zdoom")
		processZDoomLineSpecial(line);
}

// -----------------------------------------------------------------------------
// Sets [colour] to the parsed colour for [tag].
// Returns true if the tag has a colour, false otherwise
// -----------------------------------------------------------------------------
bool MapSpecials::tagColour(int tag, ColRGBA* colour) const
{
	unsigned a;
	// scripts
	for (a = 0; a < sector_colours_.size(); a++)
	{
		if (sector_colours_[a].tag == tag)
		{
			colour->r = sector_colours_[a].colour.r;
			colour->g = sector_colours_[a].colour.g;
			colour->b = sector_colours_[a].colour.b;
			colour->a = 255;
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Sets [colour] to the parsed fade colour for [tag].
// Returns true if the tag has a colour, false otherwise
// -----------------------------------------------------------------------------
bool MapSpecials::tagFadeColour(int tag, ColRGBA* colour) const
{
	unsigned a;
	// scripts
	for (a = 0; a < sector_fadecolours_.size(); a++)
	{
		if (sector_fadecolours_[a].tag == tag)
		{
			colour->r = sector_fadecolours_[a].colour.r;
			colour->g = sector_fadecolours_[a].colour.g;
			colour->b = sector_fadecolours_[a].colour.b;
			colour->a = 0;
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns true if any sector tags should be coloured
// -----------------------------------------------------------------------------
bool MapSpecials::tagColoursSet() const
{
	return !(sector_colours_.empty());
}

// -----------------------------------------------------------------------------
// Returns true if any sector tags should be coloured by fog
// -----------------------------------------------------------------------------
bool MapSpecials::tagFadeColoursSet() const
{
	return !(sector_fadecolours_.empty());
}

// -----------------------------------------------------------------------------
// Modify sector with [tag]
// -----------------------------------------------------------------------------
void MapSpecials::setModified(const SLADEMap* map, int tag) const
{
	for (auto& sector : map->sectors().allWithId(tag))
		sector->setModified();
}

// -----------------------------------------------------------------------------
// Returns true if [line] is translucent (via TranslucentLine special)
// -----------------------------------------------------------------------------
bool MapSpecials::lineIsTranslucent(const MapLine* line) const
{
	for (const auto& tl : translucent_lines_)
		if (tl.line == line)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns TranslucentLine special alpha for [line]
// -----------------------------------------------------------------------------
double MapSpecials::translucentLineAlpha(const MapLine* line) const
{
	for (const auto& tl : translucent_lines_)
		if (tl.line == line)
			return tl.alpha;

	return 1.;
}

// -----------------------------------------------------------------------------
// Returns TranslucentLine special additive flag for [line]
// -----------------------------------------------------------------------------
bool MapSpecials::translucentLineAdditive(const MapLine* line) const
{
	for (const auto& tl : translucent_lines_)
		if (tl.line == line)
			return tl.additive;

	return false;
}

// -----------------------------------------------------------------------------
// Updates any sectors with tags that are affected by any processed
// specials/scripts
// -----------------------------------------------------------------------------
void MapSpecials::updateTaggedSectors(const SLADEMap* map) const
{
	// scripts
	unsigned a;
	for (a = 0; a < sector_colours_.size(); a++)
		setModified(map, sector_colours_[a].tag);

	for (a = 0; a < sector_fadecolours_.size(); a++)
		setModified(map, sector_fadecolours_[a].tag);
}

// -----------------------------------------------------------------------------
// Process ZDoom map specials, mostly to convert hexen specials to UDMF
// counterparts
// -----------------------------------------------------------------------------
void MapSpecials::processZDoomMapSpecials(SLADEMap* map)
{
	// Line specials
	translucent_lines_.clear();
	for (unsigned a = 0; a < map->nLines(); a++)
		processZDoomLineSpecial(map->line(a));

	// All slope specials, which must be done in a particular order
	processZDoomSlopes(map);
}

// -----------------------------------------------------------------------------
// Process ZDoom line special
// -----------------------------------------------------------------------------
void MapSpecials::processZDoomLineSpecial(MapLine* line)
{
	// Get special
	int special = line->special();
	if (special == 0)
		return;

	// Get parent map
	auto map = line->parentMap();

	// Get args
	int args[5];
	for (unsigned arg = 0; arg < 5; arg++)
		args[arg] = line->arg(arg);

	// --- TranslucentLine ---
	if (special == 208)
	{
		// Get tagged lines
		vector<MapLine*> tagged;
		if (args[0] > 0)
			map->lines().putAllWithId(args[0], tagged);
		else
			tagged.push_back(line);

		// Get args
		double alpha = (double)args[1] / 255.0;
		string type  = (args[2] == 0) ? "translucent" : "add";

		// Set transparency
		for (auto& l : tagged)
		{
			translucent_lines_.push_back({ l, alpha, args[2] != 0 });

			log::info(3, "Line {} translucent: ({}) {:1.2f}, {}", l->index(), args[1], alpha, type);
		}
	}
}

// -----------------------------------------------------------------------------
// Process 'OPEN' ACS scripts for various specials - sector colours, slopes, etc
// -----------------------------------------------------------------------------
void MapSpecials::processACSScripts(ArchiveEntry* entry)
{
	sector_colours_.clear();
	sector_fadecolours_.clear();

	if (!entry || entry->size() == 0)
		return;

	Tokenizer tz;
	tz.setSpecialCharacters(";,:|={}/()");
	tz.openMem(entry->data(), "ACS Scripts");

	while (!tz.atEnd())
	{
		if (tz.checkNC("script"))
		{
			log::info(3, "script found");

			tz.adv(2); // Skip script #

			// Check for open script
			if (tz.checkNC("OPEN"))
			{
				log::info(3, "script is OPEN");

				// Skip to opening brace
				while (!tz.check("{"))
					tz.adv();

				// Parse script
				while (!tz.checkOrEnd("}"))
				{
					// --- Sector_SetColor ---
					if (tz.checkNC("Sector_SetColor"))
					{
						// Get parameters
						auto parameters = tz.getTokensUntil(")");

						// Parse parameters
						int val;
						int tag = -1;
						int r   = -1;
						int g   = -1;
						int b   = -1;
						for (auto& parameter : parameters)
						{
							if (!parameter.isInteger())
								continue;

							parameter.toInt(val);
							if (tag < 0)
								tag = val;
							else if (r < 0)
								r = val;
							else if (g < 0)
								g = val;
							else if (b < 0)
								b = val;
						}

						// Check everything is set
						if (b < 0)
						{
							log::info(2, "Invalid Sector_SetColor parameters");
						}
						else
						{
							SectorColour sc;
							sc.tag = tag;
							sc.colour.set(r, g, b, 255);
							log::info(3, "Sector tag {}, colour {},{},{}", tag, r, g, b);
							sector_colours_.push_back(sc);
						}
					}
					// --- Sector_SetFade ---
					else if (tz.checkNC("Sector_SetFade"))
					{
						// Get parameters
						auto parameters = tz.getTokensUntil(")");

						// Parse parameters
						int val;
						int tag = -1;
						int r   = -1;
						int g   = -1;
						int b   = -1;
						for (auto& parameter : parameters)
						{
							if (!parameter.isInteger())
								continue;

							parameter.toInt(val);
							if (tag < 0)
								tag = val;
							else if (r < 0)
								r = val;
							else if (g < 0)
								g = val;
							else if (b < 0)
								b = val;
						}

						// Check everything is set
						if (b < 0)
						{
							log::info(2, "Invalid Sector_SetFade parameters");
						}
						else
						{
							SectorColour sc;
							sc.tag = tag;
							sc.colour.set(r, g, b, 0);
							log::info(3, "Sector tag {}, fade colour {},{},{}", tag, r, g, b);
							sector_fadecolours_.push_back(sc);
						}
					}

					tz.adv();
				}
			}
		}

		tz.adv();
	}
}

// -----------------------------------------------------------------------------
// Process SRB2 slope specials
// -----------------------------------------------------------------------------
void MapSpecials::processSRB2Slopes(const SLADEMap* map) const
{
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);

		auto front = line->frontSector();
		auto back  = line->backSector();

		switch (line->special())
		{
			//
			// Sector-based slopes
			//

		case 700: // Front sector floor
			applyPlaneAlign<SurfaceType::Floor>(line, front, back);
			break;

		case 701: // Front sector ceiling
			applyPlaneAlign<SurfaceType::Ceiling>(line, front, back);
			break;

		case 702: // Front sector floor and ceiling
			applyPlaneAlign<SurfaceType::Floor>(line, front, back);
			applyPlaneAlign<SurfaceType::Ceiling>(line, front, back);
			break;

		case 703: // Front sector floor and back sector ceiling
			applyPlaneAlign<SurfaceType::Floor>(line, front, back);
			applyPlaneAlign<SurfaceType::Ceiling>(line, back, front);
			break;


		case 710: // Back sector floor
			applyPlaneAlign<SurfaceType::Floor>(line, back, front);
			break;

		case 711: // Back sector ceiling
			applyPlaneAlign<SurfaceType::Ceiling>(line, back, front);
			break;

		case 712: // Back sector floor and ceiling
			applyPlaneAlign<SurfaceType::Floor>(line, back, front);
			applyPlaneAlign<SurfaceType::Ceiling>(line, back, front);
			break;

		case 713: // Back sector floor and front sector ceiling
			applyPlaneAlign<SurfaceType::Floor>(line, back, front);
			applyPlaneAlign<SurfaceType::Ceiling>(line, front, back);
			break;


			//
			// Vertex-based slopes
			//

		case 704: // Front sector floor

		case 705: // Front sector ceiling

		case 714: // Back sector floor

		case 715: // Back sector ceiling
		{
			auto target = (line->special() == 704 || line->special() == 705) ? front : back;

			if (!target) // One-sided line
			{
				log::warning(
					"Ignoring vertex slope special on line {}, the target back/front sector for this line don't exist",
					line->index());
				break;
			}

			auto sidedef = line->s1()->sector() == target ? line->s1() : line->s2();

			Vec3d    vertices[3];
			unsigned count = 0;
			for (auto& thing : map->things())
			{
				if (thing->type() != 750)
					continue;

				if ((line->flagSet(8192)
					 && (thing->angle() == line->id() || thing->angle() == sidedef->texOffsetX()
						 || thing->angle() == sidedef->texOffsetY()))
					|| thing->angle() == line->id())
				{
					vertices[count++] = Vec3d(thing->xPos(), thing->yPos(), thing->zPos());
					if (count >= 3)
						break;
				}
			}

			if (count < 3)
			{
				log::warning(
					"Ignoring vertex slope special on line {}, No or insufficient vertex slope things (750) were "
					"provided",
					line->index());
				break;
			}

			if (line->special() == 704 || line->special() == 714)
				target->setPlane<SurfaceType::Floor>(math::planeFromTriangle(vertices[0], vertices[1], vertices[2]));
			else
				target->setPlane<SurfaceType::Ceiling>(math::planeFromTriangle(vertices[0], vertices[1], vertices[2]));
		}
		break;
		}
	}

	// Copied slopes linedefs need to be processed right after the other slope linedefs to assure ordering
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);

		auto front = line->frontSector();

		switch (line->special())
		{
			//
			// Copied slopes
			//

		case 720: // Front sector floor

		case 721: // Front sector ceiling

		case 722: // Front sector floor and ceiling
		{
			if (!front)
			{
				log::warning("Ignoring copied slopes special on line {}, no front sector on this line", line->index());
				break;
			}

			auto tagged = map->sectors().firstWithId(line->id());

			if (!tagged)
			{
				log::warning(
					"Ignoring copied slopes special on line {}, couldn't find sector with tag {}",
					line->index(),
					line->id());
				break;
			}

			if (line->special() == 720 || line->special() == 722)
				front->setFloorPlane(tagged->floor().plane);

			if (line->special() == 721 || line->special() == 722)
				front->setCeilingPlane(tagged->ceiling().plane);
		}
		break;
		}
	}
}

// -----------------------------------------------------------------------------
// Process EDGE-Classic slope specials
// -----------------------------------------------------------------------------
void MapSpecials::processEDGEClassicSlopes(SLADEMap* map) const
{
	// First things first: reset every sector to flat planes
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target = map->sector(a);
		target->setPlane<SurfaceType::Floor>(Plane::flat(target->planeHeight<SurfaceType::Floor>()));
		target->setPlane<SurfaceType::Ceiling>(Plane::flat(target->planeHeight<SurfaceType::Ceiling>()));
	}

	VertexHeightMap vertex_floor_heights;
	VertexHeightMap vertex_ceiling_heights;
	// Vertex heights -- only applies for sectors with exactly three vertices,
	// or sectors with exactly four vertices that also fulfill other criteria.
	// Heights are set by UDMF properties.
	vector<MapVertex*> vertices;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target = map->sector(a);
		vertices.clear();
		target->putVertices(vertices);
		if (vertices.size() == 4)
		{
			applyRectangularVertexHeightSlope<SurfaceType::Floor>(target, vertices, vertex_floor_heights);
			applyRectangularVertexHeightSlope<SurfaceType::Ceiling>(target, vertices, vertex_ceiling_heights);
		}
		else if (vertices.size() == 3)
		{
			applyVertexHeightSlope<SurfaceType::Floor>(target, vertices, vertex_floor_heights);
			applyVertexHeightSlope<SurfaceType::Ceiling>(target, vertices, vertex_ceiling_heights);
		}
		else
			continue;
	}
}

// -----------------------------------------------------------------------------
// Process ZDoom slope specials
// -----------------------------------------------------------------------------
void MapSpecials::processZDoomSlopes(SLADEMap* map) const
{
	// ZDoom has a variety of slope mechanisms, which must be evaluated in a
	// specific order.
	//  - UDMF plane properties
	//  - Plane_Align, in line order
	//  - line slope + sector tilt + vavoom, in thing order
	//  - slope copy things, in thing order
	//  - overwrite vertex heights with vertex height things
	//  - vertex triangle slopes, in sector order
	//  - Plane_Copy, in line order

	// First things first: reset every sector to flat planes
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target = map->sector(a);
		target->setPlane<SurfaceType::Floor>(Plane::flat(target->planeHeight<SurfaceType::Floor>()));
		target->setPlane<SurfaceType::Ceiling>(Plane::flat(target->planeHeight<SurfaceType::Ceiling>()));
	}

	// Floor/ceiling plane properties
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target        = map->sector(a);
		auto floorplane    = Plane::flat(target->floor().height);
		bool hasFloorplane = false;
		// Check for floor plane.
		// Note that these properties will only work in GZDoom if all of them are present.
		// Set A, B, and C negative to compensate for the calculation
		// differences between SLADE and GZDoom.
		if (target->hasProp("floorplane_a"))
		{
			floorplane.a  = -target->floatProperty("floorplane_a");
			hasFloorplane = true;
		}
		if (target->hasProp("floorplane_b"))
		{
			floorplane.b  = -target->floatProperty("floorplane_b");
			hasFloorplane = true;
		}
		if (target->hasProp("floorplane_c"))
		{
			floorplane.c  = -target->floatProperty("floorplane_c");
			hasFloorplane = true;
		}
		if (target->hasProp("floorplane_d"))
		{
			floorplane.d  = target->floatProperty("floorplane_d");
			hasFloorplane = true;
		}
		if (hasFloorplane && !(floorplane.a == 0 && floorplane.b == 0 && floorplane.c == -1 && floorplane.d == 0))
		{
			target->setFloorPlane(floorplane);
		}

		// Check for ceiling plane
		auto ceilingplane    = Plane::flat(target->ceiling().height);
		bool hasCeilingplane = false;
		if (target->hasProp("ceilingplane_a"))
		{
			ceilingplane.a  = -target->floatProperty("ceilingplane_a");
			hasCeilingplane = true;
		}
		if (target->hasProp("ceilingplane_b"))
		{
			ceilingplane.b  = -target->floatProperty("ceilingplane_b");
			hasCeilingplane = true;
		}
		if (target->hasProp("ceilingplane_c"))
		{
			ceilingplane.c  = -target->floatProperty("ceilingplane_c");
			hasCeilingplane = true;
		}
		if (target->hasProp("ceilingplane_d"))
		{
			ceilingplane.d  = target->floatProperty("ceilingplane_d");
			hasCeilingplane = true;
		}
		if (hasCeilingplane
			&& !(ceilingplane.a == 0 && ceilingplane.b == 0 && ceilingplane.c == -1 && ceilingplane.d == 0))
		{
			target->setCeilingPlane(ceilingplane);
		}
	}

	// Plane_Align (line special 181)
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);
		if (line->special() != 181)
			continue;

		auto sector1 = line->frontSector();
		auto sector2 = line->backSector();
		if (!sector1 || !sector2)
		{
			log::warning("Ignoring Plane_Align on one-sided line {}", line->index());
			continue;
		}
		if (sector1 == sector2)
		{
			log::warning("Ignoring Plane_Align on line {}, which has the same sector on both sides", line->index());
			continue;
		}

		int floor_arg = line->arg(0);
		if (floor_arg == 1)
			applyPlaneAlign<SurfaceType::Floor>(line, sector1, sector2);
		else if (floor_arg == 2)
			applyPlaneAlign<SurfaceType::Floor>(line, sector2, sector1);

		int ceiling_arg = line->arg(1);
		if (ceiling_arg == 1)
			applyPlaneAlign<SurfaceType::Ceiling>(line, sector1, sector2);
		else if (ceiling_arg == 2)
			applyPlaneAlign<SurfaceType::Ceiling>(line, sector2, sector1);
	}

	// Line slope things (9500/9501), sector tilt things (9502/9503), and
	// vavoom things (1500/1501), all in the same pass
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		auto thing = map->thing(a);

		// Line slope things
		if (thing->type() == 9500)
			applyLineSlopeThing<SurfaceType::Floor>(map, thing);
		else if (thing->type() == 9501)
			applyLineSlopeThing<SurfaceType::Ceiling>(map, thing);
		// Sector tilt things
		else if (thing->type() == 9502)
			applySectorTiltThing<SurfaceType::Floor>(map, thing);
		else if (thing->type() == 9503)
			applySectorTiltThing<SurfaceType::Ceiling>(map, thing);
		// Vavoom things
		else if (thing->type() == 1500)
			applyVavoomSlopeThing<SurfaceType::Floor>(map, thing);
		else if (thing->type() == 1501)
			applyVavoomSlopeThing<SurfaceType::Ceiling>(map, thing);
	}

	// Slope copy things (9510/9511)
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		auto thing = map->thing(a);

		if (thing->type() == 9510 || thing->type() == 9511)
		{
			auto target = map->sectors().atPos(thing->position());
			if (!target)
				continue;

			// First argument is the tag of a sector whose slope should be copied
			int tag = thing->arg(0);
			if (!tag)
			{
				log::warning("Ignoring slope copy thing in sector {} with no argument", target->index());
				continue;
			}

			auto tagged_sector = map->sectors().firstWithId(tag);
			if (!tagged_sector)
			{
				log::warning(
					"Ignoring slope copy thing in sector {}; no sectors have target tag {}", target->index(), tag);
				continue;
			}

			if (thing->type() == 9510)
				target->setFloorPlane(tagged_sector->floor().plane);
			else
				target->setCeilingPlane(tagged_sector->ceiling().plane);
		}
	}

	// Vertex height things
	// These only affect the calculation of slopes and shouldn't be stored in
	// the map data proper, so instead of actually changing vertex properties,
	// we store them in a hashmap.
	VertexHeightMap vertex_floor_heights;
	VertexHeightMap vertex_ceiling_heights;
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		auto thing = map->thing(a);
		if (thing->type() == 1504 || thing->type() == 1505)
		{
			// TODO there could be more than one vertex at this point
			auto vertex = map->vertices().vertexAt(thing->xPos(), thing->yPos());
			if (vertex)
			{
				if (thing->type() == 1504)
					vertex_floor_heights[vertex] = thing->zPos();
				else if (thing->type() == 1505)
					vertex_ceiling_heights[vertex] = thing->zPos();
			}
		}
	}

	// Vertex heights -- only applies for sectors with exactly three vertices.
	// Heights may be set by UDMF properties, or by a vertex height thing
	// placed exactly on the vertex (which takes priority over the prop).
	vector<MapVertex*> vertices;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target = map->sector(a);
		vertices.clear();
		target->putVertices(vertices);
		if (vertices.size() != 3)
			continue;

		applyVertexHeightSlope<SurfaceType::Floor>(target, vertices, vertex_floor_heights);
		applyVertexHeightSlope<SurfaceType::Ceiling>(target, vertices, vertex_ceiling_heights);
	}

	// Plane_Copy
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);
		if (line->special() != 118)
			continue;

		int  tag;
		auto front = line->frontSector();
		auto back  = line->backSector();
		if ((tag = line->arg(0)) && front)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setFloorPlane(sector->floor().plane);
		}
		if ((tag = line->arg(1)) && front)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setCeilingPlane(sector->ceiling().plane);
		}
		if ((tag = line->arg(2)) && back)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setFloorPlane(sector->floor().plane);
		}
		if ((tag = line->arg(3)) && back)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setCeilingPlane(sector->ceiling().plane);
		}

		// The fifth "share" argument copies from one side of the line to the
		// other
		if (front && back)
		{
			int share = line->arg(4);

			if ((share & 3) == 1)
				back->setFloorPlane(front->floor().plane);
			else if ((share & 3) == 2)
				front->setFloorPlane(back->floor().plane);

			if ((share & 12) == 4)
				back->setCeilingPlane(front->ceiling().plane);
			else if ((share & 12) == 8)
				front->setCeilingPlane(back->ceiling().plane);
		}
	}
}

// -----------------------------------------------------------------------------
// Process Eternity slope specials
// -----------------------------------------------------------------------------
void MapSpecials::processEternitySlopes(const SLADEMap* map) const
{
	// Eternity plans on having a few slope mechanisms,
	// which must be evaluated in a specific order.
	//  - Plane_Align, in line order
	//  - vertex triangle slopes, in sector order (wip)
	//  - Plane_Copy, in line order

	// First things first: reset every sector to flat planes
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		auto target = map->sector(a);
		target->setPlane<SurfaceType::Floor>(Plane::flat(target->planeHeight<SurfaceType::Floor>()));
		target->setPlane<SurfaceType::Ceiling>(Plane::flat(target->planeHeight<SurfaceType::Ceiling>()));
	}

	// Plane_Align (line special 181)
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);
		if (line->special() != 181)
			continue;

		auto sector1 = line->frontSector();
		auto sector2 = line->backSector();
		if (!sector1 || !sector2)
		{
			log::warning("Ignoring Plane_Align on one-sided line {}", line->index());
			continue;
		}
		if (sector1 == sector2)
		{
			log::warning("Ignoring Plane_Align on line {}, which has the same sector on both sides", line->index());
			continue;
		}

		int floor_arg = line->arg(0);
		if (floor_arg == 1)
			applyPlaneAlign<SurfaceType::Floor>(line, sector1, sector2);
		else if (floor_arg == 2)
			applyPlaneAlign<SurfaceType::Floor>(line, sector2, sector1);

		int ceiling_arg = line->arg(1);
		if (ceiling_arg == 1)
			applyPlaneAlign<SurfaceType::Ceiling>(line, sector1, sector2);
		else if (ceiling_arg == 2)
			applyPlaneAlign<SurfaceType::Ceiling>(line, sector2, sector1);
	}

	// Plane_Copy
	vector<MapSector*> sectors;
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		auto line = map->line(a);
		if (line->special() != 118)
			continue;

		int  tag;
		auto front = line->frontSector();
		auto back  = line->backSector();
		if ((tag = line->arg(0)) && front)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setFloorPlane(sector->floor().plane);
		}
		if ((tag = line->arg(1)) && front)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setCeilingPlane(sector->ceiling().plane);
		}
		if ((tag = line->arg(2)) && back)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setFloorPlane(sector->floor().plane);
		}
		if ((tag = line->arg(3)) && back)
		{
			if (auto sector = map->sectors().firstWithId(tag))
				front->setCeilingPlane(sector->ceiling().plane);
		}

		// The fifth "share" argument copies from one side of the line to the
		// other
		if (front && back)
		{
			int share = line->arg(4);

			if ((share & 3) == 1)
				back->setFloorPlane(front->floor().plane);
			else if ((share & 3) == 2)
				front->setFloorPlane(back->floor().plane);

			if ((share & 12) == 4)
				back->setCeilingPlane(front->ceiling().plane);
			else if ((share & 12) == 8)
				front->setCeilingPlane(back->ceiling().plane);
		}
	}
}

// -----------------------------------------------------------------------------
// Applies a Plane_Align special on [line], to [target] from [model]
// -----------------------------------------------------------------------------
template<SurfaceType T> void MapSpecials::applyPlaneAlign(MapLine* line, MapSector* target, MapSector* model) const
{
	if (!model || !target) // Do nothing, ignore
	{
		log::warning("Ignoring Plane_Align on line {}; line needs to have sectors on both sides", line->index());
		return;
	}

	vector<MapVertex*> vertices;
	target->putVertices(vertices);

	Vec2d mid    = line->getPoint(MapObject::Point::Mid);
	Vec2d v1_pos = (line->start() - mid).normalized();
	Vec2d v2_pos = (line->end() - mid).normalized();

	// Extend the line to the sector boundaries
	double max_dot_1 = 0.0;
	double max_dot_2 = 0.0;
	for (auto& vertex : vertices)
	{
		Vec2d vert = vertex->position() - mid;

		double dot = vert.dot(v1_pos);

		double& max_dot = dot > 0 ? max_dot_1 : max_dot_2;

		dot = std::fabs(dot);

		if (dot > max_dot)
			max_dot = dot;
	}

	v1_pos = (v1_pos * max_dot_1) + mid;
	v2_pos = (v2_pos * max_dot_2) + mid;

	// The slope is between the line with Plane_Align, and the point in the
	// sector furthest away from it, which can only be at a vertex
	double     furthest_dist   = 0.0;
	MapVertex* furthest_vertex = nullptr;
	Seg2d      seg(v1_pos, v2_pos);
	for (auto& vertex : vertices)
	{
		double dist = math::distanceToLine(vertex->position(), seg);

		if (!math::colinear(vertex->xPos(), vertex->yPos(), v1_pos.x, v1_pos.y, v2_pos.x, v2_pos.y)
			&& dist > furthest_dist)
		{
			furthest_vertex = vertex;
			furthest_dist   = dist;
		}
	}

	if (!furthest_vertex || furthest_dist < 0.01)
	{
		log::warning(
			"Ignoring Plane_Align on line {}; sector {} has no appropriate reference vertex",
			line->index(),
			target->index());
		return;
	}

	// Calculate slope plane from our three points: this line's endpoints
	// (at the model sector's height) and the found vertex (at this sector's height).
	double modelz  = model->planeHeight<T>();
	double targetz = target->planeHeight<T>();

	Vec3d p1(v1_pos, modelz);
	Vec3d p2(v2_pos, modelz);
	Vec3d p3(furthest_vertex->position(), targetz);

	target->setPlane<T>(math::planeFromTriangle(p1, p2, p3));
}

// -----------------------------------------------------------------------------
// Applies a line slope special on [thing], to its containing sector in [map]
// -----------------------------------------------------------------------------
template<SurfaceType T> void MapSpecials::applyLineSlopeThing(SLADEMap* map, MapThing* thing) const
{
	int lineid = thing->arg(0);
	if (!lineid)
	{
		log::warning("Ignoring line slope thing {} with no lineid argument", thing->index());
		return;
	}

	// These are computed on first use, to avoid extra work if no lines match
	MapSector* containing_sector = nullptr;
	double     thingz            = 0.;

	for (auto& line : map->lines().allWithId(lineid))
	{
		// Line slope things only affect the sector on the side of the line
		// that faces the thing
		double     side   = math::lineSide(thing->position(), line->seg());
		MapSector* target = nullptr;
		if (side < 0)
			target = line->backSector();
		else if (side > 0)
			target = line->frontSector();
		if (!target)
			continue;

		// Need to know the containing sector's height to find the thing's true height
		if (!containing_sector)
		{
			containing_sector = map->sectors().atPos(thing->position());
			if (!containing_sector)
				return;
			thingz = containing_sector->plane<T>().heightAt(thing->position()) + thing->zPos();
		}

		// Three points: endpoints of the line, and the thing itself
		auto  target_plane = target->plane<T>();
		Vec3d p1(line->x1(), line->y1(), target_plane.heightAt(line->start()));
		Vec3d p2(line->x2(), line->y2(), target_plane.heightAt(line->end()));
		Vec3d p3(thing->xPos(), thing->yPos(), thingz);
		target->setPlane<T>(math::planeFromTriangle(p1, p2, p3));
	}
}

// -----------------------------------------------------------------------------
// Applies a tilt slope special on [thing], to its containing sector in [map]
// -----------------------------------------------------------------------------
template<SurfaceType T> void MapSpecials::applySectorTiltThing(SLADEMap* map, MapThing* thing) const
{
	// TODO should this apply to /all/ sectors at this point, in the case of an
	// intersection?
	auto target = map->sectors().atPos(thing->position());
	if (!target)
		return;

	// First argument is the tilt angle, but starting with 0 as straight down;
	// subtracting 90 fixes that.
	int raw_angle = thing->arg(0);
	if (raw_angle == 0 || raw_angle == 180)
		// Exact vertical tilt is nonsense
		return;

	double angle = thing->angle() / 360.0 * TAU;
	double tilt  = (raw_angle - 90) / 360.0 * TAU;
	// Resulting plane goes through the position of the thing
	double z = target->planeHeight<T>() + thing->zPos();
	Vec3d  point(thing->xPos(), thing->yPos(), z);

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

	target->setPlane<T>(math::planeFromTriangle(point, point + vec1, point + vec2));
}

// -----------------------------------------------------------------------------
// Applies a vavoom slope special on [thing], to its containing sector in [map]
// -----------------------------------------------------------------------------
template<SurfaceType T> void MapSpecials::applyVavoomSlopeThing(SLADEMap* map, MapThing* thing) const
{
	auto target = map->sectors().atPos(thing->position());
	if (!target)
		return;

	int              tid = thing->id();
	vector<MapLine*> lines;
	target->putLines(lines);

	// TODO unclear if this is the same order that ZDoom would go through the
	// lines, which matters if two lines have the same first arg
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (tid != lines[a]->arg(0))
			continue;

		// Vavoom things use the plane defined by the thing and its two
		// endpoints, based on the sector's original (flat) plane and treating
		// the thing's height as absolute
		if (math::distanceToLineFast(thing->position(), lines[a]->seg()) == 0)
		{
			log::warning("Vavoom thing {} lies directly on its target line {}", thing->index(), a);
			return;
		}

		short height = target->planeHeight<T>();
		Vec3d p1(thing->xPos(), thing->yPos(), thing->zPos());
		Vec3d p2(lines[a]->x1(), lines[a]->y1(), height);
		Vec3d p3(lines[a]->x2(), lines[a]->y2(), height);

		target->setPlane<T>(math::planeFromTriangle(p1, p2, p3));
		return;
	}

	log::warning("Vavoom thing {} has no matching line with first arg {}", thing->index(), tid);
}

// -----------------------------------------------------------------------------
// Returns the floor/ceiling height of [vertex] in [sector]
// -----------------------------------------------------------------------------
template<SurfaceType T> double MapSpecials::vertexHeight(MapVertex* vertex, MapSector* sector) const
{
	// Return vertex height if set via UDMF property
	string prop = (T == SurfaceType::Floor ? "zfloor" : "zceiling");
	if (vertex->hasProp(prop))
		return vertex->floatProperty(prop);

	// Otherwise just return sector height
	return sector->planeHeight<T>();
}

// -----------------------------------------------------------------------------
// Applies a slope to sector [target] based on the heights of its vertices
// (triangular sectors only)
// -----------------------------------------------------------------------------
template<SurfaceType T>
void MapSpecials::applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights)
	const
{
	string prop         = (T == SurfaceType::Floor ? "zfloor" : "zceiling");
	auto   v1_hasheight = heights.count(vertices[0]) || vertices[0]->hasProp(prop);
	auto   v2_hasheight = heights.count(vertices[1]) || vertices[1]->hasProp(prop);
	auto   v3_hasheight = heights.count(vertices[2]) || vertices[2]->hasProp(prop);

	// Ignore if no vertices have a height set
	if (!v1_hasheight && !v2_hasheight && !v3_hasheight)
		return;

	double z1 = heights.count(vertices[0]) ? heights[vertices[0]] : vertexHeight<T>(vertices[0], target);
	double z2 = heights.count(vertices[1]) ? heights[vertices[1]] : vertexHeight<T>(vertices[1], target);
	double z3 = heights.count(vertices[2]) ? heights[vertices[2]] : vertexHeight<T>(vertices[2], target);

	Vec3d p1(vertices[0]->xPos(), vertices[0]->yPos(), z1);
	Vec3d p2(vertices[1]->xPos(), vertices[1]->yPos(), z2);
	Vec3d p3(vertices[2]->xPos(), vertices[2]->yPos(), z3);
	target->setPlane<T>(math::planeFromTriangle(p1, p2, p3));
}

// -----------------------------------------------------------------------------
// Applies a slope to sector [target] based on the heights of its vertices
// (EDGE-Classic rectangular sectors only; performs additional validation)
// -----------------------------------------------------------------------------
template<SurfaceType T>
void MapSpecials::applyRectangularVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights)
	const
{
	std::vector<int> height_verts;
	string prop         = (T == SurfaceType::Floor ? "zfloor" : "zceiling");
	auto   v1_hasheight = heights.count(vertices[0]) || vertices[0]->hasProp(prop);
	auto   v2_hasheight = heights.count(vertices[1]) || vertices[1]->hasProp(prop);
	auto   v3_hasheight = heights.count(vertices[2]) || vertices[2]->hasProp(prop);
	auto   v4_hasheight = heights.count(vertices[3]) || vertices[3]->hasProp(prop);
	if (v1_hasheight)
		height_verts.push_back(0);
	if (v2_hasheight)
		height_verts.push_back(1);
	if (v3_hasheight)
		height_verts.push_back(2);
	if (v4_hasheight)
		height_verts.push_back(3);
	if (height_verts.size() == 2) // Must only have two out of the four verts assigned a zfloor/ceiling value
	{
		MapVertex *v1 = vertices[height_verts[0]];
		MapVertex *v2 = vertices[height_verts[1]];
		// Must be both verts of the same line
		bool same_line = false;
		for (const auto& line : v1->connectedLines())
		{
			if ((line->v1() == v1 && line->v2() == v2) || (line->v1() == v2 && line->v2() == v1))
			{
				same_line = true;
				break;
			}
		}
		if (same_line)
		{
			// The zfloor/zceiling values must be equal
			if (fabs(heights.count(v1) ? heights[v1] : vertexHeight<T>(v1, target) - heights.count(v2) ? heights[v2] : vertexHeight<T>(v2, target)) < 0.001f)
			{
				// Psuedo-Plane_Align routine
				double     furthest_dist   = 0.0;
				MapVertex* furthest_vertex = nullptr;
				Seg2d      seg(v1->position(), v2->position());
				for (auto& vertex : vertices)
				{
					double dist = math::distanceToLine(vertex->position(), seg);

					if (!math::colinear(vertex->xPos(), vertex->yPos(), v1->xPos(), v1->yPos(), v2->xPos(), v2->yPos())
						&& dist > furthest_dist)
					{
						furthest_vertex = vertex;
						furthest_dist   = dist;
					}
				}

				if (!furthest_vertex || furthest_dist < 0.01)
					return;

				// Calculate slope plane from our three points: this line's endpoints
				// (at the model sector's height) and the found vertex (at this sector's height).
				double modelz  = heights.count(v1) ? heights[v1] : vertexHeight<T>(v1, target);
				double targetz = target->planeHeight<T>();

				Vec3d p1(v1->position(), modelz);
				Vec3d p2(v2->position(), modelz);
				Vec3d p3(furthest_vertex->position(), targetz);

				target->setPlane<T>(math::planeFromTriangle(p1, p2, p3));
			}
		}
	}
}