
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SLADEMap.cpp
 * Description: SLADEMap class, the internal SLADE map handler.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "SLADEMap.h"
#include "Utility/Parser.h"
#include "Utility/MathStuff.h"
#include "General/ResourceManager.h"
#include "General/UI.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/Formats/WadArchive.h"
#include "General/UndoRedo.h"
#include "MapEditor/SectorBuilder.h"

#define IDEQ(x) (((x) != 0) && ((x) == id))


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, map_split_auto_offset, true, CVAR_SAVE)


/*******************************************************************
 * SLADEMAP CLASS FUNCTIONS
 *******************************************************************/

/* SLADEMap::SLADEMap
 * SLADEMap class constructor
 *******************************************************************/
SLADEMap::SLADEMap()
{
	// Init variables
	this->geometry_updated = 0;
	this->position_frac = false;

	// Object id 0 is always null
	all_objects.push_back(mobj_holder_t(NULL, false));

	// Init opened time so it's not random leftover garbage values
	setOpenedTime();
}

/* SLADEMap::~SLADEMap
 * SLADEMap class destructor
 *******************************************************************/
SLADEMap::~SLADEMap()
{
	clearMap();
}

/* SLADEMap::getVertex
 * Returns the vertex at [index], or NULL if [index] is invalid
 *******************************************************************/
MapVertex* SLADEMap::getVertex(unsigned index) const
{
	// Check index
	if (index >= vertices.size())
		return nullptr;

	return vertices[index];
}

/* SLADEMap::getSide
 * Returns the side at [index], or NULL if [index] is invalid
 *******************************************************************/
MapSide* SLADEMap::getSide(unsigned index) const
{
	// Check index
	if (index >= sides.size())
		return nullptr;

	return sides[index];
}

/* SLADEMap::getLine
 * Returns the line at [index], or NULL if [index] is invalid
 *******************************************************************/
MapLine* SLADEMap::getLine(unsigned index) const
{
	// Check index
	if (index >= lines.size())
		return nullptr;

	return lines[index];
}

/* SLADEMap::getSector
 * Returns the sector at [index], or NULL if [index] is invalid
 *******************************************************************/
MapSector* SLADEMap::getSector(unsigned index) const
{
	// Check index
	if (index >= sectors.size())
		return nullptr;

	return sectors[index];
}

/* SLADEMap::getThing
 * Returns the thing at [index], or NULL if [index] is invalid
 *******************************************************************/
MapThing* SLADEMap::getThing(unsigned index) const
{
	// Check index
	if (index >= things.size())
		return nullptr;

	return things[index];
}

/* SLADEMap::getObject
 * Returns the object of [type] at [index], or NULL if [index] is
 * invalid
 *******************************************************************/
MapObject* SLADEMap::getObject(uint8_t type, unsigned index) const
{
	switch (type)
	{
	case MOBJ_VERTEX:	return getVertex(index);
	case MOBJ_LINE:		return getLine(index);
	case MOBJ_SIDE:		return getSide(index);
	case MOBJ_SECTOR:	return getSector(index);
	case MOBJ_THING:	return getThing(index);
	default:			return nullptr;
	}
}

/* SLADEMap::setGeometryUpdated
 * Sets the geometry last updated time to now
 *******************************************************************/
void SLADEMap::setGeometryUpdated()
{
	geometry_updated = App::runTimer();
}

/* SLADEMap::setThingsUpdated
 * Sets the things last updated time to now
 *******************************************************************/
void SLADEMap::setThingsUpdated()
{
	things_updated = App::runTimer();
}

/* SLADEMap::refreshIndices
 * Refreshes all map object indices
 *******************************************************************/
void SLADEMap::refreshIndices()
{
	// Vertex indices
	for (unsigned a = 0; a < vertices.size(); a++)
		vertices[a]->index = a;

	// Side indices
	for (unsigned a = 0; a < sides.size(); a++)
		sides[a]->index = a;

	// Line indices
	for (unsigned a = 0; a < lines.size(); a++)
		lines[a]->index = a;

	// Sector indices
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->index = a;

	// Thing indices
	for (unsigned a = 0; a < things.size(); a++)
		things[a]->index = a;
}

/* SLADEMap::addMapObject
 * Adds [object] to the map objects list
 *******************************************************************/
void SLADEMap::addMapObject(MapObject* object)
{
	all_objects.push_back(mobj_holder_t(object, true));
	object->id = all_objects.size() - 1;
	created_deleted_objects.push_back(mobj_cd_t(object->id, true));
}

/* SLADEMap::removeMapObject
 * Removes [object] from the map (keeps it in the objects list, but
 * removes the 'in map' flag)
 *******************************************************************/
void SLADEMap::removeMapObject(MapObject* object)
{
	all_objects[object->id].in_map = false;
	created_deleted_objects.push_back(mobj_cd_t(object->id, false));
}

/* SLADEMap::getObjectIdList
 * Adds all object ids of [type] currently in the map to [list]
 *******************************************************************/
void SLADEMap::getObjectIdList(uint8_t type, vector<unsigned>& list)
{
	if (type == MOBJ_VERTEX)
	{
		for (unsigned a = 0; a < vertices.size(); a++)
			list.push_back(vertices[a]->id);
	}
	else if (type == MOBJ_LINE)
	{
		for (unsigned a = 0; a < lines.size(); a++)
			list.push_back(lines[a]->id);
	}
	else if (type == MOBJ_SIDE)
	{
		for (unsigned a = 0; a < sides.size(); a++)
			list.push_back(sides[a]->id);
	}
	else if (type == MOBJ_SECTOR)
	{
		for (unsigned a = 0; a < sectors.size(); a++)
			list.push_back(sectors[a]->id);
	}
	else if (type == MOBJ_THING)
	{
		for (unsigned a = 0; a < things.size(); a++)
			list.push_back(things[a]->id);
	}
}

/* SLADEMap::restoreObjectIdList
 * Add all object ids in [list] to the map as [type], clearing any
 * objects of [type] currently in the map
 *******************************************************************/
void SLADEMap::restoreObjectIdList(uint8_t type, vector<unsigned>& list)
{
	if (type == MOBJ_VERTEX)
	{
		// Clear
		for (unsigned a = 0; a < vertices.size(); a++)
			all_objects[vertices[a]->id].in_map = false;
		vertices.clear();

		// Restore
		for (unsigned a = 0; a < list.size(); a++)
		{
			all_objects[list[a]].in_map = true;
			vertices.push_back((MapVertex*)all_objects[list[a]].mobj);
			vertices.back()->index = vertices.size() - 1;
		}
	}
	else if (type == MOBJ_LINE)
	{
		// Clear
		for (unsigned a = 0; a < lines.size(); a++)
			all_objects[lines[a]->id].in_map = false;
		lines.clear();

		// Restore
		for (unsigned a = 0; a < list.size(); a++)
		{
			all_objects[list[a]].in_map = true;
			lines.push_back((MapLine*)all_objects[list[a]].mobj);
			lines.back()->index = lines.size() - 1;
		}
	}
	else if (type == MOBJ_SIDE)
	{
		// Clear
		for (unsigned a = 0; a < sides.size(); a++)
			all_objects[sides[a]->id].in_map = false;
		sides.clear();

		// Restore
		for (unsigned a = 0; a < list.size(); a++)
		{
			all_objects[list[a]].in_map = true;
			sides.push_back((MapSide*)all_objects[list[a]].mobj);
			sides.back()->index = sides.size() - 1;
		}
	}
	else if (type == MOBJ_SECTOR)
	{
		// Clear
		for (unsigned a = 0; a < sectors.size(); a++)
			all_objects[sectors[a]->id].in_map = false;
		sectors.clear();

		// Restore
		for (unsigned a = 0; a < list.size(); a++)
		{
			all_objects[list[a]].in_map = true;
			sectors.push_back((MapSector*)all_objects[list[a]].mobj);
			sectors.back()->index = sectors.size() - 1;
		}
	}
	else if (type == MOBJ_THING)
	{
		// Clear
		for (unsigned a = 0; a < things.size(); a++)
			all_objects[things[a]->id].in_map = false;
		things.clear();

		// Restore
		for (unsigned a = 0; a < list.size(); a++)
		{
			all_objects[list[a]].in_map = true;
			things.push_back((MapThing*)all_objects[list[a]].mobj);
			things.back()->index = things.size() - 1;
		}
	}
}

/* SLADEMap::readMap
 * Reads map data using info in [map]
 *******************************************************************/
bool SLADEMap::readMap(Archive::mapdesc_t map)
{
	Archive::mapdesc_t omap = map;

	// Check for map archive
	Archive* tempwad = NULL;
	if (map.archive && map.head)
	{
		tempwad = new WadArchive();
		tempwad->open(map.head);
		vector<Archive::mapdesc_t> amaps = tempwad->detectMaps();
		if (amaps.size() > 0)
			omap = amaps[0];
		else
		{
			delete tempwad;
			return false;
		}
	}

	bool ok = false;
	if (omap.head)
	{
		if (omap.format == MAP_DOOM)
			ok = readDoomMap(omap);
		else if (omap.format == MAP_HEXEN)
			ok = readHexenMap(omap);
		else if (omap.format == MAP_DOOM64)
			ok = readDoom64Map(omap);
		else if (omap.format == MAP_UDMF)
			ok = readUDMFMap(omap);
	}
	else
		ok = true;

	if (tempwad)
		delete tempwad;

	// Set map name
	name = map.name;

	// Set map format
	if (ok)
	{
		current_format = map.format;
		// When creating a new map, retrieve UDMF namespace information from the configuration
		if (map.format == MAP_UDMF && udmf_namespace.IsEmpty())
			udmf_namespace = theGameConfiguration->udmfNamespace();
	}

	initSectorPolygons();
	recomputeSpecials();

	opened_time = App::runTimer() + 10;

	return ok;
}

/* SLADEMap::addVertex
 * Adds a vertex to the map from a doom vertex definition [v]
 *******************************************************************/
bool SLADEMap::addVertex(doomvertex_t& v)
{
	MapVertex* nv = new MapVertex(v.x, v.y, this);
	vertices.push_back(nv);
	return true;
}

/* SLADEMap::addVertex
 * Adds a vertex to the map from a doom64 vertex definition [v]
 *******************************************************************/
bool SLADEMap::addVertex(doom64vertex_t& v)
{
	MapVertex* nv = new MapVertex((double)v.x/65536, (double)v.y/65536, this);
	vertices.push_back(nv);
	return true;
}

/* SLADEMap::addSide
 * Adds a side to the map from a doom sidedef definition [s]
 *******************************************************************/
bool SLADEMap::addSide(doomside_t& s)
{
	// Create side
	MapSide* ns = new MapSide(getSector(s.sector), this);

	// Setup side properties
	ns->tex_upper = wxString::FromAscii(s.tex_upper, 8);
	ns->tex_lower = wxString::FromAscii(s.tex_lower, 8);
	ns->tex_middle = wxString::FromAscii(s.tex_middle, 8);
	ns->offset_x = s.x_offset;
	ns->offset_y = s.y_offset;

	// Update texture counts
	usage_tex[ns->tex_upper.Upper()] += 1;
	usage_tex[ns->tex_middle.Upper()] += 1;
	usage_tex[ns->tex_lower.Upper()] += 1;

	// Add side
	sides.push_back(ns);
	return true;
}

/* SLADEMap::addSide
 * Adds a side to the map from a doom64 sidedef definition [s]
 *******************************************************************/
bool SLADEMap::addSide(doom64side_t& s)
{
	// Create side
	MapSide* ns = new MapSide(getSector(s.sector), this);

	// Setup side properties
	ns->tex_upper = theResourceManager->getTextureName(s.tex_upper);
	ns->tex_lower = theResourceManager->getTextureName(s.tex_lower);
	ns->tex_middle = theResourceManager->getTextureName(s.tex_middle);
	ns->offset_x = s.x_offset;
	ns->offset_y = s.y_offset;

	// Update texture counts
	usage_tex[ns->tex_upper.Upper()] += 1;
	usage_tex[ns->tex_middle.Upper()] += 1;
	usage_tex[ns->tex_lower.Upper()] += 1;

	// Add side
	sides.push_back(ns);
	return true;
}

/* SLADEMap::addLine
 * Adds a line to the map from a doom linedef definition [l]
 *******************************************************************/
bool SLADEMap::addLine(doomline_t& l)
{
	// Get relevant sides
	MapSide* s1 = NULL;
	MapSide* s2 = NULL;
	if (sides.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535) s1 = getSide(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535) s2 = getSide(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = getSide(l.side1);
		s2 = getSide(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = getVertex(l.vertex1);
	MapVertex* v2 = getVertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector, this);
		ns->copy(s1);
		s1 = ns;
		sides.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector, this);
		ns->copy(s2);
		s2 = ns;
		sides.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties["arg0"] = l.sector_tag;
	nl->properties["id"] = l.sector_tag;
	nl->special = l.type;
	nl->properties["flags"] = l.flags;

	// Add line
	lines.push_back(nl);
	return true;
}

/* SLADEMap::addLine
 * Adds a line to the map from a doom64 linedef definition [l]
 *******************************************************************/
bool SLADEMap::addLine(doom64line_t& l)
{
	// Get relevant sides
	MapSide* s1 = NULL;
	MapSide* s2 = NULL;
	if (sides.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535) s1 = getSide(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535) s2 = getSide(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = getSide(l.side1);
		s2 = getSide(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = getVertex(l.vertex1);
	MapVertex* v2 = getVertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector, this);
		ns->copy(s1);
		s1 = ns;
		sides.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector, this);
		ns->copy(s2);
		s2 = ns;
		sides.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties["arg0"] = l.sector_tag;
	if (l.type & 0x100)
		nl->properties["macro"] = l.type & 0xFF;
	else
		nl->special = l.type & 0xFF;
	nl->properties["flags"] = (int)l.flags;
	nl->properties["extraflags"] = l.type >> 9;

	// Add line
	lines.push_back(nl);
	return true;
}

/* SLADEMap::addSector
 * Adds a sector to the map from a doom sector definition [s]
 *******************************************************************/
bool SLADEMap::addSector(doomsector_t& s)
{
	// Create sector
	MapSector* ns = new MapSector(wxString::FromAscii(s.f_tex, 8), wxString::FromAscii(s.c_tex, 8), this);

	// Setup sector properties
	ns->setFloorHeight(s.f_height);
	ns->setCeilingHeight(s.c_height);
	ns->light = s.light;
	ns->special = s.special;
	ns->tag = s.tag;

	// Update texture counts
	usage_flat[ns->f_tex.Upper()] += 1;
	usage_flat[ns->c_tex.Upper()] += 1;

	// Add sector
	sectors.push_back(ns);
	return true;
}

/* SLADEMap::addSector
 * Adds a sector to the map from a doom64 sector definition [s]
 *******************************************************************/
bool SLADEMap::addSector(doom64sector_t& s)
{
	// Create sector
	// We need to retrieve the texture name from the hash value
	MapSector* ns = new MapSector(theResourceManager->getTextureName(s.f_tex),
								  theResourceManager->getTextureName(s.c_tex), this);

	// Setup sector properties
	ns->setFloorHeight(s.f_height);
	ns->setCeilingHeight(s.c_height);
	ns->light = 255;
	ns->special = s.special;
	ns->tag = s.tag;
	ns->properties["flags"] = s.flags;
	ns->properties["color_things"] = s.color[0];
	ns->properties["color_floor"] = s.color[1];
	ns->properties["color_ceiling"] = s.color[2];
	ns->properties["color_upper"] = s.color[3];
	ns->properties["color_lower"] = s.color[4];

	// Update texture counts
	usage_flat[ns->f_tex.Upper()] += 1;
	usage_flat[ns->c_tex.Upper()] += 1;

	// Add sector
	sectors.push_back(ns);
	return true;
}

/* SLADEMap::addThing
 * Adds a thing to the map from a doom thing definition [t]
 *******************************************************************/
bool SLADEMap::addThing(doomthing_t& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle = t.angle;
	nt->properties["flags"] = t.flags;

	// Add thing
	things.push_back(nt);
	return true;
}

/* SLADEMap::addThing
 * Adds a thing to the map from a doom64 thing definition [t]
 *******************************************************************/
bool SLADEMap::addThing(doom64thing_t& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle = t.angle;
	nt->properties["height"] = (double)t.z;
	nt->properties["flags"] = t.flags;
	nt->properties["id"] = t.tid;

	// Add thing
	things.push_back(nt);
	return true;
}

