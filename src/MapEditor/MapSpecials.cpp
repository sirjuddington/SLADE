
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapSpecials.cpp
 * Description: Various functions for processing map specials andit
 *              scripts, mostly for visual effects (transparency,
 *              colours, slopes, etc.)
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
#include "SLADEMap/SLADEMap.h"
#include "MapSpecials.h"
#include "GameConfiguration/GameConfiguration.h"
#include "Utility/Tokenizer.h"
#include "Utility/MathStuff.h"



// Number of radians in the unit circle
const double TAU = PI * 2;


/*******************************************************************
 * MAPSPECIALS NAMESPACE FUNCTIONS
 *******************************************************************/

/* MapSpecials::reset
 * Clear out all internal state
 *******************************************************************/
void MapSpecials::reset()
{
	sector_colours.clear();
	sector_fadecolours.clear();
}

/* MapSpecials::processMapSpecials
 * Process map specials, depending on the current game/port
 *******************************************************************/
void MapSpecials::processMapSpecials(SLADEMap* map)
{
	// ZDoom
	if (theGameConfiguration->currentPort() == "zdoom")
		processZDoomMapSpecials(map);
   // Eternity, currently no need for processEternityMapSpecials
   else if (theGameConfiguration->currentPort() == "eternity")
      processEternitySlopes(map);
}

/* MapSpecials::processLineSpecial
 * Process a line's special, depending on the current game/port
 *******************************************************************/
void MapSpecials::processLineSpecial(MapLine* line)
{
	if (theGameConfiguration->currentPort() == "zdoom")
		processZDoomLineSpecial(line);
}

/* MapSpecials::getTagColour
 * Sets [colour] to the parsed colour for [tag]. Returns true if the
 * tag has a colour, false otherwise
 *******************************************************************/
bool MapSpecials::getTagColour(int tag, rgba_t* colour)
{
	unsigned a;
	// scripts
	for (a = 0; a < sector_colours.size(); a++)
	{
		if (sector_colours[a].tag == tag)
		{
			colour->r = sector_colours[a].colour.r;
			colour->g = sector_colours[a].colour.g;
			colour->b = sector_colours[a].colour.b;
			colour->a = 255;
			return true;
		}
	}

	return false;
}

/* MapSpecials::getTagFadeColour
 * Sets [colour] to the parsed fade colour for [tag]. Returns true if the
 * tag has a colour, false otherwise
 *******************************************************************/
bool MapSpecials::getTagFadeColour(int tag, rgba_t *colour)
{
	unsigned a;
	// scripts
	for (a = 0; a < sector_fadecolours.size(); a++)
	{
		if (sector_fadecolours[a].tag == tag)
		{
			colour->r = sector_fadecolours[a].colour.r;
			colour->g = sector_fadecolours[a].colour.g;
			colour->b = sector_fadecolours[a].colour.b;
			colour->a = 0;
			return true;
		}
	}

	return false;
}

/* MapSpecials::tagColoursSet
 * Returns true if any sector tags should be coloured
 *******************************************************************/
bool MapSpecials::tagColoursSet()
{
	return !(sector_colours.empty());
}

/* MapSpecials::tagFadeColoursSet
 * Returns true if any sector tags should be coloured by fog
 *******************************************************************/
bool MapSpecials::tagFadeColoursSet()
{
	return !(sector_fadecolours.empty());
}

/* MapSpecials::setModified
 * Modify sector with [tag]
 *******************************************************************/
void MapSpecials::setModified(SLADEMap *map, int tag)
{
	vector<MapSector*> tagged;
	map->getSectorsByTag(tag, tagged);
	for (unsigned s = 0; s < tagged.size(); s++)
		tagged[s]->setModified();
}

/* MapSpecials::updateTaggedSecors
 * Updates any sectors with tags that are affected by any processed
 * specials/scripts
 *******************************************************************/
