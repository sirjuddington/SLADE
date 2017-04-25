
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapChecks.cpp
 * Description: Various handler classes for map error/problem checks
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
#include "UI/WxStuff.h"
#include "SLADEMap/SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration/GameConfiguration.h"
#include "MapTextureManager.h"
#include "UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapEditContext.h"
#include "UI/Dialogs/ThingTypeBrowser.h"
#include "Utility/MathStuff.h"
#include "General/SAction.h"


/*******************************************************************
 * MISSINGTEXTURECHECK CLASS
 *******************************************************************
 * Checks for any missing textures on lines
 */
class MissingTextureCheck : public MapCheck
{
private:
	vector<MapLine*>	lines;
	vector<int>			parts;

public:
	MissingTextureCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		string sky_flat = theGameConfiguration->skyFlat();
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			// Check what textures the line needs
			MapLine* line = map->getLine(a);
			MapSide* side1 = line->s1();
			MapSide* side2 = line->s2();
			int needs = line->needsTexture();

			// Detect if sky hack might apply
			bool sky_hack = false;
			if (side1 && S_CMPNOCASE(sky_flat, side1->getSector()->getCeilingTex()) &&
				side2 && S_CMPNOCASE(sky_flat, side2->getSector()->getCeilingTex()))
				sky_hack = true;

			// Check for missing textures (front side)
			if (side1)
			{
				// Upper
				if ((needs & TEX_FRONT_UPPER) > 0 && side1->stringProperty("texturetop") == "-" && !sky_hack)
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_UPPER);
				}

				// Middle
				if ((needs & TEX_FRONT_MIDDLE) > 0 && side1->stringProperty("texturemiddle") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_MIDDLE);
				}

				// Lower
				if ((needs & TEX_FRONT_LOWER) > 0 && side1->stringProperty("texturebottom") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_LOWER);
				}
			}

			// Check for missing textures (back side)
			if (side2)
			{
				// Upper
				if ((needs & TEX_BACK_UPPER) > 0 && side2->stringProperty("texturetop") == "-" && !sky_hack)
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_UPPER);
				}

				// Middle
				if ((needs & TEX_BACK_MIDDLE) > 0 && side2->stringProperty("texturemiddle") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_MIDDLE);
				}

				// Lower
				if ((needs & TEX_BACK_LOWER) > 0 && side2->stringProperty("texturebottom") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_LOWER);
				}
			}
		}

		LOG_MESSAGE(3, "Missing Texture Check: %lu missing textures", parts.size());
	}

	unsigned nProblems()
	{
		return lines.size();
	}

	string texName(int part)
	{
		switch (part)
		{
		case TEX_FRONT_UPPER: return "front upper texture";
		case TEX_FRONT_MIDDLE: return "front middle texture";
		case TEX_FRONT_LOWER: return "front lower texture";
		case TEX_BACK_UPPER: return "back upper texture";
		case TEX_BACK_MIDDLE: return "back middle texture";
		case TEX_BACK_LOWER: return "back lower texture";
		}

		return "";
	}

	string problemDesc(unsigned index)
	{
		if (index < lines.size())
			return S_FMT("Line %d missing %s", lines[index]->getIndex(), texName(parts[index]));
		else
			return "No missing textures found";
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(MapEditor::windowWx(), 0, "-", map);
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Texture", true, false, false);

				// Set texture if one selected
				string texture = browser.getSelectedItem()->getName();
				switch (parts[index])
				{
				case TEX_FRONT_UPPER:
					lines[index]->setStringProperty("side1.texturetop", texture);
					break;
				case TEX_FRONT_MIDDLE:
					lines[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case TEX_FRONT_LOWER:
					lines[index]->setStringProperty("side1.texturebottom", texture);
					break;
				case TEX_BACK_UPPER:
					lines[index]->setStringProperty("side2.texturetop", texture);
					break;
				case TEX_BACK_MIDDLE:
					lines[index]->setStringProperty("side2.texturemiddle", texture);
					break;
				case TEX_BACK_LOWER:
					lines[index]->setStringProperty("side2.texturebottom", texture);
					break;
				default:
					return false;
				}

				editor->endUndoRecord();

				// Remove problem
				lines.erase(lines.begin() + index);
				parts.erase(parts.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= lines.size())
			return NULL;

		return lines[index];
	}

	string progressText()
	{
		return "Checking for missing textures...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}
};


/*******************************************************************
 * SPECIALTAGSCHECK CLASS
 *******************************************************************
 * Checks for lines with an action special that requires a tag (non-
 * zero) but have no tag set
 */
class SpecialTagsCheck : public MapCheck
{
private:
	vector<MapObject*>	objects;

public:
	SpecialTagsCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			// Get special and tag
			int special = map->getLine(a)->intProperty("special");
			int tag = map->getLine(a)->intProperty("arg0");

			// Get action special
			ActionSpecial* as = theGameConfiguration->actionSpecial(special);
			int tagged = as->needsTag();

			// Check for back sector that removes need for tagged sector
			if ((tagged == AS_TT_SECTOR_BACK || AS_TT_SECTOR_OR_BACK) && map->getLine(a)->backSector())
				continue;

			// Check if tag is required but not set
			if (tagged != AS_TT_NO && tag == 0)
				objects.push_back(map->getLine(a));
		}
		// Hexen and UDMF allow specials on things
		if (map->currentFormat() == MAP_HEXEN || map->currentFormat() == MAP_UDMF)
		{
			for (unsigned a = 0; a < map->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				ThingType* tt = theGameConfiguration->thingType(map->getThing(a)->getType());
				if (tt->getFlags() & THING_SCRIPT)
					continue;

				// Get special and tag
				int special = map->getThing(a)->intProperty("special");
				int tag = map->getThing(a)->intProperty("arg0");

				// Get action special
				ActionSpecial* as = theGameConfiguration->actionSpecial(special);
				int tagged = as->needsTag();

				// Check if tag is required but not set
				if (tagged != AS_TT_NO && tagged != AS_TT_SECTOR_BACK && tag == 0)
					objects.push_back(map->getThing(a));
			}
		}
	}

	unsigned nProblems()
	{
		return objects.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= objects.size())
			return "No missing special tags found";

		MapObject* mo = objects[index];
		int special = mo->intProperty("special");
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("%s %d: Special %d (%s) requires a tag",
			mo->getObjType() == MOBJ_LINE ? "Line" : "Thing",
			mo->getIndex(), special, as->getName());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		// Begin tag edit
		SActionHandler::doAction("mapw_line_tagedit");

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= objects.size())
			return NULL;

		return objects[index];
	}

	string progressText()
	{
		return "Checking for missing special tags...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Set Tagged...";

		return "";
	}
};


