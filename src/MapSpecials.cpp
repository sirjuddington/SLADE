
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapSpecials.cpp
 * Description: Various functions for processing map specials and
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
#include "SLADEMap.h"
#include "MapSpecials.h"
#include "GameConfiguration.h"
#include "Tokenizer.h"
#include "MathStuff.h"
#include <wx/colour.h>


/*******************************************************************
 * MAPSPECIALS NAMESPACE FUNCTIONS
 *******************************************************************/

/* MapSpecials::reset
 * Clear out all internal state
 *******************************************************************/
void MapSpecials::reset()
{
	sector_colours.clear();
}

/* MapSpecials::processMapSpecials
 * Process map specials, depending on the current game/port
 *******************************************************************/
void MapSpecials::processMapSpecials(SLADEMap* map)
{
	// ZDoom
	if (theGameConfiguration->currentPort() == "zdoom")
		processZDoomMapSpecials(map);
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
	for (unsigned a = 0; a < sector_colours.size(); a++)
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

/* MapSpecials::tagColoursSet
 * Returns true if any sector tags should be coloured
 *******************************************************************/
bool MapSpecials::tagColoursSet()
{
	return !(sector_colours.empty());
}

/* MapSpecials::updateTaggedSecors
 * Updates any sectors with tags that are affected by any processed
 * specials/scripts
 *******************************************************************/
void MapSpecials::updateTaggedSectors(SLADEMap* map)
{
	for (unsigned a = 0; a < sector_colours.size(); a++)
	{
		vector<MapSector*> tagged;
		map->getSectorsByTag(sector_colours[a].tag, tagged);
		for (unsigned s = 0; s < tagged.size(); s++)
			tagged[s]->setModified();
	}
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

	// --- Plane_Align ---
	if (special == 181)
	{
		// Get tagged lines
		vector<MapLine*> tagged;
		if (args[2] > 0)
			map->getLinesById(args[2], tagged);
		else
			tagged.push_back(line);

		// Setup slopes
		for (unsigned a = 0; a < tagged.size(); a++)
		{
			// Floor
			if (args[0] == 1 || args[0] == 2)
				setupPlaneAlignSlope(tagged[a], true, (args[0] == 1));

			// Ceiling
			if (args[1] == 1 || args[1] == 2)
				setupPlaneAlignSlope(tagged[a], false, (args[1] == 1));
		}
	}
}

/* MapSpecials::setupPlaneAlignSlope
 * Calculates the floor/ceiling plane for the sector affected by
 * [line]'s Plane_Align special
 *******************************************************************/
void MapSpecials::setupPlaneAlignSlope(MapLine* line, bool floor, bool front)
{
	LOG_MESSAGE(3, "Line %d %s slope, %s side", line->getIndex(), floor ? "floor" : "ceiling", front ? "front" : "back");

	// Get sectors
	MapSector* sloping_sector;
	MapSector* control_sector;
	if (front)
	{
		sloping_sector = line->frontSector();
		control_sector = line->backSector();
	}
	else
	{
		sloping_sector = line->backSector();
		control_sector = line->frontSector();
	}
	if (!sloping_sector || !control_sector)
	{
		LOG_MESSAGE(1, "Line %d is not two-sided, Plane_Align not processed", line->getIndex());
		return;
	}

	// The slope is between the line with Plane_Align, and the point in the
	// sector furthest away from it, which must be at a vertex
	vector<MapVertex*> vertices;
	sloping_sector->getVertices(vertices);

	double this_dist;
	MapVertex* this_vertex;
	double furthest_dist = 0.0;
	MapVertex* furthest_vertex = NULL;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		this_vertex = vertices[a];
		this_dist = line->distanceTo(this_vertex->xPos(), this_vertex->yPos());
		if (this_dist > furthest_dist)
		{
			furthest_dist = this_dist;
			furthest_vertex = this_vertex;
		}
	}

	if (!furthest_vertex || furthest_dist < 0.01)
	{
		LOG_MESSAGE(1, "Can't find a reference point not on line %d; Plane_Align not processed", line->getIndex());
		return;
	}

	// Calculate slope plane
	// We now have three points: this line's endpoints (at the control sector's
	// height) and the found vertex (at the sloped sector's height).
	double controlz = floor ? control_sector->getFloorHeight() : control_sector->getCeilingHeight();
	double slopingz = floor ? sloping_sector->getFloorHeight() : sloping_sector->getCeilingHeight();
	fpoint3_t p1(line->x1(), line->y1(), controlz);
	fpoint3_t p2(line->x2(), line->y2(), controlz);
	fpoint3_t p3(furthest_vertex->xPos(), furthest_vertex->yPos(), slopingz);
	plane_t plane = MathStuff::planeFromTriangle(p1, p2, p3);
	if (floor)
		sloping_sector->setFloorPlane(plane);
	else
		sloping_sector->setCeilingPlane(plane);
}

/* MapSpecials::processACSScripts
 * Process 'OPEN' ACS scripts for various specials - sector colours,
 * slopes, etc.
 *******************************************************************/
void MapSpecials::processACSScripts(ArchiveEntry* entry)
{
	sector_colours.clear();

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

					tz.getToken(&token);
				}
			}
		}

		tz.getToken(&token);
	}
}