void MapSpecials::updateTaggedSectors(SLADEMap* map)
{
	// scripts
	unsigned a;
	for (a = 0; a < sector_colours.size(); a++)
		setModified(map, sector_colours[a].tag);

	for (a = 0; a < sector_fadecolours.size(); a++)
		setModified(map, sector_fadecolours[a].tag);
}

/* MapSpecials::processZDoomMapSpecials
 * Process ZDoom map specials, mostly to convert hexen specials to
 * UDMF counterparts
 *******************************************************************/
void MapSpecials::processZDoomMapSpecials(SLADEMap* map)
{
	// Line specials
	for (unsigned a = 0; a < map->nLines(); a++)
		processZDoomLineSpecial(map->getLine(a));

	// All slope specials, which must be done in a particular order
	processZDoomSlopes(map);
}

/* MapSpecials::processZDoomLineSpecial
 * Process ZDoom line special
 *******************************************************************/
void MapSpecials::processZDoomLineSpecial(MapLine* line)
{
	// Get special
	int special = line->getSpecial();
	if (special == 0)
		return;

	// Get parent map
	SLADEMap* map = line->getParentMap();

	// Get args
	int args[5];
	for (unsigned arg = 0; arg < 5; arg++)
		args[arg] = line->intProperty(S_FMT("arg%d", arg));

	// --- TranslucentLine ---
	if (special == 208)
	{
		// Get tagged lines
		vector<MapLine*> tagged;
		if (args[0] > 0)
			map->getLinesById(args[0], tagged);
		else
			tagged.push_back(line);

		// Get args
		double alpha = (double)args[1] / 255.0;
		string type = (args[2] == 0) ? "translucent" : "add";

		// Set transparency
		for (unsigned l = 0; l < tagged.size(); l++)
		{
			tagged[l]->setFloatProperty("alpha", alpha);
			tagged[l]->setStringProperty("renderstyle", type);

			LOG_MESSAGE(3, S_FMT("Line %d translucent: (%d) %1.2f, %s", tagged[l]->getIndex(), args[1], alpha, CHR(type)));
		}
	}
}

/* MapSpecials::processACSScripts
 * Process 'OPEN' ACS scripts for various specials - sector colours,
 * slopes, etc.
 *******************************************************************/
void MapSpecials::processACSScripts(ArchiveEntry* entry)
{
	sector_colours.clear();
	sector_fadecolours.clear();

	if (!entry || entry->getSize() == 0)
		return;

	Tokenizer tz;
	tz.setSpecialCharacters(";,:|={}/()");
	tz.openMem(entry->getData(), entry->getSize(), "ACS Scripts");

	string token = tz.getToken();
	while (!tz.isAtEnd())
	{
		if (S_CMPNOCASE(token, "script"))
		{
			LOG_MESSAGE(3, "script found");

			tz.skipToken();	// Skip script #
			tz.getToken(&token);

			// Check for open script
			if (S_CMPNOCASE(token, "OPEN"))
			{
				LOG_MESSAGE(3, "script is OPEN");

				// Skip to opening brace
				while (token != "{")
					tz.getToken(&token);

				// Parse script
				tz.getToken(&token);
				while (token != "}")
				{
					// --- Sector_SetColor ---
					if (S_CMPNOCASE(token, "Sector_SetColor"))
					{
						// Get parameters
						vector<string> parameters;
						tz.getTokensUntil(parameters, ")");

						// Parse parameters
						long val;
						int tag = -1;
						int r = -1;
						int g = -1;
						int b = -1;
						for (unsigned a = 0; a < parameters.size(); a++)
						{
							if (parameters[a].ToLong(&val))
							{
								if (tag < 0)
									tag = val;
								else if (r < 0)
									r = val;
								else if (g < 0)
									g = val;
								else if (b < 0)
									b = val;
							}
						}

						// Check everything is set
						if (b < 0)
						{
							LOG_MESSAGE(2, "Invalid Sector_SetColor parameters");
						}
						else
						{
							sector_colour_t sc;
							sc.tag = tag;
							sc.colour.set(r, g, b, 255);
							LOG_MESSAGE(3, "Sector tag %d, colour %d,%d,%d", tag, r, g, b);
							sector_colours.push_back(sc);
						}
					}
					// --- Sector_SetFade ---
					else if (S_CMPNOCASE(token, "Sector_SetFade"))
					{
						// Get parameters
						vector<string> parameters;
						tz.getTokensUntil(parameters, ")");

						// Parse parameters
						long val;
						int tag = -1;
						int r = -1;
						int g = -1;
						int b = -1;
						for (unsigned a = 0; a < parameters.size(); a++)
						{
							if (parameters[a].ToLong(&val))
							{
								if (tag < 0)
									tag = val;
								else if (r < 0)
									r = val;
								else if (g < 0)
									g = val;
								else if (b < 0)
									b = val;
							}
						}

						// Check everything is set
						if (b < 0)
						{
							LOG_MESSAGE(2, "Invalid Sector_SetFade parameters");
						}
						else
						{
							sector_colour_t sc;
							sc.tag = tag;
							sc.colour.set(r, g, b, 0);
							LOG_MESSAGE(3, "Sector tag %d, fade colour %d,%d,%d", tag, r, g, b);
							sector_fadecolours.push_back(sc);
						}
					}

					tz.getToken(&token);
				}
			}
		}

		tz.getToken(&token);
	}
}

