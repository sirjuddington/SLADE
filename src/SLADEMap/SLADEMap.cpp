
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#include "Archive/Formats/WadArchive.h"
#include "Game/Configuration.h"
#include "MapEditor/SectorBuilder.h"
#include "MapFormat/MapFormatHandler.h"
#include "Utility/MathStuff.h"


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
SLADEMap::SLADEMap() : data_{ this }
{
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
// Reads map data using info in [map]
// -----------------------------------------------------------------------------
bool SLADEMap::readMap(const Archive::MapDesc& map)
{
	auto omap = &map;

	// Check for map archive
	WadArchive tempwad;
	auto       m_head = map.head.lock();
	if (map.archive && m_head)
	{
		tempwad.open(m_head->data());
		auto amaps = tempwad.detectMaps();
		if (!amaps.empty())
			omap = &amaps[0];
		else
			return false;
	}

	bool ok = false;
	if (omap->head.lock())
	{
		auto map_handler = MapFormatHandler::get(omap->format);
		ok               = map_handler->readMap(*omap, data_, udmf_props_);
		udmf_namespace_  = map_handler->udmfNamespace();
	}
	else
		ok = true;

	// Copy extra entries
	for (auto& entry : map.unk)
		udmf_extra_entries_.push_back(new ArchiveEntry(*entry));

	// Set map info
	name_ = map.name;

	// Set map format
	if (ok)
	{
		// Update variables
		current_format_   = map.format;
		geometry_updated_ = App::runTimer();

		// When creating a new map, retrieve UDMF namespace information from the configuration
		if (map.format == MapFormat::UDMF && udmf_namespace_.empty())
			udmf_namespace_ = Game::configuration().udmfNamespace();
	}

	mapOpenChecks();

	data_.sectors().initBBoxes();
	data_.sectors().initPolygons();
	recomputeSpecials();

	opened_time_ = App::runTimer() + 10;

	return ok;
}

// -----------------------------------------------------------------------------
// Clears all map data
// -----------------------------------------------------------------------------
void SLADEMap::clearMap()
{
	map_specials_.reset();

	// Clear map objects
	data_.clear();

	// Clear usage counts
	usage_thing_type_.clear();

	// Clear UDMF extra entries
	for (auto& entry : udmf_extra_entries_)
		delete entry;
	udmf_extra_entries_.clear();
}

// -----------------------------------------------------------------------------
// Returns a bounding box for the entire map.
// If [include_things] is true, the bounding box will include things, otherwise
// it will be for sectors (vertices) only
// -----------------------------------------------------------------------------
BBox SLADEMap::bounds(bool include_things)
{
	auto bbox = data_.sectors().allSectorBounds();

	if (include_things)
		bbox.extend(data_.things().allThingBounds());

	return bbox;
}

// -----------------------------------------------------------------------------
// Updates geometry info (polygons/bbox/etc) for anything modified since
// [modified_time]
// -----------------------------------------------------------------------------
void SLADEMap::updateGeometryInfo(long modified_time)
{
	for (auto& vertex : data_.vertices())
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

		double dist = MathStuff::distanceRayLine(mid, mid + vec, s_line->start(), s_line->end());

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
// Adds all things with TID [id] that are also within a sector with tag [tag] to
// [list]
// -----------------------------------------------------------------------------
void SLADEMap::putThingsWithIdInSectorTag(int id, int tag, vector<MapThing*>& list)
{
	if (id == 0 && tag == 0)
		return;

	// Find things with matching id contained in sector with matching tag
	for (auto& thing : data_.things())
	{
		if (thing->id() == id)
		{
			auto sector = data_.sectors().atPos(thing->position());
			if (sector && sector->id_ == tag)
				list.push_back(thing);
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
				data_.things().putAllWithId(val, list);
			}
		}
		++i;
	}
}

// -----------------------------------------------------------------------------
// Returns the first texture at [tex_part] found on lines connected to [vertex]
// -----------------------------------------------------------------------------
string SLADEMap::adjacentLineTexture(MapVertex* vertex, int tex_part) const
{
	// Go through adjacent lines
	auto tex = MapSide::TEX_NONE;
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		auto l = vertex->connectedLine(a);

		if (l->side1_)
		{
			// Front middle
			if (tex_part & MapLine::Part::FrontMiddle)
			{
				tex = l->side1_->texMiddle();
				if (tex != MapSide::TEX_NONE)
					return tex;
			}

			// Front upper
			if (tex_part & MapLine::Part::FrontUpper)
			{
				tex = l->side1_->texUpper();
				if (tex != MapSide::TEX_NONE)
					return tex;
			}

			// Front lower
			if (tex_part & MapLine::Part::FrontLower)
			{
				tex = l->side1_->texLower();
				if (tex != MapSide::TEX_NONE)
					return tex;
			}
		}

		if (l->side2_)
		{
			// Back middle
			if (tex_part & MapLine::Part::BackMiddle)
			{
				tex = l->side2_->texMiddle();
				if (tex != MapSide::TEX_NONE)
					return tex;
			}

			// Back upper
			if (tex_part & MapLine::Part::BackUpper)
			{
				tex = l->side2_->texUpper();
				if (tex != MapSide::TEX_NONE)
					return tex;
			}

			// Back lower
			if (tex_part & MapLine::Part::BackLower)
			{
				tex = l->side2_->texLower();
				if (tex != MapSide::TEX_NONE)
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
	double      dist;
	double      min_dist = 99999999;
	int         index    = -1;
	const auto& lines    = this->lines();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a] == line)
			continue;

		dist = MathStuff::distanceRayLine(mid, dir, lines[a]->start(), lines[a]->end());
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
		// Log::info(3, "Closest line %d", index);
		auto l = lines[index];

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
// Returns true if any map object has been modified since it was opened or last
// saved
// -----------------------------------------------------------------------------
bool SLADEMap::isModified() const
{
	return data_.lastModifiedTime() > opened_time_;
}

// -----------------------------------------------------------------------------
// Sets the map opened time to now
// -----------------------------------------------------------------------------
void SLADEMap::setOpenedTime()
{
	opened_time_ = App::runTimer();
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
// Writes the map to [map_entries] in the current format
// -----------------------------------------------------------------------------
bool SLADEMap::writeMap(vector<ArchiveEntry*>& map_entries) const
{
	auto out = MapFormatHandler::get(current_format_)->writeMap(data_, udmf_props_);
	if (out.empty())
		return false;

	// TODO: Make map_entries (and MapEditorWindow::writeMap) use UPtr instead of raw pointers
	for (auto& entry : out)
		map_entries.push_back(entry.release());

	// Add extra entries
	for (const auto& entry : udmf_extra_entries_)
		map_entries.push_back(new ArchiveEntry(*entry));

	return true;
}

// -----------------------------------------------------------------------------
// Creates a new vertex at [x,y] and returns it.
// Splits any lines within [split_dist] from the position
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::createVertex(Vec2d pos, double split_dist)
{
	// Round position to integral if fractional positions are disabled
	if (!position_frac_)
	{
		pos.x = MathStuff::round(pos.x);
		pos.y = MathStuff::round(pos.y);
	}

	// First check that it won't overlap any other vertex
	if (auto overlap = vertices().vertexAt(pos.x, pos.y))
		return overlap;

	// Create the vertex
	auto nv = data_.addVertex(std::make_unique<MapVertex>(pos));

	// Check if this vertex splits any lines (if needed)
	if (split_dist >= 0)
	{
		auto lines = data_.lines();
		for (auto line : lines)
		{
			// Skip line if it shares the vertex
			if (line->v1() == nv || line->v2() == nv)
				continue;

			if (line->distanceTo(pos) < split_dist)
			{
				Log::debug("Vertex at ({:1.2f},{:1.2f}) splits line {}", pos.x, pos.y, line->index());
				splitLine(line, nv);
			}
		}
	}

	// Set geometry age
	geometry_updated_ = App::runTimer();

	return nv;
}

// -----------------------------------------------------------------------------
// Creates a new line and needed vertices from [p1] to [p2] and returns it
// -----------------------------------------------------------------------------
MapLine* SLADEMap::createLine(Vec2d p1, Vec2d p2, double split_dist)
{
	// Round coordinates to integral if fractional positions are disabled
	if (!position_frac_)
	{
		p1.x = MathStuff::round(p1.x);
		p1.y = MathStuff::round(p1.y);
		p2.x = MathStuff::round(p2.x);
		p2.y = MathStuff::round(p2.y);
	}

	// Log::info(1, "Create line (%1.2f,%1.2f) to (%1.2f,%1.2f)", p1.x, p1.y, p2.x, p2.y);

	// Get vertices at points
	auto vertex1 = vertices().vertexAt(p1.x, p1.y);
	auto vertex2 = vertices().vertexAt(p2.x, p2.y);

	// Create vertices if required
	if (!vertex1)
		vertex1 = createVertex(p1, split_dist);
	if (!vertex2)
		vertex2 = createVertex(p2, split_dist);

	// Create line between vertices
	return createLine(vertex1, vertex2);
}

// -----------------------------------------------------------------------------
// Creates a new line between [vertex1] and [vertex2] and returns it.
// If [force] is false and another line exists with the given vertices, returns
// that line instead of creating one
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
		if (auto existing = lines().withVertices(vertex1, vertex2))
			return existing;

	// Create new line between vertices
	auto nl = data_.addLine(std::make_unique<MapLine>(vertex1, vertex2, nullptr, nullptr));

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
MapThing* SLADEMap::createThing(Vec2d pos, int type)
{
	// Create the thing
	return data_.addThing(std::make_unique<MapThing>(pos, type));
}

// -----------------------------------------------------------------------------
// Creates a new sector and returns it
// -----------------------------------------------------------------------------
MapSector* SLADEMap::createSector()
{
	return data_.addSector(std::make_unique<MapSector>());
}

// -----------------------------------------------------------------------------
// Creates a new side and returns it
// -----------------------------------------------------------------------------
MapSide* SLADEMap::createSide(MapSector* sector)
{
	// Check sector
	if (!sector)
		return nullptr;

	return data_.addSide(std::make_unique<MapSide>(sector));
}

// -----------------------------------------------------------------------------
// Merges vertices at index [vertex1] and [vertex2], removing the second
// -----------------------------------------------------------------------------
void SLADEMap::mergeVertices(unsigned vertex1, unsigned vertex2)
{
	// Check indices
	auto v1 = vertex(vertex1);
	auto v2 = vertex(vertex2);
	if (!v1 || !v2 || vertex1 == vertex2)
		return;

	// Go through lines of second vertex
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
	Log::info(4, "Merging vertices {} and {} (removing {})", vertex1, vertex2, vertex2);
	data_.removeVertex(vertex2);

	// Delete any resulting zero-length lines
	for (auto& zline : zlines)
	{
		Log::info(4, "Removing zero-length line {}", zline->index());
		data_.removeLine(zline);
	}

	geometry_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Merges all vertices at [x,1] and returns the resulting single vertex
// -----------------------------------------------------------------------------
MapVertex* SLADEMap::mergeVerticesPoint(const Vec2d& pos)
{
	// Go through all vertices
	int merge = -1;
	for (unsigned a = 0; a < vertices().size(); a++)
	{
		// Skip if vertex isn't on the point
		if (vertex(a)->position_.x != pos.x || vertex(a)->position_.y != pos.y)
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
		s1 = data_.duplicateSide(line->side1_);
		if (s1->sector_)
		{
			s1->sector_->resetBBox();
			s1->sector_->resetPolygon();
		}
	}
	if (line->side2_)
	{
		s2 = data_.duplicateSide(line->side2_);
		if (s2->sector_)
		{
			s2->sector_->resetBBox();
			s2->sector_->resetPolygon();
		}
	}

	// Create and add new line
	auto nl = data_.addLine(std::make_unique<MapLine>(vertex, v2, s1, s2));
	nl->copy(line);
	nl->setModified();

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
// Splits any lines within [split_dist] from [vertex]
// -----------------------------------------------------------------------------
void SLADEMap::splitLinesAt(MapVertex* vertex, double split_dist)
{
	// Check if this vertex splits any lines (if needed)
	auto nlines = data_.lines().size();
	for (unsigned i = 0; i < nlines; ++i)
	{
		auto line = this->line(i);

		// Skip line if it shares the vertex
		if (line->v1() == vertex || line->v2() == vertex)
			continue;

		if (line->distanceTo(vertex->position()) < split_dist)
		{
			Log::info(2, "Vertex at ({:1.2f},{:1.2f}) splits line {}", vertex->position_.x, vertex->position_.y, i);
			splitLine(line, vertex);
		}
	}
}

// -----------------------------------------------------------------------------
// Sets the front or back side of the line at index [line] to be part of
// [sector]. Returns true if a new side was created
// -----------------------------------------------------------------------------
bool SLADEMap::setLineSector(unsigned line_index, unsigned sector_index, bool front)
{
	// Check indices
	auto line   = this->line(line_index);
	auto sector = this->sector(sector_index);
	if (!line || !sector)
		return false;

	// Get the MapSide to set
	MapSide* side = nullptr;
	if (front)
		side = line->side1_;
	else
		side = line->side2_;

	// Do nothing if already the same sector
	if (side && side->sector_ == sector)
		return false;

	// Create side if needed
	if (!side)
	{
		side = createSide(sector);

		// Add to line
		line->setModified();
		side->parent_ = line;
		if (front)
			line->side1_ = side;
		else
			line->side2_ = side;

		// Set appropriate line flags
		bool twosided = (line->side1_ && line->side2_);
		Game::configuration().setLineBasicFlag("blocking", line, current_format_, !twosided);
		Game::configuration().setLineBasicFlag("twosided", line, current_format_, twosided);

		// Invalidate sector polygon
		sector->resetPolygon();
		setGeometryUpdated();

		return true;
	}
	else
	{
		// Set the side's sector
		side->setSector(sector);

		return false;
	}
}

// -----------------------------------------------------------------------------
// Removes any lines overlapping the line at [index].
// Returns the number of lines removed
// -----------------------------------------------------------------------------
int SLADEMap::mergeLine(unsigned index)
{
	// Check index
	auto line = this->line(index);
	if (!line)
		return 0;

	// Go through lines connected to first vertex
	int merged = 0;
	for (unsigned a = 0; a < line->vertex1_->connected_lines_.size(); a++)
	{
		auto other_line = line->vertex1_->connected_lines_[a];
		if (other_line == line)
			continue;

		// Check overlap
		if (line->overlaps(other_line))
		{
			// Remove line
			data_.removeLine(other_line);
			a--;
			merged++;
		}
	}

	// Correct sector references
	if (merged > 0)
		correctLineSectors(line);

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
			data_.removeSide(line->side1_);
		changed = true;
	}

	// Back side
	auto s2 = lineSideSector(line, false);
	if (s2 != s2_current)
	{
		if (s2)
			setLineSector(line->index_, s2->index_, false);
		else if (line->side2_)
			data_.removeSide(line->side2_);
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
		data_.removeSide(side_current);

	// If the new side is already part of another line, copy it
	if (side->parent_)
		side = data_.duplicateSide(side);

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
	unsigned n_lines     = nLines();
	auto     last_vertex = this->vertices().last();
	auto     last_line   = lines().last();

	// Merge vertices
	vector<MapVertex*> merged_vertices;
	for (auto& vertex : vertices)
	{
		auto v = mergeVerticesPoint(vertex->position_);
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
		unsigned nvertices = nVertices();
		for (unsigned b = 0; b < nvertices; b++)
		{
			auto vertex = this->vertex(b);

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
	Seg2d seg1;
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		auto line1 = connected_lines_[a];
		seg1       = line1->seg();

		unsigned n_lines = nLines();
		for (unsigned b = 0; b < n_lines; b++)
		{
			auto line2 = line(b);

			// Can't intersect if they share a vertex
			if (line1->vertex1_ == line2->vertex1_ || line1->vertex1_ == line2->vertex2_
				|| line2->vertex1_ == line1->vertex2_ || line2->vertex2_ == line1->vertex2_)
				continue;

			// Check for intersection
			Vec2d intersection;
			if (MathStuff::linesIntersect(seg1, line2->seg(), intersection))
			{
				// Create split vertex
				auto nv = createVertex(intersection);
				merged_vertices.push_back(nv);

				// Split lines
				splitLine(line1, nv);
				connected_lines_.push_back(lines().last());
				splitLine(line2, nv);
				connected_lines_.push_back(lines().last());

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
		Log::info(4, "Removing overlapping line {} (#{})", remove_line->objId(), remove_line->index());
		data_.removeLine(remove_line);
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
	if (nVertices() != n_vertices || nLines() != n_lines)
		merged = true;
	if (this->vertices().last() != last_vertex || lines().last() != last_line)
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
		Log::info(4, "Architecture merged");
	}
	else
	{
		Log::info(4, "No Architecture merged");
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
			if (sectors().atPos(mid))
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
	unsigned           ns_start  = nSectors();
	unsigned           nsd_start = nSides();
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

		data_.removeSide(edge.front ? edge.line->side1_ : edge.line->side2_);
	}

	// Log::info(1, "Ran sector builder %d times", runs);

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
	for (unsigned a = ns_start; a < sectors().size(); a++)
	{
		// Skip if sector already has properties
		if (!sector(a)->ceiling_.texture.empty())
			continue;

		// Copy from adjacent sector if any
		if (sector_copy)
		{
			sector(a)->copy(sector_copy);
			continue;
		}

		// Otherwise, use defaults from game configuration
		Game::configuration().applyDefaults(sector(a), current_format_ == MapFormat::UDMF);
	}

	// Update line textures
	for (unsigned a = nsd_start; a < sides().size(); a++)
	{
		// Clear any unneeded textures
		auto side = this->side(a);
		auto line = side->parentLine();
		line->clearUnneededTextures();

		// Set middle texture if needed
		if (side == line->s1() && !line->s2() && side->texMiddle() == MapSide::TEX_NONE)
		{
			// Log::info(1, "midtex");
			// Find adjacent texture (any)
			auto tex = adjacentLineTexture(line->v1());
			if (tex == MapSide::TEX_NONE)
				tex = adjacentLineTexture(line->v2());

			// If no adjacent texture, get default from game configuration
			if (tex == MapSide::TEX_NONE)
				tex = Game::configuration().defaultString(MapObject::Type::Side, "texturemiddle");

			// Set texture
			side->setTexMiddle(tex);
		}
	}

	// Remove any extra sectors
	data_.removeDetachedSectors();
}

// -----------------------------------------------------------------------------
// Performs checks for when a map is first opened
// -----------------------------------------------------------------------------
void SLADEMap::mapOpenChecks()
{
	int rverts  = data_.removeDetachedVertices();
	int rsides  = data_.removeDetachedSides();
	int rsec    = data_.removeDetachedSectors();
	int risides = data_.removeInvalidSides();

	Log::info(
		"Removed {} detached vertices, {} detached sides, {} invalid sides and {} detached sectors",
		rverts,
		rsides,
		risides,
		rsec);
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
		for (auto& line : lines())
		{
			int special = line->special();
			int flags   = 0;
			int id, hi;
			switch (special)
			{
			case 1:
				id = line->arg(3);
				line->setId(id);
				line->setArg(3, 0);
				break;

			case 5:
				id = line->arg(4);
				line->setId(id);
				line->setArg(4, 0);
				break;

			case 121:
				id    = line->arg(0);
				hi    = line->arg(4);
				id    = (hi * 256) + id;
				flags = line->arg(1);

				line->setSpecial(0);
				line->setId(id);
				line->setArg(0, 0);
				line->setArg(1, 0);
				line->setArg(2, 0);
				line->setArg(3, 0);
				line->setArg(4, 0);
				break;

			case 160:
				hi = id = line->arg(4);
				flags   = line->arg(1);
				if (flags & 8)
				{
					line->setId(id);
				}
				else
				{
					id = line->arg(0);
					line->setId((hi * 256) + id);
				}
				line->setArg(4, 0);
				flags = 0; // don't keep it set!
				break;

			case 181:
				id = line->arg(2);
				line->setId(id);
				line->setArg(2, 0);
				break;

			case 208:
				id    = line->arg(0);
				flags = line->arg(3);

				line->setId(id); // arg0 must be preserved
				line->setArg(3, 0);
				break;

			case 215:
				id = line->arg(0);
				line->setId(id);
				line->setArg(0, 0);
				break;

			case 222:
				id = line->arg(0);
				line->setId(id); // arg0 must be preserved
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
// Adjusts the usage count for thing type [type] by [adjust]
// -----------------------------------------------------------------------------
void SLADEMap::updateThingTypeUsage(int type, int adjust)
{
	usage_thing_type_[type] += adjust;
}

// -----------------------------------------------------------------------------
// Returns the usage count for the thing type [type]
// -----------------------------------------------------------------------------
int SLADEMap::thingTypeUsageCount(int type)
{
	return usage_thing_type_[type];
}
