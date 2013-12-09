
#ifndef __MAP_CHECKS_H__
#define __MAP_CHECKS_H__

class MapLine;
class MapThing;
class MapSector;
class MapTextureManager;
namespace MapChecks
{
	struct missing_tex_t
	{
		MapLine*	line;
		int			part;
		missing_tex_t(MapLine* line, int part) { this->line = line; this->part = part; }
	};

	struct intersect_line_t
	{
		MapLine* line1;
		MapLine* line2;
		intersect_line_t(MapLine* line1, MapLine* line2)
		{
			this->line1 = line1;
			this->line2 = line2;
		}
	};

	struct overlap_thing_t
	{
		MapThing*	thing1;
		MapThing*	thing2;
		overlap_thing_t(MapThing* thing1, MapThing* thing2)
		{
			this->thing1 = thing1;
			this->thing2 = thing2;
		}
	};

	struct unknown_ftex_t
	{
		MapSector*	sector;
		bool		floor;
		unknown_ftex_t(MapSector* sector, bool floor)
		{
			this->sector = sector;
			this->floor = floor;
		}
	};

	vector<missing_tex_t>		checkMissingTextures(SLADEMap* map);
	vector<MapLine*>			checkSpecialTags(SLADEMap* map);
	vector<intersect_line_t>	checkIntersectingLines(SLADEMap* map);
	vector<intersect_line_t>	checkOverlappingLines(SLADEMap* map);
	vector<overlap_thing_t>		checkOverlappingThings(SLADEMap* map);
	vector<missing_tex_t>		checkUnknownTextures(SLADEMap* map, MapTextureManager* texman);
	vector<unknown_ftex_t>		checkUnknownFlats(SLADEMap* map, MapTextureManager* texman);
}

#endif//__MAP_CHECKS_H__
