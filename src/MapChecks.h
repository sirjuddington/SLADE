
#ifndef __MAP_CHECKS_H__
#define __MAP_CHECKS_H__

class MapLine;
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

	vector<missing_tex_t>		checkMissingTextures(SLADEMap* map);
	vector<MapLine*>			checkSpecialTags(SLADEMap* map);
	vector<intersect_line_t>	checkIntersectingLines(SLADEMap* map);
	vector<intersect_line_t>	checkOverlappingLines(SLADEMap* map);
}

#endif//__MAP_CHECKS_H__