/* SLADEMap::readDoomVertexes
 * Reads in doom format vertex definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoomVertexes(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no VERTEXES entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doomvertex_t))
	{
		Log::info(3, "Read 0 vertices");
		return true;
	}

	doomvertex_t* vert_data = (doomvertex_t*)entry->getData(true);
	unsigned nv = entry->getSize() / sizeof(doomvertex_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < nv; a++)
	{
		UI::setSplashProgress(p + ((float)a / nv) * 0.2f);
		addVertex(vert_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu vertices", vertices.size());

	return true;
}

/* SLADEMap::readDoomSidedefs
 * Reads in doom format sidedef definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoomSidedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SIDEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doomside_t))
	{
		LOG_MESSAGE(3, "Read 0 sides");
		return true;
	}

	doomside_t* side_data = (doomside_t*)entry->getData(true);
	unsigned ns = entry->getSize() / sizeof(doomside_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);
		addSide(side_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sides", sides.size());

	return true;
}

/* SLADEMap::readDoomLinedefs
 * Reads in doom format linedef definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoomLinedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doomline_t))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	doomline_t* line_data = (doomline_t*)entry->getData(true);
	unsigned nl = entry->getSize() / sizeof(doomline_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		UI::setSplashProgress(p + ((float)a / nl) * 0.2f);
		if (!addLine(line_data[a]))
			LOG_MESSAGE(2, "Line %lu invalid, not added", a);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines.size());

	return true;
}

/* SLADEMap::readDoomSectors
 * Reads in doom format sector definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoomSectors(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SECTORS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doomsector_t))
	{
		LOG_MESSAGE(3, "Read 0 sectors");
		return true;
	}

	doomsector_t* sect_data = (doomsector_t*)entry->getData(true);
	unsigned ns = entry->getSize() / sizeof(doomsector_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);
		addSector(sect_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sectors", sectors.size());

	return true;
}

/* SLADEMap::readDoomThings
 * Reads in doom format thing definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoomThings(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doomthing_t))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	doomthing_t* thng_data = (doomthing_t*)entry->getData(true);
	unsigned nt = entry->getSize() / sizeof(doomthing_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things.size());

	return true;
}

/* SLADEMap::readDoomMap
 * Reads a doom format map using info in [map]
 *******************************************************************/
bool SLADEMap::readDoomMap(Archive::mapdesc_t map)
{
	LOG_MESSAGE(2, "Reading Doom format map");

	// Find map entries
	ArchiveEntry* v = NULL;
	ArchiveEntry* si = NULL;
	ArchiveEntry* l = NULL;
	ArchiveEntry* se = NULL;
	ArchiveEntry* t = NULL;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->getName() == "VERTEXES")
			v = entry;
		else if (!si && entry->getName() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->getName() == "LINEDEFS")
			l = entry;
		else if (!se && entry->getName() == "SECTORS")
			se = entry;
		else if (!t && entry->getName() == "THINGS")
			t = entry;

		// Next entry
		entry = entry->nextEntry();
	}

	// ---- Read vertices ----
	UI::setSplashProgressMessage("Reading Vertices");
	UI::setSplashProgress(0.0f);
	if (!readDoomVertexes(v))
		return false;

	// ---- Read sectors ----
	UI::setSplashProgressMessage("Reading Sectors");
	UI::setSplashProgress(0.2f);
	if (!readDoomSectors(se))
		return false;

	// ---- Read sides ----
	UI::setSplashProgressMessage("Reading Sides");
	UI::setSplashProgress(0.4f);
	if (!readDoomSidedefs(si))
		return false;

	// ---- Read lines ----
	UI::setSplashProgressMessage("Reading Lines");
	UI::setSplashProgress(0.6f);
	if (!readDoomLinedefs(l))
		return false;

	// ---- Read things ----
	UI::setSplashProgressMessage("Reading Things");
	UI::setSplashProgress(0.8f);
	if (!readDoomThings(t))
		return false;

	UI::setSplashProgressMessage("Init Map Data");
	UI::setSplashProgress(1.0f);

	// Remove detached vertices
	mapOpenChecks();

	// Update item indices
	refreshIndices();

	// Update sector bounding boxes
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->updateBBox();

	// Update variables
	geometry_updated = App::runTimer();

	return true;
}

/* SLADEMap::addLine
 * Adds a line to the map from a hexen linedef definition [l]
 *******************************************************************/
bool SLADEMap::addLine(hexenline_t& l)
{
	// Get relevant sides
	MapSide* s1 = NULL;
	MapSide* s2 = NULL;
	if (sides.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535) s1 = getSide(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535) s2 = getSide(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = getSide(l.side1);
		s2 = getSide(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = getVertex(l.vertex1);
	MapVertex* v2 = getVertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector, this);
		ns->copy(s1);
		s1 = ns;
		sides.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector, this);
		ns->copy(s2);
		s2 = ns;
		sides.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties["arg0"] = l.args[0];
	nl->properties["arg1"] = l.args[1];
	nl->properties["arg2"] = l.args[2];
	nl->properties["arg3"] = l.args[3];
	nl->properties["arg4"] = l.args[4];
	nl->special = l.type;
	nl->properties["flags"] = l.flags;

	// Handle some special cases
	if (l.type)
	{
		int needs_tag = theGameConfiguration->actionSpecial(l.type)->needsTag();
		if (needs_tag == AS_TT_LINEID || needs_tag == AS_TT_1LINEID_2LINE)
		{
			nl->properties["id"] = l.args[0];
		}
		else if (needs_tag == AS_TT_LINEID_HI5)
		{
			nl->properties["id"] = (l.args[0] + (l.args[4]<<8));
		}
	}

	// Add line
	lines.push_back(nl);
	return true;
}

/* SLADEMap::addThing
 * Adds a thing to the map from a hexen thing definition [t]
 *******************************************************************/
bool SLADEMap::addThing(hexenthing_t& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle = t.angle;
	nt->properties["height"] = (double)t.z;
	nt->properties["special"] = t.special;
	nt->properties["flags"] = t.flags;
	nt->properties["id"] = t.tid;
	nt->properties["arg0"] = t.args[0];
	nt->properties["arg1"] = t.args[1];
	nt->properties["arg2"] = t.args[2];
	nt->properties["arg3"] = t.args[3];
	nt->properties["arg4"] = t.args[4];

	// Add thing
	things.push_back(nt);
	return true;
}

/* SLADEMap::readHexenLinedefs
 * Reads in hexen format linedef definitions from [entry]
 *******************************************************************/
bool SLADEMap::readHexenLinedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(hexenline_t))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	hexenline_t* line_data = (hexenline_t*)entry->getData(true);
	unsigned nl = entry->getSize() / sizeof(hexenline_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		UI::setSplashProgress(p + ((float)a / nl) * 0.2f);
		addLine(line_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines.size());

	return true;
}

/* SLADEMap::readHexenThings
 * Reads in hexen format thing definitions from [entry]
 *******************************************************************/
bool SLADEMap::readHexenThings(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(hexenthing_t))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	hexenthing_t* thng_data = (hexenthing_t*)entry->getData(true);
	unsigned nt = entry->getSize() / sizeof(hexenthing_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things.size());

	return true;
}

/* SLADEMap::readHexenMap
 * Reads a hexen format using info in [map]
 *******************************************************************/
bool SLADEMap::readHexenMap(Archive::mapdesc_t map)
{
	LOG_MESSAGE(2, "Reading Hexen format map");

	// Find map entries
	ArchiveEntry* v = NULL;
	ArchiveEntry* si = NULL;
	ArchiveEntry* l = NULL;
	ArchiveEntry* se = NULL;
	ArchiveEntry* t = NULL;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->getName() == "VERTEXES")
			v = entry;
		else if (!si && entry->getName() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->getName() == "LINEDEFS")
			l = entry;
		else if (!se && entry->getName() == "SECTORS")
			se = entry;
		else if (!t && entry->getName() == "THINGS")
			t = entry;

		// Next entry
		entry = entry->nextEntry();
	}

	// ---- Read vertices ----
	UI::setSplashProgressMessage("Reading Vertices");
	UI::setSplashProgress(0.0f);
	if (!readDoomVertexes(v))
		return false;

	// ---- Read sectors ----
	UI::setSplashProgressMessage("Reading Sectors");
	UI::setSplashProgress(0.2f);
	if (!readDoomSectors(se))
		return false;

	// ---- Read sides ----
	UI::setSplashProgressMessage("Reading Sides");
	UI::setSplashProgress(0.4f);
	if (!readDoomSidedefs(si))
		return false;

	// ---- Read lines ----
	UI::setSplashProgressMessage("Reading Lines");
	UI::setSplashProgress(0.6f);
	if (!readHexenLinedefs(l))
		return false;

	// ---- Read things ----
	UI::setSplashProgressMessage("Reading Things");
	UI::setSplashProgress(0.8f);
	if (!readHexenThings(t))
		return false;

	UI::setSplashProgressMessage("Init Map Data");
	UI::setSplashProgress(1.0f);

	// Remove detached vertices
	mapOpenChecks();

	// Update item indices
	refreshIndices();

	// Update sector bounding boxes
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->updateBBox();

	return true;
}

/* SLADEMap::readDoom64Vertexes
 * Reads in doom64 format vertex definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoom64Vertexes(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no VERTEXES entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doom64vertex_t))
	{
		LOG_MESSAGE(3, "Read 0 vertices");
		return true;
	}

	doom64vertex_t* vert_data = (doom64vertex_t*)entry->getData(true);
	unsigned n = entry->getSize() / sizeof(doom64vertex_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addVertex(vert_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu vertices", vertices.size());

	return true;
}

/* SLADEMap::readDoom64Sidedefs
 * Reads in doom64 format sidedef definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoom64Sidedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SIDEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doom64side_t))
	{
		LOG_MESSAGE(3, "Read 0 sides");
		return true;
	}

	doom64side_t* side_data = (doom64side_t*)entry->getData(true);
	unsigned n = entry->getSize() / sizeof(doom64side_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addSide(side_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sides", sides.size());

	return true;
}

/* SLADEMap::readDoom64Linedefs
 * Reads in doom64 format linedef definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoom64Linedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doom64line_t))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	doom64line_t* line_data = (doom64line_t*)entry->getData(true);
	unsigned n = entry->getSize() / sizeof(doom64line_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addLine(line_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines.size());

	return true;
}

/* SLADEMap::readDoom64Sectors
 * Reads in doom64 format sector definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoom64Sectors(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SECTORS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doom64sector_t))
	{
		LOG_MESSAGE(3, "Read 0 sectors");
		return true;
	}

	doom64sector_t* sect_data = (doom64sector_t*)entry->getData(true);
	unsigned n = entry->getSize() / sizeof(doom64sector_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addSector(sect_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sectors", sectors.size());

	return true;
}

/* SLADEMap::readDoom64Things
 * Reads in doom64 format thing definitions from [entry]
 *******************************************************************/
bool SLADEMap::readDoom64Things(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->getSize() < sizeof(doom64thing_t))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	doom64thing_t* thng_data = (doom64thing_t*)entry->getData(true);
	unsigned n = entry->getSize() / sizeof(doom64thing_t);
	float p = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things.size());

	return true;
}

/* SLADEMap::readDoom64Map
 * Reads a doom64 format using info in [map]
 *******************************************************************/
bool SLADEMap::readDoom64Map(Archive::mapdesc_t map)
{
	LOG_MESSAGE(2, "Reading Doom 64 format map");

	// Find map entries
	ArchiveEntry* v = NULL;
	ArchiveEntry* si = NULL;
	ArchiveEntry* l = NULL;
	ArchiveEntry* se = NULL;
	ArchiveEntry* t = NULL;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->getName() == "VERTEXES")
			v = entry;
		else if (!si && entry->getName() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->getName() == "LINEDEFS")
			l = entry;
		else if (!se && entry->getName() == "SECTORS")
			se = entry;
		else if (!t && entry->getName() == "THINGS")
			t = entry;

		// Next entry
		entry = entry->nextEntry();
	}

	// ---- Read vertices ----
	UI::setSplashProgressMessage("Reading Vertices");
	UI::setSplashProgress(0.0f);
	if (!readDoom64Vertexes(v))
		return false;

	// ---- Read sectors ----
	UI::setSplashProgressMessage("Reading Sectors");
	UI::setSplashProgress(0.2f);
	if (!readDoom64Sectors(se))
		return false;

	// ---- Read sides ----
	UI::setSplashProgressMessage("Reading Sides");
	UI::setSplashProgress(0.4f);
	if (!readDoom64Sidedefs(si))
		return false;

	// ---- Read lines ----
	UI::setSplashProgressMessage("Reading Lines");
	UI::setSplashProgress(0.6f);
	if (!readDoom64Linedefs(l))
		return false;

	// ---- Read things ----
	UI::setSplashProgressMessage("Reading Things");
	UI::setSplashProgress(0.8f);
	if (!readDoom64Things(t))
		return false;

	UI::setSplashProgressMessage("Init Map Data");
	UI::setSplashProgress(1.0f);

	// Remove detached vertices
	mapOpenChecks();

	// Update item indices
	refreshIndices();

	// Update sector bounding boxes
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->updateBBox();

	return true;
}

/* SLADEMap::addVertex
 * Adds a vertex to the map from parsed UDMF vertex definition [def]
 *******************************************************************/
bool SLADEMap::addVertex(ParseTreeNode* def)
{
	// Check for required properties
	ParseTreeNode* prop_x = (ParseTreeNode*)def->getChild("x");
	ParseTreeNode* prop_y = (ParseTreeNode*)def->getChild("y");
	if (!prop_x || !prop_y)
		return false;

	// Create new vertex
	MapVertex* nv = new MapVertex(prop_x->getFloatValue(), prop_y->getFloatValue(), this);

	// Add extra vertex info
	ParseTreeNode* prop = NULL;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = (ParseTreeNode*)def->getChild(a);

		// Skip required properties
		if (prop == prop_x || prop == prop_y)
			continue;

		nv->properties[prop->getName()] = prop->getValue();
	}

	// Add vertex to map
	vertices.push_back(nv);

	return true;
}

/* SLADEMap::addSide
 * Adds a side to the map from parsed UDMF side definition [def]
 *******************************************************************/
bool SLADEMap::addSide(ParseTreeNode* def)
{
	// Check for required properties
	ParseTreeNode* prop_sector = (ParseTreeNode*)def->getChild("sector");
	if (!prop_sector)
		return false;

	// Check sector index
	int sector = prop_sector->getIntValue();
	if (sector < 0 || sector >= (int)sectors.size())
		return false;

	// Create new side
	MapSide* ns = new MapSide(sectors[sector], this);

	// Set defaults
	ns->offset_x = 0;
	ns->offset_y = 0;
	ns->tex_upper = "-";
	ns->tex_middle = "-";
	ns->tex_lower = "-";

	// Add extra side info
	ParseTreeNode* prop = NULL;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = (ParseTreeNode*)def->getChild(a);

		// Skip required properties
		if (prop == prop_sector)
			continue;

		if (S_CMPNOCASE(prop->getName(), "texturetop"))
			ns->tex_upper = prop->getStringValue();
		else if (S_CMPNOCASE(prop->getName(), "texturemiddle"))
			ns->tex_middle = prop->getStringValue();
		else if (S_CMPNOCASE(prop->getName(), "texturebottom"))
			ns->tex_lower = prop->getStringValue();
		else if (S_CMPNOCASE(prop->getName(), "offsetx"))
			ns->offset_x = prop->getIntValue();
		else if (S_CMPNOCASE(prop->getName(), "offsety"))
			ns->offset_y = prop->getIntValue();
		else
			ns->properties[prop->getName()] = prop->getValue();
		//LOG_MESSAGE(1, "Property %s type %s (%s)", prop->getName(), prop->getValue().typeString(), prop->getValue().getStringValue());
	}

	// Update texture counts
	usage_tex[ns->tex_upper.Upper()] += 1;
	usage_tex[ns->tex_middle.Upper()] += 1;
	usage_tex[ns->tex_lower.Upper()] += 1;

	// Add side to map
	sides.push_back(ns);

	return true;
}

/* SLADEMap::addLine
 * Adds a line to the map from parsed UDMF line definition [def]
 *******************************************************************/
bool SLADEMap::addLine(ParseTreeNode* def)
{
	// Check for required properties
	ParseTreeNode* prop_v1 = (ParseTreeNode*)def->getChild("v1");
	ParseTreeNode* prop_v2 = (ParseTreeNode*)def->getChild("v2");
	ParseTreeNode* prop_s1 = (ParseTreeNode*)def->getChild("sidefront");
	if (!prop_v1 || !prop_v2 || !prop_s1)
		return false;

	// Check indices
	int v1 = prop_v1->getIntValue();
	int v2 = prop_v2->getIntValue();
	int s1 = prop_s1->getIntValue();
	if (v1 < 0 || v1 >= (int)vertices.size())
		return false;
	if (v2 < 0 || v2 >= (int)vertices.size())
		return false;
	if (s1 < 0 || s1 >= (int)sides.size())
		return false;

	// Get second side if any
	MapSide* side2 = NULL;
	ParseTreeNode* prop_s2 = (ParseTreeNode*)def->getChild("sideback");
	if (prop_s2) side2 = getSide(prop_s2->getIntValue());

	// Create new line
	MapLine* nl = new MapLine(vertices[v1], vertices[v2], sides[s1], side2, this);

	// Set defaults
	nl->special = 0;

	// Add extra line info
	ParseTreeNode* prop = NULL;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = (ParseTreeNode*)def->getChild(a);

		// Skip required properties
		if (prop == prop_v1 || prop == prop_v2 || prop == prop_s1 || prop == prop_s2)
			continue;

		if (prop->getName() == "special")
			nl->special = prop->getIntValue();
		else
			nl->properties[prop->getName()] = prop->getValue();
	}

	// Add line to map
	lines.push_back(nl);

	return true;
}