/*******************************************************************
 * MISSINGTAGGEDCHECK CLASS
 *******************************************************************
 * Checks for lines with an action special that have a tag that does
 * not point to anything that exists
 */
class MissingTaggedCheck : public MapCheck
{
private:
	vector<MapObject*>	objects;

public:
	MissingTaggedCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		unsigned nlines = map->nLines();
		unsigned nthings = 0;
		if (map->currentFormat() == MAP_HEXEN || map->currentFormat() == MAP_UDMF)
			nthings = map->nThings();
		for (unsigned a = 0; a < (nlines + nthings); a++)
		{
			MapObject* mo = NULL;
			bool thingmode = false;
			if (a >= nlines)
			{
				mo = map->getThing(a - nlines);
				thingmode = true;
			}
			else mo = map->getLine(a);

			// Ignore the Heresiarch which does not have a real special
			if (thingmode)
			{
				ThingType* tt = theGameConfiguration->thingType(((MapThing*)mo)->getType());
				if (tt->getFlags() & THING_SCRIPT)
					continue;
			}

			// Get special and tag
			int special = mo->intProperty("special");
			int tag = mo->intProperty("arg0");

			// Get action special
			ActionSpecial* as = theGameConfiguration->actionSpecial(special);
			int tagged = as->needsTag();

			// Check if tag is required and set (not set is a different check...)
			if (tagged != AS_TT_NO && /*(thingmode || (tagged != AS_TT_SECTOR_BACK && tagged != AS_TT_SECTOR_OR_BACK)) &&*/ tag != 0)
			{
				bool okay = false;
				switch (tagged)
				{
				case AS_TT_SECTOR:
				case AS_TT_SECTOR_OR_BACK:
					{
						std::vector<MapSector *> foundsectors;
						map->getSectorsByTag(tag, foundsectors);
						if (foundsectors.size() > 0)
							okay = true;
					}
					break;
				case AS_TT_LINE:
					{
						std::vector<MapLine *> foundlines;
						map->getLinesById(tag, foundlines);
						if (foundlines.size() > 0)
							okay = true;
					}
					break;
				case AS_TT_THING:
					{
						std::vector<MapThing *> foundthings;
						map->getThingsById(tag, foundthings);
						if (foundthings.size() > 0)
							okay = true;
					}
					break;
				default:
					// Ignore the rest for now...
					okay = true;
					break;
				}
				if (!okay)
				{
					objects.push_back(mo);
				}
			}
		}
	}

	unsigned nProblems()
	{
		return objects.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= objects.size())
			return "No missing tagged objects found";

		int special = objects[index]->intProperty("special");
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("%s %d: No object tagged %d for Special %d (%s)", 
			objects[index]->getObjType() == MOBJ_LINE ? "Line" : "Thing",
			objects[index]->getIndex(), 
			objects[index]->intProperty("arg0"), 
			special, as->getName());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		// Can't automatically fix that
		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= objects.size())
			return NULL;

		return objects[index];
	}

	string progressText()
	{
		return "Checking for missing tagged objects...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "...";

		return "";
	}
};


/*******************************************************************
 * LINESINTERSECT CLASS
 *******************************************************************
 * Checks for any intersecting lines
 */
class LinesIntersectCheck : public MapCheck
{
private:
	struct line_intersect_t
	{
		MapLine*	line1;
		MapLine*	line2;
		fpoint2_t	intersect_point;

		line_intersect_t(MapLine* line1, MapLine* line2, double x, double y)
		{
			this->line1 = line1;
			this->line2 = line2;
			intersect_point.set(x, y);
		}
	};
	vector<line_intersect_t>	intersections;

public:
	LinesIntersectCheck(SLADEMap* map) : MapCheck(map) {}

	void checkIntersections(vector<MapLine*> lines)
	{
		double x, y;
		MapLine* line1;
		MapLine* line2;

		// Clear existing intersections
		intersections.clear();

		// Go through lines
		for (unsigned a = 0; a < lines.size(); a++)
		{
			line1 = lines[a];

			// Go through uncompared lines
			for (unsigned b = a + 1; b < lines.size(); b++)
			{
				line2 = lines[b];

				// Check intersection
				if (map->linesIntersect(line1, line2, x, y))
					intersections.push_back(line_intersect_t(line1, line2, x, y));
			}
		}
	}

	void doCheck()
	{
		// Get all map lines
		vector<MapLine*> all_lines;
		for (unsigned a = 0; a < map->nLines(); a++)
			all_lines.push_back(map->getLine(a));

		// Check for intersections
		checkIntersections(all_lines);
	}

	unsigned nProblems()
	{
		return intersections.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= intersections.size())
			return "No intersecting lines found";

