
#ifndef __MAP_SPECIALS_H__
#define __MAP_SPECIALS_H__

class SLADEMap;
class ArchiveEntry;

class MapSpecials
{
	struct sector_colour_t
	{
		int		tag;
		rgba_t	colour;
	};

	vector<sector_colour_t> sector_colours;

	void	processZDoomSlopes(SLADEMap* map);
	template<PlaneType>
	void	applyPlaneAlign(MapLine* line, MapSector* sector, MapSector* model_sector);
	template<PlaneType>
	void	applyLineSlopeThing(SLADEMap* map, MapThing* thing);
	template<PlaneType>
	void	applySectorTiltThing(SLADEMap* map, MapThing* thing);
	template<PlaneType>
	void	applyVavoomSlopeThing(SLADEMap* map, MapThing* thing);
	template<PlaneType>
	void	applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices);

public:
	void	reset();

	void	processMapSpecials(SLADEMap* map);
	void	processLineSpecial(MapLine* line);

	bool	getTagColour(int tag, rgba_t* colour);
	bool	tagColoursSet();
	void	updateTaggedSectors(SLADEMap* map);

	// ZDoom
	void	processZDoomMapSpecials(SLADEMap* map);
	void	processZDoomLineSpecial(MapLine* line);
	void	updateZDoomSector(MapSector* line);
	void	processACSScripts(ArchiveEntry* entry);
};

#endif//__MAP_SPECIALS_H__