/* MapSpecials::processZDoomSlopes
 * Process ZDoom slope specials
 *******************************************************************/
void MapSpecials::processZDoomSlopes(SLADEMap* map)
{
	// ZDoom has a variety of slope mechanisms, which must be evaluated in a
	// specific order.
	//  - Plane_Align, in line order
	//  - line slope + sector tilt + vavoom, in thing order
	//  - slope copy things, in thing order
	//  - overwrite vertex heights with vertex height things
	//  - vertex triangle slopes, in sector order
	//  - Plane_Copy, in line order

	// First things first: reset every sector to flat planes
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		MapSector* target = map->getSector(a);
		target->setPlane<FLOOR_PLANE>(plane_t::flat(target->getPlaneHeight<FLOOR_PLANE>()));
		target->setPlane<CEILING_PLANE>(plane_t::flat(target->getPlaneHeight<CEILING_PLANE>()));
	}

	// Plane_Align (line special 181)
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		if (line->getSpecial() != 181)
			continue;

		MapSector* sector1 = line->frontSector();
		MapSector* sector2 = line->backSector();
		if (!sector1 || !sector2)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on one-sided line %d", line->getIndex());
			continue;
		}
		if (sector1 == sector2)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on line %d, which has the same sector on both sides", line->getIndex());
			continue;
		}

		int floor_arg = line->intProperty("arg0");
		if (floor_arg == 1)
			applyPlaneAlign<FLOOR_PLANE>(line, sector1, sector2);
		else if (floor_arg == 2)
			applyPlaneAlign<FLOOR_PLANE>(line, sector2, sector1);

		int ceiling_arg = line->intProperty("arg1");
		if (ceiling_arg == 1)
			applyPlaneAlign<CEILING_PLANE>(line, sector1, sector2);
		else if (ceiling_arg == 2)
			applyPlaneAlign<CEILING_PLANE>(line, sector2, sector1);
	}

	// Line slope things (9500/9501), sector tilt things (9502/9503), and
	// vavoom things (1500/1501), all in the same pass
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		MapThing* thing = map->getThing(a);

		// Line slope things
		if (thing->getType() == 9500)
			applyLineSlopeThing<FLOOR_PLANE>(map, thing);
		else if (thing->getType() == 9501)
			applyLineSlopeThing<CEILING_PLANE>(map, thing);
		// Sector tilt things
		else if (thing->getType() == 9502)
			applySectorTiltThing<FLOOR_PLANE>(map, thing);
		else if (thing->getType() == 9503)
			applySectorTiltThing<CEILING_PLANE>(map, thing);
		// Vavoom things
		else if (thing->getType() == 1500)
			applyVavoomSlopeThing<FLOOR_PLANE>(map, thing);
		else if (thing->getType() == 1501)
			applyVavoomSlopeThing<CEILING_PLANE>(map, thing);
	}

	// Slope copy things (9510/9511)
	for (unsigned a = 0; a < map->nThings(); a++)
	{
		MapThing* thing = map->getThing(a);

		if (thing->getType() == 9510 || thing->getType() == 9511)
		{
			int target_idx = map->sectorAt(thing->point());
			if (target_idx < 0)
				continue;
			MapSector* target = map->getSector(target_idx);

			// First argument is the tag of a sector whose slope should be copied
			int tag = thing->intProperty("arg0");
			if (!tag)
			{
				LOG_MESSAGE(1, "Ignoring slope copy thing in sector %d with no argument", target_idx);
				continue;
			}

			vector<MapSector*> tagged_sectors;
			map->getSectorsByTag(tag, tagged_sectors);
			if (tagged_sectors.empty())
			{
				LOG_MESSAGE(1, "Ignoring slope copy thing in sector %d; no sectors have target tag %d", target_idx, tag);
				continue;
			}

			if (thing->getType() == 9510)
				target->setFloorPlane(tagged_sectors[0]->getFloorPlane());
			else
				target->setCeilingPlane(tagged_sectors[0]->getCeilingPlane());
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
		MapThing* thing = map->getThing(a);
		if (thing->getType() == 1504 || thing->getType() == 1505)
		{
			// TODO there could be more than one vertex at this point
			MapVertex* vertex = map->vertexAt(thing->xPos(), thing->yPos());
			if (vertex)
			{
				if (thing->getType() == 1504)
					vertex_floor_heights[vertex] = thing->floatProperty("height");
				else if (thing->getType() == 1505)
					vertex_ceiling_heights[vertex] = thing->floatProperty("height");
			}
		}
	}

	// Vertex heights -- only applies for sectors with exactly three vertices.
	// Heights may be set by UDMF properties, or by a vertex height thing
	// placed exactly on the vertex (which takes priority over the prop).
	vector<MapVertex*> vertices;
	for (unsigned a = 0; a < map->nSectors(); a++)
	{
		MapSector* target = map->getSector(a);
		vertices.clear();
		target->getVertices(vertices);
		if (vertices.size() != 3)
			continue;

		applyVertexHeightSlope<FLOOR_PLANE>(target, vertices, vertex_floor_heights);
		applyVertexHeightSlope<CEILING_PLANE>(target, vertices, vertex_ceiling_heights);
	}

	// Plane_Copy
	vector<MapSector*> sectors;
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		if (line->getSpecial() != 118)
			continue;

		int tag;
		MapSector* front = line->frontSector();
		MapSector* back = line->backSector();
		if ((tag = line->intProperty("arg0")) && front)
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if (sectors.size())
				front->setFloorPlane(sectors[0]->getFloorPlane());
		}
		if ((tag = line->intProperty("arg1")) && front)
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if (sectors.size())
				front->setCeilingPlane(sectors[0]->getCeilingPlane());
		}
		if ((tag = line->intProperty("arg2")) && back)
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if (sectors.size())
				back->setFloorPlane(sectors[0]->getFloorPlane());
		}
		if ((tag = line->intProperty("arg3")) && back)
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if (sectors.size())
				back->setCeilingPlane(sectors[0]->getCeilingPlane());
		}

		// The fifth "share" argument copies from one side of the line to the
		// other
		if (front && back)
		{
			int share = line->intProperty("arg4");

			if ((share & 3) == 1)
				back->setFloorPlane(front->getFloorPlane());
			else if ((share & 3) == 2)
				front->setFloorPlane(back->getFloorPlane());

			if ((share & 12) == 4)
				back->setCeilingPlane(front->getCeilingPlane());
			else if ((share & 12) == 8)
				front->setCeilingPlane(back->getCeilingPlane());
		}
	}
}