/* SLADEMap::addSector
 * Adds a sector to the map from parsed UDMF sector definition [def]
 *******************************************************************/
bool SLADEMap::addSector(ParseTreeNode* def)
{
	// Check for required properties
	ParseTreeNode* prop_ftex = (ParseTreeNode*)def->getChild("texturefloor");
	ParseTreeNode* prop_ctex = (ParseTreeNode*)def->getChild("textureceiling");
	if (!prop_ftex || !prop_ctex)
		return false;

	// Create new sector
	MapSector* ns = new MapSector(prop_ftex->getStringValue(), prop_ctex->getStringValue(), this);
	usage_flat[ns->f_tex.Upper()] += 1;
	usage_flat[ns->c_tex.Upper()] += 1;

	// Set defaults
	ns->setFloorHeight(0);
	ns->setCeilingHeight(0);
	ns->light = 160;
	ns->special = 0;
	ns->tag = 0;

	// Add extra sector info
	ParseTreeNode* prop = NULL;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = (ParseTreeNode*)def->getChild(a);

		// Skip required properties
		if (prop == prop_ftex || prop == prop_ctex)
			continue;

		if (S_CMPNOCASE(prop->getName(), "heightfloor"))
			ns->setFloorHeight(prop->getIntValue());
		else if (S_CMPNOCASE(prop->getName(), "heightceiling"))
			ns->setCeilingHeight(prop->getIntValue());
		else if (S_CMPNOCASE(prop->getName(), "lightlevel"))
			ns->light = prop->getIntValue();
		else if (S_CMPNOCASE(prop->getName(), "special"))
			ns->special = prop->getIntValue();
		else if (S_CMPNOCASE(prop->getName(), "id"))
			ns->tag = prop->getIntValue();
		else
			ns->properties[prop->getName()] = prop->getValue();
	}

	// Add sector to map
	sectors.push_back(ns);

	return true;
}

/* SLADEMap::addThing
 * Adds a thing to the map from parsed UDMF thing definition [def]
 *******************************************************************/
bool SLADEMap::addThing(ParseTreeNode* def)
{
	// Check for required properties
	ParseTreeNode* prop_x = (ParseTreeNode*)def->getChild("x");
	ParseTreeNode* prop_y = (ParseTreeNode*)def->getChild("y");
	ParseTreeNode* prop_type = (ParseTreeNode*)def->getChild("type");
	if (!prop_x || !prop_y || !prop_type)
		return false;

	// Create new thing
	MapThing* nt = new MapThing(prop_x->getFloatValue(), prop_y->getFloatValue(), prop_type->getIntValue(), this);

	// Add extra thing info
	ParseTreeNode* prop = NULL;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = (ParseTreeNode*)def->getChild(a);

		// Skip required properties
		if (prop == prop_x || prop == prop_y || prop == prop_type)
			continue;

		// Builtin properties
		if (S_CMPNOCASE(prop->getName(), "angle"))
			nt->angle = prop->getIntValue();
		else
			nt->properties[prop->getName()] = prop->getValue();
	}

	// Add thing to map
	things.push_back(nt);

	return true;
}

/* SLADEMap::readDoomMap
 * Reads a UDMF format map using info in [map]
 *******************************************************************/
bool SLADEMap::readUDMFMap(Archive::mapdesc_t map)
{
	// Get TEXTMAP entry (will always be after the 'head' entry)
	ArchiveEntry* textmap = map.head->nextEntry();

	// --- Parse UDMF text ---
	UI::setSplashProgressMessage("Parsing TEXTMAP");
	UI::setSplashProgress(-100.0f);
	Parser parser;
	if (!parser.parseText(textmap->getMCData()))
		return false;

	// --- Process parsed data ---

	// First we have to sort the definition blocks by type so they can
	// be created in the correct order (verts->sides->lines->sectors->things),
	// even if they aren't defined in that order.
	// Unknown definitions are also kept, just in case
	UI::setSplashProgressMessage("Sorting definitions");
	ParseTreeNode* root = parser.parseTreeRoot();
	vector<ParseTreeNode*> defs_vertices;
	vector<ParseTreeNode*> defs_lines;
	vector<ParseTreeNode*> defs_sides;
	vector<ParseTreeNode*> defs_sectors;
	vector<ParseTreeNode*> defs_things;
	vector<ParseTreeNode*> defs_other;
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		UI::setSplashProgress((float)a / root->nChildren());

		ParseTreeNode* node = (ParseTreeNode*)root->getChild(a);

		// Vertex definition
		if (S_CMPNOCASE(node->getName(), "vertex"))
			defs_vertices.push_back(node);

		// Line definition
		else if (S_CMPNOCASE(node->getName(), "linedef"))
			defs_lines.push_back(node);

		// Side definition
		else if (S_CMPNOCASE(node->getName(), "sidedef"))
			defs_sides.push_back(node);

		// Sector definition
		else if (S_CMPNOCASE(node->getName(), "sector"))
			defs_sectors.push_back(node);

		// Thing definition
		else if (S_CMPNOCASE(node->getName(), "thing"))
			defs_things.push_back(node);

		// Namespace
		else if (S_CMPNOCASE(node->getName(), "namespace"))
			udmf_namespace = node->getStringValue();

		// Unknown
		else
			defs_other.push_back(node);
	}

	// Now create map structures from parsed data, in the right order

	// Create vertices from parsed data
	UI::setSplashProgressMessage("Reading Vertices");
	for (unsigned a = 0; a < defs_vertices.size(); a++)
	{
		UI::setSplashProgress(((float)a / defs_vertices.size()) * 0.2f);
		addVertex(defs_vertices[a]);
	}

	// Create sectors from parsed data
	UI::setSplashProgressMessage("Reading Sectors");
	for (unsigned a = 0; a < defs_sectors.size(); a++)
	{
		UI::setSplashProgress(0.2f + ((float)a / defs_sectors.size()) * 0.2f);
		addSector(defs_sectors[a]);
	}

	// Create sides from parsed data
	UI::setSplashProgressMessage("Reading Sides");
	for (unsigned a = 0; a < defs_sides.size(); a++)
	{
		UI::setSplashProgress(0.4f + ((float)a / defs_sides.size()) * 0.2f);
		addSide(defs_sides[a]);
	}

	// Create lines from parsed data
	UI::setSplashProgressMessage("Reading Lines");
	for (unsigned a = 0; a < defs_lines.size(); a++)
	{
		UI::setSplashProgress(0.6f + ((float)a / defs_lines.size()) * 0.2f);
		addLine(defs_lines[a]);
	}

	// Create things from parsed data
	UI::setSplashProgressMessage("Reading Things");
	for (unsigned a = 0; a < defs_things.size(); a++)
	{
		UI::setSplashProgress(0.8f + ((float)a / defs_things.size()) * 0.2f);
		addThing(defs_things[a]);
	}

	UI::setSplashProgressMessage("Init map data");

	// Remove detached vertices
	mapOpenChecks();

	// Update item indices
	refreshIndices();

	// Update sector bounding boxes
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->updateBBox();

	// Copy extra entries
	for (unsigned a = 0; a < map.unk.size(); a++)
		udmf_extra_entries.push_back(new ArchiveEntry(*(map.unk[a])));

	return true;
}

/* SLADEMap::writeDoomVertexes
 * Writes doom format vertex definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoomVertexes(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(vertices.size() * 4, false);
	entry->seek(0, 0);

	// Write vertex data
	short x, y;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		x = vertices[a]->xPos();
		y = vertices[a]->yPos();
		entry->write(&x, 2);
		entry->write(&y, 2);
	}

	return true;
}

/* SLADEMap::writeDoomSidedefs
 * Writes doom format sidedef definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoomSidedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sides.size() * 30, false);
	entry->seek(0, 0);

	// Write side data
	doomside_t side;
	string t_m, t_u, t_l;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		memset(&side, 0, 30);

		// Offsets
		side.x_offset = sides[a]->offset_x;
		side.y_offset = sides[a]->offset_y;

		// Sector
		side.sector = -1;
		if (sides[a]->sector) side.sector = sides[a]->sector->getIndex();

		// Textures
		t_m = sides[a]->tex_middle;
		t_u = sides[a]->tex_upper;
		t_l = sides[a]->tex_lower;
		memcpy(side.tex_middle, CHR(t_m), t_m.Length());
		memcpy(side.tex_upper, CHR(t_u), t_u.Length());
		memcpy(side.tex_lower, CHR(t_l), t_l.Length());

		entry->write(&side, 30);
	}

	return true;
}

/* SLADEMap::writeDoomLinedefs
 * Writes doom format linedef definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoomLinedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines.size() * 14, false);
	entry->seek(0, 0);

	// Write line data
	doomline_t line;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Vertices
		line.vertex1 = lines[a]->v1Index();
		line.vertex2 = lines[a]->v2Index();

		// Properties
		line.flags = lines[a]->intProperty("flags");
		line.type = lines[a]->intProperty("special");
		line.sector_tag = lines[a]->intProperty("arg0");

		// Sides
		line.side1 = -1;
		line.side2 = -1;
		if (lines[a]->side1) line.side1 = lines[a]->side1->getIndex();
		if (lines[a]->side2) line.side2 = lines[a]->side2->getIndex();

		entry->write(&line, 14);
	}

	return true;
}

/* SLADEMap::writeDoomSectors
 * Writes doom format sector definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoomSectors(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sectors.size() * 26, false);
	entry->seek(0, 0);

	// Write sector data
	doomsector_t sector;
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		memset(&sector, 0, 26);

		// Height
		sector.f_height = sectors[a]->f_height;
		sector.c_height = sectors[a]->c_height;

		// Textures
		memcpy(sector.f_tex, CHR(sectors[a]->f_tex), sectors[a]->f_tex.Length());
		memcpy(sector.c_tex, CHR(sectors[a]->c_tex), sectors[a]->c_tex.Length());

		// Properties
		sector.light = sectors[a]->light;
		sector.special = sectors[a]->special;
		sector.tag = sectors[a]->tag;

		entry->write(&sector, 26);
	}

	return true;
}

/* SLADEMap::writeDoomThings
 * Writes doom format thing definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoomThings(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things.size() * 10, false);
	entry->seek(0, 0);

	// Write thing data
	doomthing_t thing;
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Position
		thing.x = things[a]->xPos();
		thing.y = things[a]->yPos();

		// Properties
		thing.angle = things[a]->getAngle();
		thing.type = things[a]->type;
		thing.flags = things[a]->intProperty("flags");

		entry->write(&thing, 10);
	}

	return true;
}

/* SLADEMap::writeDoomMap
 * Writes doom format map entries and adds them to [map_entries]
 *******************************************************************/
bool SLADEMap::writeDoomMap(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	ArchiveEntry* entry = new ArchiveEntry("THINGS");
	writeDoomThings(entry);
	map_entries.push_back(entry);

	// Write LINEDEFS
	entry = new ArchiveEntry("LINEDEFS");
	writeDoomLinedefs(entry);
	map_entries.push_back(entry);

	// Write SIDEDEFS
	entry = new ArchiveEntry("SIDEDEFS");
	writeDoomSidedefs(entry);
	map_entries.push_back(entry);

	// Write VERTEXES
	entry = new ArchiveEntry("VERTEXES");
	writeDoomVertexes(entry);
	map_entries.push_back(entry);

	// Write SECTORS
	entry = new ArchiveEntry("SECTORS");
	writeDoomSectors(entry);
	map_entries.push_back(entry);

	return true;
}

/* SLADEMap::writeHexenLinedefs
 * Writes hexen format linedef definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeHexenLinedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines.size() * 16, false);
	entry->seek(0, 0);

	// Write line data
	hexenline_t line;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Vertices
		line.vertex1 = lines[a]->v1Index();
		line.vertex2 = lines[a]->v2Index();

		// Properties
		line.flags = lines[a]->intProperty("flags");
		line.type = lines[a]->intProperty("special");

		// Args
		for (unsigned arg = 0; arg < 5; arg++)
			line.args[arg] = lines[a]->intProperty(S_FMT("arg%u", arg));

		// Sides
		line.side1 = -1;
		line.side2 = -1;
		if (lines[a]->side1) line.side1 = lines[a]->side1->getIndex();
		if (lines[a]->side2) line.side2 = lines[a]->side2->getIndex();

		entry->write(&line, 16);
	}

	return true;
}

/* SLADEMap::writeHexenThings
 * Writes hexen format thing definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeHexenThings(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things.size() * 20, false);
	entry->seek(0, 0);

	// Write thing data
	hexenthing_t thing;
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Position
		thing.x = things[a]->xPos();
		thing.y = things[a]->yPos();
		thing.z = things[a]->intProperty("height");

		// Properties
		thing.angle = things[a]->getAngle();
		thing.type = things[a]->type;
		thing.flags = things[a]->intProperty("flags");
		thing.tid = things[a]->intProperty("id");
		thing.special = things[a]->intProperty("special");

		// Args
		for (unsigned arg = 0; arg < 5; arg++)
			thing.args[arg] = things[a]->intProperty(S_FMT("arg%u", arg));

		entry->write(&thing, 20);
	}

	return true;
}

/* SLADEMap::writeHexenMap
 * Writes hexen format map entries and adds them to [map_entries]
 *******************************************************************/
bool SLADEMap::writeHexenMap(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	ArchiveEntry* entry = new ArchiveEntry("THINGS");
	writeHexenThings(entry);
	map_entries.push_back(entry);

	// Write LINEDEFS
	entry = new ArchiveEntry("LINEDEFS");
	writeHexenLinedefs(entry);
	map_entries.push_back(entry);

	// Write SIDEDEFS
	entry = new ArchiveEntry("SIDEDEFS");
	writeDoomSidedefs(entry);
	map_entries.push_back(entry);

	// Write VERTEXES
	entry = new ArchiveEntry("VERTEXES");
	writeDoomVertexes(entry);
	map_entries.push_back(entry);

	// Write SECTORS
	entry = new ArchiveEntry("SECTORS");
	writeDoomSectors(entry);
	map_entries.push_back(entry);

	return true;
}

/* SLADEMap::writeDoom64Vertexes
 * Writes doom64 format vertex definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoom64Vertexes(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(vertices.size() * 8, false);
	entry->seek(0, 0);

	// Write vertex data
	int32_t x, y;	// Those are actually fixed_t, so shift by FRACBIT (16)
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		x = vertices[a]->xPos()*65536;
		y = vertices[a]->yPos()*65536;
		entry->write(&x, 4);
		entry->write(&y, 4);
	}

	return true;
}

/* SLADEMap::writeDoom64Sidedefs
 * Writes doom64 format sidedef definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoom64Sidedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sides.size() * sizeof(doom64side_t), false);
	entry->seek(0, 0);

	// Write side data
	doom64side_t side;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		memset(&side, 0, sizeof(doom64side_t));

		// Offsets
		side.x_offset = sides[a]->offset_x;
		side.y_offset = sides[a]->offset_y;

		// Sector
		side.sector = -1;
		if (sides[a]->sector) side.sector = sides[a]->sector->getIndex();

		// Textures
		side.tex_middle	= theResourceManager->getTextureHash(sides[a]->tex_middle);
		side.tex_upper	= theResourceManager->getTextureHash(sides[a]->tex_upper);
		side.tex_lower	= theResourceManager->getTextureHash(sides[a]->tex_lower);

		entry->write(&side, sizeof(doom64side_t));
	}

	return true;
}

/* SLADEMap::writeDoom64Linedefs
 * Writes doom64 format linedef definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoom64Linedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines.size() * sizeof(doom64line_t), false);
	entry->seek(0, 0);

	// Write line data
	doom64line_t line;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Vertices
		line.vertex1 = lines[a]->v1Index();
		line.vertex2 = lines[a]->v2Index();

		// Properties
		line.flags = lines[a]->intProperty("flags");
		line.type = lines[a]->intProperty("special");
		line.sector_tag = lines[a]->intProperty("arg0");

		// Sides
		line.side1 = -1;
		line.side2 = -1;
		if (lines[a]->side1) line.side1 = lines[a]->side1->getIndex();
		if (lines[a]->side2) line.side2 = lines[a]->side2->getIndex();

		entry->write(&line, sizeof(doom64line_t));
	}

	return true;
}

/* SLADEMap::writeDoom64Sectors
 * Writes doom64 format sector definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoom64Sectors(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sectors.size() * sizeof(doom64sector_t), false);
	entry->seek(0, 0);

	// Write sector data
	doom64sector_t sector;
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		memset(&sector, 0, sizeof(doom64sector_t));

		// Height
		sector.f_height = sectors[a]->f_height;
		sector.c_height = sectors[a]->c_height;

		// Textures
		sector.f_tex = theResourceManager->getTextureHash(sectors[a]->stringProperty("texturefloor"));
		sector.c_tex = theResourceManager->getTextureHash(sectors[a]->stringProperty("textureceiling"));

		// Colors
		sector.color[0] = sectors[a]->intProperty("color_things");
		sector.color[1] = sectors[a]->intProperty("color_floor");
		sector.color[2] = sectors[a]->intProperty("color_ceiling");
		sector.color[3] = sectors[a]->intProperty("color_upper");
		sector.color[4] = sectors[a]->intProperty("color_lower");

		// Properties
		sector.special = sectors[a]->special;
		sector.flags = sectors[a]->intProperty("flags");
		sector.tag = sectors[a]->tag;

		entry->write(&sector, sizeof(doom64sector_t));
	}

	return true;
}

/* SLADEMap::writeDoom64Things
 * Writes doom64 format thing definitions to [entry]
 *******************************************************************/
