
#include "Main.h"
#include "SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration.h"
#include "MapTextureManager.h"

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
			return S_FMT("Line %d missing %s texture", lines[index]->getIndex(), texName(parts[index]));
		else
			return "";
	}

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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
			return "";

		int special = lines[index]->getSpecial();
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("Line %d: Special %d (%s) requires a tag", lines[index]->getIndex(), special, as->getName());
	}

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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

	void doCheck()
	{
		double x, y;
		MapLine* line1;
		MapLine* line2;

		// Go through lines
		for (unsigned a = 0; a < map->nLines(); a++)
		{
			line1 = map->getLine(a);

			// Go through uncompared lines
			for (unsigned b = a + 1; b < map->nLines(); b++)
			{
				line2 = map->getLine(b);

				// Check intersection
				if (map->linesIntersect(line1, line2, x, y))
					intersections.push_back(line_intersect_t(line1, line2, x, y));
			}
		}
	}

	unsigned nProblems()
	{
		return intersections.size();
	}

	string problemDesc(unsigned index)
	{
		if (index >= intersections.size())
			return "";

		return S_FMT("Lines %d and %d are intersecting at (%1.2f, %1.2f)",
			intersections[index].line1->getIndex(), intersections[index].line2->getIndex(),
			intersections[index].intersect_point.x, intersections[index].intersect_point.y);
	}

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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
			return "";

		return S_FMT("Lines %d and %d are overlapping", overlaps[index].line1->getIndex(), overlaps[index].line2->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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
		double r1, r2;

		// Go through things
		for (unsigned a = 0; a < map->nThings(); a++)
		{
			MapThing* thing1 = map->getThing(a);
			r1 = theGameConfiguration->thingType(thing1->getType())->getRadius() - 1;

			// Ignore if no radius
			if (r1 < 0)
				continue;

			// Go through uncompared things
			for (unsigned b = a + 1; b < map->nThings(); b++)
			{
				MapThing* thing2 = map->getThing(b);
				r2 = theGameConfiguration->thingType(thing2->getType())->getRadius() - 1;

				// Ignore if no radius
				if (r2 < 0)
					continue;

				// TODO: Check flags

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
			return "";

		return S_FMT("Things %d and %d are overlapping", overlaps[index].thing1->getIndex(), overlaps[index].thing2->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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
			return "";

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

	bool fixProblem(unsigned index, unsigned fix_type)
	{
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
			return "";

		MapSector* sector = sectors[index];
		if (floor[index])
			return S_FMT("Sector %d has unknown floor texture \"%s\"", sector->getIndex(), sector->getFloorTex());
		else
			return S_FMT("Sector %d has unknown ceiling texture \"%s\"", sector->getIndex(), sector->getCeilingTex());
	}

	virtual bool fixProblem(unsigned index, unsigned fix_type)
	{
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
			return "";

		return S_FMT("Thing %d has unknown type %d", things[index]->getIndex(), things[index]->getType());
	}

	virtual bool fixProblem(unsigned index, unsigned fix_type)
	{
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
