
#include "Main.h"
#include "SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration.h"

vector<MapChecks::missing_tex_t> MapChecks::checkMissingTextures(SLADEMap* map)
{
	vector<MapChecks::missing_tex_t> missing;

	for (unsigned a = 0; a < map->nLines(); a++)
	{
		// Check what textures the line needs
		MapLine* line = map->getLine(a);
		MapSide* side1 = line->s1();
		MapSide* side2 = line->s2();
		int needs = line->needsTexture();

		// Check for missing textures
		if (side1)
		{
			if (needs == TEX_FRONT_UPPER && side1->stringProperty("texturetop") == "-")
				missing.push_back(missing_tex_t(line, TEX_FRONT_UPPER));
			if (needs == TEX_FRONT_MIDDLE && side1->stringProperty("texturemiddle") == "-")
				missing.push_back(missing_tex_t(line, TEX_FRONT_MIDDLE));
			if (needs == TEX_FRONT_LOWER && side1->stringProperty("texturebottom") == "-")
				missing.push_back(missing_tex_t(line, TEX_FRONT_LOWER));
		}
		if (side2)
		{
			if (needs == TEX_BACK_UPPER && side2->stringProperty("texturetop") == "-")
				missing.push_back(missing_tex_t(line, TEX_BACK_UPPER));
			if (needs == TEX_BACK_MIDDLE && side2->stringProperty("texturemiddle") == "-")
				missing.push_back(missing_tex_t(line, TEX_BACK_MIDDLE));
			if (needs == TEX_BACK_LOWER && side2->stringProperty("texturebottom") == "-")
				missing.push_back(missing_tex_t(line, TEX_BACK_LOWER));
		}
	}

	return missing;
}

vector<MapLine*> MapChecks::checkSpecialTags(SLADEMap* map)
{
	vector<MapLine*> lines;

	for (unsigned a = 0; a < map->nLines(); a++)
	{
		int special = map->getLine(a)->intProperty("special");
		int tag = map->getLine(a)->intProperty("arg0");

		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		int tagged = as->needsTag();

		if (tagged != AS_TT_NO && tagged != AS_TT_SECTOR_BACK && tagged != AS_TT_SECTOR_OR_BACK && tag == 0)
			lines.push_back(map->getLine(a));
	}

	return lines;
}

vector<MapChecks::intersect_line_t> MapChecks::checkIntersectingLines(SLADEMap* map)
{
	vector<intersect_line_t> lines;

	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line1 = map->getLine(a);
		for (unsigned b = a + 1; b < map->nLines(); b++)
		{
			MapLine* line2 = map->getLine(b);
			if (map->linesIntersect(line1, line2))
				lines.push_back(intersect_line_t(line1, line2));
		}
	}

	return lines;
}

vector<MapChecks::intersect_line_t> MapChecks::checkOverlappingLines(SLADEMap* map)
{
	vector<intersect_line_t> lines;

	for (unsigned a = 0; a < map->nLines(); a++)
	{
		MapLine* line1 = map->getLine(a);
		for (unsigned b = a + 1; b < map->nLines(); b++)
		{
			MapLine* line2 = map->getLine(b);
			if ((line1->v1() == line2->v1() && line1->v2() == line2->v2()) ||
				(line1->v2() == line2->v1() && line1->v1() == line2->v2()))
				lines.push_back(intersect_line_t(line1, line2));
		}
	}

	return lines;
}
