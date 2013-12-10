
#include "Main.h"
#include "WxStuff.h"
#include "SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration.h"
#include "MapTextureManager.h"
#include "MapTextureBrowser.h"
#include "MapEditorWindow.h"
#include "ThingTypeBrowser.h"


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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(theMapEditor, 0, "-");
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

	string fixText(unsigned fix_type)
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
			return "";

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

	string fixText(unsigned fix_type)
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
			return "";

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

	string fixText(unsigned fix_type)
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
			return "";

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

	string fixText(unsigned fix_type)
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
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

	bool fixProblem(unsigned index, unsigned fix_type, MapEditor* editor)
	{
		if (index >= lines.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(theMapEditor, 0, "-");
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

	string fixText(unsigned fix_type)
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
			return "";

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
			MapTextureBrowser browser(theMapEditor, 1);
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

	string fixText(unsigned fix_type)
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
			return "";

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

	string fixText(unsigned fix_type)
	{
		if (fix_type == 0)
			return "Browse Type...";

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
