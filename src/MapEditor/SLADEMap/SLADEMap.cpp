
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SLADEMap.cpp
// Description: SLADEMap class, the internal SLADE map handler.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "SLADEMap.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/Formats/WadArchive.h"
#include "Game/Configuration.h"
#include "General/ResourceManager.h"
#include "General/UI.h"
#include "MapEditor/SectorBuilder.h"
#include "Utility/MathStuff.h"
#include "Utility/Parser.h"

#define IDEQ(x) (((x) != 0) && ((x) == id))


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map_split_auto_offset, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// SLADEMap Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SLADEMap class constructor
// -----------------------------------------------------------------------------
SLADEMap::SLADEMap()
{
	// Object id 0 is always null
	all_objects_.emplace_back(nullptr, false);

	// Init opened time so it's not random leftover garbage values
	setOpenedTime();
}

// -----------------------------------------------------------------------------
// SLADEMap class destructor
// -----------------------------------------------------------------------------
SLADEMap::~SLADEMap()
{
	clearMap();
}

// -----------------------------------------------------------------------------
// Returns the vertex at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::vertex(unsigned index) const
{
	// Check index
	if (index >= vertices_.size())
		return nullptr;

	return vertices_[index];
}

// -----------------------------------------------------------------------------
// Returns the side at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapSide* SLADEMap::side(unsigned index) const
{
	// Check index
	if (index >= sides_.size())
		return nullptr;

	return sides_[index];
}

// -----------------------------------------------------------------------------
// Returns the line at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapLine* SLADEMap::line(unsigned index) const
{
	// Check index
	if (index >= lines_.size())
		return nullptr;

	return lines_[index];
}

// -----------------------------------------------------------------------------
// Returns the sector at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapSector* SLADEMap::sector(unsigned index) const
{
	// Check index
	if (index >= sectors_.size())
		return nullptr;

	return sectors_[index];
}

// -----------------------------------------------------------------------------
// Returns the thing at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapThing* SLADEMap::thing(unsigned index) const
{
	// Check index
	if (index >= things_.size())
		return nullptr;

	return things_[index];
}