/* MapSpecials::processEternitySlopes
* Process Eternity slope specials
*******************************************************************/
void MapSpecials::processEternitySlopes(SLADEMap* map)
{
	// Eternity plans on having a few slope mechanisms,
	// which must be evaluated in a specific order.
	//  - Plane_Align, in line order
	//  - vertex triangle slopes, in sector order (wip)
	//  - Plane_Copy, in line order

	// First things first: reset every sector to flat planes
	for(unsigned a = 0; a < map->nSectors(); a++)
	{
		MapSector* target = map->getSector(a);
		target->setPlane<FLOOR_PLANE>(plane_t::flat(target->getPlaneHeight<FLOOR_PLANE>()));
		target->setPlane<CEILING_PLANE>(plane_t::flat(target->getPlaneHeight<CEILING_PLANE>()));
	}

	// Plane_Align (line special 181)
	for(unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		if(line->getSpecial() != 181)
			continue;

		MapSector* sector1 = line->frontSector();
		MapSector* sector2 = line->backSector();
		if(!sector1 || !sector2)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on one-sided line %d", line->getIndex());
			continue;
		}
		if(sector1 == sector2)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on line %d, which has the same sector on both sides", line->getIndex());
			continue;
		}

		int floor_arg = line->intProperty("arg0");
		if(floor_arg == 1)
			applyPlaneAlign<FLOOR_PLANE>(line, sector1, sector2);
		else if(floor_arg == 2)
			applyPlaneAlign<FLOOR_PLANE>(line, sector2, sector1);

		int ceiling_arg = line->intProperty("arg1");
		if(ceiling_arg == 1)
			applyPlaneAlign<CEILING_PLANE>(line, sector1, sector2);
		else if(ceiling_arg == 2)
			applyPlaneAlign<CEILING_PLANE>(line, sector2, sector1);
	}

	// Plane_Copy
	vector<MapSector*> sectors;
	for(unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line = map->getLine(a);
		if(line->getSpecial() != 118)
			continue;

		int tag;
		MapSector* front = line->frontSector();
		MapSector* back = line->backSector();
		if((tag = line->intProperty("arg0")))
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if(sectors.size())
				front->setFloorPlane(sectors[0]->getFloorPlane());
		}
		if((tag = line->intProperty("arg1")))
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if(sectors.size())
				front->setCeilingPlane(sectors[0]->getCeilingPlane());
		}
		if((tag = line->intProperty("arg2")))
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if(sectors.size())
				back->setFloorPlane(sectors[0]->getFloorPlane());
		}
		if((tag = line->intProperty("arg3")))
		{
			sectors.clear();
			map->getSectorsByTag(tag, sectors);
			if(sectors.size())
				back->setCeilingPlane(sectors[0]->getCeilingPlane());
		}

		// The fifth "share" argument copies from one side of the line to the
		// other
		if(front && back)
		{
			int share = line->intProperty("arg4");

			if((share & 3) == 1)
				back->setFloorPlane(front->getFloorPlane());
			else if((share & 3) == 2)
				front->setFloorPlane(back->getFloorPlane());

			if((share & 12) == 4)
				back->setCeilingPlane(front->getCeilingPlane());
			else if((share & 12) == 8)
				front->setCeilingPlane(back->getCeilingPlane());
		}
	}
}


