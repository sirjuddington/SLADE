#include "common.h"
#include "SLADEMap/MapLine.h"
#include "SLADEMap/MapSector.h"
#include "SLADEMap/MapThing.h"

#ifndef __MAP_SPECIALS_H__
#define __MAP_SPECIALS_H__

class SLADEMap;
class ArchiveEntry;

WX_DECLARE_HASH_MAP(MapVertex*, double, wxPointerHash, wxPointerEqual, VertexHeightMap);

class MapSpecials
{
	struct sector_colour_t
	{
		int		tag;
		rgba_t	colour;
	};

	vector<sector_colour_t> sector_colours;
	vector<sector_colour_t> sector_fadecolours;

	void	processZDoomSlopes(SLADEMap* map);
	void  processEternitySlopes(SLADEMap* map);
	template<MapSector::Surface>
	void	applyPlaneAlign(MapLine* line, MapSector* sector, MapSector* model_sector);
	template<MapSector::Surface>
	void	applyLineSlopeThing(SLADEMap* map, MapThing* thing);
	template<MapSector::Surface>
	void	applySectorTiltThing(SLADEMap* map, MapThing* thing);
	template<MapSector::Surface>
	void	applyVavoomSlopeThing(SLADEMap* map, MapThing* thing);
	template<MapSector::Surface>
	double	vertexHeight(MapVertex* vertex, MapSector* sector);
	template<MapSector::Surface>
	void	applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights);

public:
	void	reset();

	void	processMapSpecials(SLADEMap* map);
	void	processLineSpecial(MapLine* line);

	bool	getTagColour(int tag, rgba_t* colour);
	bool	getTagFadeColour(int tag, rgba_t *colour);
	bool	tagColoursSet();
	bool	tagFadeColoursSet();
	void	updateTaggedSectors(SLADEMap* map);

	// ZDoom
	void	processZDoomMapSpecials(SLADEMap* map);
	void	processZDoomLineSpecial(MapLine* line);
	void	updateZDoomSector(MapSector* line);
	void	processACSScripts(ArchiveEntry* entry);
	void	setModified(SLADEMap *map, int tag);
};

#endif//__MAP_SPECIALS_H__