		return S_FMT("Lines %d and %d are intersecting at (%1.2f, %1.2f)",
			intersections[index].line1->getIndex(), intersections[index].line2->getIndex(),
			intersections[index].intersect_point.x, intersections[index].intersect_point.y);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= intersections.size())
			return false;

		if (fix_type == 0)
		{
			MapLine* line1 = intersections[index].line1;
			MapLine* line2 = intersections[index].line2;

			editor->beginUndoRecord("Split Lines");

			// Create split vertex
			MapVertex* nv = map->createVertex(intersections[index].intersect_point.x, intersections[index].intersect_point.y, -1);

			// Split first line
			map->splitLine(line1, nv);
			MapLine* nl1 = map->getLine(map->nLines() - 1);

			// Split second line
			map->splitLine(line2, nv);
			MapLine* nl2 = map->getLine(map->nLines() - 1);

			// Remove intersection
			intersections.erase(intersections.begin() + index);

			editor->endUndoRecord();

			// Create list of lines to re-check
			vector<MapLine*> lines;
			lines.push_back(line1);
			lines.push_back(line2);
			lines.push_back(nl1);
			lines.push_back(nl2);
			for (unsigned a = 0; a < intersections.size(); a++)
			{
				VECTOR_ADD_UNIQUE(lines, intersections[a].line1);
				VECTOR_ADD_UNIQUE(lines, intersections[a].line2);
			}

			// Re-check intersections
			checkIntersections(lines);

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= intersections.size())
			return NULL;

		return intersections[index].line1;
	}

	string progressText()
	{
		return "Checking for intersecting lines...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Split Lines";

		return "";
	}
};


/*******************************************************************
 * LINESOVERLAPCHECK CLASS
 *******************************************************************
 * Checks for any overlapping lines (lines that share both vertices)
 */
class LinesOverlapCheck : public MapCheck
{
private:
	struct line_overlap_t
	{
		MapLine* line1;
		MapLine* line2;

		line_overlap_t(MapLine* line1, MapLine* line2)
		{
			this->line1 = line1;
			this->line2 = line2;
		}
	};
	vector<line_overlap_t>	overlaps;

public:
	LinesOverlapCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		// Go through lines
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			MapLine* line1 = map->getLine(a);

			// Go through uncompared lines
			for (unsigned b = a + 1; b < map->nLines(); b++)
			{
				MapLine* line2 = map->getLine(b);

				// Check for overlap (both vertices shared)
				if ((line1->v1() == line2->v1() && line1->v2() == line2->v2()) ||
					(line1->v2() == line2->v1() && line1->v1() == line2->v2()))
					overlaps.push_back(line_overlap_t(line1, line2));
			}
		}
	}

	unsigned nProblems()
	{
		return overlaps.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= overlaps.size())
			return "No overlapping lines found";

		return S_FMT("Lines %d and %d are overlapping", overlaps[index].line1->getIndex(), overlaps[index].line2->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= overlaps.size())
			return false;

		if (fix_type == 0)
		{
			MapLine* line1 = overlaps[index].line1;
			MapLine* line2 = overlaps[index].line2;

			editor->beginUndoRecord("Merge Lines");

			// Remove first line and correct sectors
			map->removeLine(line1);
			map->correctLineSectors(line2);

			editor->endUndoRecord();

			// Remove any overlaps for line1 (since it was removed)
			for (unsigned a = 0; a < overlaps.size(); a++)
			{
				if (overlaps[a].line1 == line1 || overlaps[a].line2 == line1)
				{
					overlaps.erase(overlaps.begin() + a);
					a--;
				}
			}

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= overlaps.size())
			return NULL;

		return overlaps[index].line1;
	}

	string progressText()
	{
		return "Checking for overlapping lines...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Merge Lines";

		return "";
	}
};


/*******************************************************************
 * THINGSOVERLAPCHECK CLASS
 *******************************************************************
 * Checks for any overlapping things, taking radius and flags into
 * account
 */
class ThingsOverlapCheck : public MapCheck
{
private:
	struct thing_overlap_t
	{
		MapThing* thing1;
		MapThing* thing2;
		thing_overlap_t(MapThing* thing1, MapThing* thing2)
		{
			this->thing1 = thing1;
			this->thing2 = thing2;
		}
	};
	vector<thing_overlap_t>	overlaps;

public:
	ThingsOverlapCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		ThingType* tt1, *tt2;
		double r1, r2;

