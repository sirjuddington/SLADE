#pragma once

#include "SLADEMap/MapLine.h"
#include "SLADEMap/MapSector.h"
#include "SLADEMap/MapThing.h"
#include "common.h"

class SLADEMap;
class ArchiveEntry;

WX_DECLARE_HASH_MAP(MapVertex*, double, wxPointerHash, wxPointerEqual, VertexHeightMap);

class MapSpecials
{
public:
	void reset();

	void processMapSpecials(SLADEMap* map);

	bool tagColour(int tag, ColRGBA* colour);
	bool tagFadeColour(int tag, ColRGBA* colour);
	bool tagColoursSet();
	bool tagFadeColoursSet();
	void updateTaggedSectors(SLADEMap* map);

	// ZDoom
	void processZDoomMapSpecials(SLADEMap* map);
	void processZDoomLineSpecial(MapLine* line);
	void updateZDoomSector(MapSector* line);
	void processACSScripts(ArchiveEntry* entry);
	void setModified(SLADEMap* map, int tag);

private:
	struct SectorColour
	{
		int     tag;
		ColRGBA colour;
	};

	vector<SectorColour> sector_colours_;
	vector<SectorColour> sector_fadecolours_;

	void                                    processZDoomSlopes(SLADEMap* map);
	void                                    processEternitySlopes(SLADEMap* map);
	template<MapSector::SurfaceType> void   applyPlaneAlign(MapLine* line, MapSector* sector, MapSector* model_sector);
	template<MapSector::SurfaceType> void   applyLineSlopeThing(SLADEMap* map, MapThing* thing);
	template<MapSector::SurfaceType> void   applySectorTiltThing(SLADEMap* map, MapThing* thing);
	template<MapSector::SurfaceType> void   applyVavoomSlopeThing(SLADEMap* map, MapThing* thing);
	template<MapSector::SurfaceType> double vertexHeight(MapVertex* vertex, MapSector* sector);
	template<MapSector::SurfaceType>
	void applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights);
};