bool SLADEMap::writeDoom64Things(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things.size() * sizeof(doom64thing_t), false);
	entry->seek(0, 0);

	// Write thing data
	doom64thing_t thing;
	for (unsigned a = 0; a < things.size(); a++)
	{
		// Position
		thing.x = things[a]->xPos();
		thing.y = things[a]->yPos();
		thing.z = things[a]->intProperty("height");

		// Properties
		thing.angle = things[a]->getAngle();
		thing.type = things[a]->type;
		thing.flags = things[a]->intProperty("flags");
		thing.tid = things[a]->intProperty("id");

		entry->write(&thing, sizeof(doom64thing_t));
	}

	return true;
}

/* SLADEMap::writeDoom64Map
 * Writes doom64 format map entries and adds them to [map_entries]
 *******************************************************************/
bool SLADEMap::writeDoom64Map(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	ArchiveEntry* entry = new ArchiveEntry("THINGS");
	writeDoom64Things(entry);
	map_entries.push_back(entry);

	// Write LINEDEFS
	entry = new ArchiveEntry("LINEDEFS");
	writeDoom64Linedefs(entry);
	map_entries.push_back(entry);

	// Write SIDEDEFS
	entry = new ArchiveEntry("SIDEDEFS");
	writeDoom64Sidedefs(entry);
	map_entries.push_back(entry);

	// Write VERTEXES
	entry = new ArchiveEntry("VERTEXES");
	writeDoom64Vertexes(entry);
	map_entries.push_back(entry);

	// Write SECTORS
	entry = new ArchiveEntry("SECTORS");
	writeDoom64Sectors(entry);
	map_entries.push_back(entry);

	// TODO: Write LIGHTS and MACROS

	return true;
}

/* SLADEMap::writeUDMFMap
 * Writes map as UDMF format text to [textmap]
 *******************************************************************/
bool SLADEMap::writeUDMFMap(ArchiveEntry* textmap)
{
	// Check entry was given
	if (!textmap)
		return false;

	// Open temp text file
	wxFile tempfile(App::path("sladetemp.txt", App::Dir::Temp), wxFile::write);

	// Write map namespace
	tempfile.Write("// Written by SLADE3\n");
	tempfile.Write(S_FMT("namespace=\"%s\";\n", udmf_namespace));

	//sf::Clock clock;

	// Locale for float number format
	setlocale(LC_NUMERIC, "C");

	// Write things
	string object_def;
	for (unsigned a = 0; a < things.size(); a++)
	{
		object_def = S_FMT("thing//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("x=%1.3f;\ny=%1.3f;\ntype=%d;\n", things[a]->x, things[a]->y, things[a]->type);
		if (things[a]->angle != 0) object_def += S_FMT("angle=%d;\n", things[a]->angle);

		// Remove internal 'flags' property if it exists
		things[a]->props().removeProperty("flags");

		// Other properties
		if (!things[a]->properties.isEmpty())
		{
			theGameConfiguration->cleanObjectUDMFProps(things[a]);
			object_def += things[a]->properties.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	//LOG_MESSAGE(1, "Writing things took %dms", clock.getElapsedTime().asMilliseconds());

	// Write lines
	//clock.restart();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		object_def = S_FMT("linedef//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("v1=%d;\nv2=%d;\nsidefront=%d;\n", lines[a]->v1Index(), lines[a]->v2Index(), lines[a]->s1Index());
		if (lines[a]->s2())
			object_def += S_FMT("sideback=%d;\n", lines[a]->s2Index());
		if (lines[a]->special != 0)
			object_def += S_FMT("special=%d;\n", lines[a]->special);

		// Remove internal 'flags' property if it exists
		lines[a]->props().removeProperty("flags");

		// Other properties
		if (!lines[a]->properties.isEmpty())
		{
			theGameConfiguration->cleanObjectUDMFProps(lines[a]);
			object_def += lines[a]->properties.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	//LOG_MESSAGE(1, "Writing lines took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sides
	//clock.restart();
	for (unsigned a = 0; a < sides.size(); a++)
	{
		object_def = S_FMT("sidedef//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("sector=%u;\n", sides[a]->sector->getIndex());
		if (sides[a]->tex_upper != "-")
			object_def += S_FMT("texturetop=\"%s\";\n", sides[a]->tex_upper);
		if (sides[a]->tex_middle != "-")
			object_def += S_FMT("texturemiddle=\"%s\";\n", sides[a]->tex_middle);
		if (sides[a]->tex_lower != "-")
			object_def += S_FMT("texturebottom=\"%s\";\n", sides[a]->tex_lower);
		if (sides[a]->offset_x != 0)
			object_def += S_FMT("offsetx=%d;\n", sides[a]->offset_x);
		if (sides[a]->offset_y != 0)
			object_def += S_FMT("offsety=%d;\n", sides[a]->offset_y);

		// Other properties
		if (!sides[a]->properties.isEmpty())
		{
			theGameConfiguration->cleanObjectUDMFProps(sides[a]);
			object_def += sides[a]->properties.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	//LOG_MESSAGE(1, "Writing sides took %dms", clock.getElapsedTime().asMilliseconds());

	// Write vertices
	//clock.restart();
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		object_def = S_FMT("vertex//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("x=%1.3f;\ny=%1.3f;\n", vertices[a]->x, vertices[a]->y);

		// Other properties
		if (!vertices[a]->properties.isEmpty())
		{
			theGameConfiguration->cleanObjectUDMFProps(vertices[a]);
			object_def += vertices[a]->properties.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	//LOG_MESSAGE(1, "Writing vertices took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sectors
	//clock.restart();
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		object_def = S_FMT("sector//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("texturefloor=\"%s\";\ntextureceiling=\"%s\";\n", sectors[a]->f_tex, sectors[a]->c_tex);
		if (sectors[a]->f_height != 0) object_def += S_FMT("heightfloor=%d;\n", sectors[a]->f_height);
		if (sectors[a]->c_height != 0) object_def += S_FMT("heightceiling=%d;\n", sectors[a]->c_height);
		if (sectors[a]->light != 160) object_def += S_FMT("lightlevel=%d;\n", sectors[a]->light);
		if (sectors[a]->special != 0) object_def += S_FMT("special=%d;\n", sectors[a]->special);
		if (sectors[a]->tag != 0) object_def += S_FMT("id=%d;\n", sectors[a]->tag);

		// Other properties
		if (!sectors[a]->properties.isEmpty())
		{
			theGameConfiguration->cleanObjectUDMFProps(sectors[a]);
			object_def += sectors[a]->properties.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	//LOG_MESSAGE(1, "Writing sectors took %dms", clock.getElapsedTime().asMilliseconds());

	// Close file
	tempfile.Close();

	// Load file to entry
	textmap->importFile(App::path("sladetemp.txt", App::Dir::Temp));

	return true;
}

/* SLADEMap::clearMap
 * Clears all map data
 *******************************************************************/
void SLADEMap::clearMap()
{
	map_specials.reset();

	// Clear vectors
	sides.clear();
	lines.clear();
	vertices.clear();
	sectors.clear();
	things.clear();

	// Clear map objects
	for (unsigned a = 0; a < all_objects.size(); a++)
	{
		if (all_objects[a].mobj)
			delete all_objects[a].mobj;
	}
	all_objects.clear();

	// Object id 0 is always null
	all_objects.push_back(mobj_holder_t(NULL, false));

	// Clear usage counts
	usage_flat.clear();
	usage_tex.clear();
	usage_thing_type.clear();

	// Clear UDMF extra entries
	for (unsigned a = 0; a < udmf_extra_entries.size(); a++)
		delete udmf_extra_entries[a];
	udmf_extra_entries.clear();
}

/* SLADEMap::removeVertex
 * Removes [vertex] from the map
 *******************************************************************/
bool SLADEMap::removeVertex(MapVertex* vertex, bool merge_lines)
{
	// Check vertex was given
	if (!vertex)
		return false;

	return removeVertex(vertex->index, merge_lines);
}

/* SLADEMap::removeVertex
 * Removes the vertex at [index] from the map
 *******************************************************************/
bool SLADEMap::removeVertex(unsigned index, bool merge_lines)
{
	// Check index
	if (index >= vertices.size())
		return false;

	// Check if we should merge connected lines
	bool merged = false;
	if (merge_lines && vertices[index]->connected_lines.size() == 2)
	{
		// Get other end vertex of second connected line
		MapLine* l_first = vertices[index]->connected_lines[0];
		MapLine* l_second = vertices[index]->connected_lines[1];
		MapVertex* v_end = l_second->vertex2;
		if (v_end == vertices[index])
			v_end = l_second->vertex1;

		// Remove second connected line
		removeLine(l_second);

		// Connect first connected line to other end vertex
		l_first->setModified();
		MapVertex* v_start = l_first->vertex1;
		if (l_first->vertex1 == vertices[index])
		{
			l_first->vertex1 = v_end;
			v_start = l_first->vertex2;
		}
		else
			l_first->vertex2 = v_end;
		vertices[index]->disconnectLine(l_first);
		v_end->connectLine(l_first);
		l_first->resetInternals();

		// Check if we ended up with overlapping lines (ie. there was a triangle)
		for (unsigned a = 0; a < v_end->nConnectedLines(); a++)
		{
			if (v_end->connected_lines[a] == l_first)
				continue;

			if ((v_end->connected_lines[a]->vertex1 == v_end && v_end->connected_lines[a]->vertex2 == v_start) ||
				(v_end->connected_lines[a]->vertex2 == v_end && v_end->connected_lines[a]->vertex1 == v_start))
			{
				// Overlap found, remove line
				removeLine(l_first);
				break;
			}
		}

		merged = true;
	}
	
	if (!merged)
	{
		// Remove all connected lines
		vector<MapLine*> clines = vertices[index]->connected_lines;
		for (unsigned a = 0; a < clines.size(); a++)
			removeLine(clines[a]);
	}

	// Remove the vertex
	removeMapObject(vertices[index]);
	vertices[index] = vertices.back();
	vertices[index]->index = index;
	//vertices[index]->modified_time = App::runTimer();
	vertices.pop_back();

	geometry_updated = App::runTimer();

	return true;
}

/* SLADEMap::removeLine
 * Removes [line] from the map
 *******************************************************************/
bool SLADEMap::removeLine(MapLine* line)
{
	// Check line was given
	if (!line)
		return false;

	return removeLine(line->index);
}

/* SLADEMap::removeLine
 * Removes the line at [index] from the map
 *******************************************************************/
bool SLADEMap::removeLine(unsigned index)
{
	// Check index
	if (index >= lines.size())
		return false;

	LOG_MESSAGE(4, "id %u  index %u  objindex %u", lines[index]->id, index, lines[index]->index);

	// Init
	lines[index]->resetInternals();
	MapVertex* v1 = lines[index]->vertex1;
	MapVertex* v2 = lines[index]->vertex2;

	// Remove the line's sides
	if (lines[index]->side1)
		removeSide(lines[index]->side1, false);
	if (lines[index]->side2)
		removeSide(lines[index]->side2, false);

	// Disconnect from vertices
	lines[index]->vertex1->disconnectLine(lines[index]);
	lines[index]->vertex2->disconnectLine(lines[index]);

	// Remove the line
	removeMapObject(lines[index]);
	lines[index] = lines[lines.size()-1];
	lines[index]->index = index;
	//lines[index]->modified_time = App::runTimer();
	lines.pop_back();

	geometry_updated = App::runTimer();

	return true;
}

/* SLADEMap::removeSide
 * Removes [side] from the map
 *******************************************************************/
bool SLADEMap::removeSide(MapSide* side, bool remove_from_line)
{
	// Check side was given
	if (!side)
		return false;

	return removeSide(side->index, remove_from_line);
}

/* SLADEMap::removeSide
 * Removes the side at [index] from the map
 *******************************************************************/
bool SLADEMap::removeSide(unsigned index, bool remove_from_line)
{
	// Check index
	if (index >= sides.size())
		return false;

	if (remove_from_line)
	{
		// Remove from parent line
		MapLine* l = sides[index]->parent;
		l->setModified();
		if (l->side1 == sides[index])
			l->side1 = NULL;
		if (l->side2 == sides[index])
			l->side2 = NULL;

		// Set appropriate line flags
		theGameConfiguration->setLineBasicFlag("blocking", l, current_format, true);
		theGameConfiguration->setLineBasicFlag("twosided", l, current_format, false);
	}

	// Remove side from its sector, if any
	if (sides[index]->sector)
	{
		for (unsigned a = 0; a < sides[index]->sector->connected_sides.size(); a++)
		{
			if (sides[index]->sector->connected_sides[a] == sides[index])
			{
				sides[index]->sector->connected_sides.erase(sides[index]->sector->connected_sides.begin() + a);
				break;
			}
		}
	}

	// Update texture usage
	usage_tex[sides[index]->tex_lower.Upper()] -= 1;
	usage_tex[sides[index]->tex_middle.Upper()] -= 1;
	usage_tex[sides[index]->tex_upper.Upper()] -= 1;

	// Remove the side
	removeMapObject(sides[index]);
	sides[index] = sides.back();
	sides[index]->index = index;
	//sides[index]->modified_time = App::runTimer();
	sides.pop_back();

	return true;
}

/* SLADEMap::removeSector
 * Removes [sector] from the map
 *******************************************************************/
bool SLADEMap::removeSector(MapSector* sector)
{
	// Check sector was given
	if (!sector)
		return false;

	return removeSector(sector->index);
}

/* SLADEMap::removeSector
 * Removes the sector at [index] from the map
 *******************************************************************/
bool SLADEMap::removeSector(unsigned index)
{
	// Check index
	if (index >= sectors.size())
		return false;

	// Clear connected sides' sectors
	//for (unsigned a = 0; a < sectors[index]->connected_sides.size(); a++)
	//	sectors[index]->connected_sides[a]->sector = NULL;

	// Update texture usage
	usage_flat[sectors[index]->f_tex.Upper()] -= 1;
	usage_flat[sectors[index]->c_tex.Upper()] -= 1;

	// Remove the sector
	removeMapObject(sectors[index]);
	sectors[index] = sectors.back();
	sectors[index]->index = index;
	//sectors[index]->modified_time = App::runTimer();
	sectors.pop_back();

	return true;
}

/* SLADEMap::removeThing
 * Removes [thing] from the map
 *******************************************************************/
bool SLADEMap::removeThing(MapThing* thing)
{
	// Check thing was given
	if (!thing)
		return false;

	return removeThing(thing->index);
}

/* SLADEMap::removeThing
 * Removes the thing at [index] from the map
 *******************************************************************/
bool SLADEMap::removeThing(unsigned index)
{
	// Check index
	if (index >= things.size())
		return false;

	// Remove the thing
	removeMapObject(things[index]);
	things[index] = things.back();
	things[index]->index = index;
	//things[index]->modified_time = App::runTimer();
	things.pop_back();

	things_updated = App::runTimer();

	return true;
}

/* SLADEMap::nearestVertex
 * Returns the index of the vertex closest to the point, or -1 if none
 * found. Igonres any vertices further away than [min]
 *******************************************************************/
int SLADEMap::nearestVertex(fpoint2_t point, double min)
{
	// Go through vertices
	double min_dist = 999999999;
	MapVertex* v = NULL;
	double dist = 0;
	int index = -1;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		v = vertices[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(v->point());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			index = a;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest vertex,
	// to check for minimum hilight distance
	if (index >= 0)
	{
		v = vertices[index];
		double rdist = MathStuff::distance(v->point(), point);
		if (rdist > min)
			return -1;
	}

	return index;
}

/* SLADEMap::nearestLine
 * Returns the index of the line closest to the point, or -1 if none
 * is found. Ignores lines further away than [mindist]
 *******************************************************************/
int SLADEMap::nearestLine(fpoint2_t point, double mindist)
{
	// Go through lines
	double min_dist = mindist;
	double dist = 0;
	int index = -1;
	MapLine* l;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		l = lines[a];

		// Check with line bounding box first (since we have a minimum distance)
		fseg2_t bbox = l->seg();
		bbox.expand(mindist, mindist);
		if (! bbox.contains(point))
			continue;

		// Calculate distance to line
		dist = l->distanceTo(point);

		// Check if it's nearer than the previous nearest
		if (dist < min_dist && dist < mindist)
		{
			index = a;
			min_dist = dist;
		}
	}

	return index;
}

/* SLADEMap::nearestThing
 * Returns the index of the thing closest to the point, or -1 if none
 * found. Igonres any thing further away than [min]
 *******************************************************************/
int SLADEMap::nearestThing(fpoint2_t point, double min)
{
	// Go through things
	double min_dist = 999999999;
	MapThing* t = NULL;
	double dist = 0;
	int index = -1;
	for (unsigned a = 0; a < things.size(); a++)
	{
		t = things[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(t->point());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			index = a;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest thing,
	// to check for minimum hilight distance
	if (index >= 0)
	{
		t = things[index];
		double rdist = MathStuff::distance(t->point(), point);
		if (rdist > min)
			return -1;
	}

	return index;
}

/* SLADEMap::nearestThingMulti
 * Same as nearestThing, but returns a list of indices for the case
 * where there are multiple things at the same point
 *******************************************************************/
vector<int> SLADEMap::nearestThingMulti(fpoint2_t point)
{
	// Go through things
	vector<int> ret;
	double min_dist = 999999999;
	MapThing* t = NULL;
	double dist = 0;
	for (unsigned a = 0; a < things.size(); a++)
	{
		t = things[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(t->point());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			ret.clear();
			ret.push_back(a);
			min_dist = dist;
		}
		else if (dist == min_dist)
			ret.push_back(a);
	}

	return ret;
}

/* SLADEMap::sectorAt
 * Returns the index of the sector at the given point, or -1 if not
 * within a sector
 *******************************************************************/
int SLADEMap::sectorAt(fpoint2_t point)
{
	// Go through sectors
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		// Check if point is within sector
		if (sectors[a]->isWithin(point))
			return a;
	}

	// Not within a sector
	return -1;
}

/* SLADEMap::getMapBBox
 * Returns a bounding box for the entire map
 *******************************************************************/
bbox_t SLADEMap::getMapBBox()
{
	bbox_t bbox;

	// Return invalid bbox if no sectors
	if (sectors.size() == 0)
		return bbox;

	// Go through sectors
	// This is quicker than generating it from vertices,
	// but relies on sector bboxes being up-to-date (which they should be)
	bbox = sectors[0]->boundingBox();
	for (unsigned a = 1; a < sectors.size(); a++)
	{
		bbox_t sbb = sectors[a]->boundingBox();
		if (sbb.min.x < bbox.min.x)
			bbox.min.x = sbb.min.x;
		if (sbb.min.y < bbox.min.y)
			bbox.min.y = sbb.min.y;
		if (sbb.max.x > bbox.max.x)
			bbox.max.x = sbb.max.x;
		if (sbb.max.y > bbox.max.y)
			bbox.max.y = sbb.max.y;
	}

	return bbox;
}

/* SLADEMap::vertexAt
 * Returns the vertex at [x,y], or NULL if none there
 *******************************************************************/
MapVertex* SLADEMap::vertexAt(double x, double y)
{
	// Go through all vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a]->x == x && vertices[a]->y == y)
			return vertices[a];
	}

	// No vertex at [x,y]
	return NULL;
}

// Sorting functions for SLADEMap::cutLines
bool sortVPosXAsc(const fpoint2_t& left, const fpoint2_t& right)
{
	return left.x < right.x;
}
bool sortVPosXDesc(const fpoint2_t& left, const fpoint2_t& right)
{
	return left.x > right.x;
}
bool sortVPosYAsc(const fpoint2_t& left, const fpoint2_t& right)
{
	return left.y < right.y;
}
bool sortVPosYDesc(const fpoint2_t& left, const fpoint2_t& right)
{
	return left.y > right.y;
}

/* SLADEMap::cutLines
 * Returns a list of points that the 'cutting' line from [x1,y1] to
 * [x2,y2] crosses any existing lines on the map. The list is sorted
 * along the direction of the 'cutting' line
 *******************************************************************/
vector<fpoint2_t> SLADEMap::cutLines(double x1, double y1, double x2, double y2)
{
	fseg2_t cutter(x1, y1, x2, y2);
	// Init
	vector<fpoint2_t> intersect_points;
	fpoint2_t intersection;

	// Go through map lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Check for intersection
		intersection = cutter.p1();
		if (MathStuff::linesIntersect(cutter, lines[a]->seg(), intersection))
		{
			// Add intersection point to vector
			intersect_points.push_back(intersection);
			LOG_DEBUG("Intersection point", intersection, "valid with", lines[a]);
		}
		else if (intersection != cutter.p1())
		{
			LOG_DEBUG("Intersection point", intersection, "invalid");
		}
	}

	// Return if no intersections
	if (intersect_points.size() == 0)
		return intersect_points;

	// Check cutting line direction
	double xdif = x2 - x1;
	double ydif = y2 - y1;
	if ((xdif*xdif) > (ydif*ydif))
	{
		// Sort points along x axis
		if (xdif >= 0)
			std::sort(intersect_points.begin(), intersect_points.end(), sortVPosXAsc);
		else
			std::sort(intersect_points.begin(), intersect_points.end(), sortVPosXDesc);
	}
	else
	{
		// Sort points along y axis
		if (ydif >= 0)
			std::sort(intersect_points.begin(), intersect_points.end(), sortVPosYAsc);
		else
			std::sort(intersect_points.begin(), intersect_points.end(), sortVPosYDesc);
	}

	return intersect_points;
}

/* SLADEMap::lineCrossVertex
 * Returns the first vertex that the line from [x1,y1] to [x2,y2]
 * crosses over
 *******************************************************************/
MapVertex* SLADEMap::lineCrossVertex(double x1, double y1, double x2, double y2)
{
	fseg2_t seg(x1, y1, x2, y2);

	// Go through vertices
	MapVertex* cv = NULL;
	double min_dist = 999999;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		MapVertex* vertex = vertices[a];
		fpoint2_t point = vertex->point();

		// Skip if outside line bbox
		if (!seg.contains(point))
			continue;

		// Skip if it's at an end of the line
		if (point == seg.p1() || point == seg.p2())
			continue;

		// Check if on line
		if (MathStuff::distanceToLineFast(point, seg) == 0)
		{
			// Check distance between line start and vertex
			double dist = MathStuff::distance(seg.p1(), point);
			if (dist < min_dist)
			{
				cv = vertex;
				min_dist = dist;
			}
		}
	}

	// Return closest overlapping vertex to line start
	return cv;
}

/* SLADEMap::updateGeometryInfo
 * Updates geometry info (polygons/bbox/etc) for anything modified
 * since [modified_time]
 *******************************************************************/
void SLADEMap::updateGeometryInfo(long modified_time)
{
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a]->modifiedTime() > modified_time)
		{
			for (unsigned l = 0; l < vertices[a]->connected_lines.size(); l++)
			{
				MapLine* line = vertices[a]->connected_lines[l];

				// Update line geometry
				line->resetInternals();

				// Update front sector
				if (line->frontSector())
				{
					line->frontSector()->resetPolygon();
					line->frontSector()->updateBBox();
				}

				// Update back sector
				if (line->backSector())
				{
					line->backSector()->resetPolygon();
					line->backSector()->updateBBox();
				}
			}
		}
	}
}

/* SLADEMap::linesIntersect
 * Returns true if [line1] and [line2] intersect. If an intersection
 * occurs, [x] and [y] are set to the intersection point
 *******************************************************************/
bool SLADEMap::linesIntersect(MapLine* line1, MapLine* line2, double& x, double& y)
{
	fpoint2_t intersection;
	bool res = MathStuff::linesIntersect(line1->seg(), line2->seg(), intersection);
	x = intersection.x;
	y = intersection.y;
	return res;
}

/* SLADEMap::findSectorTextPoint
 * Finds the 'text point' for [sector]. This is a point within the
 * sector that is reasonably close to the middle of the sector bbox
 * while still being within the sector itself
 *******************************************************************/
void SLADEMap::findSectorTextPoint(MapSector* sector)
{
	// Check sector
	if (!sector)
		return;

	// Check if actual sector midpoint can be used
	sector->text_point = sector->getPoint(MOBJ_POINT_MID);
	if (sector->isWithin(sector->text_point))
		return;

	if (sector->connected_sides.size() == 0)
		return;

	// Find nearest line to sector midpoint (that is also part of the sector)
	double min_dist = 9999999999.0;
	MapSide* mid_side = sector->connected_sides[0];
	for (unsigned a = 0; a < sector->connected_sides.size(); a++)
	{
		MapLine* l = sector->connected_sides[a]->parent;
		double dist = MathStuff::distanceToLineFast(sector->text_point, l->seg());

		if (dist < min_dist)
		{
			min_dist = dist;
			mid_side = sector->connected_sides[a];
		}
	}

	// Calculate ray
	fpoint2_t r_o = mid_side->parent->getPoint(MOBJ_POINT_MID);
	fpoint2_t r_d = mid_side->parent->frontVector();
	if (mid_side == mid_side->parent->side1)
		r_d.set(-r_d.x, -r_d.y);

	// Find nearest intersecting line
	min_dist = 9999999999.0;
	for (unsigned a = 0; a < sector->connected_sides.size(); a++)
	{
		if (sector->connected_sides[a] == mid_side)
			continue;

		MapLine* line = sector->connected_sides[a]->parent;
		double dist = MathStuff::distanceRayLine(r_o, r_o + r_d, line->point1(), line->point2());

		if (dist > 0 && dist < min_dist)
			min_dist = dist;
	}

	// Set text point to halfway between the two lines
	sector->text_point.set(r_o.x + (r_d.x * min_dist * 0.5), r_o.y + (r_d.y * min_dist * 0.5));
}

/* SLADEMap::initSectorPolygons
 * Forces building of polygons for all sectors
 *******************************************************************/
void SLADEMap::initSectorPolygons()
{
	UI::setSplashProgressMessage("Building sector polygons");
	UI::setSplashProgress(0.0f);
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		UI::setSplashProgress((float)a / (float)sectors.size());
		sectors[a]->getPolygon();
	}
	UI::setSplashProgress(1.0f);
}

MapLine* SLADEMap::lineVectorIntersect(MapLine* line, bool front, double& hit_x, double& hit_y)
{
	// Get sector
	MapSector* sector = front ? line->frontSector() : line->backSector();
	if (!sector)
		return NULL;

	// Get lines to test
	vector<MapLine*> lines;
	sector->getLines(lines);

	// Get nearest line intersecting with line vector
	MapLine* nearest = NULL;
	fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
	fpoint2_t vec = line->frontVector();
	if (front)
	{
		vec.x = -vec.x;
		vec.y = -vec.y;
	}
	double min_dist = 99999999999;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a] == line)
			continue;

		double dist = MathStuff::distanceRayLine(mid, mid + vec, lines[a]->point1(), lines[a]->point2());

		if (dist < min_dist && dist > 0)
		{
			min_dist = dist;
			nearest = lines[a];
		}
	}

	// Set intersection point
	if (nearest)
	{
		hit_x = mid.x + (vec.x * min_dist);
		hit_y = mid.y + (vec.y * min_dist);
	}

	return nearest;
}

/* SLADEMap::getSectorsByTag
 * Adds all sectors with tag [tag] to [list]
 *******************************************************************/
void SLADEMap::getSectorsByTag(int tag, vector<MapSector*>& list)
{
	if (tag == 0)
		return;

	// Find sectors with matching tag
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		if (sectors[a]->intProperty("id") == tag)
			list.push_back(sectors[a]);
	}
}

