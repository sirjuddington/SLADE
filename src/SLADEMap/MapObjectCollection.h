#pragma once

#include "General/Defs.h"
#include "MapObjectList/LineList.h"
#include "MapObjectList/SectorList.h"
#include "MapObjectList/SideList.h"
#include "MapObjectList/ThingList.h"
#include "MapObjectList/VertexList.h"

class MapObjectCollection
{
public:
	MapObjectCollection(SLADEMap* parent_map = nullptr);

	SLADEMap*         parentMap() const { return parent_map_; }
	MapFormat         mapFormat() const { return current_format_; }
	VertexList&       vertices() { return vertices_; }
	SideList&         sides() { return sides_; }
	LineList&         lines() { return lines_; }
	SectorList&       sectors() { return sectors_; }
	ThingList&        things() { return things_; }
	const VertexList& vertices() const { return vertices_; }
	const SideList&   sides() const { return sides_; }
	const LineList&   lines() const { return lines_; }
	const SectorList& sectors() const { return sectors_; }
	const ThingList&  things() const { return things_; }

	void setParentMap(SLADEMap* map) { parent_map_ = map; }

	// MapObject id stuff (used for undo/redo)
	void       addMapObject(unique_ptr<MapObject> object);
	void       removeMapObject(MapObject* object);
	MapObject* getObjectById(unsigned id) const { return objects_[id].object.get(); }
	void       putObjectIdList(MapObject::Type type, vector<unsigned>& list) const;
	void       restoreObjectIdList(MapObject::Type type, vector<unsigned>& list);

	void refreshIndices();
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
	bool removeVertex(MapVertex* vertex, bool merge_lines = false);
	bool removeVertex(unsigned index, bool merge_lines = false);
	bool removeLine(MapLine* line);
	bool removeLine(unsigned index);
	bool removeSide(MapSide* side, bool remove_from_line = true);
	bool removeSide(unsigned index, bool remove_from_line = true);
	bool removeSector(MapSector* sector);
	bool removeSector(unsigned index);
	bool removeThing(MapThing* thing);
	bool removeThing(unsigned index);

	// Modified times
	vector<MapObject*> modifiedObjects(long since, MapObject::Type type) const;
	vector<MapObject*> allModifiedObjects(long since) const;
	long               lastModifiedTime() const;
	bool               modifiedSince(long since, MapObject::Type type) const;

	// Checks
	int removeDetachedVertices();
	int removeDetachedSides();
	int removeDetachedSectors();
	int removeZeroLengthLines();
	int removeInvalidSides();

	// Cleanup/Extra
	void rebuildConnectedLines();
	void rebuildConnectedSides();

private:
	struct MapObjectHolder
	{
		unique_ptr<MapObject> object;
		bool                  in_map;

		MapObjectHolder(unique_ptr<MapObject> object, bool in_map) : object{ std::move(object) }, in_map{ in_map } {}
	};

	SLADEMap* parent_map_     = nullptr;
	MapFormat current_format_ = MapFormat::Doom;
	bool      position_frac_  = false;

	vector<MapObjectHolder> objects_;
	VertexList              vertices_;
	SideList                sides_;
	LineList                lines_;
	SectorList              sectors_;
	ThingList               things_;
};