/* MapSpecials::applyPlaneAlign
 * Applies a Plane_Align special on [line], to [target] from [model]
 *******************************************************************/
template<PlaneType p>
void MapSpecials::applyPlaneAlign(MapLine* line, MapSector* target, MapSector* model)
{
	vector<MapVertex*> vertices;
	target->getVertices(vertices);

	// The slope is between the line with Plane_Align, and the point in the
	// sector furthest away from it, which can only be at a vertex
	double this_dist;
	MapVertex* this_vertex;
	double furthest_dist = 0.0;
	MapVertex* furthest_vertex = NULL;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		this_vertex = vertices[a];
		this_dist = line->distanceTo(this_vertex->point());
		if (this_dist > furthest_dist)
		{
			furthest_dist = this_dist;
			furthest_vertex = this_vertex;
		}
	}

	if (!furthest_vertex || furthest_dist < 0.01)
	{
		LOG_MESSAGE(1, "Ignoring Plane_Align on line %d; sector %d has no appropriate reference vertex", line->getIndex(), target->getIndex());
		return;
	}

	// Calculate slope plane from our three points: this line's endpoints
	// (at the model sector's height) and the found vertex (at this sector's height).
	double modelz = model->getPlaneHeight<p>();
	double targetz = target->getPlaneHeight<p>();
	fpoint3_t p1(line->x1(), line->y1(), modelz);
	fpoint3_t p2(line->x2(), line->y2(), modelz);
	fpoint3_t p3(furthest_vertex->point(), targetz);
	target->setPlane<p>(MathStuff::planeFromTriangle(p1, p2, p3));
}