// -----------------------------------------------------------------------------
// Returns the object of [type] at [index], or NULL if [index] is invalid
// -----------------------------------------------------------------------------
MapObject* SLADEMap::object(MapObject::Type type, unsigned index) const
{
	switch (type)
	{
	case MapObject::Type::Vertex: return vertex(index);
	case MapObject::Type::Line: return line(index);
	case MapObject::Type::Side: return side(index);
	case MapObject::Type::Sector: return sector(index);
	case MapObject::Type::Thing: return thing(index);
	default: return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Sets the geometry last updated time to now
// -----------------------------------------------------------------------------
void SLADEMap::setGeometryUpdated()
{
	geometry_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Sets the things last updated time to now
// -----------------------------------------------------------------------------
void SLADEMap::setThingsUpdated()
{
	things_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Refreshes all map object indices
// -----------------------------------------------------------------------------
void SLADEMap::refreshIndices()
{
	// Vertex indices
	for (unsigned a = 0; a < vertices_.size(); a++)
		vertices_[a]->index_ = a;

	// Side indices
	for (unsigned a = 0; a < sides_.size(); a++)
		sides_[a]->index_ = a;

	// Line indices
	for (unsigned a = 0; a < lines_.size(); a++)
		lines_[a]->index_ = a;

	// Sector indices
	for (unsigned a = 0; a < sectors_.size(); a++)
		sectors_[a]->index_ = a;

	// Thing indices
	for (unsigned a = 0; a < things_.size(); a++)
		things_[a]->index_ = a;
}

// -----------------------------------------------------------------------------
// Adds [object] to the map objects list
// -----------------------------------------------------------------------------
void SLADEMap::addMapObject(MapObject* object)
{
	all_objects_.emplace_back(object, true);
	object->obj_id_ = all_objects_.size() - 1;
	created_deleted_objects_.emplace_back(object->obj_id_, true);
}

// -----------------------------------------------------------------------------
// Removes [object] from the map
// (keeps it in the objects list, but removes the 'in map' flag)
// -----------------------------------------------------------------------------
void SLADEMap::removeMapObject(MapObject* object)
{
	all_objects_[object->obj_id_].in_map = false;
	created_deleted_objects_.emplace_back(object->obj_id_, false);
}

// -----------------------------------------------------------------------------
// Adds all object ids of [type] currently in the map to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putObjectIdList(MapObject::Type type, vector<unsigned>& list)
{
	if (type == MapObject::Type::Vertex)
	{
		for (auto& vertex : vertices_)
			list.push_back(vertex->obj_id_);
	}
	else if (type == MapObject::Type::Line)
	{
		for (auto& line : lines_)
			list.push_back(line->obj_id_);
	}
	else if (type == MapObject::Type::Side)
	{
		for (auto& side : sides_)
			list.push_back(side->obj_id_);
	}
	else if (type == MapObject::Type::Sector)
	{
		for (auto& sector : sectors_)
			list.push_back(sector->id_);
	}
	else if (type == MapObject::Type::Thing)
	{
		for (auto& thing : things_)
			list.push_back(thing->obj_id_);
	}
}

// -----------------------------------------------------------------------------
// Add all object ids in [list] to the map as [type], clearing any objects of
// [type] currently in the map
// -----------------------------------------------------------------------------
void SLADEMap::restoreObjectIdList(MapObject::Type type, vector<unsigned>& list)
{
	if (type == MapObject::Type::Vertex)
	{
		// Clear
		for (auto& vertex : vertices_)
			all_objects_[vertex->obj_id_].in_map = false;
		vertices_.clear();

		// Restore
		for (auto id : list)
		{
			all_objects_[id].in_map = true;
			vertices_.push_back(dynamic_cast<MapVertex*>(all_objects_[id].mobj));
			vertices_.back()->index_ = vertices_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Line)
	{
		// Clear
		for (auto& line : lines_)
			all_objects_[line->obj_id_].in_map = false;
		lines_.clear();

		// Restore
		for (auto id : list)
		{
			all_objects_[id].in_map = true;
			lines_.push_back(dynamic_cast<MapLine*>(all_objects_[id].mobj));
			lines_.back()->index_ = lines_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Side)
	{
		// Clear
		for (auto& side : sides_)
			all_objects_[side->obj_id_].in_map = false;
		sides_.clear();

		// Restore
		for (auto id : list)
		{
			all_objects_[id].in_map = true;
			sides_.push_back(dynamic_cast<MapSide*>(all_objects_[id].mobj));
			sides_.back()->index_ = sides_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Sector)
	{
		// Clear
		for (auto& sector : sectors_)
			all_objects_[sector->id_].in_map = false;
		sectors_.clear();

		// Restore
		for (auto id : list)
		{
			all_objects_[id].in_map = true;
			sectors_.push_back(dynamic_cast<MapSector*>(all_objects_[id].mobj));
			sectors_.back()->index_ = sectors_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Thing)
	{
		// Clear
		for (auto& thing : things_)
			all_objects_[thing->obj_id_].in_map = false;
		things_.clear();

		// Restore
		for (auto id : list)
		{
			all_objects_[id].in_map = true;
			things_.push_back(dynamic_cast<MapThing*>(all_objects_[id].mobj));
			things_.back()->index_ = things_.size() - 1;
		}
	}
}

// -----------------------------------------------------------------------------
// Reads map data using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readMap(Archive::MapDesc map)
{
	auto omap = map;

	// Check for map archive
	if (map.archive && map.head)
	{
		WadArchive tempwad;
		tempwad.open(map.head->data());
		auto amaps = tempwad.detectMaps();
		if (!amaps.empty())
			omap = amaps[0];
		else
			return false;
	}

	bool ok = false;
	if (omap.head)
	{
		if (omap.format == MapFormat::Doom)
			ok = readDoomMap(omap);
		else if (omap.format == MapFormat::Hexen)
			ok = readHexenMap(omap);
		else if (omap.format == MapFormat::Doom64)
			ok = readDoom64Map(omap);
		else if (omap.format == MapFormat::UDMF)
			ok = readUDMFMap(omap);
	}
	else
		ok = true;

	// Set map name
	name_ = map.name;

	// Set map format
	if (ok)
	{
		current_format_ = map.format;
		// When creating a new map, retrieve UDMF namespace information from the configuration
		if (map.format == MapFormat::UDMF && udmf_namespace_.IsEmpty())
			udmf_namespace_ = Game::configuration().udmfNamespace();
	}

	initSectorPolygons();
	recomputeSpecials();

	opened_time_ = App::runTimer() + 10;

	return ok;
}

// -----------------------------------------------------------------------------
// Adds a vertex to the map from a doom vertex definition [v]
// -----------------------------------------------------------------------------
bool SLADEMap::addVertex(MapVertex::DoomData& v)
{
	MapVertex* nv = new MapVertex(v.x, v.y, this);
	vertices_.push_back(nv);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a vertex to the map from a doom64 vertex definition [v]
// -----------------------------------------------------------------------------
bool SLADEMap::addVertex(MapVertex::Doom64Data& v)
{
	MapVertex* nv = new MapVertex((double)v.x / 65536, (double)v.y / 65536, this);
	vertices_.push_back(nv);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a side to the map from a doom sidedef definition [s]
// -----------------------------------------------------------------------------
bool SLADEMap::addSide(MapSide::DoomData& s)
{
	// Create side
	MapSide* ns = new MapSide(sector(s.sector), this);

	// Setup side properties
	ns->tex_upper_  = wxString::FromAscii(s.tex_upper, 8);
	ns->tex_lower_  = wxString::FromAscii(s.tex_lower, 8);
	ns->tex_middle_ = wxString::FromAscii(s.tex_middle, 8);
	ns->offset_x_   = s.x_offset;
	ns->offset_y_   = s.y_offset;

	// Update texture counts
	usage_tex_[ns->tex_upper_.Upper()] += 1;
	usage_tex_[ns->tex_middle_.Upper()] += 1;
	usage_tex_[ns->tex_lower_.Upper()] += 1;

	// Add side
	sides_.push_back(ns);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a side to the map from a doom64 sidedef definition [s]
// -----------------------------------------------------------------------------
bool SLADEMap::addSide(MapSide::Doom64Data& s)
{
	// Create side
	MapSide* ns = new MapSide(sector(s.sector), this);

	// Setup side properties
	ns->tex_upper_  = theResourceManager->doom64TextureName(s.tex_upper);
	ns->tex_lower_  = theResourceManager->doom64TextureName(s.tex_lower);
	ns->tex_middle_ = theResourceManager->doom64TextureName(s.tex_middle);
	ns->offset_x_   = s.x_offset;
	ns->offset_y_   = s.y_offset;

	// Update texture counts
	usage_tex_[ns->tex_upper_.Upper()] += 1;
	usage_tex_[ns->tex_middle_.Upper()] += 1;
	usage_tex_[ns->tex_lower_.Upper()] += 1;

	// Add side
	sides_.push_back(ns);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a line to the map from a doom linedef definition [l]
// -----------------------------------------------------------------------------
bool SLADEMap::addLine(MapLine::DoomData& l)
{
	// Get relevant sides
	MapSide* s1 = nullptr;
	MapSide* s2 = nullptr;
	if (sides_.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535)
			s1 = side(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535)
			s2 = side(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = side(l.side1);
		s2 = side(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = vertex(l.vertex1);
	MapVertex* v2 = vertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector_, this);
		ns->copy(s1);
		s1 = ns;
		sides_.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector_, this);
		ns->copy(s2);
		s2 = ns;
		sides_.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties_["arg0"]  = l.sector_tag;
	nl->properties_["id"]    = l.sector_tag;
	nl->special_             = l.type;
	nl->properties_["flags"] = l.flags;

	// Add line
	lines_.push_back(nl);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a line to the map from a doom64 linedef definition [l]
// -----------------------------------------------------------------------------
bool SLADEMap::addLine(MapLine::Doom64Data& l)
{
	// Get relevant sides
	MapSide* s1 = nullptr;
	MapSide* s2 = nullptr;
	if (sides_.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535)
			s1 = side(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535)
			s2 = side(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = side(l.side1);
		s2 = side(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = vertex(l.vertex1);
	MapVertex* v2 = vertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector_, this);
		ns->copy(s1);
		s1 = ns;
		sides_.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector_, this);
		ns->copy(s2);
		s2 = ns;
		sides_.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties_["arg0"] = l.sector_tag;
	if (l.type & 0x100)
		nl->properties_["macro"] = l.type & 0xFF;
	else
		nl->special_ = l.type & 0xFF;
	nl->properties_["flags"]      = (int)l.flags;
	nl->properties_["extraflags"] = l.type >> 9;

	// Add line
	lines_.push_back(nl);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a sector to the map from a doom sector definition [s]
// -----------------------------------------------------------------------------
bool SLADEMap::addSector(MapSector::DoomData& s)
{
	// Create sector
	MapSector* ns = new MapSector(wxString::FromAscii(s.f_tex, 8), wxString::FromAscii(s.c_tex, 8), this);

	// Setup sector properties
	ns->setFloorHeight(s.f_height);
	ns->setCeilingHeight(s.c_height);
	ns->light_   = s.light;
	ns->special_ = s.special;
	ns->id_      = s.tag;

	// Update texture counts
	usage_flat_[ns->floor_.texture.Upper()] += 1;
	usage_flat_[ns->ceiling_.texture.Upper()] += 1;

	// Add sector
	sectors_.push_back(ns);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a sector to the map from a doom64 sector definition [s]
// -----------------------------------------------------------------------------
bool SLADEMap::addSector(MapSector::Doom64Data& s)
{
	// Create sector
	// We need to retrieve the texture name from the hash value
	MapSector* ns = new MapSector(
		theResourceManager->doom64TextureName(s.f_tex), theResourceManager->doom64TextureName(s.c_tex), this);

	// Setup sector properties
	ns->setFloorHeight(s.f_height);
	ns->setCeilingHeight(s.c_height);
	ns->light_                       = 255;
	ns->special_                     = s.special;
	ns->id_                          = s.tag;
	ns->properties_["flags"]         = s.flags;
	ns->properties_["color_things"]  = s.color[0];
	ns->properties_["color_floor"]   = s.color[1];
	ns->properties_["color_ceiling"] = s.color[2];
	ns->properties_["color_upper"]   = s.color[3];
	ns->properties_["color_lower"]   = s.color[4];

	// Update texture counts
	usage_flat_[ns->floor_.texture.Upper()] += 1;
	usage_flat_[ns->ceiling_.texture.Upper()] += 1;

	// Add sector
	sectors_.push_back(ns);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a thing to the map from a doom thing definition [t]
// -----------------------------------------------------------------------------
bool SLADEMap::addThing(MapThing::DoomData& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle_               = t.angle;
	nt->properties_["flags"] = t.flags;

	// Add thing
	things_.push_back(nt);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a thing to the map from a doom64 thing definition [t]
// -----------------------------------------------------------------------------
bool SLADEMap::addThing(MapThing::Doom64Data& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle_                = t.angle;
	nt->properties_["height"] = (double)t.z;
	nt->properties_["flags"]  = t.flags;
	nt->properties_["id"]     = t.tid;

	// Add thing
	things_.push_back(nt);
	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom format vertex definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomVertexes(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no VERTEXES entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapVertex::DoomData))
	{
		Log::info(3, "Read 0 vertices");
		return true;
	}

	auto     vert_data = (MapVertex::DoomData*)entry->rawData(true);
	unsigned nv        = entry->size() / sizeof(MapVertex::DoomData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nv; a++)
	{
		UI::setSplashProgress(p + ((float)a / nv) * 0.2f);
		addVertex(vert_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu vertices", vertices_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom format sidedef definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomSidedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SIDEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapSide::DoomData))
	{
		LOG_MESSAGE(3, "Read 0 sides");
		return true;
	}

	auto     side_data = (MapSide::DoomData*)entry->rawData(true);
	unsigned ns        = entry->size() / sizeof(MapSide::DoomData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);
		addSide(side_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sides", sides_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom format linedef definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomLinedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapLine::DoomData))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	auto     line_data = (MapLine::DoomData*)entry->rawData(true);
	unsigned nl        = entry->size() / sizeof(MapLine::DoomData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		UI::setSplashProgress(p + ((float)a / nl) * 0.2f);
		if (!addLine(line_data[a]))
			LOG_MESSAGE(2, "Line %lu invalid, not added", a);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom format sector definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomSectors(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SECTORS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapSector::DoomData))
	{
		LOG_MESSAGE(3, "Read 0 sectors");
		return true;
	}

	auto     sect_data = (MapSector::DoomData*)entry->rawData(true);
	unsigned ns        = entry->size() / sizeof(MapSector::DoomData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);
		addSector(sect_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sectors", sectors_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom format thing definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomThings(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapThing::DoomData))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	auto     thng_data = (MapThing::DoomData*)entry->rawData(true);
	unsigned nt        = entry->size() / sizeof(MapThing::DoomData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a doom format map using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoomMap(Archive::MapDesc map)
{
	LOG_MESSAGE(2, "Reading Doom format map");

	// Find map entries
	ArchiveEntry* v     = nullptr;
	ArchiveEntry* si    = nullptr;
	ArchiveEntry* l     = nullptr;
	ArchiveEntry* se    = nullptr;
	ArchiveEntry* t     = nullptr;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->name() == "VERTEXES")
			v = entry;
		else if (!si && entry->name() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->name() == "LINEDEFS")
			l = entry;
		else if (!se && entry->name() == "SECTORS")
			se = entry;
		else if (!t && entry->name() == "THINGS")
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
	for (auto& sector : sectors_)
		sector->updateBBox();

	// Update variables
	geometry_updated_ = App::runTimer();

	return true;
}

// -----------------------------------------------------------------------------
// Adds a line to the map from a hexen linedef definition [l]
// -----------------------------------------------------------------------------
bool SLADEMap::addLine(MapLine::HexenData& l)
{
	// Get relevant sides
	MapSide* s1 = nullptr;
	MapSide* s2 = nullptr;
	if (sides_.size() > 32767)
	{
		// Support for > 32768 sides
		if (l.side1 != 65535)
			s1 = side(static_cast<unsigned short>(l.side1));
		if (l.side2 != 65535)
			s2 = side(static_cast<unsigned short>(l.side2));
	}
	else
	{
		s1 = side(l.side1);
		s2 = side(l.side2);
	}

	// Get relevant vertices
	MapVertex* v1 = vertex(l.vertex1);
	MapVertex* v2 = vertex(l.vertex2);

	// Check everything is valid
	if (!v1 || !v2)
		return false;

	// Check if side1 already belongs to a line
	if (s1 && s1->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s1->sector_, this);
		ns->copy(s1);
		s1 = ns;
		sides_.push_back(s1);
	}

	// Check if side2 already belongs to a line
	if (s2 && s2->parent_)
	{
		// Duplicate side
		MapSide* ns = new MapSide(s2->sector_, this);
		ns->copy(s2);
		s2 = ns;
		sides_.push_back(s2);
	}

	// Create line
	MapLine* nl = new MapLine(v1, v2, s1, s2, this);

	// Setup line properties
	nl->properties_["arg0"]  = l.args[0];
	nl->properties_["arg1"]  = l.args[1];
	nl->properties_["arg2"]  = l.args[2];
	nl->properties_["arg3"]  = l.args[3];
	nl->properties_["arg4"]  = l.args[4];
	nl->special_             = l.type;
	nl->properties_["flags"] = l.flags;

	// Handle some special cases
	if (l.type)
	{
		switch (Game::configuration().actionSpecial(l.type).needsTag())
		{
		case Game::TagType::LineId:
		case Game::TagType::LineId1Line2: nl->properties_["id"] = l.args[0]; break;
		case Game::TagType::LineIdHi5: nl->properties_["id"] = (l.args[0] + (l.args[4] << 8)); break;
		default: break;
		}
	}

	// Add line
	lines_.push_back(nl);
	return true;
}

// -----------------------------------------------------------------------------
// Adds a thing to the map from a hexen thing definition [t]
// -----------------------------------------------------------------------------
bool SLADEMap::addThing(MapThing::HexenData& t)
{
	// Create thing
	MapThing* nt = new MapThing(t.x, t.y, t.type, this);

	// Setup thing properties
	nt->angle_                 = t.angle;
	nt->properties_["height"]  = (double)t.z;
	nt->properties_["special"] = t.special;
	nt->properties_["flags"]   = t.flags;
	nt->properties_["id"]      = t.tid;
	nt->properties_["arg0"]    = t.args[0];
	nt->properties_["arg1"]    = t.args[1];
	nt->properties_["arg2"]    = t.args[2];
	nt->properties_["arg3"]    = t.args[3];
	nt->properties_["arg4"]    = t.args[4];

	// Add thing
	things_.push_back(nt);
	return true;
}

// -----------------------------------------------------------------------------
// Reads in hexen format linedef definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readHexenLinedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapLine::HexenData))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	auto     line_data = (MapLine::HexenData*)entry->rawData(true);
	unsigned nl        = entry->size() / sizeof(MapLine::HexenData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		UI::setSplashProgress(p + ((float)a / nl) * 0.2f);
		addLine(line_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in hexen format thing definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readHexenThings(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapThing::HexenData))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	auto     thng_data = (MapThing::HexenData*)entry->rawData(true);
	unsigned nt        = entry->size() / sizeof(MapThing::HexenData);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a hexen format using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readHexenMap(Archive::MapDesc map)
{
	LOG_MESSAGE(2, "Reading Hexen format map");

	// Find map entries
	ArchiveEntry* v     = nullptr;
	ArchiveEntry* si    = nullptr;
	ArchiveEntry* l     = nullptr;
	ArchiveEntry* se    = nullptr;
	ArchiveEntry* t     = nullptr;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->name() == "VERTEXES")
			v = entry;
		else if (!si && entry->name() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->name() == "LINEDEFS")
			l = entry;
		else if (!se && entry->name() == "SECTORS")
			se = entry;
		else if (!t && entry->name() == "THINGS")
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
	for (auto& sector : sectors_)
		sector->updateBBox();

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom64 format vertex definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Vertexes(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no VERTEXES entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapVertex::Doom64Data))
	{
		LOG_MESSAGE(3, "Read 0 vertices");
		return true;
	}

	auto     vert_data = (MapVertex::Doom64Data*)entry->rawData(true);
	unsigned n         = entry->size() / sizeof(MapVertex::Doom64Data);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addVertex(vert_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu vertices", vertices_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom64 format sidedef definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Sidedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SIDEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapSide::Doom64Data))
	{
		LOG_MESSAGE(3, "Read 0 sides");
		return true;
	}

	auto     side_data = (MapSide::Doom64Data*)entry->rawData(true);
	unsigned n         = entry->size() / sizeof(MapSide::Doom64Data);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addSide(side_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sides", sides_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom64 format linedef definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Linedefs(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapLine::Doom64Data))
	{
		LOG_MESSAGE(3, "Read 0 lines");
		return true;
	}

	auto     line_data = (MapLine::Doom64Data*)entry->rawData(true);
	unsigned n         = entry->size() / sizeof(MapLine::Doom64Data);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addLine(line_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu lines", lines_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom64 format sector definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Sectors(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no SECTORS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapSector::Doom64Data))
	{
		LOG_MESSAGE(3, "Read 0 sectors");
		return true;
	}

	auto     sect_data = (MapSector::Doom64Data*)entry->rawData(true);
	unsigned n         = entry->size() / sizeof(MapSector::Doom64Data);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addSector(sect_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu sectors", sectors_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads in doom64 format thing definitions from [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Things(ArchiveEntry* entry)
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(MapThing::Doom64Data))
	{
		LOG_MESSAGE(3, "Read 0 things");
		return true;
	}

	auto     thng_data = (MapThing::Doom64Data*)entry->rawData(true);
	unsigned n         = entry->size() / sizeof(MapThing::Doom64Data);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < n; a++)
	{
		UI::setSplashProgress(p + ((float)a / n) * 0.2f);
		addThing(thng_data[a]);
	}

	LOG_MESSAGE(3, "Read %lu things", things_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a doom64 format using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readDoom64Map(Archive::MapDesc map)
{
	LOG_MESSAGE(2, "Reading Doom 64 format map");

	// Find map entries
	ArchiveEntry* v     = nullptr;
	ArchiveEntry* si    = nullptr;
	ArchiveEntry* l     = nullptr;
	ArchiveEntry* se    = nullptr;
	ArchiveEntry* t     = nullptr;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
	{
		if (!v && entry->name() == "VERTEXES")
			v = entry;
		else if (!si && entry->name() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->name() == "LINEDEFS")
			l = entry;
		else if (!se && entry->name() == "SECTORS")
			se = entry;
		else if (!t && entry->name() == "THINGS")
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
	for (auto& sector : sectors_)
		sector->updateBBox();

	return true;
}

// -----------------------------------------------------------------------------
// Adds a vertex to the map from parsed UDMF vertex definition [def]
// -----------------------------------------------------------------------------
bool SLADEMap::addVertex(ParseTreeNode* def)
{
	// Check for required properties
	auto prop_x = def->childPTN("x");
	auto prop_y = def->childPTN("y");
	if (!prop_x || !prop_y)
		return false;

	// Create new vertex
	MapVertex* nv = new MapVertex(prop_x->floatValue(), prop_y->floatValue(), this);

	// Add extra vertex info
	ParseTreeNode* prop = nullptr;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop == prop_x || prop == prop_y)
			continue;

		nv->properties_[prop->name()] = prop->value();
	}

	// Add vertex to map
	vertices_.push_back(nv);

	return true;
}

// -----------------------------------------------------------------------------
// Adds a side to the map from parsed UDMF side definition [def]
// -----------------------------------------------------------------------------
bool SLADEMap::addSide(ParseTreeNode* def)
{
	// Check for required properties
	auto prop_sector = def->childPTN("sector");
	if (!prop_sector)
		return false;

	// Check sector index
	int sector = prop_sector->intValue();
	if (sector < 0 || sector >= (int)sectors_.size())
		return false;

	// Create new side
	MapSide* ns = new MapSide(sectors_[sector], this);

	// Set defaults
	ns->offset_x_   = 0;
	ns->offset_y_   = 0;
	ns->tex_upper_  = "-";
	ns->tex_middle_ = "-";
	ns->tex_lower_  = "-";

	// Add extra side info
	ParseTreeNode* prop = nullptr;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop == prop_sector)
			continue;

		if (S_CMPNOCASE(prop->name(), "texturetop"))
			ns->tex_upper_ = prop->stringValue();
		else if (S_CMPNOCASE(prop->name(), "texturemiddle"))
			ns->tex_middle_ = prop->stringValue();
		else if (S_CMPNOCASE(prop->name(), "texturebottom"))
			ns->tex_lower_ = prop->stringValue();
		else if (S_CMPNOCASE(prop->name(), "offsetx"))
			ns->offset_x_ = prop->intValue();
		else if (S_CMPNOCASE(prop->name(), "offsety"))
			ns->offset_y_ = prop->intValue();
		else
			ns->properties_[prop->name()] = prop->value();
		// LOG_MESSAGE(1, "Property %s type %s (%s)", prop->getName(), prop->getValue().typeString(),
		// prop->getValue().getStringValue());
	}

	// Update texture counts
	usage_tex_[ns->tex_upper_.Upper()] += 1;
	usage_tex_[ns->tex_middle_.Upper()] += 1;
	usage_tex_[ns->tex_lower_.Upper()] += 1;

	// Add side to map
	sides_.push_back(ns);

	return true;
}

// -----------------------------------------------------------------------------
// Adds a line to the map from parsed UDMF line definition [def]
// -----------------------------------------------------------------------------
bool SLADEMap::addLine(ParseTreeNode* def)
{
	// Check for required properties
	auto prop_v1 = def->childPTN("v1");
	auto prop_v2 = def->childPTN("v2");
	auto prop_s1 = def->childPTN("sidefront");
	if (!prop_v1 || !prop_v2 || !prop_s1)
		return false;

	// Check indices
	int v1 = prop_v1->intValue();
	int v2 = prop_v2->intValue();
	int s1 = prop_s1->intValue();
	if (v1 < 0 || v1 >= (int)vertices_.size())
		return false;
	if (v2 < 0 || v2 >= (int)vertices_.size())
		return false;
	if (s1 < 0 || s1 >= (int)sides_.size())
		return false;

	// Get second side if any
	MapSide* side2   = nullptr;
	auto     prop_s2 = def->childPTN("sideback");
	if (prop_s2)
		side2 = side(prop_s2->intValue());

	// Create new line
	MapLine* nl = new MapLine(vertices_[v1], vertices_[v2], sides_[s1], side2, this);

	// Set defaults
	nl->special_ = 0;
	nl->line_id_ = 0;

	// Add extra line info
	ParseTreeNode* prop = nullptr;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop == prop_v1 || prop == prop_v2 || prop == prop_s1 || prop == prop_s2)
			continue;

		if (prop->name() == "special")
			nl->special_ = prop->intValue();
		else if (prop->name() == "id")
			nl->line_id_ = prop->intValue();
		else
			nl->properties_[prop->name()] = prop->value();
	}

	// Add line to map
	lines_.push_back(nl);

	return true;
}

// -----------------------------------------------------------------------------
// Adds a sector to the map from parsed UDMF sector definition [def]
// -----------------------------------------------------------------------------
bool SLADEMap::addSector(ParseTreeNode* def)
{
	// Check for required properties
	auto prop_ftex = def->childPTN("texturefloor");
	auto prop_ctex = def->childPTN("textureceiling");
	if (!prop_ftex || !prop_ctex)
		return false;

	// Create new sector
	MapSector* ns = new MapSector(prop_ftex->stringValue(), prop_ctex->stringValue(), this);
	usage_flat_[ns->floor_.texture.Upper()] += 1;
	usage_flat_[ns->ceiling_.texture.Upper()] += 1;

	// Set defaults
	ns->setFloorHeight(0);
	ns->setCeilingHeight(0);
	ns->light_   = 160;
	ns->special_ = 0;
	ns->id_      = 0;

	// Add extra sector info
	ParseTreeNode* prop = nullptr;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop == prop_ftex || prop == prop_ctex)
			continue;

		if (S_CMPNOCASE(prop->name(), "heightfloor"))
			ns->setFloorHeight(prop->intValue());
		else if (S_CMPNOCASE(prop->name(), "heightceiling"))
			ns->setCeilingHeight(prop->intValue());
		else if (S_CMPNOCASE(prop->name(), "lightlevel"))
			ns->light_ = prop->intValue();
		else if (S_CMPNOCASE(prop->name(), "special"))
			ns->special_ = prop->intValue();
		else if (S_CMPNOCASE(prop->name(), "id"))
			ns->id_ = prop->intValue();
		else
			ns->properties_[prop->name()] = prop->value();
	}

	// Add sector to map
	sectors_.push_back(ns);

	return true;
}

// -----------------------------------------------------------------------------
// Adds a thing to the map from parsed UDMF thing definition [def]
// -----------------------------------------------------------------------------
bool SLADEMap::addThing(ParseTreeNode* def)
{
	// Check for required properties
	auto prop_x    = def->childPTN("x");
	auto prop_y    = def->childPTN("y");
	auto prop_type = def->childPTN("type");
	if (!prop_x || !prop_y || !prop_type)
		return false;

	// Create new thing
	MapThing* nt = new MapThing(prop_x->floatValue(), prop_y->floatValue(), prop_type->intValue(), this);

	// Add extra thing info
	ParseTreeNode* prop = nullptr;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop == prop_x || prop == prop_y || prop == prop_type)
			continue;

		// Builtin properties
		if (S_CMPNOCASE(prop->name(), "angle"))
			nt->angle_ = prop->intValue();
		else
			nt->properties_[prop->name()] = prop->value();
	}

	// Add thing to map
	things_.push_back(nt);

	return true;
}

// -----------------------------------------------------------------------------
// Reads a UDMF format map using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readUDMFMap(Archive::MapDesc map)
{
	// Get TEXTMAP entry (will always be after the 'head' entry)
	auto textmap = map.head->nextEntry();

	// --- Parse UDMF text ---
	UI::setSplashProgressMessage("Parsing TEXTMAP");
	UI::setSplashProgress(-100.0f);
	Parser parser;
	if (!parser.parseText(textmap->data()))
		return false;

	// --- Process parsed data ---

	// First we have to sort the definition blocks by type so they can
	// be created in the correct order (verts->sides->lines->sectors->things),
	// even if they aren't defined in that order.
	// Unknown definitions are also kept, just in case
	UI::setSplashProgressMessage("Sorting definitions");
	auto                   root = parser.parseTreeRoot();
	vector<ParseTreeNode*> defs_vertices;
	vector<ParseTreeNode*> defs_lines;
	vector<ParseTreeNode*> defs_sides;
	vector<ParseTreeNode*> defs_sectors;
	vector<ParseTreeNode*> defs_things;
	vector<ParseTreeNode*> defs_other;
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		UI::setSplashProgress((float)a / root->nChildren());

		auto node = root->childPTN(a);

		// Vertex definition
		if (S_CMPNOCASE(node->name(), "vertex"))
			defs_vertices.push_back(node);

		// Line definition
		else if (S_CMPNOCASE(node->name(), "linedef"))
			defs_lines.push_back(node);

		// Side definition
		else if (S_CMPNOCASE(node->name(), "sidedef"))
			defs_sides.push_back(node);

		// Sector definition
		else if (S_CMPNOCASE(node->name(), "sector"))
			defs_sectors.push_back(node);

		// Thing definition
		else if (S_CMPNOCASE(node->name(), "thing"))
			defs_things.push_back(node);

		// Namespace
		else if (S_CMPNOCASE(node->name(), "namespace"))
			udmf_namespace_ = node->stringValue();

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

	// Keep map-scope values
	for (auto node : defs_other)
	{
		if (node->nValues() > 0)
			udmf_props_[node->name()] = node->value();

		// TODO: Unknown blocks
	}

	UI::setSplashProgressMessage("Init map data");

	// Remove detached vertices
	mapOpenChecks();

	// Update item indices
	refreshIndices();

	// Update sector bounding boxes
	for (auto& sector : sectors_)
		sector->updateBBox();

	// Copy extra entries
	for (auto& a : map.unk)
		udmf_extra_entries_.push_back(new ArchiveEntry(*a));

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format vertex definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomVertexes(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(vertices_.size() * 4, false);
	entry->seek(0, 0);

	// Write vertex data
	short x, y;
	for (auto& vertex : vertices_)
	{
		x = vertex->xPos();
		y = vertex->yPos();
		entry->write(&x, 2);
		entry->write(&y, 2);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format sidedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomSidedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sides_.size() * 30, false);
	entry->seek(0, 0);

	// Write side data
	MapSide::DoomData d_side;
	string            t_m, t_u, t_l;
	for (auto& side : sides_)
	{
		memset(&d_side, 0, 30);

		// Offsets
		d_side.x_offset = side->offset_x_;
		d_side.y_offset = side->offset_y_;

		// Sector
		d_side.sector = -1;
		if (side->sector_)
			d_side.sector = side->sector_->index();

		// Textures
		t_m = side->tex_middle_;
		t_u = side->tex_upper_;
		t_l = side->tex_lower_;
		memcpy(d_side.tex_middle, CHR(t_m), t_m.Length());
		memcpy(d_side.tex_upper, CHR(t_u), t_u.Length());
		memcpy(d_side.tex_lower, CHR(t_l), t_l.Length());

		entry->write(&d_side, 30);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format linedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomLinedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines_.size() * 14, false);
	entry->seek(0, 0);

	// Write line data
	MapLine::DoomData d_line;
	for (auto& line : lines_)
	{
		// Vertices
		d_line.vertex1 = line->v1Index();
		d_line.vertex2 = line->v2Index();

		// Properties
		d_line.flags      = line->intProperty("flags");
		d_line.type       = line->intProperty("special");
		d_line.sector_tag = line->intProperty("arg0");

		// Sides
		d_line.side1 = -1;
		d_line.side2 = -1;
		if (line->side1_)
			d_line.side1 = line->side1_->index();
		if (line->side2_)
			d_line.side2 = line->side2_->index();

		entry->write(&d_line, 14);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format sector definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomSectors(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sectors_.size() * 26, false);
	entry->seek(0, 0);

	// Write sector data
	MapSector::DoomData d_sector;
	for (auto& sector : sectors_)
	{
		memset(&d_sector, 0, 26);

		// Height
		d_sector.f_height = sector->floor_.height;
		d_sector.c_height = sector->ceiling_.height;

		// Textures
		memcpy(d_sector.f_tex, CHR(sector->floor_.texture), sector->floor_.texture.Length());
		memcpy(d_sector.c_tex, CHR(sector->ceiling_.texture), sector->ceiling_.texture.Length());

		// Properties
		d_sector.light   = sector->light_;
		d_sector.special = sector->special_;
		d_sector.tag     = sector->id_;

		entry->write(&d_sector, 26);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format thing definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomThings(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things_.size() * 10, false);
	entry->seek(0, 0);

	// Write thing data
	MapThing::DoomData d_thing;
	for (auto& thing : things_)
	{
		// Position
		d_thing.x = thing->xPos();
		d_thing.y = thing->yPos();

		// Properties
		d_thing.angle = thing->angle();
		d_thing.type  = thing->type_;
		d_thing.flags = thing->intProperty("flags");

		entry->write(&d_thing, 10);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom format map entries and adds them to [map_entries]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoomMap(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	auto entry = new ArchiveEntry("THINGS");
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

// -----------------------------------------------------------------------------
// Writes hexen format linedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeHexenLinedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines_.size() * 16, false);
	entry->seek(0, 0);

	// Write line data
	MapLine::HexenData h_line;
	for (auto& line : lines_)
	{
		// Vertices
		h_line.vertex1 = line->v1Index();
		h_line.vertex2 = line->v2Index();

		// Properties
		h_line.flags = line->intProperty("flags");
		h_line.type  = line->intProperty("special");

		// Args
		for (unsigned arg = 0; arg < 5; arg++)
			h_line.args[arg] = line->intProperty(S_FMT("arg%u", arg));

		// Sides
		h_line.side1 = -1;
		h_line.side2 = -1;
		if (line->side1_)
			h_line.side1 = line->side1_->index();
		if (line->side2_)
			h_line.side2 = line->side2_->index();

		entry->write(&h_line, 16);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes hexen format thing definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeHexenThings(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things_.size() * 20, false);
	entry->seek(0, 0);

	// Write thing data
	MapThing::HexenData h_thing;
	for (auto& thing : things_)
	{
		// Position
		h_thing.x = thing->xPos();
		h_thing.y = thing->yPos();
		h_thing.z = thing->intProperty("height");

		// Properties
		h_thing.angle   = thing->angle();
		h_thing.type    = thing->type_;
		h_thing.flags   = thing->intProperty("flags");
		h_thing.tid     = thing->intProperty("id");
		h_thing.special = thing->intProperty("special");

		// Args
		for (unsigned arg = 0; arg < 5; arg++)
			h_thing.args[arg] = thing->intProperty(S_FMT("arg%u", arg));

		entry->write(&h_thing, 20);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes hexen format map entries and adds them to [map_entries]
// -----------------------------------------------------------------------------
bool SLADEMap::writeHexenMap(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	auto entry = new ArchiveEntry("THINGS");
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

// -----------------------------------------------------------------------------
// Writes doom64 format vertex definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Vertexes(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(vertices_.size() * 8, false);
	entry->seek(0, 0);

	// Write vertex data
	int32_t x, y; // Those are actually fixed_t, so shift by FRACBIT (16)
	for (auto& vertex : vertices_)
	{
		x = vertex->xPos() * 65536;
		y = vertex->yPos() * 65536;
		entry->write(&x, 4);
		entry->write(&y, 4);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format sidedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Sidedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sides_.size() * sizeof(MapSide::Doom64Data), false);
	entry->seek(0, 0);

	// Write side data
	MapSide::Doom64Data d64_side;
	for (auto& side : sides_)
	{
		memset(&d64_side, 0, sizeof(MapSide::Doom64Data));

		// Offsets
		d64_side.x_offset = side->offset_x_;
		d64_side.y_offset = side->offset_y_;

		// Sector
		d64_side.sector = -1;
		if (side->sector_)
			d64_side.sector = side->sector_->index();

		// Textures
		d64_side.tex_middle = theResourceManager->getTextureHash(side->tex_middle_);
		d64_side.tex_upper  = theResourceManager->getTextureHash(side->tex_upper_);
		d64_side.tex_lower  = theResourceManager->getTextureHash(side->tex_lower_);

		entry->write(&d64_side, sizeof(MapSide::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format linedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Linedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines_.size() * sizeof(MapLine::Doom64Data), false);
	entry->seek(0, 0);

	// Write line data
	MapLine::Doom64Data d64_line;
	for (auto& line : lines_)
	{
		// Vertices
		d64_line.vertex1 = line->v1Index();
		d64_line.vertex2 = line->v2Index();

		// Properties
		d64_line.flags      = line->intProperty("flags");
		d64_line.type       = line->intProperty("special");
		d64_line.sector_tag = line->intProperty("arg0");

		// Sides
		d64_line.side1 = -1;
		d64_line.side2 = -1;
		if (line->side1_)
			d64_line.side1 = line->side1_->index();
		if (line->side2_)
			d64_line.side2 = line->side2_->index();

		entry->write(&d64_line, sizeof(MapLine::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format sector definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Sectors(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sectors_.size() * sizeof(MapSector::Doom64Data), false);
	entry->seek(0, 0);

	// Write sector data
	MapSector::Doom64Data d64_sector;
	for (auto& sector : sectors_)
	{
		memset(&d64_sector, 0, sizeof(MapSector::Doom64Data));

		// Height
		d64_sector.f_height = sector->floor_.height;
		d64_sector.c_height = sector->ceiling_.height;

		// Textures
		d64_sector.f_tex = theResourceManager->getTextureHash(sector->stringProperty("texturefloor"));
		d64_sector.c_tex = theResourceManager->getTextureHash(sector->stringProperty("textureceiling"));

		// Colors
		d64_sector.color[0] = sector->intProperty("color_things");
		d64_sector.color[1] = sector->intProperty("color_floor");
		d64_sector.color[2] = sector->intProperty("color_ceiling");
		d64_sector.color[3] = sector->intProperty("color_upper");
		d64_sector.color[4] = sector->intProperty("color_lower");

		// Properties
		d64_sector.special = sector->special_;
		d64_sector.flags   = sector->intProperty("flags");
		d64_sector.tag     = sector->id_;

		entry->write(&d64_sector, sizeof(MapSector::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format thing definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Things(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things_.size() * sizeof(MapThing::Doom64Data), false);
	entry->seek(0, 0);

	// Write thing data
	MapThing::Doom64Data d64_thing;
	for (auto& thing : things_)
	{
		// Position
		d64_thing.x = thing->xPos();
		d64_thing.y = thing->yPos();
		d64_thing.z = thing->intProperty("height");

		// Properties
		d64_thing.angle = thing->angle();
		d64_thing.type  = thing->type_;
		d64_thing.flags = thing->intProperty("flags");
		d64_thing.tid   = thing->intProperty("id");

		entry->write(&d64_thing, sizeof(MapThing::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format map entries and adds them to [map_entries]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Map(vector<ArchiveEntry*>& map_entries)
{
	// Init entry list
	map_entries.clear();

	// Write THINGS
	auto entry = new ArchiveEntry("THINGS");
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

// -----------------------------------------------------------------------------
// Writes map as UDMF format text to [textmap]
// -----------------------------------------------------------------------------
bool SLADEMap::writeUDMFMap(ArchiveEntry* textmap)
{
	// Check entry was given
	if (!textmap)
		return false;

	// Open temp text file
	wxFile tempfile(App::path("sladetemp.txt", App::Dir::Temp), wxFile::write);

	// Write map namespace
	tempfile.Write("// Written by SLADE3\n");
	tempfile.Write(S_FMT("namespace=\"%s\";\n", udmf_namespace_));

	// Write map-scope props
	tempfile.Write(udmf_props_.toString(true));
	tempfile.Write("\n");

	// sf::Clock clock;

	// Locale for float number format
	setlocale(LC_NUMERIC, "C");

	// Write things
	string object_def;
	for (unsigned a = 0; a < things_.size(); a++)
	{
		object_def = S_FMT("thing//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT(
			"x=%1.3f;\ny=%1.3f;\ntype=%d;\n", things_[a]->position_.x, things_[a]->position_.y, things_[a]->type_);
		if (things_[a]->angle_ != 0)
			object_def += S_FMT("angle=%d;\n", things_[a]->angle_);

		// Remove internal 'flags' property if it exists
		things_[a]->props().removeProperty("flags");

		// Other properties
		if (!things_[a]->properties_.isEmpty())
		{
			Game::configuration().cleanObjectUDMFProps(things_[a]);
			object_def += things_[a]->properties_.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	// LOG_MESSAGE(1, "Writing things took %dms", clock.getElapsedTime().asMilliseconds());

	// Write lines
	// clock.restart();
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		object_def = S_FMT("linedef//#%u\n{\n", a);

		// Basic properties
		object_def +=
			S_FMT("v1=%d;\nv2=%d;\nsidefront=%d;\n", lines_[a]->v1Index(), lines_[a]->v2Index(), lines_[a]->s1Index());
		if (lines_[a]->s2())
			object_def += S_FMT("sideback=%d;\n", lines_[a]->s2Index());
		if (lines_[a]->special_ != 0)
			object_def += S_FMT("special=%d;\n", lines_[a]->special_);
		if (lines_[a]->line_id_ != 0)
			object_def += S_FMT("id=%d;\n", lines_[a]->line_id_);

		// Remove internal 'flags' property if it exists
		lines_[a]->props().removeProperty("flags");

		// Other properties
		if (!lines_[a]->properties_.isEmpty())
		{
			Game::configuration().cleanObjectUDMFProps(lines_[a]);
			object_def += lines_[a]->properties_.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	// LOG_MESSAGE(1, "Writing lines took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sides
	// clock.restart();
	for (unsigned a = 0; a < sides_.size(); a++)
	{
		object_def = S_FMT("sidedef//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("sector=%u;\n", sides_[a]->sector_->index());
		if (sides_[a]->tex_upper_ != "-")
			object_def += S_FMT("texturetop=\"%s\";\n", sides_[a]->tex_upper_);
		if (sides_[a]->tex_middle_ != "-")
			object_def += S_FMT("texturemiddle=\"%s\";\n", sides_[a]->tex_middle_);
		if (sides_[a]->tex_lower_ != "-")
			object_def += S_FMT("texturebottom=\"%s\";\n", sides_[a]->tex_lower_);
		if (sides_[a]->offset_x_ != 0)
			object_def += S_FMT("offsetx=%d;\n", sides_[a]->offset_x_);
		if (sides_[a]->offset_y_ != 0)
			object_def += S_FMT("offsety=%d;\n", sides_[a]->offset_y_);

		// Other properties
		if (!sides_[a]->properties_.isEmpty())
		{
			Game::configuration().cleanObjectUDMFProps(sides_[a]);
			object_def += sides_[a]->properties_.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	// LOG_MESSAGE(1, "Writing sides took %dms", clock.getElapsedTime().asMilliseconds());

	// Write vertices
	// clock.restart();
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		object_def = S_FMT("vertex//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT("x=%1.3f;\ny=%1.3f;\n", vertices_[a]->position_.x, vertices_[a]->position_.y);

		// Other properties
		if (!vertices_[a]->properties_.isEmpty())
		{
			Game::configuration().cleanObjectUDMFProps(vertices_[a]);
			object_def += vertices_[a]->properties_.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	// LOG_MESSAGE(1, "Writing vertices took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sectors
	// clock.restart();
	for (unsigned a = 0; a < sectors_.size(); a++)
	{
		object_def = S_FMT("sector//#%u\n{\n", a);

		// Basic properties
		object_def += S_FMT(
			"texturefloor=\"%s\";\ntextureceiling=\"%s\";\n",
			sectors_[a]->floor_.texture,
			sectors_[a]->ceiling_.texture);
		if (sectors_[a]->floor_.height != 0)
			object_def += S_FMT("heightfloor=%d;\n", sectors_[a]->floor_.height);
		if (sectors_[a]->ceiling_.height != 0)
			object_def += S_FMT("heightceiling=%d;\n", sectors_[a]->ceiling_.height);
		if (sectors_[a]->light_ != 160)
			object_def += S_FMT("lightlevel=%d;\n", sectors_[a]->light_);
		if (sectors_[a]->special_ != 0)
			object_def += S_FMT("special=%d;\n", sectors_[a]->special_);
		if (sectors_[a]->id_ != 0)
			object_def += S_FMT("id=%d;\n", sectors_[a]->id_);

		// Other properties
		if (!sectors_[a]->properties_.isEmpty())
		{
			Game::configuration().cleanObjectUDMFProps(sectors_[a]);
			object_def += sectors_[a]->properties_.toString(true);
		}

		object_def += "}\n\n";
		tempfile.Write(object_def);
	}
	// LOG_MESSAGE(1, "Writing sectors took %dms", clock.getElapsedTime().asMilliseconds());

	// Close file
	tempfile.Close();

	// Load file to entry
	textmap->importFile(App::path("sladetemp.txt", App::Dir::Temp));

	return true;
}

// -----------------------------------------------------------------------------
// Clears all map data
// -----------------------------------------------------------------------------
void SLADEMap::clearMap()
{
	map_specials_.reset();

	// Clear vectors
	sides_.clear();
	lines_.clear();
	vertices_.clear();
	sectors_.clear();
	things_.clear();

	// Clear map objects
	for (auto& obj : all_objects_)
		delete obj.mobj;
	all_objects_.clear();

	// Object id 0 is always null
	all_objects_.emplace_back(nullptr, false);

	// Clear usage counts
	usage_flat_.clear();
	usage_tex_.clear();
	usage_thing_type_.clear();

	// Clear UDMF extra entries
	for (auto& entry : udmf_extra_entries_)
		delete entry;
	udmf_extra_entries_.clear();
}

// -----------------------------------------------------------------------------
// Removes [vertex] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeVertex(MapVertex* vertex, bool merge_lines)
{
	// Check vertex was given
	if (!vertex)
		return false;

	return removeVertex(vertex->index_, merge_lines);
}

// -----------------------------------------------------------------------------
// Removes the vertex at [index] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeVertex(unsigned index, bool merge_lines)
{
	// Check index
	if (index >= vertices_.size())
		return false;

	// Check if we should merge connected lines
	bool merged = false;
	if (merge_lines && vertices_[index]->connected_lines_.size() == 2)
	{
		// Get other end vertex of second connected line
		auto l_first  = vertices_[index]->connected_lines_[0];
		auto l_second = vertices_[index]->connected_lines_[1];
		auto v_end    = l_second->vertex2_;
		if (v_end == vertices_[index])
			v_end = l_second->vertex1_;

		// Remove second connected line
		removeLine(l_second);

		// Connect first connected line to other end vertex
		l_first->setModified();
		auto v_start = l_first->vertex1_;
		if (l_first->vertex1_ == vertices_[index])
		{
			l_first->vertex1_ = v_end;
			v_start           = l_first->vertex2_;
		}
		else
			l_first->vertex2_ = v_end;
		vertices_[index]->disconnectLine(l_first);
		v_end->connectLine(l_first);
		l_first->resetInternals();

		// Check if we ended up with overlapping lines (ie. there was a triangle)
		for (unsigned a = 0; a < v_end->nConnectedLines(); a++)
		{
			if (v_end->connected_lines_[a] == l_first)
				continue;

			if ((v_end->connected_lines_[a]->vertex1_ == v_end && v_end->connected_lines_[a]->vertex2_ == v_start)
				|| (v_end->connected_lines_[a]->vertex2_ == v_end && v_end->connected_lines_[a]->vertex1_ == v_start))
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
		auto clines = vertices_[index]->connected_lines_;
		for (auto& line : clines)
			removeLine(line);
	}

	// Remove the vertex
	removeMapObject(vertices_[index]);
	vertices_[index]         = vertices_.back();
	vertices_[index]->index_ = index;
	vertices_.pop_back();

	geometry_updated_ = App::runTimer();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [line] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeLine(MapLine* line)
{
	// Check line was given
	if (!line)
		return false;

	return removeLine(line->index_);
}

// -----------------------------------------------------------------------------
// Removes the line at [index] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeLine(unsigned index)
{
	// Check index
	if (index >= lines_.size())
		return false;

	LOG_MESSAGE(4, "id %u  index %u  objindex %u", lines_[index]->obj_id_, index, lines_[index]->index_);

	// Init
	lines_[index]->resetInternals();

	// Remove the line's sides
	if (lines_[index]->side1_)
		removeSide(lines_[index]->side1_, false);
	if (lines_[index]->side2_)
		removeSide(lines_[index]->side2_, false);

	// Disconnect from vertices
	lines_[index]->vertex1_->disconnectLine(lines_[index]);
	lines_[index]->vertex2_->disconnectLine(lines_[index]);

	// Remove the line
	removeMapObject(lines_[index]);
	lines_[index]         = lines_[lines_.size() - 1];
	lines_[index]->index_ = index;
	// lines[index]->modified_time = App::runTimer();
	lines_.pop_back();

	geometry_updated_ = App::runTimer();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [side] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeSide(MapSide* side, bool remove_from_line)
{
	// Check side was given
	if (!side)
		return false;

	return removeSide(side->index_, remove_from_line);
}

// -----------------------------------------------------------------------------
// Removes the side at [index] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeSide(unsigned index, bool remove_from_line)
{
	// Check index
	if (index >= sides_.size())
		return false;

	if (remove_from_line)
	{
		// Remove from parent line
		auto l = sides_[index]->parent_;
		l->setModified();
		if (l->side1_ == sides_[index])
			l->side1_ = nullptr;
		if (l->side2_ == sides_[index])
			l->side2_ = nullptr;

		// Set appropriate line flags
		Game::configuration().setLineBasicFlag("blocking", l, current_format_, true);
		Game::configuration().setLineBasicFlag("twosided", l, current_format_, false);
	}

	// Remove side from its sector, if any
	if (sides_[index]->sector_)
	{
		for (unsigned a = 0; a < sides_[index]->sector_->connected_sides_.size(); a++)
		{
			if (sides_[index]->sector_->connected_sides_[a] == sides_[index])
			{
				auto sector = sides_[index]->sector_;
				sector->connected_sides_.erase(sides_[index]->sector_->connected_sides_.begin() + a);

				// Remove sector if all its sides are gone
				if (sector->connected_sides_.empty())
					removeSector(sector);

				break;
			}
		}
	}

	// Update texture usage
	usage_tex_[sides_[index]->tex_lower_.Upper()] -= 1;
	usage_tex_[sides_[index]->tex_middle_.Upper()] -= 1;
	usage_tex_[sides_[index]->tex_upper_.Upper()] -= 1;

	// Remove the side
	removeMapObject(sides_[index]);
	sides_[index]         = sides_.back();
	sides_[index]->index_ = index;
	sides_.pop_back();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [sector] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeSector(MapSector* sector)
{
	// Check sector was given
	if (!sector)
		return false;

	return removeSector(sector->index_);
}

// -----------------------------------------------------------------------------
// Removes the sector at [index] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeSector(unsigned index)
{
	// Check index
	if (index >= sectors_.size())
		return false;

	// Clear connected sides' sectors
	// for (unsigned a = 0; a < sectors[index]->connected_sides.size(); a++)
	//	sectors[index]->connected_sides[a]->sector = NULL;

	// Update texture usage
	usage_flat_[sectors_[index]->floor_.texture.Upper()] -= 1;
	usage_flat_[sectors_[index]->ceiling_.texture.Upper()] -= 1;

	// Remove the sector
	removeMapObject(sectors_[index]);
	sectors_[index]         = sectors_.back();
	sectors_[index]->index_ = index;
	// sectors[index]->modified_time = App::runTimer();
	sectors_.pop_back();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [thing] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeThing(MapThing* thing)
{
	// Check thing was given
	if (!thing)
		return false;

	return removeThing(thing->index_);
}

// -----------------------------------------------------------------------------
// Removes the thing at [index] from the map
// -----------------------------------------------------------------------------
bool SLADEMap::removeThing(unsigned index)
{
	// Check index
	if (index >= things_.size())
		return false;

	// Remove the thing
	removeMapObject(things_[index]);
	things_[index]         = things_.back();
	things_[index]->index_ = index;
	// things[index]->modified_time = App::runTimer();
	things_.pop_back();

	things_updated_ = App::runTimer();

	return true;
}

// -----------------------------------------------------------------------------
// Returns the index of the vertex closest to the point, or -1 if none found.
// Igonres any vertices further away than [min]
// -----------------------------------------------------------------------------
int SLADEMap::nearestVertex(Vec2f point, double min)
{
	// Go through vertices
	double     min_dist = 999999999;
	MapVertex* v        = nullptr;
	double     dist     = 0;
	int        index    = -1;
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		v = vertices_[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(v->position());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			index    = a;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest vertex,
	// to check for minimum hilight distance
	if (index >= 0)
	{
		v            = vertices_[index];
		double rdist = MathStuff::distance(v->position(), point);
		if (rdist > min)
			return -1;
	}

	return index;
}

// -----------------------------------------------------------------------------
// Returns the index of the line closest to the point, or -1 if none is found.
// Ignores lines further away than [mindist]
// -----------------------------------------------------------------------------
int SLADEMap::nearestLine(Vec2f point, double mindist)
{
	// Go through lines
	double   min_dist = mindist;
	double   dist     = 0;
	int      index    = -1;
	MapLine* l;
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		l = lines_[a];

		// Check with line bounding box first (since we have a minimum distance)
		auto bbox = l->seg();
		bbox.expand(mindist, mindist);
		if (!bbox.contains(point))
			continue;

		// Calculate distance to line
		dist = l->distanceTo(point);

		// Check if it's nearer than the previous nearest
		if (dist < min_dist && dist < mindist)
		{
			index    = a;
			min_dist = dist;
		}
	}

	return index;
}

// -----------------------------------------------------------------------------
// Returns the index of the thing closest to the point, or -1 if none found.
// Igonres any thing further away than [min]
// -----------------------------------------------------------------------------
int SLADEMap::nearestThing(Vec2f point, double min)
{
	// Go through things
	double    min_dist = 999999999;
	MapThing* t        = nullptr;
	double    dist     = 0;
	int       index    = -1;
	for (unsigned a = 0; a < things_.size(); a++)
	{
		t = things_[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(t->position());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			index    = a;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest thing,
	// to check for minimum hilight distance
	if (index >= 0)
	{
		t            = things_[index];
		double rdist = MathStuff::distance(t->position(), point);
		if (rdist > min)
			return -1;
	}

	return index;
}

// -----------------------------------------------------------------------------
// Same as nearestThing, but returns a list of indices for the case where there
// are multiple things at the same point
// -----------------------------------------------------------------------------
vector<int> SLADEMap::nearestThingMulti(Vec2f point)
{
	// Go through things
	vector<int> ret;
	double      min_dist = 999999999;
	MapThing*   t        = nullptr;
	double      dist     = 0;
	for (unsigned a = 0; a < things_.size(); a++)
	{
		t = things_[a];

		// Get 'quick' distance (no need to get real distance)
		dist = point.taxicab_distance_to(t->position());

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

// -----------------------------------------------------------------------------
// Returns the index of the sector at the given point, or -1 if not within a
// sector
// -----------------------------------------------------------------------------
int SLADEMap::sectorAt(Vec2f point)
{
	// Go through sectors
	for (unsigned a = 0; a < sectors_.size(); a++)
	{
		// Check if point is within sector
		if (sectors_[a]->isWithin(point))
			return a;
	}

	// Not within a sector
	return -1;
}

// -----------------------------------------------------------------------------
// Returns a bounding box for the entire map
// -----------------------------------------------------------------------------
BBox SLADEMap::bounds()
{
	BBox bbox;

	// Return invalid bbox if no sectors
	if (sectors_.empty())
		return bbox;

	// Go through sectors
	// This is quicker than generating it from vertices,
	// but relies on sector bboxes being up-to-date (which they should be)
	bbox = sectors_[0]->boundingBox();
	for (unsigned a = 1; a < sectors_.size(); a++)
	{
		auto sbb = sectors_[a]->boundingBox();
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

// -----------------------------------------------------------------------------
// Returns the vertex at [x,y], or NULL if none there
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::vertexAt(double x, double y)
{
	// Go through all vertices
	for (auto& vertex : vertices_)
	{
		if (vertex->position_.x == x && vertex->position_.y == y)
			return vertex;
	}

	// No vertex at [x,y]
	return nullptr;
}

// Sorting functions for SLADEMap::cutLines
bool sortVPosXAsc(const Vec2f& left, const Vec2f& right)
{
	return left.x < right.x;
}
bool sortVPosXDesc(const Vec2f& left, const Vec2f& right)
{
	return left.x > right.x;
}
bool sortVPosYAsc(const Vec2f& left, const Vec2f& right)
{
	return left.y < right.y;
}
bool sortVPosYDesc(const Vec2f& left, const Vec2f& right)
{
	return left.y > right.y;
}

// -----------------------------------------------------------------------------
// Returns a list of points that the 'cutting' line from [x1,y1] to [x2,y2]
// crosses any existing lines on the map.
// The list is sorted along the direction of the 'cutting' line
// -----------------------------------------------------------------------------
vector<Vec2f> SLADEMap::cutLines(double x1, double y1, double x2, double y2)
{
	Seg2f cutter(x1, y1, x2, y2);
	// Init
	vector<Vec2f> intersect_points;
	Vec2f         intersection;

	// Go through map lines
	for (auto& line : lines_)
	{
		// Check for intersection
		intersection = cutter.p1();
		if (MathStuff::linesIntersect(cutter, line->seg(), intersection))
		{
			// Add intersection point to vector
			intersect_points.push_back(intersection);
			LOG_DEBUG("Intersection point", intersection, "valid with", lines_[a]);
		}
		else if (intersection != cutter.p1())
		{
			LOG_DEBUG("Intersection point", intersection, "invalid");
		}
	}

	// Return if no intersections
	if (intersect_points.empty())
		return intersect_points;

	// Check cutting line direction
	double xdif = x2 - x1;
	double ydif = y2 - y1;
	if ((xdif * xdif) > (ydif * ydif))
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

// -----------------------------------------------------------------------------
// Returns the first vertex that the line from [x1,y1] to [x2,y2] crosses over
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::lineCrossVertex(double x1, double y1, double x2, double y2)
{
	Seg2f seg(x1, y1, x2, y2);

	// Go through vertices
	MapVertex* cv       = nullptr;
	double     min_dist = 999999;
	for (auto vertex : vertices_)
	{
		auto point = vertex->position();

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
				cv       = vertex;
				min_dist = dist;
			}
		}
	}

	// Return closest overlapping vertex to line start
	return cv;
}

// -----------------------------------------------------------------------------
// Updates geometry info (polygons/bbox/etc) for anything modified since
// [modified_time]
// -----------------------------------------------------------------------------
void SLADEMap::updateGeometryInfo(long modified_time)
{
	for (auto& vertex : vertices_)
	{
		if (vertex->modifiedTime() > modified_time)
		{
			for (auto line : vertex->connected_lines_)
			{
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

// -----------------------------------------------------------------------------
// Returns true if [line1] and [line2] intersect.
// If an intersection occurs, [x] and [y] are set to the intersection point
// -----------------------------------------------------------------------------
bool SLADEMap::linesIntersect(MapLine* line1, MapLine* line2, double& x, double& y) const
{
	Vec2f intersection;
	bool  res = MathStuff::linesIntersect(line1->seg(), line2->seg(), intersection);
	x         = intersection.x;
	y         = intersection.y;
	return res;
}

// -----------------------------------------------------------------------------
// Finds the 'text point' for [sector].
// This is a point within the sector that is reasonably close to the middle of
// the sector bbox while still being within the sector itself
// -----------------------------------------------------------------------------
void SLADEMap::findSectorTextPoint(MapSector* sector) const
{
	// Check sector
	if (!sector)
		return;

	// Check if actual sector midpoint can be used
	sector->text_point_ = sector->getPoint(MapObject::Point::Mid);
	if (sector->isWithin(sector->text_point_))
		return;

	if (sector->connected_sides_.empty())
		return;

	// Find nearest line to sector midpoint (that is also part of the sector)
	double min_dist = 9999999999.0;
	auto   mid_side = sector->connected_sides_[0];
	for (unsigned a = 0; a < sector->connected_sides_.size(); a++)
	{
		auto   l    = sector->connected_sides_[a]->parent_;
		double dist = MathStuff::distanceToLineFast(sector->text_point_, l->seg());

		if (dist < min_dist)
		{
			min_dist = dist;
			mid_side = sector->connected_sides_[a];
		}
	}

	// Calculate ray
	auto r_o = mid_side->parent_->getPoint(MapObject::Point::Mid);
	auto r_d = mid_side->parent_->frontVector();
	if (mid_side == mid_side->parent_->side1_)
		r_d.set(-r_d.x, -r_d.y);

	// Find nearest intersecting line
	min_dist = 9999999999.0;
	for (auto& connected_side : sector->connected_sides_)
	{
		if (connected_side == mid_side)
			continue;

		auto   line = connected_side->parent_;
		double dist = MathStuff::distanceRayLine(r_o, r_o + r_d, line->point1(), line->point2());

		if (dist > 0 && dist < min_dist)
			min_dist = dist;
	}

	// Set text point to halfway between the two lines
	sector->text_point_.set(r_o.x + (r_d.x * min_dist * 0.5), r_o.y + (r_d.y * min_dist * 0.5));
}

// -----------------------------------------------------------------------------
// Forces building of polygons for all sectors
// -----------------------------------------------------------------------------
void SLADEMap::initSectorPolygons()
{
	UI::setSplashProgressMessage("Building sector polygons");
	UI::setSplashProgress(0.0f);
	for (unsigned a = 0; a < sectors_.size(); a++)
	{
		UI::setSplashProgress((float)a / (float)sectors_.size());
		sectors_[a]->polygon();
	}
	UI::setSplashProgress(1.0f);
}

// -----------------------------------------------------------------------------
// Returns the nearest line that intersects with the vector from the middle of
// [line] outwards from the [front] or back of the line perpendicular.
// If an intersection is found the coordinates are set in [hit_x],[hit_y]
// -----------------------------------------------------------------------------
MapLine* SLADEMap::lineVectorIntersect(MapLine* line, bool front, double& hit_x, double& hit_y) const
{
	// Get sector
	auto sector = front ? line->frontSector() : line->backSector();
	if (!sector)
		return nullptr;

	// Get lines to test
	vector<MapLine*> lines;
	sector->putLines(lines);

	// Get nearest line intersecting with line vector
	MapLine* nearest = nullptr;
	auto     mid     = line->getPoint(MapObject::Point::Mid);
	auto     vec     = line->frontVector();
	if (front)
	{
		vec.x = -vec.x;
		vec.y = -vec.y;
	}
	double min_dist = 99999999999;
	for (auto& s_line : lines)
	{
		if (s_line == line)
			continue;

		double dist = MathStuff::distanceRayLine(mid, mid + vec, s_line->point1(), s_line->point2());

		if (dist < min_dist && dist > 0)
		{
			min_dist = dist;
			nearest  = s_line;
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

// -----------------------------------------------------------------------------
// Adds all sectors with tag [tag] to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putSectorsWithTag(int tag, vector<MapSector*>& list)
{
	if (tag == 0)
		return;

	// Find sectors with matching tag
	for (auto& sector : sectors_)
	{
		if (sector->id_ == tag)
			list.push_back(sector);
	}
}

// -----------------------------------------------------------------------------
// Adds all things with TID [id] to [list].
// If [type] is not 0, only checks things of that type
// -----------------------------------------------------------------------------
void SLADEMap::putThingsWithId(int id, vector<MapThing*>& list, unsigned start, int type)
{
	if (id == 0)
		return;

	// Find things with matching id
	for (unsigned a = start; a < things_.size(); a++)
	{
		if (things_[a]->intProperty("id") == id && (type == 0 || things_[a]->type_ == type))
			list.push_back(things_[a]);
	}
}

// -----------------------------------------------------------------------------
// Returns the first thing in the map with TID [id]
// -----------------------------------------------------------------------------
MapThing* SLADEMap::findFirstThingWithId(int id)
{
	if (id == 0)
		return nullptr;

	// Find things with matching id, but ignore dragons, we don't want them!
	for (auto& thing : things_)
	{
		auto& tt = Game::configuration().thingType(thing->type());
		if (thing->intProperty("id") == id && !(tt.flags() & Game::ThingType::Flags::Dragon))
			return thing;
	}
	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all things with TID [id] that are also within a sector with tag [tag] to
// [list]
// -----------------------------------------------------------------------------
void SLADEMap::putThingsWithIdInSectorTag(int id, int tag, vector<MapThing*>& list)
{
	if (id == 0 && tag == 0)
		return;

	// Find things with matching id contained in sector with matching tag
	for (auto& thing : things_)
	{
		if (thing->intProperty("id") == id)
		{
			int si = sectorAt(thing->position());
			if (si > -1 && (unsigned)si < sectors_.size() && sectors_[si]->id_ == tag)
			{
				list.push_back(thing);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Gets dragon targets (needs better description)
// -----------------------------------------------------------------------------
void SLADEMap::putDragonTargets(MapThing* first, vector<MapThing*>& list)
{
	std::map<int, int> used;
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
				putThingsWithId(val, list);
			}
		}
		++i;
	}
}

// -----------------------------------------------------------------------------
// Adds all things with a 'pathed' type to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putPathedThings(vector<MapThing*>& list)
{
	// Find things that need to be pathed
	for (auto& thing : things_)
	{
		auto& tt = Game::configuration().thingType(thing->type());
		if (tt.flags() & (Game::ThingType::Flags::Pathed | Game::ThingType::Flags::Dragon))
			list.push_back(thing);
	}
}

// -----------------------------------------------------------------------------
// Adds all lines with [id] to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putLinesWithId(int id, vector<MapLine*>& list)
{
	if (id == 0)
		return;

	// Find lines with matching id
	for (auto& line : lines_)
	{
		if (line->line_id_ == id)
			list.push_back(line);
	}
}

// -----------------------------------------------------------------------------
// Adds all things with special affecting matching id to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putTaggingThingsWithId(int id, int type, vector<MapThing*>& list, int ttype)
{
	using Game::TagType;

	// Find things with special affecting matching id
	int tag, arg2, arg3, arg4, arg5, tid;
	for (auto& thing : things_)
	{
		auto& tt        = Game::configuration().thingType(thing->type());
		auto  needs_tag = tt.needsTag();
		if (needs_tag != TagType::None
			|| (thing->intProperty("special") && !(tt.flags() & Game::ThingType::Flags::Script)))
		{
			if (needs_tag == TagType::None)
				needs_tag = Game::configuration().actionSpecial(thing->intProperty("special")).needsTag();
			tag       = thing->intProperty("arg0");
			bool fits = false;
			int  path_type;
			switch (needs_tag)
			{
			case TagType::Sector:
			case TagType::SectorOrBack:
			case TagType::SectorAndBack: fits = (IDEQ(tag) && type == SECTORS); break;
			case TagType::LineNegative: tag = abs(tag);
			case TagType::Line: fits = (IDEQ(tag) && type == LINEDEFS); break;
			case TagType::Thing: fits = (IDEQ(tag) && type == THINGS); break;
			case TagType::Thing1Sector2:
				arg2 = thing->intProperty("arg1");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SECTORS));
				break;
			case TagType::Thing1Sector3:
				arg3 = thing->intProperty("arg2");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SECTORS));
				break;
			case TagType::Thing1Thing2:
				arg2 = thing->intProperty("arg1");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Thing1Thing4:
				arg4 = thing->intProperty("arg3");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case TagType::Thing1Thing2Thing3:
				arg2 = thing->intProperty("arg1");
				arg3 = thing->intProperty("arg2");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case TagType::Sector1Thing2Thing3Thing5:
				arg2 = thing->intProperty("arg1");
				arg3 = thing->intProperty("arg2");
				arg5 = thing->intProperty("arg4");
				fits = (type == SECTORS ? (IDEQ(tag)) : (type == THINGS && (IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case TagType::LineId1Line2:
				arg2 = thing->intProperty("arg1");
				fits = (type == LINEDEFS && IDEQ(arg2));
				break;
			case TagType::Thing4:
				arg4 = thing->intProperty("arg3");
				fits = (type == THINGS && IDEQ(arg4));
				break;
			case TagType::Thing5:
				arg5 = thing->intProperty("arg4");
				fits = (type == THINGS && IDEQ(arg5));
				break;
			case TagType::Line1Sector2:
				arg2 = thing->intProperty("arg1");
				fits = (type == LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SECTORS));
				break;
			case TagType::Sector1Sector2:
				arg2 = thing->intProperty("arg1");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Sector1Sector2Sector3Sector4:
				arg2 = thing->intProperty("arg1");
				arg3 = thing->intProperty("arg2");
				arg4 = thing->intProperty("arg3");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case TagType::Sector2Is3Line:
				arg2 = thing->intProperty("arg1");
				fits = (IDEQ(tag) && (arg2 == 3 ? type == LINEDEFS : type == SECTORS));
				break;
			case TagType::Sector1Thing2:
				arg2 = thing->intProperty("arg1");
				fits = (type == SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == THINGS));
				break;
			case TagType::Patrol: path_type = 9047;
			case TagType::Interpolation:
			{
				path_type = 9075;

				tid = thing->intProperty("id");
				// auto& tt = Game::configuration().thingType(thing->type());
				fits = ((path_type == ttype) && (IDEQ(tid)) && (tt.needsTag() == needs_tag));
			}
			break;
			default: break;
			}
			if (fits)
				list.push_back(thing);
		}
	}
}

// -----------------------------------------------------------------------------
// Adds all lines with special affecting matching id to [list]
// -----------------------------------------------------------------------------
void SLADEMap::putTaggingLinesWithId(int id, int type, vector<MapLine*>& list)
{
	using Game::TagType;

	// Find lines with special affecting matching id
	int tag, arg2, arg3, arg4, arg5;
	for (auto& line : lines_)
	{
		int special = line->special_;
		if (special)
		{
			tag       = line->intProperty("arg0");
			bool fits = false;
			switch (Game::configuration().actionSpecial(line->special_).needsTag())
			{
			case TagType::Sector:
			case TagType::SectorOrBack:
			case TagType::SectorAndBack: fits = (IDEQ(tag) && type == SECTORS); break;
			case TagType::LineNegative: tag = abs(tag);
			case TagType::Line: fits = (IDEQ(tag) && type == LINEDEFS); break;
			case TagType::Thing: fits = (IDEQ(tag) && type == THINGS); break;
			case TagType::Thing1Sector2:
				arg2 = line->intProperty("arg1");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SECTORS));
				break;
			case TagType::Thing1Sector3:
				arg3 = line->intProperty("arg2");
				fits = (type == THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SECTORS));
				break;
			case TagType::Thing1Thing2:
				arg2 = line->intProperty("arg1");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Thing1Thing4:
				arg4 = line->intProperty("arg3");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case TagType::Thing1Thing2Thing3:
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				fits = (type == THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case TagType::Sector1Thing2Thing3Thing5:
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				arg5 = line->intProperty("arg4");
				fits = (type == SECTORS ? (IDEQ(tag)) : (type == THINGS && (IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case TagType::LineId1Line2:
				arg2 = line->intProperty("arg1");
				fits = (type == LINEDEFS && IDEQ(arg2));
				break;
			case TagType::Thing4:
				arg4 = line->intProperty("arg3");
				fits = (type == THINGS && IDEQ(arg4));
				break;
			case TagType::Thing5:
				arg5 = line->intProperty("arg4");
				fits = (type == THINGS && IDEQ(arg5));
				break;
			case TagType::Line1Sector2:
				arg2 = line->intProperty("arg1");
				fits = (type == LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SECTORS));
				break;
			case TagType::Sector1Sector2:
				arg2 = line->intProperty("arg1");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Sector1Sector2Sector3Sector4:
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				arg4 = line->intProperty("arg3");
				fits = (type == SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case TagType::Sector2Is3Line:
				arg2 = line->intProperty("arg1");
				fits = (IDEQ(tag) && (arg2 == 3 ? type == LINEDEFS : type == SECTORS));
				break;
			case TagType::Sector1Thing2:
				arg2 = line->intProperty("arg1");
				fits = (type == SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == THINGS));
				break;
			default: break;
			}
			if (fits)
				list.push_back(line);
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the lowest unused sector tag
// -----------------------------------------------------------------------------
int SLADEMap::findUnusedSectorTag()
{
	int tag = 1;
	for (unsigned a = 0; a < sectors_.size(); a++)
	{
		if (sectors_[a]->id_ == tag)
		{
			tag++;
			a = 0;
		}
	}

	return tag;
}

// -----------------------------------------------------------------------------
// Returns the lowest unused thing id
// -----------------------------------------------------------------------------
int SLADEMap::findUnusedThingId()
{
	int tag = 1;
	for (unsigned a = 0; a < things_.size(); a++)
	{
		if (things_[a]->intProperty("id") == tag)
		{
			tag++;
			a = 0;
		}
	}

	return tag;
}

// -----------------------------------------------------------------------------
// Returns the lowest unused line id
// -----------------------------------------------------------------------------
int SLADEMap::findUnusedLineId()
{
	int tag = 1;

	// UDMF (id property)
	if (current_format_ == MapFormat::UDMF)
	{
		for (unsigned a = 0; a < lines_.size(); a++)
		{
			if (lines_[a]->line_id_ == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	// Hexen (special 121 arg0)
	else if (current_format_ == MapFormat::Hexen)
	{
		for (unsigned a = 0; a < lines_.size(); a++)
		{
			if (lines_[a]->special_ == 121 && lines_[a]->intProperty("arg0") == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	// Boom (sector tag (arg0))
	else if (current_format_ == MapFormat::Doom && Game::configuration().featureSupported(Game::Feature::Boom))
	{
		for (unsigned a = 0; a < lines_.size(); a++)
		{
			if (lines_[a]->intProperty("arg0") == tag)
			{
				tag++;
				a = 0;
			}
		}
	}

	return tag;
}

// -----------------------------------------------------------------------------
// Returns the first texture at [tex_part] found on lines connected to [vertex]
// -----------------------------------------------------------------------------
string SLADEMap::adjacentLineTexture(MapVertex* vertex, int tex_part) const
{
	// Go through adjacent lines
	string tex = "-";
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		auto l = vertex->connectedLine(a);

		if (l->side1_)
		{
			// Front middle
			if (tex_part & MapLine::Part::FrontMiddle)
			{
				tex = l->stringProperty("side1.texturemiddle");
				if (tex != "-")
					return tex;
			}

			// Front upper
			if (tex_part & MapLine::Part::FrontUpper)
			{
				tex = l->stringProperty("side1.texturetop");
				if (tex != "-")
					return tex;
			}

			// Front lower
			if (tex_part & MapLine::Part::FrontLower)
			{
				tex = l->stringProperty("side1.texturebottom");
				if (tex != "-")
					return tex;
			}
		}

		if (l->side2_)
		{
			// Back middle
			if (tex_part & MapLine::Part::BackMiddle)
			{
				tex = l->stringProperty("side2.texturemiddle");
				if (tex != "-")
					return tex;
			}

			// Back upper
			if (tex_part & MapLine::Part::BackUpper)
			{
				tex = l->stringProperty("side2.texturetop");
				if (tex != "-")
					return tex;
			}

			// Back lower
			if (tex_part & MapLine::Part::BackLower)
			{
				tex = l->stringProperty("side2.texturebottom");
				if (tex != "-")
					return tex;
			}
		}
	}

	return tex;
}

// -----------------------------------------------------------------------------
// Returns the sector on the front or back side of [line]
// (ignoring the line side itself, used for correcting sector refs)
// -----------------------------------------------------------------------------
MapSector* SLADEMap::lineSideSector(MapLine* line, bool front)
{
	// Get mid and direction points
	auto mid = line->getPoint(MapObject::Point::Mid);
	auto dir = line->frontVector();
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
	int    index    = -1;
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		if (lines_[a] == line)
			continue;

		dist = MathStuff::distanceRayLine(mid, dir, lines_[a]->point1(), lines_[a]->point2());
		if (dist < min_dist && dist > 0)
		{
			min_dist = dist;
			index    = a;
		}
	}

	// If any intersection found, check what side of the intersected line this is on
	// and return the appropriate sector
	if (index >= 0)
	{
		// LOG_MESSAGE(3, "Closest line %d", index);
		auto l = lines_[index];

		// Check side of line
		MapSector* sector = nullptr;
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
			if (builder.edgeLine(a) == line && builder.edgeIsFront(a) == front)
				return l->frontSector();
		}

		// Can't trace back from front side, must be back side
		return l->backSector();
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a list of objects of [type] that have a modified time later than
// [since]
// -----------------------------------------------------------------------------
vector<MapObject*> SLADEMap::modifiedObjects(long since, MapObject::Type type)
{
	vector<MapObject*> modified_objects;

	// Vertices
	if (type == MapObject::Type::Object || type == MapObject::Type::Vertex)
	{
		for (auto& vertex : vertices_)
		{
			if (vertex->modifiedTime() >= since)
				modified_objects.push_back(vertex);
		}
	}

	// Sides
	if (type == MapObject::Type::Object || type == MapObject::Type::Side)
	{
		for (auto& side : sides_)
		{
			if (side->modifiedTime() >= since)
				modified_objects.push_back(side);
		}
	}

	// Lines
	if (type == MapObject::Type::Object || type == MapObject::Type::Line)
	{
		for (auto& line : lines_)
		{
			if (line->modifiedTime() >= since)
				modified_objects.push_back(line);
		}
	}

	// Sectors
	if (type == MapObject::Type::Object || type == MapObject::Type::Sector)
	{
		for (auto& sector : sectors_)
		{
			if (sector->modifiedTime() >= since)
				modified_objects.push_back(sector);
		}
	}

	// Things
	if (type == MapObject::Type::Object || type == MapObject::Type::Thing)
	{
		for (auto& thing : things_)
		{
			if (thing->modifiedTime() >= since)
				modified_objects.push_back(thing);
		}
	}

	return modified_objects;
}

// -----------------------------------------------------------------------------
// Returns a list of objects that have a modified time later than [since]
// -----------------------------------------------------------------------------
vector<MapObject*> SLADEMap::allModifiedObjects(long since)
{
	vector<MapObject*> modified_objects;

	for (auto& object : all_objects_)
	{
		if (object.mobj && object.mobj->modifiedTime() >= since)
			modified_objects.push_back(object.mobj);
	}

	return modified_objects;
}

// -----------------------------------------------------------------------------
// Returns the newest modified time on any map object
// -----------------------------------------------------------------------------
long SLADEMap::lastModifiedTime()
{
	long mod_time = 0;

	for (auto& object : all_objects_)
	{
		if (object.mobj && object.mobj->modifiedTime() > mod_time)
			mod_time = object.mobj->modifiedTime();
	}

	return mod_time;
}

// -----------------------------------------------------------------------------
// Returns true if any map object has been modified since it was opened or last
// saved
// -----------------------------------------------------------------------------
bool SLADEMap::isModified()
{
	return lastModifiedTime() > opened_time_;
}

// -----------------------------------------------------------------------------
// Sets the map opened time to now
// -----------------------------------------------------------------------------
void SLADEMap::setOpenedTime()
{
	opened_time_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Returns true if any objects of [type] have a modified time newer than [since]
// -----------------------------------------------------------------------------
bool SLADEMap::modifiedSince(long since, MapObject::Type type)
{
	// Any type
	if (type == MapObject::Type::Object)
		return lastModifiedTime() > since;

	// Vertices
	else if (type == MapObject::Type::Vertex)
	{
		for (auto& vertex : vertices_)
		{
			if (vertex->modified_time_ > since)
				return true;
		}
	}

	// Lines
	else if (type == MapObject::Type::Line)
	{
		for (auto& line : lines_)
		{
			if (line->modified_time_ > since)
				return true;
		}
	}

	// Sides
	else if (type == MapObject::Type::Side)
	{
		for (auto& side : sides_)
		{
			if (side->modified_time_ > since)
				return true;
		}
	}

	// Sectors
	else if (type == MapObject::Type::Sector)
	{
		for (auto& sector : sectors_)
		{
			if (sector->modified_time_ > since)
				return true;
		}
	}

	// Things
	else if (type == MapObject::Type::Thing)
	{
		for (auto& thing : things_)
		{
			if (thing->modified_time_ > since)
				return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Re-applies all the currently calculated special map properties (currently
// this just means ZDoom slopes).
// Since this needs to be done anytime the map changes, it's called whenever a
// map is read, an undo record ends, or an undo/redo is performed.
// -----------------------------------------------------------------------------
void SLADEMap::recomputeSpecials()
{
	map_specials_.processMapSpecials(this);
}

// -----------------------------------------------------------------------------
// Creates a new vertex at [x,y] and returns it.
// Splits any lines within [split_dist] from the position
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::createVertex(double x, double y, double split_dist)
{
	// Round position to integral if fractional positions are disabled
	if (!position_frac_)
	{
		x = MathStuff::round(x);
		y = MathStuff::round(y);
	}

	Vec2f point(x, y);

	// First check that it won't overlap any other vertex
	for (auto& vertex : vertices_)
	{
		if (vertex->position_.x == x && vertex->position_.y == y)
			return vertex;
	}

	// Create the vertex
	auto nv    = new MapVertex(x, y, this);
	nv->index_ = vertices_.size();
	vertices_.push_back(nv);

	// Check if this vertex splits any lines (if needed)
	if (split_dist >= 0)
	{
		unsigned nlines = lines_.size();
		for (unsigned a = 0; a < nlines; a++)
		{
			// Skip line if it shares the vertex
			if (lines_[a]->v1() == nv || lines_[a]->v2() == nv)
				continue;

			if (lines_[a]->distanceTo(point) < split_dist)
			{
				// LOG_MESSAGE(1, "Vertex at (%1.2f,%1.2f) splits line %d", x, y, a);
				splitLine(lines_[a], nv);
			}
		}
	}

	// Set geometry age
	geometry_updated_ = App::runTimer();

	return nv;
}

// -----------------------------------------------------------------------------
// Creates a new line and needed vertices from [x1,y1] to [x2,y2] and returns it
// -----------------------------------------------------------------------------
MapLine* SLADEMap::createLine(double x1, double y1, double x2, double y2, double split_dist)
{
	// Round coordinates to integral if fractional positions are disabled
	if (!position_frac_)
	{
		x1 = MathStuff::round(x1);
		y1 = MathStuff::round(y1);
		x2 = MathStuff::round(x2);
		y2 = MathStuff::round(y2);
	}

	// LOG_MESSAGE(1, "Create line (%1.2f,%1.2f) to (%1.2f,%1.2f)", x1, y1, x2, y2);

	// Get vertices at points
	auto vertex1 = vertexAt(x1, y1);
	auto vertex2 = vertexAt(x2, y2);

	// Create vertices if required
	if (!vertex1)
		vertex1 = createVertex(x1, y1, split_dist);
	if (!vertex2)
		vertex2 = createVertex(x2, y2, split_dist);

	// Create line between vertices
	return createLine(vertex1, vertex2);
}

// -----------------------------------------------------------------------------
// Creates a new line between [vertex1] and [vertex2] and returns it
// -----------------------------------------------------------------------------
MapLine* SLADEMap::createLine(MapVertex* vertex1, MapVertex* vertex2, bool force)
{
	// Check both vertices were given
	if (!vertex1 || vertex1->parent_map_ != this)
		return nullptr;
	if (!vertex2 || vertex2->parent_map_ != this)
		return nullptr;

	// Check if there is already a line along the two given vertices
	if (!force)
	{
		for (auto& line : lines_)
		{
			if ((line->vertex1_ == vertex1 && line->vertex2_ == vertex2)
				|| (line->vertex2_ == vertex1 && line->vertex1_ == vertex2))
				return line;
		}
	}

	// Create new line between vertices
	auto nl    = new MapLine(vertex1, vertex2, nullptr, nullptr, this);
	nl->index_ = lines_.size();
	lines_.push_back(nl);

	// Connect line to vertices
	vertex1->connectLine(nl);
	vertex2->connectLine(nl);

	// Set geometry age
	geometry_updated_ = App::runTimer();

	return nl;
}

// -----------------------------------------------------------------------------
// Creates a new thing at [x,y] and returns it
// -----------------------------------------------------------------------------
MapThing* SLADEMap::createThing(double x, double y)
{
	// Create the thing
	auto nt = new MapThing(this);

	// Setup initial values
	nt->position_.x = x;
	nt->position_.y = y;
	nt->index_      = things_.size();
	nt->type_       = 1;

	// Add to things
	things_.push_back(nt);
	things_updated_ = App::runTimer();

	return nt;
}

// -----------------------------------------------------------------------------
// Creates a new sector and returns it
// -----------------------------------------------------------------------------
MapSector* SLADEMap::createSector()
{
	// Create the sector
	auto ns = new MapSector(this);

	// Setup initial values
	ns->index_ = sectors_.size();

	// Add to sectors
	sectors_.push_back(ns);

	return ns;
}

// -----------------------------------------------------------------------------
// Creates a new side and returns it
// -----------------------------------------------------------------------------
MapSide* SLADEMap::createSide(MapSector* sector)
{
	// Check sector
	if (!sector)
		return nullptr;

	// Create side
	auto side = new MapSide(sector, this);

	// Setup initial values
	side->index_      = sides_.size();
	side->tex_middle_ = "-";
	side->tex_upper_  = "-";
	side->tex_lower_  = "-";
	usage_tex_["-"] += 3;

	// Add to sides
	sides_.push_back(side);

	return side;
}

// -----------------------------------------------------------------------------
// Moves [vertex] to new position [nx,ny]
// -----------------------------------------------------------------------------
void SLADEMap::moveVertex(unsigned vertex, double nx, double ny)
{
	// Check index
	if (vertex >= vertices_.size())
		return;

	// Move the vertex
	auto v = vertices_[vertex];
	v->setModified();
	v->position_.x = nx;
	v->position_.y = ny;

	// Reset all attached lines' geometry info
	for (auto& connected_line : v->connected_lines_)
		connected_line->resetInternals();

	geometry_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Merges vertices at index [vertex1] and [vertex2], removing the second
// -----------------------------------------------------------------------------
void SLADEMap::mergeVertices(unsigned vertex1, unsigned vertex2)
{
	// Check indices
	if (vertex1 >= vertices_.size() || vertex2 >= vertices_.size() || vertex1 == vertex2)
		return;

	// Go through lines of second vertex
	auto             v1 = vertices_[vertex1];
	auto             v2 = vertices_[vertex2];
	vector<MapLine*> zlines;
	for (unsigned a = 0; a < v2->connected_lines_.size(); a++)
	{
		auto line = v2->connected_lines_[a];

		// Change first vertex if needed
		if (line->vertex1_ == v2)
		{
			line->setModified();
			line->vertex1_ = v1;
			line->length_  = -1;
			v1->connectLine(line);
		}

		// Change second vertex if needed
		if (line->vertex2_ == v2)
		{
			line->setModified();
			line->vertex2_ = v1;
			line->length_  = -1;
			v1->connectLine(line);
		}

		if (line->vertex1_ == v1 && line->vertex2_ == v1)
			zlines.push_back(line);
	}

	// Delete the vertex
	LOG_MESSAGE(4, "Merging vertices %u and %u (removing %u)", vertex1, vertex2, vertex2);
	removeMapObject(v2);
	vertices_[vertex2]         = vertices_.back();
	vertices_[vertex2]->index_ = vertex2;
	vertices_.pop_back();

	// Delete any resulting zero-length lines
	for (auto& zline : zlines)
	{
		LOG_MESSAGE(4, "Removing zero-length line %u", zline->index());
		removeLine(zline);
	}

	geometry_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Merges all vertices at [x,1] and returns the resulting single vertex
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::mergeVerticesPoint(double x, double y)
{
	// Go through all vertices
	int merge = -1;
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		// Skip if vertex isn't on the point
		if (vertices_[a]->position_.x != x || vertices_[a]->position_.y != y)
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

	geometry_updated_ = App::runTimer();

	// Return the final merged vertex
	return vertex(merge);
}

// -----------------------------------------------------------------------------
// Splits [line] at [vertex]
// -----------------------------------------------------------------------------
MapLine* SLADEMap::splitLine(MapLine* line, MapVertex* vertex)
{
	if (!line || !vertex)
		return nullptr;

	// Shorten line
	auto v2 = line->vertex2_;
	line->setModified();
	v2->disconnectLine(line);
	line->vertex2_ = vertex;
	vertex->connectLine(line);
	line->length_ = -1;

	// Create and add new sides
	MapSide* s1 = nullptr;
	MapSide* s2 = nullptr;
	if (line->side1_)
	{
		// Create side 1
		s1 = new MapSide(this);
		s1->copy(line->side1_);
		s1->setSector(line->side1_->sector_);
		if (s1->sector_)
		{
			s1->sector_->resetBBox();
			s1->sector_->resetPolygon();
		}

		// Add side
		s1->index_ = sides_.size();
		sides_.push_back(s1);

		// Update texture counts
		usage_tex_[s1->tex_upper_.Upper()] += 1;
		usage_tex_[s1->tex_middle_.Upper()] += 1;
		usage_tex_[s1->tex_lower_.Upper()] += 1;
	}
	if (line->side2_)
	{
		// Create side 2
		s2 = new MapSide(this);
		s2->copy(line->side2_);
		s2->setSector(line->side2_->sector_);
		if (s2->sector_)
		{
			s2->sector_->resetBBox();
			s2->sector_->resetPolygon();
		}

		// Add side
		s2->index_ = sides_.size();
		sides_.push_back(s2);

		// Update texture counts
		usage_tex_[s2->tex_upper_.Upper()] += 1;
		usage_tex_[s2->tex_middle_.Upper()] += 1;
		usage_tex_[s2->tex_lower_.Upper()] += 1;
	}

	// Create and add new line
	auto nl = new MapLine(vertex, v2, s1, s2, this);
	nl->copy(line);
	nl->index_ = lines_.size();
	nl->setModified();
	lines_.push_back(nl);

	// Update x-offsets
	if (map_split_auto_offset)
	{
		int xoff1 = line->intProperty("side1.offsetx");
		int xoff2 = line->intProperty("side2.offsetx");
		nl->setIntProperty("side1.offsetx", xoff1 + line->length());
		line->setIntProperty("side2.offsetx", xoff2 + nl->length());
	}

	geometry_updated_ = App::runTimer();

	return nl;
}

// -----------------------------------------------------------------------------
// Moves the thing at index [thing] to new position [nx,ny]
// -----------------------------------------------------------------------------
void SLADEMap::moveThing(unsigned thing, double nx, double ny)
{
	// Check index
	if (thing >= things_.size())
		return;

	// Move the thing
	auto t = things_[thing];
	t->setModified();
	t->position_.x = nx;
	t->position_.y = ny;
}

// -----------------------------------------------------------------------------
// Splits any lines withing [split_dist] from [vertex]
// -----------------------------------------------------------------------------
void SLADEMap::splitLinesAt(MapVertex* vertex, double split_dist)
{
	// Check if this vertex splits any lines (if needed)
	unsigned nlines = lines_.size();
	for (unsigned a = 0; a < nlines; a++)
	{
		// Skip line if it shares the vertex
		if (lines_[a]->v1() == vertex || lines_[a]->v2() == vertex)
			continue;

		if (lines_[a]->distanceTo(vertex->position()) < split_dist)
		{
			LOG_MESSAGE(2, "Vertex at (%1.2f,%1.2f) splits line %u", vertex->position_.x, vertex->position_.y, a);
			splitLine(lines_[a], vertex);
		}
	}
}

// -----------------------------------------------------------------------------
// Sets the front or back side of the line at index [line] to be part of
// [sector]. Returns true if a new side was created
// -----------------------------------------------------------------------------
bool SLADEMap::setLineSector(unsigned line, unsigned sector, bool front)
{
	// Check indices
	if (line >= lines_.size() || sector >= sectors_.size())
		return false;

	// Get the MapSide to set
	MapSide* side = nullptr;
	if (front)
		side = lines_[line]->side1_;
	else
		side = lines_[line]->side2_;

	// Do nothing if already the same sector
	if (side && side->sector_ == sectors_[sector])
		return false;

	// Create side if needed
	if (!side)
	{
		side = createSide(sectors_[sector]);

		// Add to line
		lines_[line]->setModified();
		side->parent_ = lines_[line];
		if (front)
			lines_[line]->side1_ = side;
		else
			lines_[line]->side2_ = side;

		// Set appropriate line flags
		bool twosided = (lines_[line]->side1_ && lines_[line]->side2_);
		Game::configuration().setLineBasicFlag("blocking", lines_[line], current_format_, !twosided);
		Game::configuration().setLineBasicFlag("twosided", lines_[line], current_format_, twosided);

		// Invalidate sector polygon
		sectors_[sector]->resetPolygon();
		setGeometryUpdated();

		return true;
	}
	else
	{
		// Set the side's sector
		side->setSector(sectors_[sector]);

		return false;
	}
}

// -----------------------------------------------------------------------------
// Not used
// -----------------------------------------------------------------------------
void SLADEMap::splitLinesByLine(MapLine* split_line)
{
	Vec2f intersection;
	auto  split_segment = split_line->seg();

	for (auto& line : lines_)
	{
		if (line == split_line)
			continue;

		if (MathStuff::linesIntersect(split_segment, line->seg(), intersection))
		{
			auto v = createVertex(intersection.x, intersection.y, 0.9);
			// splitLine(lines[a], v);
		}
	}
}

// -----------------------------------------------------------------------------
// Removes any lines overlapping the line at index [line].
// Returns the number of lines removed
// -----------------------------------------------------------------------------
int SLADEMap::mergeLine(unsigned line)
{
	// Check index
	if (line >= lines_.size())
		return 0;

	auto ml = lines_[line];
	auto v1 = lines_[line]->vertex1_;
	auto v2 = lines_[line]->vertex2_;

	// Go through lines connected to first vertex
	int merged = 0;
	for (unsigned a = 0; a < v1->connected_lines_.size(); a++)
	{
		auto l = v1->connected_lines_[a];
		if (l == ml)
			continue;

		// Check overlap
		if ((l->vertex1_ == v1 && l->vertex2_ == v2) || (l->vertex2_ == v1 && l->vertex1_ == v2))
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

// -----------------------------------------------------------------------------
// Attempts to set [line]'s side sector references to the correct sectors.
// Returns true if any side sector was changed
// -----------------------------------------------------------------------------
bool SLADEMap::correctLineSectors(MapLine* line)
{
	bool changed    = false;
	auto s1_current = line->side1_ ? line->side1_->sector_ : nullptr;
	auto s2_current = line->side2_ ? line->side2_->sector_ : nullptr;

	// Front side
	auto s1 = lineSideSector(line, true);
	if (s1 != s1_current)
	{
		if (s1)
			setLineSector(line->index_, s1->index_, true);
		else if (line->side1_)
			removeSide(line->side1_);
		changed = true;
	}

	// Back side
	auto s2 = lineSideSector(line, false);
	if (s2 != s2_current)
	{
		if (s2)
			setLineSector(line->index_, s2->index_, false);
		else if (line->side2_)
			removeSide(line->side2_);
		changed = true;
	}

	// Flip if needed
	if (changed && !line->side1_ && line->side2_)
		line->flip();

	return changed;
}

// -----------------------------------------------------------------------------
// Sets [line]'s front or back [side] (depending on [front]).
// If [side] already belongs to another line, use a copy of it instead
// -----------------------------------------------------------------------------
void SLADEMap::setLineSide(MapLine* line, MapSide* side, bool front)
{
	// Remove current side
	auto side_current = front ? line->side1_ : line->side2_;
	if (side_current == side)
		return;
	if (side_current)
		removeSide(side_current);

	// If the new side is already part of another line, copy it
	if (side->parent_)
	{
		auto new_side = createSide(side->sector_);
		new_side->copy(side);
		side = new_side;
	}

	// Set side
	if (front)
		line->side1_ = side;
	else
		line->side2_ = side;
	side->parent_ = line;
}

// -----------------------------------------------------------------------------
// Merges any map architecture (lines and vertices) connected to vertices in
// [vertices]
// -----------------------------------------------------------------------------
bool SLADEMap::mergeArch(vector<MapVertex*> vertices)
{
	// Check any map architecture exists
	if (nVertices() == 0 || nLines() == 0)
		return false;

	unsigned n_vertices  = nVertices();
	unsigned n_lines     = lines_.size();
	auto     last_vertex = this->vertices_.back();
	auto     last_line   = lines_.back();

	// Merge vertices
	vector<MapVertex*> merged_vertices;
	for (auto& vertex : vertices)
	{
		auto v = mergeVerticesPoint(vertex->position_.x, vertex->position_.y);
		if (v)
			VECTOR_ADD_UNIQUE(merged_vertices, v);
	}

	// Get all connected lines
	vector<MapLine*> connected_lines_;
	for (auto& vertex : merged_vertices)
	{
		for (auto connected_line : vertex->connected_lines_)
			VECTOR_ADD_UNIQUE(connected_lines_, connected_line);
	}

	// Split lines (by vertices)
	const double split_dist = 0.1;
	// Split existing lines that vertices moved onto
	for (auto& merged_vertice : merged_vertices)
		splitLinesAt(merged_vertice, split_dist);

	// Split lines that moved onto existing vertices
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		unsigned nvertices = this->vertices_.size();
		for (unsigned b = 0; b < nvertices; b++)
		{
			auto vertex = this->vertices_[b];

			// Skip line if it shares the vertex
			if (connected_lines_[a]->v1() == vertex || connected_lines_[a]->v2() == vertex)
				continue;

			if (connected_lines_[a]->distanceTo(vertex->position()) < split_dist)
			{
				connected_lines_.push_back(splitLine(connected_lines_[a], vertex));
				VECTOR_ADD_UNIQUE(merged_vertices, vertex);
			}
		}
	}

	// Split lines (by lines)
	Seg2f seg1;
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		auto line1 = connected_lines_[a];
		seg1       = line1->seg();

		unsigned n_lines = lines_.size();
		for (unsigned b = 0; b < n_lines; b++)
		{
			auto line2 = lines_[b];

			// Can't intersect if they share a vertex
			if (line1->vertex1_ == line2->vertex1_ || line1->vertex1_ == line2->vertex2_
				|| line2->vertex1_ == line1->vertex2_ || line2->vertex2_ == line1->vertex2_)
				continue;

			// Check for intersection
			Vec2f intersection;
			if (MathStuff::linesIntersect(seg1, line2->seg(), intersection))
			{
				// Create split vertex
				auto nv = createVertex(intersection.x, intersection.y);
				merged_vertices.push_back(nv);

				// Split lines
				splitLine(line1, nv);
				connected_lines_.push_back(lines_.back());
				splitLine(line2, nv);
				connected_lines_.push_back(lines_.back());

				LOG_DEBUG("Lines", line1, "and", line2, "intersect");

				a--;
				break;
			}
		}
	}

	// Refresh connected lines
	connected_lines_.clear();
	for (auto& vertex : merged_vertices)
	{
		for (auto connected_line : vertex->connected_lines_)
			VECTOR_ADD_UNIQUE(connected_lines_, connected_line);
	}

	// Find overlapping lines
	vector<MapLine*> remove_lines;
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		auto line1 = connected_lines_[a];

		// Skip if removing already
		if (VECTOR_EXISTS(remove_lines, line1))
			continue;

		for (unsigned l = a + 1; l < connected_lines_.size(); l++)
		{
			auto line2 = connected_lines_[l];

			// Skip if removing already
			if (VECTOR_EXISTS(remove_lines, line2))
				continue;

			if ((line1->vertex1_ == line2->vertex1_ && line1->vertex2_ == line2->vertex2_)
				|| (line1->vertex1_ == line2->vertex2_ && line1->vertex2_ == line2->vertex1_))
			{
				auto remove_line = mergeOverlappingLines(line2, line1);
				VECTOR_ADD_UNIQUE(remove_lines, remove_line);

				// Don't check against any more lines if we just decided to remove this one
				if (remove_line == line1)
					break;
			}
		}
	}

	// Remove overlapping lines
	for (auto& remove_line : remove_lines)
	{
		LOG_MESSAGE(4, "Removing overlapping line %u (#%u)", remove_line->objId(), remove_line->index());
		removeLine(remove_line);
	}
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		if (VECTOR_EXISTS(remove_lines, connected_lines_[a]))
		{
			connected_lines_[a] = connected_lines_.back();
			connected_lines_.pop_back();
			a--;
		}
	}

	// Check if anything was actually merged
	bool merged = false;
	if (nVertices() != n_vertices || lines_.size() != n_lines)
		merged = true;
	if (this->vertices_.back() != last_vertex || lines_.back() != last_line)
		merged = true;
	if (!remove_lines.empty())
		merged = true;

	// Correct sector references
	correctSectors(connected_lines_, true);
	/*if (merged)
		correctSectors(connected_lines_, true);
	else
	{
		for (unsigned a = 0; a < connected_lines_.size(); a++)
		{
			MapSector* s1 = getLineSideSector(connected_lines_[a], true);
			MapSector* s2 = getLineSideSector(connected_lines_[a], false);

			if (s1)
				setLineSector(connected_lines_[a]->index, s1->index, true);
			else
				removeSide(connected_lines_[a]->side1);

			if (s2)
				setLineSector(connected_lines_[a]->index, s2->index, false);
			else
				removeSide(connected_lines_[a]->side2);
		}
	}*/

	// Flip any one-sided lines that only have a side 2
	for (auto& connected_line : connected_lines_)
	{
		if (connected_line->side2_ && !connected_line->side1_)
			connected_line->flip();
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

// -----------------------------------------------------------------------------
// Merges [line1] and [line2], returning the resulting line
// -----------------------------------------------------------------------------
MapLine* SLADEMap::mergeOverlappingLines(MapLine* line1, MapLine* line2)
{
	// Determine which line to remove (prioritise 2s)
	MapLine *remove, *keep;
	if (line1->side2_ && !line2->side2_)
	{
		remove = line1;
		keep   = line2;
	}
	else
	{
		remove = line2;
		keep   = line1;
	}

	// Front-facing overlap
	if (remove->vertex1_ == keep->vertex1_)
	{
		// Set keep front sector to remove front sector
		if (remove->side1_)
			setLineSector(keep->index_, remove->side1_->sector_->index_);
		else
			setLineSector(keep->index_, -1);
	}
	else
	{
		if (remove->side2_)
			setLineSector(keep->index_, remove->side2_->sector_->index_);
		else
			setLineSector(keep->index_, -1);
	}

	return remove;
}

// -----------------------------------------------------------------------------
// Corrects/builds sectors for all lines in [lines]
// -----------------------------------------------------------------------------
void SLADEMap::correctSectors(vector<MapLine*> lines, bool existing_only)
{
	struct Edge
	{
		MapLine* line;
		bool     front;
		bool     ignore;
		Edge(MapLine* line, bool front)
		{
			this->line  = line;
			this->front = front;
			ignore      = false;
		}
	};

	// Create a list of line sides (edges) to perform sector creation with
	vector<Edge> edges;
	for (auto& line : lines)
	{
		if (existing_only)
		{
			// Add only existing sides as edges
			// (or front side if line has none)
			if (line->side1_ || (!line->side1_ && !line->side2_))
				edges.emplace_back(line, true);
			if (line->side2_)
				edges.emplace_back(line, false);
		}
		else
		{
			edges.emplace_back(line, true);
			auto mid = line->getPoint(MapObject::Point::Mid);
			if (sectorAt(mid) >= 0)
				edges.emplace_back(line, false);
		}
	}

	vector<MapSide*> sides_correct;
	for (auto& edge : edges)
	{
		if (edge.front && edge.line->side1_)
			sides_correct.push_back(edge.line->side1_);
		else if (!edge.front && edge.line->side2_)
			sides_correct.push_back(edge.line->side2_);
	}

	// Build sectors
	SectorBuilder      builder;
	int                runs      = 0;
	unsigned           ns_start  = sectors_.size();
	unsigned           nsd_start = sides_.size();
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
		bool           has_existing_lines   = false;
		bool           has_existing_sides   = false;
		bool           has_zero_sided_lines = false;
		vector<size_t> edges_in_sector;
		for (unsigned b = 0; b < builder.nEdges(); b++)
		{
			auto line     = builder.edgeLine(b);
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
		for (auto i : edges_in_sector)
			edges[i].ignore = true;

		// Check if sector traced is already valid
		if (builder.isValidSector())
			continue;

		// Check if we traced over an existing sector (or part of one)
		auto sector = builder.findExistingSector(sides_correct);
		if (sector)
		{
			// Check if it's already been (re)used
			bool reused = false;
			for (auto& sec : sectors_reused)
			{
				if (sec == sector)
				{
					reused = true;
					break;
				}
			}

			// If we can reuse the sector, do so
			if (!reused)
				sectors_reused.push_back(sector);
			else
				sector = nullptr;
		}

		// Create sector
		builder.createSector(sector);
	}

	// Remove any sides that weren't part of a sector
	for (auto& edge : edges)
	{
		if (edge.ignore || !edge.line)
			continue;

		if (edge.front)
			removeSide(edge.line->side1_);
		else
			removeSide(edge.line->side2_);
	}

	// LOG_MESSAGE(1, "Ran sector builder %d times", runs);

	// Check if any lines need to be flipped
	for (auto& line : lines)
	{
		if (line->backSector() && !line->frontSector())
			line->flip(true);
	}

	// Find an adjacent sector to copy properties from
	MapSector* sector_copy = nullptr;
	for (auto& line : lines)
	{
		// Check front sector
		auto sector = line->frontSector();
		if (sector && sector->index() < ns_start)
		{
			// Copy this sector if it isn't newly created
			sector_copy = sector;
			break;
		}

		// Check back sector
		sector = line->backSector();
		if (sector && sector->index() < ns_start)
		{
			// Copy this sector if it isn't newly created
			sector_copy = sector;
			break;
		}
	}

	// Go through newly created sectors
	for (unsigned a = ns_start; a < sectors_.size(); a++)
	{
		// Skip if sector already has properties
		if (!sectors_[a]->ceiling_.texture.IsEmpty())
			continue;

		// Copy from adjacent sector if any
		if (sector_copy)
		{
			sectors_[a]->copy(sector_copy);
			continue;
		}

		// Otherwise, use defaults from game configuration
		Game::configuration().applyDefaults(sectors_[a], current_format_ == MapFormat::UDMF);
	}

	// Update line textures
	for (unsigned a = nsd_start; a < sides_.size(); a++)
	{
		// Clear any unneeded textures
		auto line = sides_[a]->parentLine();
		line->clearUnneededTextures();

		// Set middle texture if needed
		if (sides_[a] == line->s1() && !line->s2() && sides_[a]->stringProperty("texturemiddle") == "-")
		{
			// LOG_MESSAGE(1, "midtex");
			// Find adjacent texture (any)
			string tex = adjacentLineTexture(line->v1());
			if (tex == "-")
				tex = adjacentLineTexture(line->v2());

			// If no adjacent texture, get default from game configuration
			if (tex == "-")
				tex = Game::configuration().defaultString(MapObject::Type::Side, "texturemiddle");

			// Set texture
			sides_[a]->setStringProperty("texturemiddle", tex);
		}
	}

	// Remove any extra sectors
	removeDetachedSectors();
}

// -----------------------------------------------------------------------------
// Performs checks for when a map is first opened
// -----------------------------------------------------------------------------
void SLADEMap::mapOpenChecks()
{
	int rverts  = removeDetachedVertices();
	int rsides  = removeDetachedSides();
	int rsec    = removeDetachedSectors();
	int risides = removeInvalidSides();

	LOG_MESSAGE(
		1,
		"Removed %d detached vertices, %d detached sides, %d invalid sides and %d detached sectors",
		rverts,
		rsides,
		risides,
		rsec);
}

// -----------------------------------------------------------------------------
// Removes any vertices not attached to any lines. Returns the number of
// vertices removed
// -----------------------------------------------------------------------------
int SLADEMap::removeDetachedVertices()
{
	int count = 0;
	for (int a = vertices_.size() - 1; a >= 0; a--)
	{
		if (vertices_[a]->nConnectedLines() == 0)
		{
			removeVertex(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sides that have no parent line.
// Returns the number of sides removed
// -----------------------------------------------------------------------------
int SLADEMap::removeDetachedSides()
{
	int count = 0;
	for (int a = sides_.size() - 1; a >= 0; a--)
	{
		if (!sides_[a]->parent_)
		{
			removeSide(a, false);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sectors that are not referenced by any sides.
// Returns the number of sectors removed
// -----------------------------------------------------------------------------
int SLADEMap::removeDetachedSectors()
{
	int count = 0;
	for (int a = sectors_.size() - 1; a >= 0; a--)
	{
		if (sectors_[a]->connectedSides().empty())
		{
			removeSector(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any lines that have identical first and second vertices.
// Returns the number of lines removed
// -----------------------------------------------------------------------------
int SLADEMap::removeZeroLengthLines()
{
	int count = 0;
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		if (lines_[a]->vertex1_ == lines_[a]->vertex2_)
		{
			removeLine(a);
			a--;
			count++;
		}
	}

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sides that reference non-existant sectors
// -----------------------------------------------------------------------------
int SLADEMap::removeInvalidSides()
{
	int count = 0;
	for (unsigned a = 0; a < sides_.size(); a++)
	{
		if (!sides_[a]->sector())
		{
			removeSide(a);
			a--;
			count++;
		}
	}

	return count;
}

// -----------------------------------------------------------------------------
// Converts the map to hexen format (not implemented)
// -----------------------------------------------------------------------------
bool SLADEMap::convertToHexen() const
{
	// Already hexen format
	return current_format_ == MapFormat::Hexen;
}

// -----------------------------------------------------------------------------
// Converts the map to UDMF format (not implemented)
// -----------------------------------------------------------------------------
bool SLADEMap::convertToUDMF()
{
	// Already UDMF format
	if (current_format_ == MapFormat::UDMF)
		return true;

	if (current_format_ == MapFormat::Hexen)
	{
		// Handle special cases for conversion from Hexen format
		for (auto& line : lines_)
		{
			int special = line->intProperty("special");
			int flags   = 0;
			int id, hi;
			switch (special)
			{
			case 1:
				id = line->intProperty("arg3");
				line->setIntProperty("id", id);
				line->setIntProperty("arg3", 0);
				break;

			case 5:
				id = line->intProperty("arg4");
				line->setIntProperty("id", id);
				line->setIntProperty("arg4", 0);
				break;

			case 121:
				id    = line->intProperty("arg0");
				hi    = line->intProperty("arg4");
				id    = (hi * 256) + id;
				flags = line->intProperty("arg1");

				line->setIntProperty("special", 0);
				line->setIntProperty("id", id);
				line->setIntProperty("arg0", 0);
				line->setIntProperty("arg1", 0);
				line->setIntProperty("arg2", 0);
				line->setIntProperty("arg3", 0);
				line->setIntProperty("arg4", 0);
				break;

			case 160:
				hi = id = line->intProperty("arg4");
				flags   = line->intProperty("arg1");
				if (flags & 8)
				{
					line->setIntProperty("id", id);
				}
				else
				{
					id = line->intProperty("arg0");
					line->setIntProperty("id", (hi * 256) + id);
				}
				line->setIntProperty("arg4", 0);
				flags = 0; // don't keep it set!
				break;

			case 181:
				id = line->intProperty("arg2");
				line->setIntProperty("id", id);
				line->setIntProperty("arg2", 0);
				break;

			case 208:
				id    = line->intProperty("arg0");
				flags = line->intProperty("arg3");

				line->setIntProperty("id", id); // arg0 must be preserved
				line->setIntProperty("arg3", 0);
				break;

			case 215:
				id = line->intProperty("arg0");
				line->setIntProperty("id", id);
				line->setIntProperty("arg0", 0);
				break;

			case 222:
				id = line->intProperty("arg0");
				line->setIntProperty("id", id); // arg0 must be preserved
				break;
			default: break;
			}

			// flags (only set by 121 and 208)
			if (flags & 1)
				line->setBoolProperty("zoneboundary", true);
			if (flags & 2)
				line->setBoolProperty("jumpover", true);
			if (flags & 4)
				line->setBoolProperty("blockfloaters", true);
			if (flags & 8)
				line->setBoolProperty("clipmidtex", true);
			if (flags & 16)
				line->setBoolProperty("wrapmidtex", true);
			if (flags & 32)
				line->setBoolProperty("midtex3d", true);
			if (flags & 64)
				line->setBoolProperty("checkswitchrange", true);
		}
	}
	else
		return false;

	// Set format
	current_format_ = MapFormat::UDMF;
	return true;
}

// -----------------------------------------------------------------------------
// Rebuilds the connected lines lists for all map vertices
// -----------------------------------------------------------------------------
void SLADEMap::rebuildConnectedLines()
{
	// Clear vertex connected lines lists
	for (auto& vertex : vertices_)
		vertex->connected_lines_.clear();

	// Connect lines to their vertices
	for (auto& line : lines_)
	{
		line->vertex1_->connected_lines_.push_back(line);
		line->vertex2_->connected_lines_.push_back(line);
	}
}

// -----------------------------------------------------------------------------
// Rebuilds the connected sides lists for all map sectors
// -----------------------------------------------------------------------------
void SLADEMap::rebuildConnectedSides()
{
	// Clear sector connected sides lists
	for (auto& sector : sectors_)
		sector->connected_sides_.clear();

	// Connect sides to their sectors
	for (auto& side : sides_)
	{
		if (side->sector_)
			side->sector_->connected_sides_.push_back(side);
	}
}

// -----------------------------------------------------------------------------
// Adjusts the usage count for texture [tex] by [adjust]
// -----------------------------------------------------------------------------
void SLADEMap::updateTexUsage(const string& tex, int adjust)
{
	usage_tex_[tex.Upper()] += adjust;
}

// -----------------------------------------------------------------------------
// Adjusts the usage count for flat [flat] by [adjust]
// -----------------------------------------------------------------------------
void SLADEMap::updateFlatUsage(const string& flat, int adjust)
{
	usage_flat_[flat.Upper()] += adjust;
}

// -----------------------------------------------------------------------------
// Adjusts the usage count for thing type [type] by [adjust]
// -----------------------------------------------------------------------------
void SLADEMap::updateThingTypeUsage(int type, int adjust)
{
	usage_thing_type_[type] += adjust;
}

// -----------------------------------------------------------------------------
// Returns the usage count for the texture [tex]
// -----------------------------------------------------------------------------
int SLADEMap::texUsageCount(const string& tex)
{
	return usage_tex_[tex.Upper()];
}

// -----------------------------------------------------------------------------
// Returns the usage count for the flat [tex]
// -----------------------------------------------------------------------------
int SLADEMap::flatUsageCount(const string& tex)
{
	return usage_flat_[tex.Upper()];
}

// -----------------------------------------------------------------------------
// Returns the usage count for the thing type [type]
// -----------------------------------------------------------------------------
int SLADEMap::thingTypeUsageCount(int type)
{
	return usage_thing_type_[type];
}
