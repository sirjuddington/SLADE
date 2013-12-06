
#ifndef __SLADEMAP_H__
#define __SLADEMAP_H__

#include "MapLine.h"
#include "MapSide.h"
#include "MapSector.h"
#include "MapVertex.h"
#include "MapThing.h"
#include "Archive.h"
#include "PropertyList.h"

struct mobj_holder_t
{
	MapObject*	mobj;
	bool		in_map;

	mobj_holder_t() { mobj = NULL; in_map = false; }
	mobj_holder_t(MapObject* mobj, bool in_map) { this->mobj = mobj; this->in_map = in_map; }

	void set(MapObject* object, bool in_map)
	{
		this->mobj = object;
		this->in_map = in_map;
	}
};

struct mobj_cd_t
{
	unsigned	id;
	bool		created;

	mobj_cd_t(unsigned id, bool created)
	{
		this->id = id;
		this->created = created;
	}
};

class ParseTreeNode;
class SLADEMap
{
	friend class MapEditor;
private:
	vector<MapLine*>	lines;
	vector<MapSide*>	sides;
	vector<MapSector*>	sectors;
	vector<MapVertex*>	vertices;
	vector<MapThing*>	things;
	string				udmf_namespace;
	PropertyList		udmf_props;
	bool				position_frac;
	string				name;
	int					current_format;
	long				opened_time;

	vector<mobj_holder_t>	all_objects;
	vector<unsigned>		deleted_objects;
	vector<unsigned>		created_objects;
	vector<mobj_cd_t>		created_deleted_objects;

	// The last time the map geometry was updated
	long	geometry_updated;

	// Doom format
	bool	addVertex(doomvertex_t& v);
	bool	addSide(doomside_t& s);
	bool	addLine(doomline_t& l);
	bool	addSector(doomsector_t& s);
	bool	addThing(doomthing_t& t);

	bool	readDoomVertexes(ArchiveEntry* entry);
	bool	readDoomSidedefs(ArchiveEntry* entry);
	bool	readDoomLinedefs(ArchiveEntry* entry);
	bool	readDoomSectors(ArchiveEntry* entry);
	bool	readDoomThings(ArchiveEntry* entry);

	bool	writeDoomVertexes(ArchiveEntry* entry);
	bool	writeDoomSidedefs(ArchiveEntry* entry);
	bool	writeDoomLinedefs(ArchiveEntry* entry);
	bool	writeDoomSectors(ArchiveEntry* entry);
	bool	writeDoomThings(ArchiveEntry* entry);

	// Hexen format
	bool	addLine(hexenline_t& l);
	bool	addThing(hexenthing_t& t);

	bool	readHexenLinedefs(ArchiveEntry* entry);
	bool	readHexenThings(ArchiveEntry* entry);

	bool	writeHexenLinedefs(ArchiveEntry* entry);
	bool	writeHexenThings(ArchiveEntry* entry);

	// Doom 64 format
	bool	addVertex(doom64vertex_t& v);
	bool	addSide(doom64side_t& s);
	bool	addLine(doom64line_t& l);
	bool	addSector(doom64sector_t& s);
	bool	addThing(doom64thing_t& t);

	bool	readDoom64Vertexes(ArchiveEntry* entry);
	bool	readDoom64Sidedefs(ArchiveEntry* entry);
	bool	readDoom64Linedefs(ArchiveEntry* entry);
	bool	readDoom64Sectors(ArchiveEntry* entry);
	bool	readDoom64Things(ArchiveEntry* entry);

	bool	writeDoom64Vertexes(ArchiveEntry* entry);
	bool	writeDoom64Sidedefs(ArchiveEntry* entry);
	bool	writeDoom64Linedefs(ArchiveEntry* entry);
	bool	writeDoom64Sectors(ArchiveEntry* entry);
	bool	writeDoom64Things(ArchiveEntry* entry);

	// UDMF
	bool	addVertex(ParseTreeNode* def);
	bool	addSide(ParseTreeNode* def);
	bool	addLine(ParseTreeNode* def);
	bool	addSector(ParseTreeNode* def);
	bool	addThing(ParseTreeNode* def);

public:
	SLADEMap();
	~SLADEMap();

	// Map entry ordering
	enum MapEntries
	{
	    THINGS = 0,
	    LINEDEFS,
	    SIDEDEFS,
	    VERTEXES,
	    SECTORS
	};

	string		mapName() { return name; }
	string		udmfNamespace() { return udmf_namespace; }
	int			currentFormat() { return current_format; }
	MapVertex*	getVertex(unsigned index);
	MapSide*	getSide(unsigned index);
	MapLine*	getLine(unsigned index);
	MapSector*	getSector(unsigned index);
	MapThing*	getThing(unsigned index);
	MapObject*	getObject(uint8_t type, unsigned index);
	size_t		nVertices() { return vertices.size(); }
	size_t		nLines() { return lines.size(); }
	size_t		nSides() { return sides.size(); }
	size_t		nSectors() { return sectors.size(); }
	size_t		nThings() { return things.size(); }
	long		geometryUpdated() { return geometry_updated; }