		// Go through things
		for (unsigned a = 0; a < map->nThings(); a++)
		{
			MapThing* thing1 = map->getThing(a);
			tt1 = theGameConfiguration->thingType(thing1->getType());
			r1 = tt1->getRadius() - 1;

			// Ignore if no radius
			if (r1 < 0 || !tt1->isSolid())
				continue;

			// Go through uncompared things
			int map_format = map->currentFormat();
			bool udmf_zdoom = (map_format == MAP_UDMF && S_CMPNOCASE(theGameConfiguration->udmfNamespace(), "zdoom"));
			bool udmf_eternity = (map_format == MAP_UDMF && S_CMPNOCASE(theGameConfiguration->udmfNamespace(), "eternity"));
			int min_skill = udmf_zdoom || udmf_eternity ? 1 : 2;
			int max_skill = udmf_zdoom ? 17 : 5;
			int max_class = udmf_zdoom ? 17 : 4;

			for (unsigned b = a + 1; b < map->nThings(); b++)
			{
				MapThing* thing2 = map->getThing(b);
				tt2 = theGameConfiguration->thingType(thing2->getType());
				r2 = tt2->getRadius() - 1;

				// Ignore if no radius
				if (r2 < 0 || !tt2->isSolid())
					continue;

				// Check flags
				// Case #1: different skill levels
				bool shareflag = false;
				for (int s = min_skill; s < max_skill; ++s)
				{
					string skill = S_FMT("skill%d", s);
					if (theGameConfiguration->thingBasicFlagSet(skill, thing1, map_format) && 
						theGameConfiguration->thingBasicFlagSet(skill, thing2, map_format))
					{
						shareflag = true;
						s = max_skill;
					}
				}
				if (!shareflag)
					continue;

				// Booleans for single, coop, deathmatch, and teamgame status for each thing
				bool s1, s2, c1, c2, d1, d2, t1, t2;
				s1 = theGameConfiguration->thingBasicFlagSet("single", thing1, map_format);
				s2 = theGameConfiguration->thingBasicFlagSet("single", thing2, map_format);
				c1 = theGameConfiguration->thingBasicFlagSet("coop", thing1, map_format);
				c2 = theGameConfiguration->thingBasicFlagSet("coop", thing2, map_format);
				d1 = theGameConfiguration->thingBasicFlagSet("dm", thing1, map_format);
				d2 = theGameConfiguration->thingBasicFlagSet("dm", thing2, map_format);

				// Player starts
				// P1 are automatically S and C; P2+ are automatically C;
				// Deathmatch starts are automatically D, and team start are T.
				if (tt1->getFlags() & THING_COOPSTART)
				{
					c1 = true; d1 = t1 = false;
					if (thing1->getType() == 1)
						s1 = true;
					else s1 = false;
				}
				else if (tt1->getFlags() & THING_DMSTART)
				{
					s1 = c1 = t1 = false; d1 = true;
				}
				else if (tt1->getFlags() & THING_TEAMSTART)
				{
					s1 = c1 = d1 = false; t1 = true;
				}
				if (tt2->getFlags() & THING_COOPSTART)
				{
					c2 = true; d2 = t2 = false;
					if (thing2->getType() == 1)
						s2 = true;
					else s2 = false;
				}
				else if (tt2->getFlags() & THING_DMSTART)
				{
					s2 = c2 = t2 = false; d2 = true;
				}
				else if (tt2->getFlags() & THING_TEAMSTART)
				{
					s2 = c2 = d2 = false; t2 = true;
				}

				// Case #2: different game modes (single, coop, dm)
				shareflag = false;
				if ((c1 && c2)||(d1 && d2)||(t1 && t2))
				{
					shareflag = true;
				}
				if (!shareflag && s1 && s2)
				{
					// Case #3: things flagged for single player with different class filters
					for (int c = 1; c < max_class; ++c)
					{
						string pclass = S_FMT("class%d", c);
						if (theGameConfiguration->thingBasicFlagSet(pclass, thing1, map_format) && 
							theGameConfiguration->thingBasicFlagSet(pclass, thing2, map_format))
						{
							shareflag = true;
							c = max_class;
						}
					}
				}
				if (!shareflag)
					continue;

				// Also check player start spots in Hexen-style hubs
				shareflag = false;
				if (tt1->getFlags() & THING_COOPSTART && tt2->getFlags() & THING_COOPSTART)
				{
					if (thing1->intProperty("arg0") == thing2->intProperty("arg0"))
						shareflag = true;
				}
				if (!shareflag)
					continue;

				// Check x non-overlap
				if (thing2->xPos() + r2 < thing1->xPos() - r1 || thing2->xPos() - r2 > thing1->xPos() + r1)
					continue;

				// Check y non-overlap
				if (thing2->yPos() + r2 < thing1->yPos() - r1 || thing2->yPos() - r2 > thing1->yPos() + r1)
					continue;

				// Overlap detected
				overlaps.push_back(thing_overlap_t(thing1, thing2));
			}
		}
	}

	unsigned nProblems()
	{
		return overlaps.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= overlaps.size())
			return "No overlapping things found";

		return S_FMT("Things %d and %d are overlapping", overlaps[index].thing1->getIndex(), overlaps[index].thing2->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= overlaps.size())
			return false;

		// Get thing to remove (depending on fix)
		MapThing* thing = NULL;
		if (fix_type == 0)
			thing = overlaps[index].thing1;
		else if (fix_type == 1)
			thing = overlaps[index].thing2;

		if (thing)
		{
			// Remove thing
			editor->beginUndoRecord("Delete Thing", false, false, true);
			map->removeThing(thing);
			editor->endUndoRecord();

			// Clear any overlaps involving the removed thing
			for (unsigned a = 0; a < overlaps.size(); a++)
			{
				if (overlaps[a].thing1 == thing || overlaps[a].thing2 == thing)
				{
					overlaps.erase(overlaps.begin() + a);
					a--;
				}
			}

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= overlaps.size())
			return NULL;

		return overlaps[index].thing1;
	}

	string progressText()
	{
		return "Checking for overlapping things...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (index >= overlaps.size())
			return "";

		if (fix_type == 0)
			return S_FMT("Delete Thing #%d", overlaps[index].thing1->getIndex());
		if (fix_type == 1)
			return S_FMT("Delete Thing #%d", overlaps[index].thing2->getIndex());

		return "";
	}
};


/*******************************************************************
 * UNKNOWNTEXTURESCHECK CLASS
 *******************************************************************
 * Checks for any unknown/invalid textures on lines
 */
