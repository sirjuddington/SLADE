#pragma once

#include "SLADEMap/Types.h"

namespace slade
{
class MapSpecial;
class PlaneAlignSpecial;

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
	void   processACSScripts(const ArchiveEntry* entry);
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

	void processZDoomSlopes(SLADEMap* map);
	void processEternitySlopes(const SLADEMap* map) const;
	void processSRB2Slopes(const SLADEMap* map) const;
	void processSRB2FOFs(const SLADEMap* map) const;
	void processEDGEClassicSlopes(const SLADEMap* map) const;

	template<SectorSurfaceType> void   applyPlaneAlign(MapLine* line, MapSector* target, MapSector* model_sector) const;
	template<SectorSurfaceType> void   applyLineSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<SectorSurfaceType> void   applySectorTiltThing(SLADEMap* map, MapThing* thing) const;
	template<SectorSurfaceType> void   applyVavoomSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<SectorSurfaceType> double vertexHeight(MapVertex* vertex, MapSector* sector) const;
	template<SectorSurfaceType>
	void applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights) const;
	template<SectorSurfaceType>
	void applyRectangularVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights)
		const;
};
} // namespace slade
