#pragma once

#include "SLADEMap/MapObject/MapSector.h"

namespace slade
{
class MapVertex;
class MapThing;
class MapLine;
class SLADEMap;
class ArchiveEntry;

class MapSpecials
{
public:
	void reset();

	void processMapSpecials(SLADEMap* map);
	void processLineSpecial(MapLine* line);

	bool tagColour(int tag, ColRGBA* colour) const;
	bool tagFadeColour(int tag, ColRGBA* colour) const;
	bool tagColoursSet() const;
	bool tagFadeColoursSet() const;
	void updateTaggedSectors(const SLADEMap* map) const;

	// ZDoom
	void   processZDoomMapSpecials(SLADEMap* map);
	void   processZDoomLineSpecial(MapLine* line);
	void   processACSScripts(ArchiveEntry* entry);
	void   setModified(const SLADEMap* map, int tag) const;
	bool   lineIsTranslucent(const MapLine* line) const;
	double translucentLineAlpha(const MapLine* line) const;
	bool   translucentLineAdditive(const MapLine* line) const;

private:
	struct SectorColour
	{
		int     tag;
		ColRGBA colour;
	};

	struct TranslucentLine
	{
		MapLine* line;
		double   alpha;
		bool     additive;
	};

	typedef std::map<MapVertex*, double> VertexHeightMap;

	vector<SectorColour> sector_colours_;
	vector<SectorColour> sector_fadecolours_;

	vector<TranslucentLine> translucent_lines_;

	void processZDoomSlopes(SLADEMap* map) const;
	void processEternitySlopes(const SLADEMap* map) const;

	void processSRB2Slopes(const SLADEMap* map) const;
	void processSRB2FOFs(const SLADEMap* map) const;
	void processEDGEClassicSlopes(SLADEMap* map) const;

	template<MapSector::SurfaceType>
	void applyPlaneAlign(MapLine* line, MapSector* target, MapSector* model_sector) const;
	template<MapSector::SurfaceType> void   applyLineSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType> void   applySectorTiltThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType> void   applyVavoomSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType> double vertexHeight(MapVertex* vertex, MapSector* sector) const;
	template<MapSector::SurfaceType>
	void applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights) const;
	template<MapSector::SurfaceType>
	void applyRectangularVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights) const;
};
} // namespace slade
