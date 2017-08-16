
#ifndef __SLADEMAP_H__
#define __SLADEMAP_H__

#include "MapLine.h"
#include "MapSide.h"
#include "MapSector.h"
#include "MapVertex.h"
#include "MapThing.h"
#include "Archive/Archive.h"
#include "Utility/PropertyList/PropertyList.h"
#include "MapEditor/MapSpecials.h"

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
namespace Game { enum class TagType; }

class SLADEMap
{
public:
	SLADEMap();
	SLADEMap(const SLADEMap& copy) = delete;
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

	string		mapName() const { return name_; }
	string		udmfNamespace() const { return udmf_namespace_; }
	int			currentFormat() const { return current_format_; }
	long		geometryUpdated() const { return geometry_updated_; }
	long		thingsUpdated() const { return things_updated_; }
	void		setGeometryUpdated();
	void		setThingsUpdated();

	// MapObject access
	MapVertex*	getVertex(unsigned index) const;
	MapSide*	getSide(unsigned index) const;
	MapLine*	getLine(unsigned index) const;
	MapSector*	getSector(unsigned index) const;
	MapThing*	getThing(unsigned index) const;
	MapObject*	getObject(uint8_t type, unsigned index) const;
	size_t		nVertices() const { return vertices_.size(); }
	size_t		nLines() const { return lines_.size(); }
	size_t		nSides() const { return sides_.size(); }
	size_t		nSectors() const { return sectors_.size(); }
	size_t		nThings() const { return things_.size(); }

	const vector<MapVertex*>&	vertices() const { return vertices_; }
	const vector<MapLine*>&		lines() const { return lines_; }
	const vector<MapSide*>&		sides() const { return sides_; }
	const vector<MapSector*>&	sectors() const { return sectors_; }
	const vector<MapThing*>&	things() const { return things_; }

	vector<ArchiveEntry*>&	udmfExtraEntries() { return udmf_extra_entries_; }

	// MapObject id stuff (used for undo/redo)
	void		addMapObject(MapObject* object);
	void		removeMapObject(MapObject* object);
	MapObject*	getObjectById(unsigned id) { return all_objects_[id].mobj; }
	void		getObjectIdList(uint8_t type, vector<unsigned>& list);
	void		restoreObjectIdList(uint8_t type, vector<unsigned>& list);

	void	refreshIndices();
	bool	readMap(Archive::MapDesc map);
	void	clearMap();

	MapSpecials*	mapSpecials() { return &map_specials_; }
	void			recomputeSpecials();

	// Map loading
	bool	readDoomMap(Archive::MapDesc map);
	bool	readHexenMap(Archive::MapDesc map);
	bool	readDoom64Map(Archive::MapDesc map);
	bool	readUDMFMap(Archive::MapDesc map);

	// Map saving
	bool	writeDoomMap(vector<ArchiveEntry*>& map_entries);
	bool	writeHexenMap(vector<ArchiveEntry*>& map_entries);
	bool	writeDoom64Map(vector<ArchiveEntry*>& map_entries);
	bool	writeUDMFMap(ArchiveEntry* textmap);

	// Item removal
	bool	removeVertex(MapVertex* vertex, bool merge_lines = false);
	bool	removeVertex(unsigned index, bool merge_lines = false);
	bool	removeLine(MapLine* line);
	bool	removeLine(unsigned index);
	bool	removeSide(MapSide* side, bool remove_from_line = true);
	bool	removeSide(unsigned index, bool remove_from_line = true);
	bool	removeSector(MapSector* sector);
	bool	removeSector(unsigned index);
	bool	removeThing(MapThing* thing);
	bool	removeThing(unsigned index);

	// Geometry
	int					nearestVertex(fpoint2_t point, double min = 64);
	int					nearestLine(fpoint2_t point, double min = 64);
	int					nearestThing(fpoint2_t point, double min = 64);
	vector<int>			nearestThingMulti(fpoint2_t point);
	int					sectorAt(fpoint2_t point);
	bbox_t				getMapBBox();
	MapVertex*			vertexAt(double x, double y);
	vector<fpoint2_t>	cutLines(double x1, double y1, double x2, double y2);
	MapVertex*			lineCrossVertex(double x1, double y1, double x2, double y2);
	void				updateGeometryInfo(long modified_time);
	bool				linesIntersect(MapLine* line1, MapLine* line2, double& x, double& y);
	void				findSectorTextPoint(MapSector* sector);
	void				initSectorPolygons();
	MapLine*			lineVectorIntersect(MapLine* line, bool front, double& hit_x, double& hit_y);

	// Tags/Ids
	MapThing* getFirstThingWithId(int id);
	void	getSectorsByTag(int tag, vector<MapSector*>& list);
	void	getThingsById(int id, vector<MapThing*>& list, unsigned start = 0, int type = 0);
	void	getLinesById(int id, vector<MapLine*>& list);
	void	getThingsByIdInSectorTag(int id, int tag, vector<MapThing*>& list);
	void	getTaggingThingsById(int id, int type, vector<MapThing*>& list, int ttype);
	void	getPathedThings(vector<MapThing*>& list);
	void	getDragonTargets(MapThing* first, vector<MapThing*>& list);
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
	bool				modifiedSince(long since, int type = -1);

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
	MapLine*	splitLine(MapLine* line, MapVertex* vertex);
	void		moveThing(unsigned thing, double nx, double ny);
	void		splitLinesAt(MapVertex* vertex, double split_dist = 0);
	bool		setLineSector(unsigned line, unsigned sector, bool front = true);
	void		splitLinesByLine(MapLine* split_line);
	int			mergeLine(unsigned line);
	bool		correctLineSectors(MapLine* line);
	void		setLineSide(MapLine* line, MapSide* side, bool front);

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
	int		removeInvalidSides();

	// Convert
	bool	convertToHexen();
	bool	convertToUDMF();

	// Cleanup/Extra
	void	rebuildConnectedLines();
	void	rebuildConnectedSides();

	// Usage counts
	void	clearTexUsage() { usage_tex_.clear(); }
	void	clearFlatUsage() { usage_flat_.clear(); }
	void	clearThingTypeUsage() { usage_thing_type_.clear(); }
	void	updateTexUsage(string tex, int adjust);
	void	updateFlatUsage(string flat, int adjust);
	void	updateThingTypeUsage(int type, int adjust);
	int		texUsageCount(string tex);
	int		flatUsageCount(string tex);
	int		thingTypeUsageCount(int type);

private:
	vector<MapLine*>	lines_;
	vector<MapSide*>	sides_;
	vector<MapSector*>	sectors_;
	vector<MapVertex*>	vertices_;
	vector<MapThing*>	things_;
	string				udmf_namespace_;
	PropertyList		udmf_props_;
	bool				position_frac_;
	string				name_;
	int					current_format_;
	long				opened_time_;
	MapSpecials			map_specials_;
	int					bulk_op_level_;
	bool				specials_expired_;

	vector<ArchiveEntry*>	udmf_extra_entries_;	// UDMF Extras

	// For undo/redo
	vector<mobj_holder_t>	all_objects_;
	vector<unsigned>		deleted_objects_;
	vector<unsigned>		created_objects_;
	vector<mobj_cd_t>		created_deleted_objects_;

	long	geometry_updated_;	// The last time the map geometry was updated
	long	things_updated_;	// The last time the thing list was modified

	// Usage counts
	std::map<string, int>	usage_tex_;
	std::map<string, int>	usage_flat_;
	std::map<int, int>		usage_thing_type_;

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
};

#endif //__SLADEMAP_H__