class UnknownTexturesCheck : public MapCheck
{
private:
	MapTextureManager*	texman;
	vector<MapLine*>	lines;
	vector<int>			parts;

public:
	UnknownTexturesCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map)
	{
		this->texman = texman;
	}

	void doCheck()
	{
		bool mixed = theGameConfiguration->mixTexFlats();

		// Go through lines
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			MapLine* line = map->getLine(a);

			// Check front side textures
			if (line->s1())
			{
				// Get textures
				string upper = line->s1()->stringProperty("texturetop");
				string middle = line->s1()->stringProperty("texturemiddle");
				string lower = line->s1()->stringProperty("texturebottom");

				// Upper
				if (upper != "-" && texman->getTexture(upper, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_UPPER);
				}

				// Middle
				if (middle != "-" && texman->getTexture(middle, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_MIDDLE);
				}

				// Lower
				if (lower != "-" && texman->getTexture(lower, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_LOWER);
				}
			}

			// Check back side textures
			if (line->s2())
			{
				// Get textures
				string upper = line->s2()->stringProperty("texturetop");
				string middle = line->s2()->stringProperty("texturemiddle");
				string lower = line->s2()->stringProperty("texturebottom");

				// Upper
				if (upper != "-" && texman->getTexture(upper, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_UPPER);
				}

				// Middle
				if (middle != "-" && texman->getTexture(middle, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_MIDDLE);
				}

				// Lower
				if (lower != "-" && texman->getTexture(lower, mixed) == &(GLTexture::missingTex()))
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_LOWER);
				}
			}
		}
	}

	unsigned nProblems()
	{
		return lines.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= lines.size())
			return "No unknown wall textures found";

		string line = S_FMT("Line %d has unknown ", lines[index]->getIndex());
		switch (parts[index])
		{
		case TEX_FRONT_UPPER:
			line += S_FMT("front upper texture \"%s\"", lines[index]->s1()->stringProperty("texturetop"));
			break;
		case TEX_FRONT_MIDDLE:
			line += S_FMT("front middle texture \"%s\"", lines[index]->s1()->stringProperty("texturemiddle"));
			break;
		case TEX_FRONT_LOWER:
			line += S_FMT("front lower texture \"%s\"", lines[index]->s1()->stringProperty("texturebottom"));
			break;
		case TEX_BACK_UPPER:
			line += S_FMT("back upper texture \"%s\"", lines[index]->s2()->stringProperty("texturetop"));
			break;
		case TEX_BACK_MIDDLE:
			line += S_FMT("back middle texture \"%s\"", lines[index]->s2()->stringProperty("texturemiddle"));
			break;
		case TEX_BACK_LOWER:
			line += S_FMT("back lower texture \"%s\"", lines[index]->s2()->stringProperty("texturebottom"));
			break;
		default: break;
		}

		return line;
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(MapEditor::windowWx(), 0, "-", map);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				string texture = browser.getSelectedItem()->getName();
				editor->beginUndoRecord("Change Texture", true, false, false);
				switch (parts[index])
				{
				case TEX_FRONT_UPPER:
					lines[index]->setStringProperty("side1.texturetop", texture);
					break;
				case TEX_FRONT_MIDDLE:
					lines[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case TEX_FRONT_LOWER:
					lines[index]->setStringProperty("side1.texturebottom", texture);
					break;
				case TEX_BACK_UPPER:
					lines[index]->setStringProperty("side2.texturetop", texture);
					break;
				case TEX_BACK_MIDDLE:
					lines[index]->setStringProperty("side2.texturemiddle", texture);
					break;
				case TEX_BACK_LOWER:
					lines[index]->setStringProperty("side2.texturebottom", texture);
					break;
				default:
					return false;
				}

				editor->endUndoRecord();

				// Remove problem
				lines.erase(lines.begin() + index);
				parts.erase(parts.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= lines.size())
			return NULL;

		return lines[index];
	}

	string progressText()
	{
		return "Checking for unknown wall textures...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}
};


/*******************************************************************
 * UNKNOWNFLATSCHECK CLASS
 *******************************************************************
 * Checks for any unknown/invalid flats on sectors
 */
class UnknownFlatsCheck : public MapCheck
{
private:
	MapTextureManager*	texman;
	vector<MapSector*>	sectors;
	vector<bool>		floor;

public:
	UnknownFlatsCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map)
	{
		this->texman = texman;
	}

	virtual void doCheck()
	{
		bool mixed = theGameConfiguration->mixTexFlats();

		// Go through sectors
		for (unsigned a = 0; a < map->nSectors(); a++)
		{
			// Check floor texture
			if (texman->getFlat(map->getSector(a)->getFloorTex(), mixed) == &(GLTexture::missingTex()))
			{
				sectors.push_back(map->getSector(a));
				floor.push_back(true);
			}

			// Check ceiling texture
			if (texman->getFlat(map->getSector(a)->getCeilingTex(), mixed) == &(GLTexture::missingTex()))
			{
				sectors.push_back(map->getSector(a));
				floor.push_back(false);
			}
		}
	}

	virtual unsigned nProblems()
	{
		return sectors.size();
	}

	virtual string problemDesc(unsigned index)
	{
		if (index >= sectors.size())
			return "No unknown flats found";

		MapSector* sector = sectors[index];
		if (floor[index])
			return S_FMT("Sector %d has unknown floor texture \"%s\"", sector->getIndex(), sector->getFloorTex());
		else
			return S_FMT("Sector %d has unknown ceiling texture \"%s\"", sector->getIndex(), sector->getCeilingTex());
	}

	virtual bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= sectors.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(MapEditor::windowWx(), 1, "", map);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				string texture = browser.getSelectedItem()->getName();
				editor->beginUndoRecord("Change Texture");
				if (floor[index])
					sectors[index]->setStringProperty("texturefloor", texture);
				else
					sectors[index]->setStringProperty("textureceiling", texture);

				editor->endUndoRecord();

				// Remove problem
				sectors.erase(sectors.begin() + index);
				floor.erase(floor.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= sectors.size())
			return NULL;

		return sectors[index];
	}

	string progressText()
	{
		return "Checking for unknown flats...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}
};