/* SLADEMap::getThingsById
 * Adds all things with TID [id] to [list]. If [type] is not 0, only
 * checks things of that type
 *******************************************************************/
void SLADEMap::getThingsById(int id, vector<MapThing*>& list, unsigned start, int type)
{
	if (id == 0)
		return;

	// Find things with matching id
	for (unsigned a = start; a < things.size(); a++)
	{
		if (things[a]->intProperty("id") == id && (type == 0 || things[a]->type == type))
			list.push_back(things[a]);
	}
}

/* SLADEMap::getFirstThingWithId
 * Returns the first thing in the map with TID [id]
 *******************************************************************/
MapThing* SLADEMap::getFirstThingWithId(int id)
{
	if (id == 0)
		return NULL;

	// Find things with matching id, but ignore dragons, we don't want them!
	for (unsigned a = 0; a < things.size(); a++)
	{
		ThingType* tt = theGameConfiguration->thingType(things[a]->getType());
		if (things[a]->intProperty("id") == id && !(tt->getFlags() & THING_DRAGON))
			return things[a];
	}
	return NULL;
}

/* SLADEMap::getThingsByIdInSectorTag
 * Adds all things with TID [id] that are also within a sector with
 * tag [tag] to [list]
 *******************************************************************/
void SLADEMap::getThingsByIdInSectorTag(int id, int tag, vector<MapThing*>& list)
{
	if (id==0 && tag==0)
		return;

	// Find things with matching id contained in sector with matching tag
	for (unsigned a = 0; a < things.size(); a++)
	{
		if (things[a]->intProperty("id") == id)
		{
			int si = sectorAt(things[a]->point());
			if (si > -1 && (unsigned)si < sectors.size() && sectors[si]->intProperty("id") == tag)
			{
				list.push_back(things[a]);
			}
		}
	}
}

/* SLADEMap::getDragonTargets
 * Gets dragon targets (needs better description)
 *******************************************************************/
WX_DECLARE_HASH_MAP(int, int, wxIntegerHash, wxIntegerEqual, UsedValuesMap);
void SLADEMap::getDragonTargets(MapThing* first, vector<MapThing*>& list)
{
	UsedValuesMap used;
	list.clear();
	list.push_back(first);
	unsigned i = 0;
	while (i < list.size())
	{
		string prop = "arg_";
		for (int a = 0; a < 5; ++a)
		{
			prop[3] = ('0' + a);
			int val = list[i]->intProperty(prop);
			if (val && used[val] == 0)
			{
				used[val] = 1;
				getThingsById(val, list);
			}
		}
		++i;
	}
}

/* SLADEMap::getPathedThings
 * Adds all things with a 'pathed' type to [list]
 *******************************************************************/
void SLADEMap::getPathedThings(vector<MapThing*>& list)
{
	// Find things that need to be pathed
	for (unsigned a = 0; a < things.size(); a++)
	{
		ThingType* tt = theGameConfiguration->thingType(things[a]->getType());
		if (tt->getFlags() & (THING_PATHED|THING_DRAGON))
			list.push_back(things[a]);
	}
}

/* SLADEMap::getLinesById
 * Adds all lines with [id] to [list]
 *******************************************************************/
void SLADEMap::getLinesById(int id, vector<MapLine*>& list)
{
	if (id == 0)
		return;

	// Find lines with matching id
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a]->intProperty("id") == id)
			list.push_back(lines[a]);
	}
}

/* SLADEMap::getTaggingThingsById
 * Adds all things with special affecting matching id to [list]
 *******************************************************************/
