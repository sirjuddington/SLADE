#pragma once

namespace slade
{
class ThingList;
class SectorList;
class LineList;
class SideList;
class VertexList;
namespace map
{
	enum class ObjectType : u8;
}

class MapObjectCollection
{
public:
	MapObjectCollection(SLADEMap* parent_map = nullptr);

	SLADEMap*         parentMap() const { return parent_map_; }
	VertexList&       vertices() { return *vertices_; }
	SideList&         sides() { return *sides_; }
	LineList&         lines() { return *lines_; }
	SectorList&       sectors() { return *sectors_; }
	ThingList&        things() { return *things_; }
	const VertexList& vertices() const { return *vertices_; }
	const SideList&   sides() const { return *sides_; }
	const LineList&   lines() const { return *lines_; }
	const SectorList& sectors() const { return *sectors_; }
	const ThingList&  things() const { return *things_; }

	void setParentMap(SLADEMap* map) { parent_map_ = map; }

	// MapObject id stuff (used for undo/redo)
	void       addMapObject(unique_ptr<MapObject> object);
	void       removeMapObject(const MapObject* object);
	MapObject* getObjectById(unsigned id) const { return objects_[id].object.get(); }
	void       putObjectIdList(map::ObjectType type, vector<unsigned>& list) const;
	void       restoreObjectIdList(map::ObjectType type, const vector<unsigned>& list);

	void refreshIndices() const;
	void clear();

	// Object add
	MapVertex* addVertex(unique_ptr<MapVertex> vertex);
	MapSide*   addSide(unique_ptr<MapSide> side);
	MapLine*   addLine(unique_ptr<MapLine> line);
	MapSector* addSector(unique_ptr<MapSector> sector);
	MapThing*  addThing(unique_ptr<MapThing> thing);

	// Object duplicate
	MapSide* duplicateSide(MapSide* side);

	// Object remove
	bool removeVertex(const MapVertex* vertex, bool merge_lines = false);
	bool removeVertex(unsigned index, bool merge_lines = false);
	bool removeLine(const MapLine* line);
	bool removeLine(unsigned index);
	bool removeSide(const MapSide* side, bool remove_from_line = true);
	bool removeSide(unsigned index, bool remove_from_line = true);
	bool removeSector(const MapSector* sector);
	bool removeSector(unsigned index);
	bool removeThing(const MapThing* thing);
	bool removeThing(unsigned index);

	// Modified times
	vector<MapObject*> modifiedObjects(long since, map::ObjectType type) const;
	vector<MapObject*> allModifiedObjects(long since) const;
	long               lastModifiedTime() const;
	bool               modifiedSince(long since, map::ObjectType type) const;

	// Checks
	int removeDetachedVertices();
	int removeDetachedSides();
	int removeDetachedSectors();
	int removeZeroLengthLines();
	int removeInvalidSides();

	// Cleanup/Extra
	void rebuildConnectedLines() const;
	void rebuildConnectedSides() const;

private:
	struct MapObjectHolder
	{
		unique_ptr<MapObject> object;
		bool                  in_map;

		MapObjectHolder(unique_ptr<MapObject> object, bool in_map);
	};

	SLADEMap*               parent_map_ = nullptr;
	vector<MapObjectHolder> objects_;
	unique_ptr<VertexList>  vertices_;
	unique_ptr<SideList>    sides_;
	unique_ptr<LineList>    lines_;
	unique_ptr<SectorList>  sectors_;
	unique_ptr<ThingList>   things_;
};
} // namespace slade