/*******************************************************************
 * UNKNOWNTHINGTYPESCHECK CLASS
 *******************************************************************
 * Checks for any things with an unknown type
 */
class UnknownThingTypesCheck : public MapCheck
{
private:
	vector<MapThing*>	things;

public:
	UnknownThingTypesCheck(SLADEMap* map) : MapCheck(map) {}

	virtual void doCheck()
	{
		for (unsigned a = 0; a < map->nThings(); a++)
		{
			ThingType* tt = theGameConfiguration->thingType(map->getThing(a)->getType());
			if (tt->getName() == "Unknown")
				things.push_back(map->getThing(a));
		}
	}

	virtual unsigned nProblems()
	{
		return things.size();
	}

	virtual string problemDesc(unsigned index)
	{
		if (index >= things.size())
			return "No unknown thing types found";

		return S_FMT("Thing %d has unknown type %d", things[index]->getIndex(), things[index]->getType());
	}

	virtual bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= things.size())
			return false;

		if (fix_type == 0)
		{
			ThingTypeBrowser browser(MapEditor::windowWx());
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Thing Type");
				things[index]->setIntProperty("type", browser.getSelectedType());
				things.erase(things.begin() + index);
				editor->endUndoRecord();

				return true;
			}
		}

		return false;
	}

	virtual MapObject* getObject(unsigned index)
	{
		if (index >= things.size())
			return NULL;

		return things[index];
	}

	virtual string progressText()
	{
		return "Checking for unknown thing types...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Browse Type...";

		return "";
	}
};


/*******************************************************************
 * STUCKTHINGSCHECK CLASS
 *******************************************************************
 * Checks for any missing things that are stuck inside (overlapping)
 * solid lines
 */
class StuckThingsCheck : public MapCheck
{
private:
	vector<MapLine*> lines;
	vector<MapThing*> things;

public:
	StuckThingsCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		double radius;

		// Get list of lines to check
		vector<MapLine*> check_lines;
		MapLine* line;
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			line = map->getLine(a);

			// Skip if line is 2-sided and not blocking
			if (line->s2() && !theGameConfiguration->lineBasicFlagSet("blocking", line, map->currentFormat()))
				continue;

			check_lines.push_back(line);
		}

		// Go through things
		for (unsigned a = 0; a < map->nThings(); a++)
		{
			MapThing* thing = map->getThing(a);
			ThingType* tt = theGameConfiguration->thingType(thing->getType());

			// Skip if not a solid thing
			if (!tt->isSolid())
				continue;

			radius = tt->getRadius() - 1;
			frect_t bbox(thing->xPos(), thing->yPos(), radius * 2, radius * 2, 1);

			// Go through lines
			for (unsigned b = 0; b < check_lines.size(); b++)
			{
				line = check_lines[b];

				// Check intersection
				if (MathStuff::boxLineIntersect(bbox, line->seg()))
				{
					things.push_back(thing);
					lines.push_back(line);
					break;
				}
			}
		}
	}

	unsigned nProblems()
	{
		return things.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= things.size())
			return "No stuck things found";

		return S_FMT("Thing %d is stuck inside line %d", things[index]->getIndex(), lines[index]->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= things.size())
			return false;

		if (fix_type == 0)
		{
			MapThing* thing = things[index];
			MapLine* line = lines[index];

			// Get nearest line point to thing
			fpoint2_t np = MathStuff::closestPointOnLine(thing->point(), line->seg());

			// Get distance to move
			double r = theGameConfiguration->thingType(thing->getType())->getRadius();
			double dist = MathStuff::distance(fpoint2_t(), fpoint2_t(r, r));

			editor->beginUndoRecord("Move Thing", true, false, false);

			// Move along line direction
			map->moveThing(thing->getIndex(), np.x - (line->frontVector().x * dist), np.y - (line->frontVector().y * dist));

			editor->endUndoRecord();

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index >= things.size())
			return NULL;

		return things[index];
	}

	string progressText()
	{
		return "Checking for things stuck in lines...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Move Thing";

		return "";
	}
};


/*******************************************************************
 * SECTORREFERENCECHECK CLASS
 *******************************************************************
 * Checks for any possibly incorrect sidedef sector references
 */
class SectorReferenceCheck : public MapCheck
{
private:
	struct sector_ref_t
	{
		MapLine* line;
		bool front;
		MapSector* sector;
		sector_ref_t(MapLine* line, bool front, MapSector* sector)
		{
			this->line = line;
			this->front = front;
			this->sector = sector;
		}
	};
	vector<sector_ref_t> invalid_refs;

public:
	SectorReferenceCheck(SLADEMap* map) : MapCheck(map) {}

	void checkLine(MapLine* line)
	{
		// Get 'correct' sectors
		MapSector* s1 = map->getLineSideSector(line, true);
		MapSector* s2 = map->getLineSideSector(line, false);

		// Check front sector
		if (s1 != line->frontSector())
			invalid_refs.push_back(sector_ref_t(line, true, s1));

		// Check back sector
		if (s2 != line->backSector())
			invalid_refs.push_back(sector_ref_t(line, false, s2));
	}

	void doCheck()
	{
		// Go through map lines
		for (unsigned a = 0; a < map->nLines(); a++)
			checkLine(map->getLine(a));
	}

	unsigned nProblems()
	{
		return invalid_refs.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= invalid_refs.size())
			return "No wrong sector references found";

		string side, sector;
		MapSector* s1 = invalid_refs[index].line->frontSector();
		MapSector* s2 = invalid_refs[index].line->backSector();
		if (invalid_refs[index].front)
		{
			side = "front";
			sector = s1 ? S_FMT("%d", s1->getIndex()) : "(none)";
		}
		else
		{
			side = "back";
			sector = s2 ? S_FMT("%d", s2->getIndex()) : "(none)";
		}