void SLADEMap::getTaggingThingsById(int id, int type, vector<MapThing*>& list, int ttype)
{
	// Find things with special affecting matching id
	int needs_tag, tag, arg2, arg3, arg4, arg5, tid;
	for (unsigned a = 0; a < things.size(); a++)
	{
		ThingType* tt = theGameConfiguration->thingType(things[a]->getType());
		if (tt->needsTag() || (things[a]->intProperty("special") && !(tt->getFlags() & THING_SCRIPT)))
		{
			needs_tag = tt->needsTag() ? tt->needsTag() : theGameConfiguration->actionSpecial(things[a]->intProperty("special"))->needsTag();
			tag = things[a]->intProperty("arg0");
			bool fits = false;
			switch (needs_tag)
			{
			case AS_TT_SECTOR:
			case AS_TT_SECTOR_OR_BACK:
			case AS_TT_SECTOR_AND_BACK:
				fits = (IDEQ(tag) && type == SECTORS);
				break;
			case AS_TT_LINE_NEGATIVE:
				tag = abs(tag);
			case AS_TT_LINE:
				fits = (IDEQ(tag) && type == LINEDEFS);
				break;
			case AS_TT_THING:
				fits = (IDEQ(tag) && type == THINGS);
				break;
			case AS_TT_1THING_2SECTOR:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SECTORS));
				break;
			case AS_TT_1THING_3SECTOR:
				arg3 = things[a]->intProperty("arg2");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SECTORS));
				break;
			case AS_TT_1THING_2THING:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case AS_TT_1THING_4THING:
				arg4 = things[a]->intProperty("arg3");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case AS_TT_1THING_2THING_3THING:
				arg2 = things[a]->intProperty("arg1");
				arg3 = things[a]->intProperty("arg2");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case AS_TT_1SECTOR_2THING_3THING_5THING:
				arg2 = things[a]->intProperty("arg1");
				arg3 = things[a]->intProperty("arg2");
				arg5 = things[a]->intProperty("arg4");
				fits = (type == SECTORS ? (IDEQ(tag)) : (type == THINGS &&
						(IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case AS_TT_1LINEID_2LINE:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == LINEDEFS && IDEQ(arg2));
				break;
			case AS_TT_4THING:
				arg4 = things[a]->intProperty("arg3");
				fits = (type == THINGS && IDEQ(arg4));
				break;
			case AS_TT_5THING:
				arg5 = things[a]->intProperty("arg4");
				fits = (type == THINGS && IDEQ(arg5));
				break;
			case AS_TT_1LINE_2SECTOR:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SECTORS));
				break;
			case AS_TT_1SECTOR_2SECTOR:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR:
				arg2 = things[a]->intProperty("arg1");
				arg3 = things[a]->intProperty("arg2");
				arg4 = things[a]->intProperty("arg3");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case AS_TT_SECTOR_2IS3_LINE:
				arg2 = things[a]->intProperty("arg1");
				fits = (IDEQ(tag) && (arg2 == 3 ? type == LINEDEFS : type == SECTORS));
				break;
			case AS_TT_1SECTOR_2THING:
				arg2 = things[a]->intProperty("arg1");
				fits = (type == SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == THINGS));
				break;
			default:
				// Kind of a hack here. Patrol points and interpolation points only tag
				// certain thing types with the same TID as themselves. Fortunately,
				// the thing types in question are in the 9000 range, and the TagTypes
				// enum is quite unlikely to reach that far. :p
				tid = things[a]->intProperty("id");
				ThingType* tt = theGameConfiguration->thingType(things[a]->getType());
				fits = ((needs_tag == ttype) && (IDEQ(tid)) && (tt->needsTag() == needs_tag));
				break;
			}
			if (fits) list.push_back(things[a]);
		}
	}
}

/* SLADEMap::getTaggingLinesById
 * Adds all lines with special affecting matching id to [list]
 *******************************************************************/
void SLADEMap::getTaggingLinesById(int id, int type, vector<MapLine*>& list)
{
	// Find lines with special affecting matching id
	int needs_tag, tag, arg2, arg3, arg4, arg5;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		int special = lines[a]->special;
		if (special)
		{
			needs_tag = theGameConfiguration->actionSpecial(lines[a]->special)->needsTag();
			tag = lines[a]->intProperty("arg0");
			bool fits = false;
			switch (needs_tag)
			{
			case AS_TT_SECTOR:
			case AS_TT_SECTOR_OR_BACK:
			case AS_TT_SECTOR_AND_BACK:
				fits = (IDEQ(tag) && type == SECTORS);
				break;
			case AS_TT_LINE_NEGATIVE:
				tag = abs(tag);
			case AS_TT_LINE:
				fits = (IDEQ(tag) && type == LINEDEFS);
				break;
			case AS_TT_THING:
				fits = (IDEQ(tag) && type == THINGS);
				break;
			case AS_TT_1THING_2SECTOR:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SECTORS));
				break;
			case AS_TT_1THING_3SECTOR:
				arg3 = lines[a]->intProperty("arg2");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SECTORS));
				break;
			case AS_TT_1THING_2THING:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case AS_TT_1THING_4THING:
				arg4 = lines[a]->intProperty("arg3");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case AS_TT_1THING_2THING_3THING:
				arg2 = lines[a]->intProperty("arg1");
				arg3 = lines[a]->intProperty("arg2");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case AS_TT_1SECTOR_2THING_3THING_5THING:
				arg2 = lines[a]->intProperty("arg1");
				arg3 = lines[a]->intProperty("arg2");
				arg5 = lines[a]->intProperty("arg4");
				fits = (type == SECTORS ? (IDEQ(tag)) : (type == THINGS &&
						(IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case AS_TT_1LINEID_2LINE:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == LINEDEFS && IDEQ(arg2));
				break;
			case AS_TT_4THING:
				arg4 = lines[a]->intProperty("arg3");
				fits = (type == THINGS && IDEQ(arg4));
				break;
			case AS_TT_5THING:
				arg5 = lines[a]->intProperty("arg4");
				fits = (type == THINGS && IDEQ(arg5));
				break;
			case AS_TT_1LINE_2SECTOR:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SECTORS));
				break;
			case AS_TT_1SECTOR_2SECTOR:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR:
				arg2 = lines[a]->intProperty("arg1");
				arg3 = lines[a]->intProperty("arg2");
				arg4 = lines[a]->intProperty("arg3");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case AS_TT_SECTOR_2IS3_LINE:
				arg2 = lines[a]->intProperty("arg1");
				fits = (IDEQ(tag) && (arg2 == 3 ? type == LINEDEFS : type == SECTORS));
				break;
			case AS_TT_1SECTOR_2THING:
				arg2 = lines[a]->intProperty("arg1");
				fits = (type == SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == THINGS));
				break;
			default:
				break;
			}
			if (fits) list.push_back(lines[a]);
		}
	}
}

/* SLADEMap::findUnusedSectorTag
 * Returns the lowest unused sector tag
 *******************************************************************/
int SLADEMap::findUnusedSectorTag()
{
	int tag = 1;
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		if (sectors[a]->intProperty("id") == tag)
		{
			tag++;
			a = 0;
		}
	}

	return tag;
}

/* SLADEMap::findUnusedThingId
 * Returns the lowest unused thing id
 *******************************************************************/
int SLADEMap::findUnusedThingId()
{
	int tag = 1;
	for (unsigned a = 0; a < things.size(); a++)
	{
		if (things[a]->intProperty("id") == tag)
		{
			tag++;
			a = 0;
		}
	}

	return tag;
}

/* SLADEMap::findUnusedLineId
 * Returns the lowest unused line id
 *******************************************************************/
int SLADEMap::findUnusedLineId()
{
	int tag = 1;

	// UDMF (id property)
	if (current_format == MAP_UDMF)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->intProperty("id") == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	// Hexen (special 121 arg0)
	else if (current_format == MAP_HEXEN)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->special == 121 && lines[a]->intProperty("arg0") == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	// Boom (sector tag (arg0))
	else if (current_format == MAP_DOOM && theGameConfiguration->isBoom())
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->intProperty("arg0") == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	return tag;
}

/* SLADEMap::getAdjecentLineTexture
 * Returns the first texture at [tex_part] found on lines connected
 * to [vertex]
 *******************************************************************/
string SLADEMap::getAdjacentLineTexture(MapVertex* vertex, int tex_part)
{
	// Go through adjacent lines
	string tex = "-";
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		MapLine* l = vertex->connectedLine(a);

		if (l->side1)
		{
			// Front middle
			if (tex_part & TEX_FRONT_MIDDLE)
			{
				tex = l->stringProperty("side1.texturemiddle");
				if (tex != "-")
					return tex;
			}

			// Front upper
			if (tex_part & TEX_FRONT_UPPER)
			{
				tex = l->stringProperty("side1.texturetop");
				if (tex != "-")
					return tex;
			}

			// Front lower
			if (tex_part & TEX_FRONT_LOWER)
			{
				tex = l->stringProperty("side1.texturebottom");
				if (tex != "-")
					return tex;
			}
		}

		if (l->side2)
		{
			// Back middle
			if (tex_part & TEX_BACK_MIDDLE)
			{
				tex = l->stringProperty("side2.texturemiddle");
				if (tex != "-")
					return tex;
			}

			// Back upper
			if (tex_part & TEX_BACK_UPPER)
			{
				tex = l->stringProperty("side2.texturetop");
				if (tex != "-")
					return tex;
			}

			// Back lower
			if (tex_part & TEX_BACK_LOWER)
			{
				tex = l->stringProperty("side2.texturebottom");
				if (tex != "-")
					return tex;
			}
		}
	}

	return tex;
}

/* SLADEMap::getLineSideSector
 * Returns the sector on the front or back side of [line] (ignoring
 * the line side itself, used for correcting sector refs)
 *******************************************************************/
MapSector* SLADEMap::getLineSideSector(MapLine* line, bool front)
{
	// Get mid and direction points
	fpoint2_t mid = line->getPoint(MOBJ_POINT_MID);
	fpoint2_t dir = line->frontVector();
	if (front)
		dir = mid - dir;
	else
		dir = mid + dir;

	// Rotate very slightly to avoid some common cases where
	// the ray will cross a vertex exactly
	dir = MathStuff::rotatePoint(mid, dir, 0.01);

	// Find closest line intersecting front/back vector
	double dist;
	double min_dist = 99999999;
	int index = -1;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a] == line)
			continue;

		dist = MathStuff::distanceRayLine(mid, dir, lines[a]->point1(), lines[a]->point2());
		if (dist < min_dist && dist > 0)
		{
			min_dist = dist;
			index = a;
		}
	}

	// If any intersection found, check what side of the intersected line this is on
	// and return the appropriate sector
	if (index >= 0)
	{
		//LOG_MESSAGE(3, "Closest line %d", index);
		MapLine* l = lines[index];

		// Check side of line
		MapSector* sector = NULL;
		if (MathStuff::lineSide(mid, l->seg()) >= 0)
			sector = l->frontSector();
		else
			sector = l->backSector();

		// Just return the sector if it already matches
		if (front && sector == line->frontSector())
			return sector;
		if (!front && sector == line->backSector())
			return sector;

		// Check if we can trace back from the front side
		SectorBuilder builder;
		builder.traceSector(this, l, true);
		for (unsigned a = 0; a < builder.nEdges(); a++)
		{
			if (builder.getEdgeLine(a) == line && builder.edgeIsFront(a) == front)
				return l->frontSector();
		}

		// Can't trace back from front side, must be back side
		return l->backSector();
	}

	return NULL;
}

/* SLADEMap::getModifiedObjects
 * Returns a list of objects of [type] that have a modified time
 * later than [since]
 *******************************************************************/
vector<MapObject*> SLADEMap::getModifiedObjects(long since, int type)
{
	vector<MapObject*> modified_objects;

	// Vertices
	if (type < 0 || type == MOBJ_VERTEX)
	{
		for (unsigned a = 0; a < vertices.size(); a++)
		{
			if (vertices[a]->modifiedTime() >= since)
				modified_objects.push_back(vertices[a]);
		}
	}

	// Sides
	if (type < 0 || type == MOBJ_SIDE)
	{
		for (unsigned a = 0; a < sides.size(); a++)
		{
			if (sides[a]->modifiedTime() >= since)
				modified_objects.push_back(sides[a]);
		}
	}

	// Lines
	if (type < 0 || type == MOBJ_LINE)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->modifiedTime() >= since)
				modified_objects.push_back(lines[a]);
		}
	}

	// Sectors
	if (type < 0 || type == MOBJ_SECTOR)
	{
		for (unsigned a = 0; a < sectors.size(); a++)
		{
			if (sectors[a]->modifiedTime() >= since)
				modified_objects.push_back(sectors[a]);
		}
	}

	// Things
	if (type < 0 || type == MOBJ_THING)
	{
		for (unsigned a = 0; a < things.size(); a++)
		{
			if (things[a]->modifiedTime() >= since)
				modified_objects.push_back(things[a]);
		}
	}

	return modified_objects;
}

/* SLADEMap::getAllModifiedObjects
 * Returns a list of objects that have a modified time later than
 * [since]
 *******************************************************************/
vector<MapObject*> SLADEMap::getAllModifiedObjects(long since)
{
	vector<MapObject*> modified_objects;

	for (unsigned a = 0; a < all_objects.size(); a++)
	{
		if (all_objects[a].mobj && all_objects[a].mobj->modifiedTime() >= since)
			modified_objects.push_back(all_objects[a].mobj);
	}

	return modified_objects;
}

/* SLADEMap::getLastModifiedTime
 * Returns the newest modified time on any map object
 *******************************************************************/
long SLADEMap::getLastModifiedTime()
{
	long mod_time = 0;

	for (unsigned a = 0; a < all_objects.size(); a++)
	{
		if (all_objects[a].mobj && all_objects[a].mobj->modifiedTime() > mod_time)
			mod_time = all_objects[a].mobj->modifiedTime();
	}

	return mod_time;
}

/* SLADEMap::isModified
 * Returns true if any map object has been modified since it was
 * opened or last saved
 *******************************************************************/
bool SLADEMap::isModified()
{
	if (getLastModifiedTime() > opened_time)
		return true;
	else
		return false;
}

/* SLADEMap::setOpenedTime
 * Sets the map opened time to now
 *******************************************************************/
void SLADEMap::setOpenedTime()
{
	opened_time = App::runTimer();
}

/* SLADEMap::modifiedSince
 * Returns true if any objects of [type] have a modified time newer
 * than [since]
 *******************************************************************/
bool SLADEMap::modifiedSince(long since, int type)
{
	// Any type
	if (type < 0)
		return getLastModifiedTime() > since;

	// Vertices
	else if (type == MOBJ_VERTEX)
	{
		for (unsigned a = 0; a < vertices.size(); a++)
		{
			if (vertices[a]->modified_time > since)
				return true;
		}
	}

	// Lines
	else if (type == MOBJ_LINE)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->modified_time > since)
				return true;
		}
	}

	// Sides
	else if (type == MOBJ_SIDE)
	{
		for (unsigned a = 0; a < sides.size(); a++)
		{
			if (sides[a]->modified_time > since)
				return true;
		}
	}

	// Sectors
	else if (type == MOBJ_SECTOR)
	{
		for (unsigned a = 0; a < sectors.size(); a++)
		{
			if (sectors[a]->modified_time > since)
				return true;
		}
	}

	// Things
	else if (type == MOBJ_THING)
	{
		for (unsigned a = 0; a < things.size(); a++)
		{
			if (things[a]->modified_time > since)
				return true;
		}
	}

	return false;
}

/* SLADEMap::recomputeSpecials
 * Re-applies all the currently calculated special map properties (currently
 * this just means ZDoom slopes).
 * Since this needs to be done anytime the map changes, it's called whenever a
 * map is read, an undo record ends, or an undo/redo is performed.
 *******************************************************************/
void SLADEMap::recomputeSpecials()
{
	map_specials.processMapSpecials(this);
}

/* SLADEMap::createVertex
 * Creates a new vertex at [x,y] and returns it. Splits any lines
 * within [split_dist] from the position
 *******************************************************************/
MapVertex* SLADEMap::createVertex(double x, double y, double split_dist)
{
	// Round position to integral if fractional positions are disabled
	if (!position_frac)
	{
		x = MathStuff::round(x);
		y = MathStuff::round(y);
	}

	fpoint2_t point(x, y);

	// First check that it won't overlap any other vertex
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a]->x == x && vertices[a]->y == y)
			return vertices[a];
	}

	// Create the vertex
	MapVertex* nv = new MapVertex(x, y, this);
	nv->index = vertices.size();
	vertices.push_back(nv);

	// Check if this vertex splits any lines (if needed)
	if (split_dist >= 0)
	{
		unsigned nlines = lines.size();
		for (unsigned a = 0; a < nlines; a++)
		{
			// Skip line if it shares the vertex
			if (lines[a]->v1() == nv || lines[a]->v2() == nv)
				continue;

			if (lines[a]->distanceTo(point) < split_dist)
			{
				//LOG_MESSAGE(1, "Vertex at (%1.2f,%1.2f) splits line %d", x, y, a);
				splitLine(lines[a], nv);
			}
		}
	}

	// Set geometry age
	geometry_updated = App::runTimer();

	return nv;
}

/* SLADEMap::createLine
 * Creates a new line and needed vertices from [x1,y1] to [x2,y2] and
 * returns it
 *******************************************************************/
