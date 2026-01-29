#pragma once

#include "Geometry/BBox.h"
#include "MapObjectCollection.h"
#include "Utility/PropertyList.h"

namespace slade
{
struct MapDesc;
enum class MapFormat;
namespace game
{
	enum class TagType;
}
namespace map
{
	class MapSpecials;
}

class SLADEMap
{
public:
	// Map entry ordering
	enum MapEntries
	{
		THINGS = 0,
		LINEDEFS,
		SIDEDEFS,
		VERTEXES,
		SECTORS
	};

	SLADEMap();
	SLADEMap(const SLADEMap& copy) = delete;
	~SLADEMap();

	string                     mapName() const { return name_; }
	string                     udmfNamespace() const { return udmf_namespace_; }
	MapFormat                  currentFormat() const { return current_format_; }
	long                       geometryUpdated() const { return geometry_updated_; }
	long                       thingsUpdated() const { return things_updated_; }
	long                       sectorRenderInfoUpdated() const { return sector_renderinfo_updated_; }
	long                       typeLastUpdated(map::ObjectType type) const;
	const MapObjectCollection& mapData() const { return data_; }
	bool                       isOpen() const { return is_open_; }

	void setGeometryUpdated();
	void setThingsUpdated();
	void setSectorRenderInfoUpdated();
	void setTypeUpdated(map::ObjectType type);

	// MapObject access
	MapVertex*        vertex(unsigned index) const;
	MapSide*          side(unsigned index) const;
	MapLine*          line(unsigned index) const;
	MapSector*        sector(unsigned index) const;
	MapThing*         thing(unsigned index) const;
	MapObject*        object(map::ObjectType type, unsigned index) const;
	size_t            nVertices() const;
	size_t            nLines() const;
	size_t            nSides() const;
	size_t            nSectors() const;
	size_t            nThings() const;
	const VertexList& vertices() const { return data_.vertices(); }
	const LineList&   lines() const { return data_.lines(); }
	const SideList&   sides() const { return data_.sides(); }
	const SectorList& sectors() const { return data_.sectors(); }
	const ThingList&  things() const { return data_.things(); }

	vector<ArchiveEntry*>& udmfExtraEntries() { return udmf_extra_entries_; }

	bool readMap(const MapDesc& map);
	void clearMap();

	map::MapSpecials& mapSpecials() const { return *map_specials_; }
	void              recomputeSpecials() const;

	// Map saving
	bool writeMap(vector<ArchiveEntry*>& map_entries) const;

	// Creation
	MapVertex* createVertex(Vec2d pos, double split_dist = -1.);
	MapLine*   createLine(Vec2d p1, Vec2d p2, double split_dist = -1.);
	MapLine*   createLine(MapVertex* vertex1, MapVertex* vertex2, bool force = false);
	MapThing*  createThing(Vec2d pos, int type = 1);
	MapSector* createSector();
	MapSide*   createSide(MapSector* sector);

	// Removal
	bool removeVertex(const MapVertex* vertex, bool merge_lines = false)
	{
		return data_.removeVertex(vertex, merge_lines);
	}
	bool removeVertex(unsigned index, bool merge_lines = false) { return data_.removeVertex(index, merge_lines); }
	bool removeLine(const MapLine* line) { return data_.removeLine(line); }
	bool removeLine(unsigned index) { return data_.removeLine(index); }
	bool removeSide(const MapSide* side, bool remove_from_line = true)
	{
		return data_.removeSide(side, remove_from_line);
	}
	bool removeSide(unsigned index, bool remove_from_line = true) { return data_.removeSide(index, remove_from_line); }
	bool removeSector(const MapSector* sector) { return data_.removeSector(sector); }
	bool removeSector(unsigned index) { return data_.removeSector(index); }
	bool removeThing(const MapThing* thing) { return data_.removeThing(thing); }
	bool removeThing(unsigned index) { return data_.removeThing(index); }
	int  removeDetachedVertices() { return data_.removeDetachedVertices(); }

	// Geometry
	BBox     bounds(bool include_things = true);
	void     updateGeometryInfo(long modified_time);
	MapLine* lineVectorIntersect(MapLine* line, bool front, double& hit_x, double& hit_y) const;

	// Tags/Ids
	void putThingsWithIdInSectorTag(int id, int tag, vector<MapThing*>& list);
	void putDragonTargets(MapThing* first, vector<MapThing*>& list);

	// Info
	string     adjacentLineTexture(const MapVertex* vertex, int tex_part = 255) const;
	MapSector* lineSideSector(MapLine* line, bool front = true);
	bool       isModified() const;
	void       setOpenedTime();

	// Editing
	void       mergeVertices(unsigned vertex1, unsigned vertex2);
	MapVertex* mergeVerticesPoint(const Vec2d& pos);
	MapLine*   splitLine(MapLine* line, MapVertex* vertex);
	void       splitLinesAt(MapVertex* vertex, double split_dist = 0);
	bool       setLineSector(unsigned line_index, unsigned sector_index, bool front = true);
	int        mergeLine(unsigned index);
	bool       correctLineSectors(MapLine* line);
	void       setLineSide(MapLine* line, MapSide* side, bool front);

	// Merge
	bool     mergeArch(const vector<MapVertex*>& vertices);
	MapLine* mergeOverlappingLines(MapLine* line1, MapLine* line2);
	void     correctSectors(vector<MapLine*> lines, bool existing_only = false);

	// Checks
	void mapOpenChecks();

	// Misc. map data access
	void rebuildConnectedLines() const { data_.rebuildConnectedLines(); }
	void rebuildConnectedSides() const { data_.rebuildConnectedSides(); }
	void restoreObjectIdList(map::ObjectType type, const vector<unsigned>& list)
	{
		data_.restoreObjectIdList(type, list);
	}

	// Convert
	bool convertToHexen() const;
	bool convertToUDMF();

	// Usage counts
	void clearThingTypeUsage() { usage_thing_type_.clear(); }
	void updateThingTypeUsage(int type, int adjust);
	int  thingTypeUsageCount(int type);

private:
	MapObjectCollection          data_;
	string                       udmf_namespace_;
	PropertyList                 udmf_props_;
	string                       name_;
	MapFormat                    current_format_;
	long                         opened_time_ = 0;
	unique_ptr<map::MapSpecials> map_specials_;
	bool                         is_open_ = false;

	vector<ArchiveEntry*> udmf_extra_entries_; // UDMF Extras

	std::array<long, 6> type_modified_times_;           // The last modified time of each object type
	long                geometry_updated_          = 0; // The last time the map geometry was updated
	long                things_updated_            = 0; // The last time the thing list was modified
	long                sector_renderinfo_updated_ = 0; // The last time any sector render info was updated

	// Usage counts
	std::map<int, int> usage_thing_type_;
};
} // namespace slade