		return S_FMT("Line %d has potentially wrong %s sector %s", invalid_refs[index].line->getIndex(), side, sector);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= invalid_refs.size())
			return false;

		if (fix_type == 0)
		{
			editor->beginUndoRecord("Correct Line Sector");

			// Set sector
			sector_ref_t ref = invalid_refs[index];
			if (ref.sector)
				map->setLineSector(ref.line->getIndex(), ref.sector->getIndex(), ref.front);
			else
			{
				// Remove side if no sector
				if (ref.front && ref.line->s1())
					map->removeSide(ref.line->s1());
				else if (!ref.front && ref.line->s2())
					map->removeSide(ref.line->s2());
			}

			// Flip line if needed
			if (!ref.line->s1() && ref.line->s2())
				ref.line->flip();

			editor->endUndoRecord();

			// Remove problem (and any others for the line)
			for (unsigned a = 0; a < invalid_refs.size(); a++)
			{
				if (invalid_refs[a].line == ref.line)
				{
					invalid_refs.erase(invalid_refs.begin() + a);
					a--;
				}
			}

			// Re-check line
			checkLine(ref.line);

			editor->updateDisplay();

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index < invalid_refs.size())
			return invalid_refs[index].line;

		return NULL;
	}

	string progressText()
	{
		return "Checking sector references...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
		{
			if (index >= invalid_refs.size())
				return "Fix Sector reference";

			MapSector* sector = invalid_refs[index].sector;
			if (sector)
				return S_FMT("Set to Sector #%d", sector->getIndex());
			else
				return S_FMT("Clear Sector");
		}

		return "";
	}
};


/*******************************************************************
 * INVALIDLINECHECK CLASS
 *******************************************************************
 * Checks for any invalid lines (that have no first side)
 */
class InvalidLineCheck : public MapCheck
{
private:
	vector<int>	lines;

public:
	InvalidLineCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		// Go through map lines
		lines.clear();
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			if (!map->getLine(a)->s1())
				lines.push_back(a);
		}
	}

	unsigned nProblems()
	{
		return lines.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= lines.size())
			return "No invalid lines found";

		if (map->getLine(lines[index])->s2())
			return S_FMT("Line %d has no front side", lines[index]);
		else
			return S_FMT("Line %d has no sides", lines[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= lines.size())
			return false;

		MapLine* line = map->getLine(lines[index]);
		if (line->s2())
		{
			// Flip
			if (fix_type == 0)
			{
				line->flip();
				return true;
			}

			// Create sector
			else if (fix_type == 1)
			{
				fpoint2_t pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos.x, pos.y);
				doCheck();
				return true;
			}
		}
		else
		{
			// Delete
			if (fix_type == 0)
			{
				map->removeLine(line);
				doCheck();
				return true;
			}

			// Create sector
			else if (fix_type == 1)
			{
				fpoint2_t pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos.x, pos.y);
				doCheck();
				return true;
			}
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index < lines.size())
			return map->getLine(lines[index]);

		return NULL;
	}

	string progressText()
	{
		return "Checking for invalid lines...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (map->getLine(lines[index])->s2())
		{
			if (fix_type == 0)
				return "Flip line";
			else if (fix_type == 1)
				return "Create sector";
		}
		else
		{
			if (fix_type == 0)
				return "Delete line";
			else if (fix_type == 1)
				return "Create sector";
		}

		return "";
	}
};


/*******************************************************************
 * UNKNOWNSECTORCHECK CLASS
 *******************************************************************
 * Checks for any unknown sector type
 */
class UnknownSectorCheck : public MapCheck
{
private:
	vector<int>	sectors;

public:
	UnknownSectorCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		// Go through map lines
		sectors.clear();
		for (unsigned a = 0; a < map->nSectors(); a++)
		{
			int special = map->getSector(a)->getSpecial();
			int base = theGameConfiguration->baseSectorType(special);
			if (S_CMP(theGameConfiguration->sectorTypeName(special), "Unknown"))
				sectors.push_back(a);
		}
	}

	unsigned nProblems()
	{
		return sectors.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= sectors.size())
			return "No unknown sector types found";

		return S_FMT("Sector %d has no sides", sectors[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= sectors.size())
			return false;

		MapSector* sec = map->getSector(sectors[index]);
		if (fix_type == 0)
		{
			// Try to preserve flags if they exist
			int special = sec->getSpecial();
			int base = theGameConfiguration->baseSectorType(special);
			special &= ~base;
			sec->setIntProperty("special", special);
		}
		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index < sectors.size())
			return map->getSector(sectors[index]);

		return NULL;
	}

	string progressText()
	{
		return "Checking for unknown sector types...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Reset sector type";

		return "";
	}
};


/*******************************************************************
 * UNKNOWNSPECIALCHECK CLASS
 *******************************************************************
 * Checks for any unknown special
 */
