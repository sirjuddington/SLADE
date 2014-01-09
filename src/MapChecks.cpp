
#include "Main.h"
#include "WxStuff.h"
#include "SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration.h"
#include "MapTextureManager.h"
#include "MapTextureBrowser.h"
#include "MapEditorWindow.h"
#include "ThingTypeBrowser.h"
#include "MathStuff.h"
#include <SFML/System.hpp>


class MissingTextureCheck : public MapCheck
{
private:
	vector<MapLine*>	lines;
	vector<int>			parts;

public:
	MissingTextureCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck()
	{
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			// Check what textures the line needs
			MapLine* line = map->getLine(a);
			MapSide* side1 = line->s1();
			MapSide* side2 = line->s2();
			int needs = line->needsTexture();

			// Check for missing textures (front side)
			if (side1)
			{
				// Upper
				if (needs == TEX_FRONT_UPPER && side1->stringProperty("texturetop") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_UPPER);
				}

				// Middle
				if (needs == TEX_FRONT_MIDDLE && side1->stringProperty("texturemiddle") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_MIDDLE);
				}

				// Lower
				if (needs == TEX_FRONT_LOWER && side1->stringProperty("texturebottom") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_FRONT_LOWER);
				}
			}

			// Check for missing textures (back side)
			if (side2)
			{
				// Upper
				if (needs == TEX_BACK_UPPER && side2->stringProperty("texturetop") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_UPPER);
				}

				// Middle
				if (needs == TEX_BACK_MIDDLE && side2->stringProperty("texturemiddle") == "-")
				{
					lines.push_back(line);
					parts.push_back(TEX_BACK_MIDDLE);
				}

				// Lower
				if (needs == TEX_BACK_LOWER && side2->stringProperty("texturebottom") == "-")
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(theMapEditor, 0, "-", map);
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

class SpecialTagsCheck : public MapCheck
{
private:
	vector<MapLine*>	lines;

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

			// Check if tag is required but not set
			if (tagged != AS_TT_NO && tagged != AS_TT_SECTOR_BACK && tagged != AS_TT_SECTOR_OR_BACK && tag == 0)
				lines.push_back(map->getLine(a));
		}
	}

	unsigned nProblems()
	{
		return lines.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= lines.size())
			return "No missing special tags found";

		int special = lines[index]->getSpecial();
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("Line %d: Special %d (%s) requires a tag", lines[index]->getIndex(), special, as->getName());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		// Begin tag edit
		theApp->doAction("mapw_line_tagedit");

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
		return "Checking for missing special tags...";
	}

	string fixText(unsigned fix_type, unsigned index)
	{
		if (fix_type == 0)
			return "Set Tagged...";

		return "";
	}
};

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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
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
			map->splitLine(line1->getIndex(), nv->getIndex());
			MapLine* nl1 = map->getLine(map->nLines() - 1);

			// Split second line
			map->splitLine(line2->getIndex(), nv->getIndex());
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
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
			int min_skill = udmf_zdoom ? 1 : 2;
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

				// Case #2: different game modes (single, coop, dm)
				shareflag = false;
				if (theGameConfiguration->thingBasicFlagSet("coop", thing1, map_format) && 
					theGameConfiguration->thingBasicFlagSet("coop", thing2, map_format))
				{
					shareflag = true;
				}
				if (!shareflag && 
					theGameConfiguration->thingBasicFlagSet("dm", thing1, map_format) && 
					theGameConfiguration->thingBasicFlagSet("dm", thing2, map_format))
				{
					shareflag = true;
				}
				if (!shareflag && 
					theGameConfiguration->thingBasicFlagSet("single", thing1, map_format) && 
					theGameConfiguration->thingBasicFlagSet("single", thing2, map_format))
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
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
			S_FMT("front lower texture \"%s\"", lines[index]->s1()->stringProperty("texturebottom"));
			break;
		case TEX_BACK_UPPER:
			S_FMT("back upper texture \"%s\"", lines[index]->s2()->stringProperty("texturetop"));
			break;
		case TEX_BACK_MIDDLE:
			S_FMT("back middle texture \"%s\"", lines[index]->s2()->stringProperty("texturemiddle"));
			break;
		case TEX_BACK_LOWER:
			S_FMT("back lower texture \"%s\"", lines[index]->s2()->stringProperty("texturebottom"));
			break;
		default: break;
		}

		return line;
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(theMapEditor, 0, "-", map);
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

	virtual bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= sectors.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(theMapEditor, 1, "", map);
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

	virtual bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= things.size())
			return false;

		if (fix_type == 0)
		{
			ThingTypeBrowser browser(theMapEditor);
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

			// Go through lines
			for (unsigned b = 0; b < check_lines.size(); b++)
			{
				line = check_lines[b];

				// Check intersection
				if (MathStuff::boxLineIntersect(thing->xPos() - radius, thing->yPos() - radius,
					thing->xPos() + radius, thing->yPos() + radius,
					line->x1(), line->y1(), line->x2(), line->y2()))
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= things.size())
			return false;

		if (fix_type == 0)
		{
			MapThing* thing = things[index];
			MapLine* line = lines[index];

			// Get nearest line point to thing
			fpoint2_t np = MathStuff::closestPointOnLine(thing->xPos(), thing->yPos(), line->x1(), line->y1(), line->x2(), line->y2());

			// Get distance to move
			double r = theGameConfiguration->thingType(thing->getType())->getRadius();
			double dist = MathStuff::distance(0, 0, r, r);

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

		return S_FMT("Line %d has potentially wrong %s sector %s", invalid_refs[index].line->getIndex(), CHR(side), CHR(sector));
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
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

MapCheck* MapCheck::missingTextureCheck(SLADEMap* map)
{
	return new MissingTextureCheck(map);
}

MapCheck* MapCheck::specialTagCheck(SLADEMap* map)
{
	return new SpecialTagsCheck(map);
}

MapCheck* MapCheck::intersectingLineCheck(SLADEMap* map)
{
	return new LinesIntersectCheck(map);
}

MapCheck* MapCheck::overlappingLineCheck(SLADEMap* map)
{
	return new LinesOverlapCheck(map);
}

MapCheck* MapCheck::overlappingThingCheck(SLADEMap* map)
{
	return new ThingsOverlapCheck(map);
}

MapCheck* MapCheck::unknownTextureCheck(SLADEMap* map, MapTextureManager* texman)
{
	return new UnknownTexturesCheck(map, texman);
}

MapCheck* MapCheck::unknownFlatCheck(SLADEMap* map, MapTextureManager* texman)
{
	return new UnknownFlatsCheck(map, texman);
}

MapCheck* MapCheck::unknownThingTypeCheck(SLADEMap* map)
{
	return new UnknownThingTypesCheck(map);
}

MapCheck* MapCheck::stuckThingsCheck(SLADEMap* map)
{
	return new StuckThingsCheck(map);
}

MapCheck* MapCheck::sectorReferenceCheck(SLADEMap* map)
{
	return new SectorReferenceCheck(map);
}