	// MapObject id stuff (used for undo/redo)
	void				addMapObject(MapObject* object);
	void				removeMapObject(MapObject* object);
	MapObject*			getObjectById(unsigned id) { return all_objects[id].mobj; }
	void				restoreObjectById(unsigned id);
	void				removeObjectById(unsigned id);
	vector<mobj_cd_t>&	createdDeletedObjectIds() { return created_deleted_objects; }
	void				clearCreatedDeletedOjbectIds() { created_deleted_objects.clear(); }

	void	refreshIndices();
	bool	readMap(Archive::mapdesc_t map);
	void	clearMap();

	// Map loading
	bool	readDoomMap(Archive::mapdesc_t map);
	bool	readHexenMap(Archive::mapdesc_t map);
	bool	readDoom64Map(Archive::mapdesc_t map);
	bool	readUDMFMap(Archive::mapdesc_t map);

	// Map saving
	bool	writeDoomMap(vector<ArchiveEntry*>& map_entries);
	bool	writeHexenMap(vector<ArchiveEntry*>& map_entries);
	bool	writeDoom64Map(vector<ArchiveEntry*>& map_entries);
	bool	writeUDMFMap(ArchiveEntry* textmap);

	// Item removal
	bool	removeVertex(MapVertex* vertex);
	bool	removeVertex(unsigned index);
	bool	removeLine(MapLine* line);
	bool	removeLine(unsigned index);
	bool	removeSide(MapSide* side, bool remove_from_line = true);
	bool	removeSide(unsigned index, bool remove_from_line = true);
	bool	removeSector(MapSector* sector);
	bool	removeSector(unsigned index);
	bool	removeThing(MapThing* thing);
	bool	removeThing(unsigned index);

	// Geometry
	int					nearestVertex(double x, double y, double min = 64);
	int					nearestLine(double x, double y, double min = 64);
	int					nearestThing(double x, double y, double min = 64);
	vector<int>			nearestThingMulti(double x, double y);
	int					sectorAt(double x, double y);
	bbox_t				getMapBBox();
	MapVertex*			vertexAt(double x, double y);
	vector<fpoint2_t>	cutLines(double x1, double y1, double x2, double y2);
	MapVertex*			lineCrossVertex(double x1, double y1, double x2, double y2);
	void				updateGeometryInfo(long modified_time);
	bool				linesIntersect(MapLine* line1, MapLine* line2);

	// Tags/Ids
	void	getSectorsByTag(int tag, vector<MapSector*>& list);
	void	getThingsById(int id, vector<MapThing*>& list);
	void	getLinesById(int id, vector<MapLine*>& list);
	void	getThingsByIdInSectorTag(int id, int tag, vector<MapThing*>& list);
	void	getTaggingThingsById(int id, int type, vector<MapThing*>& list);
	void	getTaggingLinesById(int id, int type, vector<MapLine*>& list);
	int		findUnusedSectorTag();
	int		findUnusedThingId();
	int		findUnusedLineId();

	// Info
	string				getAdjacentLineTexture(MapVertex* vertex, int tex_part = 255);
	MapSector*			getLineSideSector(MapLine* line, bool front = true);
	vector<MapObject*>	getModifiedObjects(long since, int type = -1);
	vector<MapObject*>	getAllModifiedObjects(long since);
	long				getLastModifiedTime();
	bool				isModified();
	void				setOpenedTime();

	// Creation
	MapVertex*	createVertex(double x, double y, double split_dist = -1);
	MapLine*	createLine(double x1, double y1, double x2, double y2, double split_dist = -1);
	MapLine*	createLine(MapVertex* vertex1, MapVertex* vertex2, bool force = false);
	MapThing*	createThing(double x, double y);
	MapSector*	createSector();
	MapSide*	createSide(MapSector* sector);

	// Editing
	void		moveVertex(unsigned vertex, double nx, double ny);
	void		mergeVertices(unsigned vertex1, unsigned vertex2);
	MapVertex*	mergeVerticesPoint(double x, double y);
	void		splitLine(unsigned line, unsigned vertex);
	void		moveThing(unsigned thing, double nx, double ny);
	void		splitLinesAt(MapVertex* vertex, double split_dist = 0);
	bool		setLineSector(unsigned line, unsigned sector, bool front = true);
	void		splitLinesByLine(MapLine* split_line);
	int			mergeLine(unsigned line);

	// Merge
	bool		mergeArch(vector<MapVertex*> vertices);
	MapLine*	mergeOverlappingLines(MapLine* line1, MapLine* line2);
	void		correctSectors(vector<MapLine*> lines, bool existing_only = false);

	// Checks
	void	mapOpenChecks();
	int		removeDetachedVertices();
	int		removeDetachedSides();
	int		removeDetachedSectors();
	int		removeZeroLengthLines();

	// Convert
	bool	convertToHexen();
	bool	convertToUDMF();

	// Cleanup/Extra
	void	rebuildConnectedLines();
	void	rebuildConnectedSides();
};

#endif //__SLADEMAP_H__