/* MapSpecials::applyLineSlopeThing
 * Applies a line slope special on [thing], to its containing sector
 * in [map]
 *******************************************************************/
template<PlaneType p>
void MapSpecials::applyLineSlopeThing(SLADEMap* map, MapThing* thing)
{
	int lineid = thing->intProperty("arg0");
	if (!lineid)
	{
		LOG_MESSAGE(1, "Ignoring line slope thing %d with no lineid argument", thing->getIndex());
		return;
	}

	// These are computed on first use, to avoid extra work if no lines match
	MapSector* containing_sector = NULL;
	double thingz;

	vector<MapLine*> lines;
	map->getLinesById(lineid, lines);
	for (unsigned b = 0; b < lines.size(); b++)
	{
		MapLine* line = lines[b];

		// Line slope things only affect the sector on the side of the line
		// that faces the thing
		double side = MathStuff::lineSide(thing->point(), line->seg());
		MapSector* target = NULL;
		if (side < 0)
			target = line->backSector();
		else if (side > 0)
			target = line->frontSector();
		if (!target)
			continue;

		// Need to know the containing sector's height to find the thing's true height
		if (!containing_sector)
		{
			int containing_sector_idx = map->sectorAt(thing->point());
			if (containing_sector_idx < 0)
				return;
			containing_sector = map->getSector(containing_sector_idx);
			thingz = (
				containing_sector->getPlane<p>().height_at(thing->point())
				+ thing->floatProperty("height")
			);
		}

		// Three points: endpoints of the line, and the thing itself
		plane_t target_plane = target->getPlane<p>();
		fpoint3_t p1(lines[b]->x1(), lines[b]->y1(), target_plane.height_at(lines[b]->point1()));
		fpoint3_t p2(lines[b]->x2(), lines[b]->y2(), target_plane.height_at(lines[b]->point2()));
		fpoint3_t p3(thing->xPos(), thing->yPos(), thingz);
		target->setPlane<p>(MathStuff::planeFromTriangle(p1, p2, p3));
	}
}

/* MapSpecials::applyLineSlopeThing
 * Applies a tilt slope special on [thing], to its containing sector
 * in [map]
 *******************************************************************/
template<PlaneType p>
void MapSpecials::applySectorTiltThing(SLADEMap* map, MapThing* thing)
{
	// TODO should this apply to /all/ sectors at this point, in the case of an
	// intersection?
	int target_idx = map->sectorAt(thing->point());
	if (target_idx < 0)
		return;
	MapSector* target = map->getSector(target_idx);

	// First argument is the tilt angle, but starting with 0 as straight down;
	// subtracting 90 fixes that.
	int raw_angle = thing->intProperty("arg0");
	if (raw_angle == 0 || raw_angle == 180)
		// Exact vertical tilt is nonsense
		return;

	double angle = thing->getAngle() / 360.0 * TAU;
	double tilt = (raw_angle - 90) / 360.0 * TAU;
	// Resulting plane goes through the position of the thing
	double z = target->getPlaneHeight<p>() + thing->floatProperty("height");
	fpoint3_t point(thing->xPos(), thing->yPos(), z);

	double cos_angle = cos(angle);
	double sin_angle = sin(angle);
	double cos_tilt = cos(tilt);
	double sin_tilt = sin(tilt);
	// Need to convert these angles into vectors on the plane, so we can take a
	// normal.
	// For the first: we know that the line perpendicular to the direction the
	// thing faces lies "flat", because this is the axis the tilt thing rotates
	// around.  "Rotate" the angle a quarter turn to get this vector -- switch
	// x and y, and negate one.
	fpoint3_t vec1(-sin_angle, cos_angle, 0.0);

	// For the second: the tilt angle makes a triangle between the floor plane
	// and the z axis.  sin gives us the distance along the z-axis, but cos
	// only gives us the distance away /from/ the z-axis.  Break that into x
	// and y by multiplying by cos and sin of the thing's facing angle.
	fpoint3_t vec2(cos_tilt * cos_angle, cos_tilt * sin_angle, sin_tilt);

	target->setPlane<p>(MathStuff::planeFromTriangle(point, point + vec1, point + vec2));
}