MapLine* SLADEMap::createLine(double x1, double y1, double x2, double y2, double split_dist)
{
	// Round coordinates to integral if fractional positions are disabled
	if (!position_frac)
	{
		x1 = MathStuff::round(x1);
		y1 = MathStuff::round(y1);
		x2 = MathStuff::round(x2);
		y2 = MathStuff::round(y2);
	}

	//LOG_MESSAGE(1, "Create line (%1.2f,%1.2f) to (%1.2f,%1.2f)", x1, y1, x2, y2);

	// Get vertices at points
	MapVertex* vertex1 = vertexAt(x1, y1);
	MapVertex* vertex2 = vertexAt(x2, y2);

	// Create vertices if required
	if (!vertex1)
		vertex1 = createVertex(x1, y1, split_dist);
	if (!vertex2)
		vertex2 = createVertex(x2, y2, split_dist);

	// Create line between vertices
	return createLine(vertex1, vertex2);
}

/* SLADEMap::createLine
 * Creates a new line between [vertex1] and [vertex2] and returns it
 *******************************************************************/
MapLine* SLADEMap::createLine(MapVertex* vertex1, MapVertex* vertex2, bool force)
{
	// Check both vertices were given
	if (!vertex1 || vertex1->parent_map != this)
		return NULL;
	if (!vertex2 || vertex2->parent_map != this)
		return NULL;

	// Check if there is already a line along the two given vertices
	if(!force)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if ((lines[a]->vertex1 == vertex1 && lines[a]->vertex2 == vertex2) ||
					(lines[a]->vertex2 == vertex1 && lines[a]->vertex1 == vertex2))
				return lines[a];
		}
	}

	// Create new line between vertices
	MapLine* nl = new MapLine(vertex1, vertex2, NULL, NULL, this);
	nl->index = lines.size();
	lines.push_back(nl);

	// Connect line to vertices
	vertex1->connectLine(nl);
	vertex2->connectLine(nl);

	// Set geometry age
	geometry_updated = App::runTimer();

	return nl;
}

/* SLADEMap::createThing
 * Creates a new thing at [x,y] and returns it
 *******************************************************************/
MapThing* SLADEMap::createThing(double x, double y)
{
	// Create the thing
	MapThing* nt = new MapThing(this);

	// Setup initial values
	nt->x = x;
	nt->y = y;
	nt->index = things.size();
	nt->type = 1;

	// Add to things
	things.push_back(nt);
	things_updated = App::runTimer();

	return nt;
}

/* SLADEMap::createSector
 * Creates a new sector and returns it
 *******************************************************************/
MapSector* SLADEMap::createSector()
{
	// Create the sector
	MapSector* ns = new MapSector(this);

	// Setup initial values
	ns->index = sectors.size();

	// Add to sectors
	sectors.push_back(ns);

	return ns;
}

/* SLADEMap::createSide
 * Creates a new side and returns it
 *******************************************************************/
MapSide* SLADEMap::createSide(MapSector* sector)
{
	// Check sector
	if (!sector)
		return NULL;

	// Create side
	MapSide* side = new MapSide(sector, this);

	// Setup initial values
	side->index = sides.size();
	side->tex_middle = "-";
	side->tex_upper = "-";
	side->tex_lower = "-";
	usage_tex["-"] += 3;

	// Add to sides
	sides.push_back(side);

	return side;
}

/* SLADEMap::moveVertex
 * Moves [vertex] to new position [nx,ny]
 *******************************************************************/
void SLADEMap::moveVertex(unsigned vertex, double nx, double ny)
{
	// Check index
	if (vertex >= vertices.size())
		return;

	// Move the vertex
	MapVertex* v = vertices[vertex];
	v->setModified();
	v->x = nx;
	v->y = ny;

	// Reset all attached lines' geometry info
	for (unsigned a = 0; a < v->connected_lines.size(); a++)
		v->connected_lines[a]->resetInternals();

	geometry_updated = App::runTimer();
}

/* SLADEMap::mergeVertices
 * Merges vertices at index [vertex1] and [vertex2], removing the
 * second
 *******************************************************************/
void SLADEMap::mergeVertices(unsigned vertex1, unsigned vertex2)
{
	// Check indices
	if (vertex1 >= vertices.size() || vertex2 >= vertices.size() || vertex1 == vertex2)
		return;

	// Go through lines of second vertex
	MapVertex* v1 = vertices[vertex1];
	MapVertex* v2 = vertices[vertex2];
	vector<MapLine*> zlines;
	for (unsigned a = 0; a < v2->connected_lines.size(); a++)
	{
		MapLine* line = v2->connected_lines[a];

		// Change first vertex if needed
		if (line->vertex1 == v2)
		{
			line->setModified();
			line->vertex1 = v1;
			line->length = -1;
			v1->connectLine(line);
		}

		// Change second vertex if needed
		if (line->vertex2 == v2)
		{
			line->setModified();
			line->vertex2 = v1;
			line->length = -1;
			v1->connectLine(line);
		}

		if (line->vertex1 == v1 && line->vertex2 == v1)
			zlines.push_back(line);
	}

	// Delete the vertex
	LOG_MESSAGE(4, "Merging vertices %u and %u (removing %u)", vertex1, vertex2, vertex2);
	removeMapObject(v2);
	vertices[vertex2] = vertices.back();
	vertices[vertex2]->index = vertex2;
	vertices.pop_back();

	// Delete any resulting zero-length lines
	for (unsigned a = 0; a < zlines.size(); a++)
	{
		LOG_MESSAGE(4, "Removing zero-length line %u", zlines[a]->getIndex());
		removeLine(zlines[a]);
	}

	geometry_updated = App::runTimer();
}

/* SLADEMap::mergeVerticesPoint
 * Merges all vertices at [x,1] and returns the resulting single
 * vertex
 *******************************************************************/
MapVertex* SLADEMap::mergeVerticesPoint(double x, double y)
{
	// Go through all vertices
	int merge = -1;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// Skip if vertex isn't on the point
		if (vertices[a]->x != x || vertices[a]->y != y)
			continue;

		// Set as the merge target vertex if we don't have one already
		if (merge < 0)
		{
			merge = a;
			continue;
		}

		// Otherwise, merge this vertex with the merge target
		mergeVertices(merge, a);
		a--;
	}

	geometry_updated = App::runTimer();

	// Return the final merged vertex
	return getVertex(merge);
}

/* SLADEMap::splitLine
 * Splits [line] at [vertex]
 *******************************************************************/
MapLine* SLADEMap::splitLine(MapLine* l, MapVertex* v)
{
	if (!l || !v)
		return NULL;

	// Shorten line
	MapVertex* v2 = l->vertex2;
	l->setModified();
	v2->disconnectLine(l);
	l->vertex2 = v;
	v->connectLine(l);
	l->length = -1;

	// Create and add new sides
	MapSide* s1 = NULL;
	MapSide* s2 = NULL;
	if (l->side1)
	{
		// Create side 1
		s1 = new MapSide(this);
		s1->copy(l->side1);
		s1->setSector(l->side1->sector);
		if (s1->sector)
		{
			s1->sector->resetBBox();
			s1->sector->resetPolygon();
		}

		// Add side
		s1->index = sides.size();
		sides.push_back(s1);

		// Update texture counts
		usage_tex[s1->tex_upper.Upper()] += 1;
		usage_tex[s1->tex_middle.Upper()] += 1;
		usage_tex[s1->tex_lower.Upper()] += 1;
	}
	if (l->side2)
	{
		// Create side 2
		s2 = new MapSide(this);
		s2->copy(l->side2);
		s2->setSector(l->side2->sector);
		if (s2->sector)
		{
			s2->sector->resetBBox();
			s2->sector->resetPolygon();
		}

		// Add side
		s2->index = sides.size();
		sides.push_back(s2);

		// Update texture counts
		usage_tex[s2->tex_upper.Upper()] += 1;
		usage_tex[s2->tex_middle.Upper()] += 1;
		usage_tex[s2->tex_lower.Upper()] += 1;
	}

	// Create and add new line
	MapLine* nl = new MapLine(v, v2, s1, s2, this);
	nl->copy(l);
	nl->index = lines.size();
	nl->setModified();
	lines.push_back(nl);

	// Update x-offsets
	if (map_split_auto_offset)
	{
		int xoff1 = l->intProperty("side1.offsetx");
		int xoff2 = l->intProperty("side2.offsetx");
		nl->setIntProperty("side1.offsetx", xoff1 + l->getLength());
		l->setIntProperty("side2.offsetx", xoff2 + nl->getLength());
	}

	geometry_updated = App::runTimer();

	return nl;
}

/* SLADEMap::moveThing
 * Moves the thing at index [thing] to new position [nx,ny]
 *******************************************************************/
void SLADEMap::moveThing(unsigned thing, double nx, double ny)
{
	// Check index
	if (thing >= things.size())
		return;

	// Move the thing
	MapThing* t = things[thing];
	t->setModified();
	t->x = nx;
	t->y = ny;
}

/* SLADEMap::splitLinesAt
 * Splits any lines withing [split_dist] from [vertex]
 *******************************************************************/
void SLADEMap::splitLinesAt(MapVertex* vertex, double split_dist)
{
	// Check if this vertex splits any lines (if needed)
	unsigned nlines = lines.size();
	for (unsigned a = 0; a < nlines; a++)
	{
		// Skip line if it shares the vertex
		if (lines[a]->v1() == vertex || lines[a]->v2() == vertex)
			continue;

		if (lines[a]->distanceTo(vertex->point()) < split_dist)
		{
			LOG_MESSAGE(2, "Vertex at (%1.2f,%1.2f) splits line %u", vertex->x, vertex->y, a);
			splitLine(lines[a], vertex);
		}
	}
}

/* SLADEMap::setLineSector
 * Sets the front or back side of the line at index [line] to be
 * part of [sector]. Returns true if a new side was created
 *******************************************************************/
bool SLADEMap::setLineSector(unsigned line, unsigned sector, bool front)
{
	// Check indices
	if (line >= lines.size() || sector >= sectors.size())
		return false;

	// Get the MapSide to set
	MapSide* side = NULL;
	if (front)
		side = lines[line]->side1;
	else
		side = lines[line]->side2;

	// Do nothing if already the same sector
	if (side && side->sector == sectors[sector])
		return false;

	// Create side if needed
	if (!side)
	{
		side = createSide(sectors[sector]);

		// Add to line
		lines[line]->setModified();
		side->parent = lines[line];
		if (front)
			lines[line]->side1 = side;
		else
			lines[line]->side2 = side;

		// Set appropriate line flags
		bool twosided = (lines[line]->side1 && lines[line]->side2);
		theGameConfiguration->setLineBasicFlag("blocking", lines[line], current_format, !twosided);
		theGameConfiguration->setLineBasicFlag("twosided", lines[line], current_format, twosided);

		// Invalidate sector polygon
		sectors[sector]->resetPolygon();
		setGeometryUpdated();

		return true;
	}
	else
	{
		// Set the side's sector
		side->setSector(sectors[sector]);

		return false;
	}
}

/* SLADEMap::splitLinesByLine
 * Not used
 *******************************************************************/
void SLADEMap::splitLinesByLine(MapLine* split_line)
{
	fpoint2_t intersection;
	fseg2_t split_segment = split_line->seg();

	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a] == split_line)
			continue;

		if (MathStuff::linesIntersect(split_segment, lines[a]->seg(), intersection))
		{
			MapVertex* v = createVertex(intersection.x, intersection.y, 0.9);
			//splitLine(lines[a], v);
		}
	}
}

/* SLADEMap::mergeLine
 * Removes any lines overlapping the line at index [line]. Returns
 * the number of lines removed
 *******************************************************************/
int SLADEMap::mergeLine(unsigned line)
{
	// Check index
	if (line >= lines.size())
		return 0;

	MapLine* ml = lines[line];
	MapVertex* v1 = lines[line]->vertex1;
	MapVertex* v2 = lines[line]->vertex2;

	// Go through lines connected to first vertex
	int merged = 0;
	for (unsigned a = 0; a < v1->connected_lines.size(); a++)
	{
		MapLine* l = v1->connected_lines[a];
		if (l == ml)
			continue;

		// Check overlap
		if ((l->vertex1 == v1 && l->vertex2 == v2) ||
				(l->vertex2 == v1 && l->vertex1 == v2))
		{
			// Remove line
			removeLine(l);
			a--;
			merged++;
		}
	}

	// Correct sector references
	if (merged > 0)
		correctLineSectors(ml);

	return merged;
}

/* SLADEMap::correctLineSectors
 * Attempts to set [line]'s side sector references to the correct
 * sectors. Returns true if any side sector was changed
 *******************************************************************/
bool SLADEMap::correctLineSectors(MapLine* line)
{
	bool changed = false;
	MapSector* s1_current = line->side1 ? line->side1->sector : NULL;
	MapSector* s2_current = line->side2 ? line->side2->sector : NULL;

	// Front side
	MapSector* s1 = getLineSideSector(line, true);
	if (s1 != s1_current)
	{
		if (s1)
			setLineSector(line->index, s1->index, true);
		else if (line->side1)
			removeSide(line->side1);
		changed = true;
	}

	// Back side
	MapSector* s2 = getLineSideSector(line, false);
	if (s2 != s2_current)
	{
		if (s2)
			setLineSector(line->index, s2->index, false);
		else if (line->side2)
			removeSide(line->side2);
		changed = true;
	}

	// Flip if needed
	if (changed && !line->side1 && line->side2)
		line->flip();

	return changed;
}

/* SLADEMap::setLineSide
 * Sets [line]'s front or back [side] (depending on [front]). If
 * [side] already belongs to another line, use a copy of it instead
 *******************************************************************/
void SLADEMap::setLineSide(MapLine* line, MapSide* side, bool front)
{
	// Remove current side
	MapSide* side_current = front ? line->side1 : line->side2;
	if (side_current == side)
		return;
	if (side_current)
		removeSide(side_current);

	// If the new side is already part of another line, copy it
	if (side->parent)
	{
		MapSide* new_side = createSide(side->sector);
		new_side->copy(side);
		side = new_side;
	}

	// Set side
	if (front)
		line->side1 = side;
	else
		line->side2 = side;
	side->parent = line;
}

/* SLADEMap::mergeArch
 * Merges any map architecture (lines and vertices) connected to
 * vertices in [vertices]
 *******************************************************************/