class UnknownSpecialCheck : public MapCheck
{
private:
	vector<MapObject *>	objects;

public:
	UnknownSpecialCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		// Go through map lines
		objects.clear();
		for (unsigned a = 0; a < map->nLines(); ++a)
		{
			int special = map->getLine(a)->getSpecial();
			if (S_CMP(theGameConfiguration->actionSpecialName(special), "Unknown"))
				objects.push_back(map->getLine(a));
		}
		// In Hexen or UDMF, go through map things too since they too can have specials
		if (map->currentFormat() == MAP_HEXEN || map->currentFormat() == MAP_UDMF)
		{
			for (unsigned a = 0; a < map->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				ThingType* tt = theGameConfiguration->thingType(map->getThing(a)->getType());
				if (tt->getFlags() & THING_SCRIPT)
					continue;

				// Otherwise, check special
				int special = map->getThing(a)->intProperty("special");
				if (S_CMP(theGameConfiguration->actionSpecialName(special), "Unknown"))
					objects.push_back(map->getThing(a));
			}
		}
	}

	unsigned nProblems()
	{
		return objects.size();
	}

	string problemDesc(unsigned index)
	{
		bool special = (map->currentFormat() == MAP_HEXEN || map->currentFormat() == MAP_UDMF);
		if (index >= objects.size())
			return S_FMT("No unknown %s found", special ? "special" : "line type");

		return S_FMT("%s %d has an unknown %s",
			objects[index]->getObjType() == MOBJ_LINE ? "Line" : "Thing",
			objects[index]->getIndex(), special ? "special" : "type");
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= objects.size())
			return false;

		// Reset
		if (fix_type == 0)
		{
			objects[index]->setIntProperty("special", 0);
			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index < objects.size())
			return objects[index];

		return NULL;
	}

	string progressText()
	{
		return "Checking for unknown specials...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Reset special";

		return "";
	}
};

/*******************************************************************
 * OBSOLETETHINGCHECK CLASS
 *******************************************************************
 * Checks for any things marked as obsolete
 */
class ObsoleteThingCheck : public MapCheck
{
private:
	vector<MapThing*> things;

public:
	ObsoleteThingCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		// Go through map lines
		things.clear();
		for (unsigned a = 0; a < map->nThings(); ++a)
		{
			MapThing* thing = map->getThing(a);
			ThingType* tt = theGameConfiguration->thingType(thing->getType());
			if (tt->getFlags() & THING_OBSOLETE)
				things.push_back(thing);
		}
	}

	unsigned nProblems()
	{
		return things.size();
	}

	string problemDesc(unsigned index)
	{
		bool special = (map->currentFormat() == MAP_HEXEN || map->currentFormat() == MAP_UDMF);
		if (index >= things.size())
			return S_FMT("No obsolete things found");

		return S_FMT("Thing %d is obsolete", things[index]->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor)
	{
		if (index >= things.size())
			return false;

		// Reset
		if (fix_type == 0)
		{
			editor->beginUndoRecord("Delete Thing", false, false, true);
			map->removeThing(things[index]);
			editor->endUndoRecord();
			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index)
	{
		if (index < things.size())
			return things[index];

		return NULL;
	}

	string progressText()
	{
		return "Checking for obsolete things...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Delete thing";

		return "";
	}
};


/*******************************************************************
 * MAPCHECK STATIC FUNCTIONS
 *******************************************************************/

/* MapCheck::missingTextureCheck
 * Returns a new MissingTextureCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::missingTextureCheck(SLADEMap* map)
{
	return new MissingTextureCheck(map);
}

/* MapCheck::specialTagCheck
 * Returns a new SpecialTagsCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::specialTagCheck(SLADEMap* map)
{
	return new SpecialTagsCheck(map);
}

/* MapCheck::intersectingLineCheck
 * Returns a new LinesIntersectCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::intersectingLineCheck(SLADEMap* map)
{
	return new LinesIntersectCheck(map);
}

/* MapCheck::overlappingLineCheck
 * Returns a new LinesOverlapCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::overlappingLineCheck(SLADEMap* map)
{
	return new LinesOverlapCheck(map);
}

/* MapCheck::overlappingThingCheck
 * Returns a new ThingsOverlapCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::overlappingThingCheck(SLADEMap* map)
{
	return new ThingsOverlapCheck(map);
}

/* MapCheck::unknownTextureCheck
 * Returns a new UnknownTexturesCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::unknownTextureCheck(SLADEMap* map, MapTextureManager* texman)
{
	return new UnknownTexturesCheck(map, texman);
}

/* MapCheck::unknownFlatCheck
 * Returns a new UnknownFlatsCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::unknownFlatCheck(SLADEMap* map, MapTextureManager* texman)
{
	return new UnknownFlatsCheck(map, texman);
}

/* MapCheck::unknownThingTypeCheck
 * Returns a new UnknownThingTypesCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::unknownThingTypeCheck(SLADEMap* map)
{
	return new UnknownThingTypesCheck(map);
}

/* MapCheck::stuckThingsCheck
 * Returns a new StuckThingsCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::stuckThingsCheck(SLADEMap* map)
{
	return new StuckThingsCheck(map);
}

/* MapCheck::sectorReferenceCheck
 * Returns a new SectorReferenceCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::sectorReferenceCheck(SLADEMap* map)
{
	return new SectorReferenceCheck(map);
}

/* MapCheck::invalidLineCheck
 * Returns a new InvalidLineCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::invalidLineCheck(SLADEMap* map)
{
	return new InvalidLineCheck(map);
}

/* MapCheck::missingTaggedCheck
 * Returns a new MissingTaggedCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::missingTaggedCheck(SLADEMap* map)
{
	return new MissingTaggedCheck(map);
}

/* MapCheck::unknownSectorCheck
 * Returns a new UnknownSectorCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::unknownSectorCheck(SLADEMap* map)
{
	return new UnknownSectorCheck(map);
}

/* MapCheck::unknownSpecialCheck
 * Returns a new UnknownSpecialCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::unknownSpecialCheck(SLADEMap* map)
{
	return new UnknownSpecialCheck(map);
}

/* MapCheck::obsoleteThingCheck
 * Returns a new ObsoleteThingCheck object for [map]
 *******************************************************************/
MapCheck* MapCheck::obsoleteThingCheck(SLADEMap* map)
{
	return new ObsoleteThingCheck(map);
}