/* MapSpecials::applyVavoomSlopeThing
 * Applies a vavoom slope special on [thing], to its containing
 * sector in [map]
 *******************************************************************/
template<PlaneType p>
void MapSpecials::applyVavoomSlopeThing(SLADEMap* map, MapThing* thing)
{
	int target_idx = map->sectorAt(thing->point());
	if (target_idx < 0)
		return;
	MapSector* target = map->getSector(target_idx);

	int tid = thing->intProperty("id");
	vector<MapLine*> lines;
	target->getLines(lines);

	// TODO unclear if this is the same order that ZDoom would go through the
	// lines, which matters if two lines have the same first arg
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (tid != lines[a]->intProperty("arg0"))
			continue;

		// Vavoom things use the plane defined by the thing and its two
		// endpoints, based on the sector's original (flat) plane and treating
		// the thing's height as absolute
		if (MathStuff::distanceToLineFast(thing->point(), lines[a]->seg()) == 0)
		{
			LOG_MESSAGE(1, "Vavoom thing %d lies directly on its target line %d", thing->getIndex(), a);
			return;
		}

		short height = target->getPlaneHeight<p>();
		fpoint3_t p1(thing->xPos(), thing->yPos(), thing->floatProperty("height"));
		fpoint3_t p2(lines[a]->x1(), lines[a]->y1(), height);
		fpoint3_t p3(lines[a]->x2(), lines[a]->y2(), height);

		target->setPlane<p>(MathStuff::planeFromTriangle(p1, p2, p3));
		return;
	}

	LOG_MESSAGE(1, "Vavoom thing %d has no matching line with first arg %d", thing->getIndex(), tid);
}

/* MapSpecials::vertexHeight
 * Returns the floor/ceiling height of [vertex] in [sector]
 *******************************************************************/
template<PlaneType p>
double MapSpecials::vertexHeight(MapVertex* vertex, MapSector* sector)
{
	// Return vertex height if set via UDMF property
	string prop = (p == FLOOR_PLANE ? "zfloor" : "zceiling");
	if (vertex->hasProp(prop))
		return vertex->floatProperty(prop);

	// Otherwise just return sector height
	return sector->getPlaneHeight<p>();
}

/* MapSpecials::applyVertexHeightSlope
 * Applies a slope to sector [target] based on the heights of its
 * vertices (triangular sectors only)
 *******************************************************************/
template<PlaneType p>
void MapSpecials::applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights)
{
	double z1 = heights.count(vertices[0]) ? heights[vertices[0]] : vertexHeight<p>(vertices[0], target);
	double z2 = heights.count(vertices[1]) ? heights[vertices[1]] : vertexHeight<p>(vertices[1], target);
	double z3 = heights.count(vertices[2]) ? heights[vertices[2]] : vertexHeight<p>(vertices[2], target);

	fpoint3_t p1(vertices[0]->xPos(), vertices[0]->yPos(), z1);
	fpoint3_t p2(vertices[1]->xPos(), vertices[1]->yPos(), z2);
	fpoint3_t p3(vertices[2]->xPos(), vertices[2]->yPos(), z3);
	target->setPlane<p>(MathStuff::planeFromTriangle(p1, p2, p3));
}