bool SLADEMap::mergeArch(vector<MapVertex*> vertices)
{
	// Check any map architecture exists
	if (nVertices() == 0 || nLines() == 0)
		return false;

	unsigned n_vertices = nVertices();
	unsigned n_lines = lines.size();
	MapVertex* last_vertex = this->vertices.back();
	MapLine* last_line = lines.back();

	// Merge vertices
	vector<MapVertex*> merged_vertices;
	for (unsigned a = 0; a < vertices.size(); a++)
		VECTOR_ADD_UNIQUE(merged_vertices, mergeVerticesPoint(vertices[a]->x, vertices[a]->y));

	// Get all connected lines
	vector<MapLine*> connected_lines;
	for (unsigned a = 0; a < merged_vertices.size(); a++)
	{
		for (unsigned l = 0; l < merged_vertices[a]->connected_lines.size(); l++)
			VECTOR_ADD_UNIQUE(connected_lines, merged_vertices[a]->connected_lines[l]);
	}

	// Split lines (by vertices)
	const double split_dist = 0.1;
	// Split existing lines that vertices moved onto
	for (unsigned a = 0; a < merged_vertices.size(); a++)
		splitLinesAt(merged_vertices[a], split_dist);

	// Split lines that moved onto existing vertices
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		unsigned nvertices = this->vertices.size();
		for (unsigned b = 0; b < nvertices; b++)
		{
			MapVertex* vertex = this->vertices[b];

			// Skip line if it shares the vertex
			if (connected_lines[a]->v1() == vertex || connected_lines[a]->v2() == vertex)
				continue;

			if (connected_lines[a]->distanceTo(vertex->point()) < split_dist)
			{
				connected_lines.push_back(splitLine(connected_lines[a], vertex));
				VECTOR_ADD_UNIQUE(merged_vertices, vertex);
			}
		}
	}

	// Split lines (by lines)
	fseg2_t seg1;
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		MapLine* line1 = connected_lines[a];
		seg1 = line1->seg();

		unsigned n_lines = lines.size();
		for (unsigned b = 0; b < n_lines; b++)
		{
			MapLine* line2 = lines[b];

			// Can't intersect if they share a vertex
			if (line1->vertex1 == line2->vertex1 ||
				line1->vertex1 == line2->vertex2 ||
				line2->vertex1 == line1->vertex2 ||
				line2->vertex2 == line1->vertex2)
				continue;

			// Check for intersection
			fpoint2_t intersection;
			if (MathStuff::linesIntersect(seg1, line2->seg(), intersection))
			{
				// Create split vertex
				MapVertex* nv = createVertex(intersection.x, intersection.y);
				merged_vertices.push_back(nv);

				// Split lines
				splitLine(line1, nv);
				connected_lines.push_back(lines.back());
				splitLine(line2, nv);
				connected_lines.push_back(lines.back());

				LOG_DEBUG("Lines", line1, "and", line2, "intersect");

				a--;
				break;
			}
		}
	}

	// Refresh connected lines
	connected_lines.clear();
	for (unsigned a = 0; a < merged_vertices.size(); a++)
	{
		for (unsigned l = 0; l < merged_vertices[a]->connected_lines.size(); l++)
			VECTOR_ADD_UNIQUE(connected_lines, merged_vertices[a]->connected_lines[l]);
	}

	// Find overlapping lines
	vector<MapLine*> remove_lines;
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		MapLine* line1 = connected_lines[a];

		// Skip if removing already
		if (VECTOR_EXISTS(remove_lines, line1))
			continue;

		for (unsigned l = a + 1; l < connected_lines.size(); l++)
		{
			MapLine* line2 = connected_lines[l];

			// Skip if removing already
			if (VECTOR_EXISTS(remove_lines, line2))
				continue;

			if ((line1->vertex1 == line2->vertex1 && line1->vertex2 == line2->vertex2) ||
				(line1->vertex1 == line2->vertex2 && line1->vertex2 == line2->vertex1))
			{
				MapLine* remove_line = mergeOverlappingLines(line2, line1);
				VECTOR_ADD_UNIQUE(remove_lines, remove_line);

				// Don't check against any more lines if we just decided to remove this one
				if (remove_line == line1)
					break;
			}
		}
	}

	// Remove overlapping lines
	for (unsigned a = 0; a < remove_lines.size(); a++)
	{
		LOG_MESSAGE(4, "Removing overlapping line %u (#%u)", remove_lines[a]->getId(), remove_lines[a]->getIndex());
		removeLine(remove_lines[a]);
	}
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		if (VECTOR_EXISTS(remove_lines, connected_lines[a]))
		{
			connected_lines[a] = connected_lines.back();
			connected_lines.pop_back();
			a--;
		}
	}

	// Check if anything was actually merged
	bool merged = false;
	if (nVertices() != n_vertices || lines.size() != n_lines)
		merged = true;
	if (this->vertices.back() != last_vertex || lines.back() != last_line)
		merged = true;
	if (!remove_lines.empty())
		merged = true;

	// Correct sector references
	correctSectors(connected_lines, true);
	/*if (merged)
		correctSectors(connected_lines, true);
	else
	{
		for (unsigned a = 0; a < connected_lines.size(); a++)
		{
			MapSector* s1 = getLineSideSector(connected_lines[a], true);
			MapSector* s2 = getLineSideSector(connected_lines[a], false);
			
			if (s1)
				setLineSector(connected_lines[a]->index, s1->index, true);
			else
				removeSide(connected_lines[a]->side1);

			if (s2)
				setLineSector(connected_lines[a]->index, s2->index, false);
			else
				removeSide(connected_lines[a]->side2);
		}
	}*/

	// Flip any one-sided lines that only have a side 2
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		if (connected_lines[a]->side2 && !connected_lines[a]->side1)
			connected_lines[a]->flip();
	}

	if (merged)
	{
		LOG_MESSAGE(4, "Architecture merged");
	}
	else
	{
		LOG_MESSAGE(4, "No Architecture merged");
	}

	return merged;
}

/* SLADEMap::mergeOverlappingLines
 * Merges [line1] and [line2], returning the resulting line
 *******************************************************************/
MapLine* SLADEMap::mergeOverlappingLines(MapLine* line1, MapLine* line2)
{
	// Determine which line to remove (prioritise 2s)
	MapLine* remove, *keep;
	if (line1->side2 && !line2->side2)
	{
		remove = line1;
		keep = line2;
	}
	else
	{
		remove = line2;
		keep = line1;
	}

	// Front-facing overlap
	if (remove->vertex1 == keep->vertex1)
	{
		// Set keep front sector to remove front sector
		if (remove->side1)
			setLineSector(keep->index, remove->side1->sector->index);
		else
			setLineSector(keep->index, -1);
	}
	else
	{
		if (remove->side2)
			setLineSector(keep->index, remove->side2->sector->index);
		else
			setLineSector(keep->index, -1);
	}

	return remove;
}

// Used for sector correction
struct me_ls_t
{
	MapLine*	line;
	bool		front;
	bool		ignore;
	me_ls_t(MapLine* line, bool front) { this->line = line; this->front = front; ignore = false; }
};

/* SLADEMap::correctSectors
 * Corrects/builds sectors for all lines in [lines]
 *******************************************************************/
void SLADEMap::correctSectors(vector<MapLine*> lines, bool existing_only)
{
	// Create a list of line sides (edges) to perform sector creation with
	vector<me_ls_t> edges;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (existing_only)
		{
			// Add only existing sides as edges
			// (or front side if line has none)
			if (lines[a]->side1 || (!lines[a]->side1 && !lines[a]->side2))
				edges.push_back(me_ls_t(lines[a], true));
			if (lines[a]->side2)
				edges.push_back(me_ls_t(lines[a], false));
		}
		else
		{
			edges.push_back(me_ls_t(lines[a], true));
			fpoint2_t mid = lines[a]->getPoint(MOBJ_POINT_MID);
			if (sectorAt(mid) >= 0)
				edges.push_back(me_ls_t(lines[a], false));
		}
	}

	vector<MapSide*> sides_correct;
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (edges[a].front && edges[a].line->side1)
			sides_correct.push_back(edges[a].line->side1);
		else if (!edges[a].front && edges[a].line->side2)
			sides_correct.push_back(edges[a].line->side2);
	}

	// Build sectors
	SectorBuilder builder;
	int runs = 0;
	unsigned ns_start = sectors.size();
	unsigned nsd_start = sides.size();
	vector<MapSector*> sectors_reused;
	for (unsigned a = 0; a < edges.size(); a++)
	{
		// Skip if edge is ignored
		if (edges[a].ignore)
			continue;

		// Run sector builder on current edge
		bool ok = builder.traceSector(this, edges[a].line, edges[a].front);
		runs++;

		// Don't create sector if trace failed
		if (!ok)
			continue;

		// Find any subsequent edges that were part of the sector created
		bool has_existing_lines = false;
		bool has_existing_sides = false;
		bool has_zero_sided_lines = false;
		vector<size_t> edges_in_sector;
		for (unsigned b = 0; b < builder.nEdges(); b++)
		{
			MapLine* line = builder.getEdgeLine(b);
			bool is_front = builder.edgeIsFront(b);

			bool line_is_ours = false;
			for (unsigned e = 0; e < edges.size(); e++)
			{
				if (edges[e].line == line)
				{
					line_is_ours = true;
					if (edges[e].front == is_front)
					{
						edges_in_sector.push_back(e);
						break;
					}
				}
			}

			if (line_is_ours)
			{
				if (!line->s1() && !line->s2())
					has_zero_sided_lines = true;
			}
			else
			{
				has_existing_lines = true;
				if (is_front ? line->s1() : line->s2())
					has_existing_sides = true;
			}
		}

		// Pasting or moving a two-sided line into an enclosed void should NOT
		// create a new sector out of the entire void.
		// Heuristic: if the traced sector includes any edges that are NOT
		// "ours", and NONE of those edges already exist, that sector must be
		// in an enclosed void, and should not be drawn.
		// However, if existing_only is false, the caller expects us to create
		// new sides anyway; skip this check.
		if (existing_only && has_existing_lines && !has_existing_sides)
			continue;

		// Ignore traced edges when trying to create any further sectors
		for (size_t a = 0; a < edges_in_sector.size(); a++)
			edges[edges_in_sector[a]].ignore = true;

		// Check if sector traced is already valid
		if (builder.isValidSector())
			continue;

		// Check if we traced over an existing sector (or part of one)
		MapSector* sector = builder.findExistingSector(sides_correct);
		if (sector)
		{
			// Check if it's already been (re)used
			bool reused = false;
			for (unsigned s = 0; s < sectors_reused.size(); s++)
			{
				if (sectors_reused[s] == sector)
				{
					reused = true;
					break;
				}
			}

			// If we can reuse the sector, do so
			if (!reused)
				sectors_reused.push_back(sector);
			else
				sector = NULL;
		}

		// Create sector
		builder.createSector(sector);
	}

	// Remove any sides that weren't part of a sector
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (edges[a].ignore || !edges[a].line)
			continue;

		if (edges[a].front)
			removeSide(edges[a].line->side1);
		else
			removeSide(edges[a].line->side2);
	}

	//LOG_MESSAGE(1, "Ran sector builder %d times", runs);

	// Check if any lines need to be flipped
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a]->backSector() && !lines[a]->frontSector())
			lines[a]->flip(true);
	}

	// Find an adjacent sector to copy properties from
	MapSector* sector_copy = NULL;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Check front sector
		MapSector* sector = lines[a]->frontSector();
		if (sector && sector->getIndex() < ns_start)
		{
			// Copy this sector if it isn't newly created
			sector_copy = sector;
			break;
		}

		// Check back sector
		sector = lines[a]->backSector();
		if (sector && sector->getIndex() < ns_start)
		{
			// Copy this sector if it isn't newly created
			sector_copy = sector;
			break;
		}
	}

	// Go through newly created sectors
	for (unsigned a = ns_start; a < sectors.size(); a++)
	{
		// Skip if sector already has properties
		if (!sectors[a]->getCeilingTex().IsEmpty())
			continue;

		// Copy from adjacent sector if any
		if (sector_copy)
		{
			sectors[a]->copy(sector_copy);
			continue;
		}

		// Otherwise, use defaults from game configuration
		theGameConfiguration->applyDefaults(sectors[a], current_format == MAP_UDMF);
	}

	// Update line textures
	for (unsigned a = nsd_start; a < sides.size(); a++)
	{
		// Clear any unneeded textures
		MapLine* line = sides[a]->getParentLine();
		line->clearUnneededTextures();

		// Set middle texture if needed
		if (sides[a] == line->s1() && !line->s2() && sides[a]->stringProperty("texturemiddle") == "-")
		{
			//LOG_MESSAGE(1, "midtex");
			// Find adjacent texture (any)
			string tex = getAdjacentLineTexture(line->v1());
			if (tex == "-")
				tex = getAdjacentLineTexture(line->v2());

			// If no adjacent texture, get default from game configuration
			if (tex == "-")
				tex = theGameConfiguration->getDefaultString(MOBJ_SIDE, "texturemiddle");

			// Set texture
			sides[a]->setStringProperty("texturemiddle", tex);
		}
	}

	// Remove any extra sectors
	removeDetachedSectors();
}

/* SLADEMap::mapOpenChecks
 * Performs checks for when a map is first opened
 *******************************************************************/
void SLADEMap::mapOpenChecks()
{
	int rverts = removeDetachedVertices();
	int rsides = removeDetachedSides();
	int rsec = removeDetachedSectors();
	int risides = removeInvalidSides();

	LOG_MESSAGE(1, "Removed %d detached vertices, %d detached sides, %d invalid sides and %d detached sectors", rverts, rsides, risides, rsec);
}

/* SLADEMap::removeDetachedVertices
 * Removes any vertices not attached to any lines. Returns the number
 * of vertices removed
 *******************************************************************/
int SLADEMap::removeDetachedVertices()
{
	int count = 0;
	for (int a = vertices.size() - 1; a >= 0; a--)
	{
		if (vertices[a]->nConnectedLines() == 0)
		{
			removeVertex(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

/* SLADEMap::removeDetachedSides
 * Removes any sides that have no parent line. Returns the number of
 * sides removed
 *******************************************************************/
int SLADEMap::removeDetachedSides()
{
	int count = 0;
	for (int a = sides.size() - 1; a >= 0; a--)
	{
		if (!sides[a]->parent)
		{
			removeSide(a, false);
			count++;
		}
	}

	refreshIndices();

	return count;
}

/* SLADEMap::removeDetachedSectors
 * Removes any sectors that are not referenced by any sides. Returns
 * the number of sectors removed
 *******************************************************************/
int SLADEMap::removeDetachedSectors()
{
	int count = 0;
	for (int a = sectors.size() - 1; a >= 0; a--)
	{
		if (sectors[a]->connectedSides().size() == 0)
		{
			removeSector(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

/* SLADEMap::removeZeroLengthLines
 * Removes any lines that have identical first and second vertices.
 * Returns the number of lines removed
 *******************************************************************/
int SLADEMap::removeZeroLengthLines()
{
	int count = 0;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a]->vertex1 == lines[a]->vertex2)
		{
			removeLine(a);
			a--;
			count++;
		}
	}

	return count;
}

/* SLADEMap::removeInvalidSides
 * Removes any sides that reference non-existant sectors
 *******************************************************************/
int SLADEMap::removeInvalidSides()
{
	int count = 0;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		if (!sides[a]->getSector())
		{
			removeSide(a);
			a--;
			count++;
		}
	}

	return count;
}

/* SLADEMap::convertToHexen
 * Converts the map to hexen format (not implemented)
 *******************************************************************/
bool SLADEMap::convertToHexen()
{
	// Already hexen format
	if (current_format == MAP_HEXEN)
		return true;
	return false;
}

/* SLADEMap::convertToUDMF
 * Converts the map to UDMF format (not implemented)
 *******************************************************************/
bool SLADEMap::convertToUDMF()
{
	// Already UDMF format
	if (current_format == MAP_UDMF)
		return true;

	if (current_format == MAP_HEXEN)
	{
		// Line_SetIdentification special, set line id
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->intProperty("special") == 121)
			{
				int id = lines[a]->intProperty("arg0");

				// id high byte
				int hi = lines[a]->intProperty("arg4");
				id = (hi*256) + id;

				// flags
				int flags = lines[a]->intProperty("arg1");
				if (flags & 1) lines[a]->setBoolProperty("zoneboundary", true);
				if (flags & 2) lines[a]->setBoolProperty("jumpover", true);
				if (flags & 4) lines[a]->setBoolProperty("blockfloaters", true);
				if (flags & 8) lines[a]->setBoolProperty("clipmidtex", true);
				if (flags & 16) lines[a]->setBoolProperty("wrapmidtex", true);
				if (flags & 32) lines[a]->setBoolProperty("midtex3d", true);
				if (flags & 64) lines[a]->setBoolProperty("checkswitchrange", true);

				lines[a]->setIntProperty("special", 0);
				lines[a]->setIntProperty("id", id);
				lines[a]->setIntProperty("arg0", 0);
			}
		}
	}
	else return false;

	// flags

	// Set format
	current_format = MAP_UDMF;
	return true;
}

/* SLADEMap::rebuildConnectedLines
 * Rebuilds the connected lines lists for all map vertices
 *******************************************************************/
void SLADEMap::rebuildConnectedLines()
{
	// Clear vertex connected lines lists
	for (unsigned a = 0; a < vertices.size(); a++)
		vertices[a]->connected_lines.clear();

	// Connect lines to their vertices
	for (unsigned a = 0; a < lines.size(); a++)
	{
		lines[a]->vertex1->connected_lines.push_back(lines[a]);
		lines[a]->vertex2->connected_lines.push_back(lines[a]);
	}
}

/* SLADEMap::rebuildConnectedSides
 * Rebuilds the connected sides lists for all map sectors
 *******************************************************************/
void SLADEMap::rebuildConnectedSides()
{
	// Clear sector connected sides lists
	for (unsigned a = 0; a < sectors.size(); a++)
		sectors[a]->connected_sides.clear();

	// Connect sides to their sectors
	for (unsigned a = 0; a < sides.size(); a++)
	{
		if (sides[a]->sector)
			sides[a]->sector->connected_sides.push_back(sides[a]);
	}
}

/* SLADEMap::updateTexUsage
 * Adjusts the usage count for texture [tex] by [adjust]
 *******************************************************************/
void SLADEMap::updateTexUsage(string tex, int adjust)
{
	usage_tex[tex.Upper()] += adjust;
}

/* SLADEMap::updateFlatUsage
 * Adjusts the usage count for flat [flat] by [adjust]
 *******************************************************************/
void SLADEMap::updateFlatUsage(string flat, int adjust)
{
	usage_flat[flat.Upper()] += adjust;
}

/* SLADEMap::updateThingTypeUsage
 * Adjusts the usage count for thing type [type] by [adjust]
 *******************************************************************/
void SLADEMap::updateThingTypeUsage(int type, int adjust)
{
	usage_thing_type[type] += adjust;
}

/* SLADEMap::texUsageCount
 * Returns the usage count for the texture [tex]
 *******************************************************************/
int SLADEMap::texUsageCount(string tex)
{
	return usage_tex[tex.Upper()];
}

/* SLADEMap::flatUsageCount
 * Returns the usage count for the flat [tex]
 *******************************************************************/
int SLADEMap::flatUsageCount(string tex)
{
	return usage_flat[tex.Upper()];
}

/* SLADEMap::thingTypeUsageCount
 * Returns the usage count for the thing type [type]
 *******************************************************************/
int SLADEMap::thingTypeUsageCount(int type)
{
	return usage_thing_type[type];
}
