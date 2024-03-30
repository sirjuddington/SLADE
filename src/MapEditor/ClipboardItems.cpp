
// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ClipboardItems.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Template function to find something in an associative map.
// M::mapped_type should be default constructible, or just provide
// a value for the third argument (the default value if not found).
// This really only works for value types right now, like maps to pointers.
// -----------------------------------------------------------------------------
template<typename M>
typename M::mapped_type findInMap(
	M&                          m,
	const typename M::key_type& k,
	typename M::mapped_type     def = typename M::mapped_type())
{
	typename M::iterator i = m.find(k);
	if (i == m.end())
	{
		return const_cast<typename M::mapped_type&>(def);
	}
	else
	{
		return i->second;
	}
}


// -----------------------------------------------------------------------------
//
// MapArchClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Copies [lines] and all related map structures
// -----------------------------------------------------------------------------
void MapArchClipboardItem::addLines(const vector<MapLine*>& lines)
{
	// Get sectors and sides to copy
	vector<MapSector*> copy_sectors;
	vector<MapSide*>   copy_sides;
	for (auto line : lines)
	{
		auto s1 = line->s1();
		auto s2 = line->s2();

		// Front side
		if (s1)
		{
			copy_sides.push_back(s1);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s1->sector()) == copy_sectors.end())
				copy_sectors.push_back(s1->sector());
		}

		// Back side
		if (s2)
		{
			copy_sides.push_back(s2);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s2->sector()) == copy_sectors.end())
				copy_sectors.push_back(s2->sector());
		}
	}

	// Copy sectors
	for (auto& sector : copy_sectors)
	{
		auto copy = std::make_unique<MapSector>();
		copy->copy(sector);
		sectors_.push_back(std::move(copy));
	}

	// Copy sides
	for (auto& side : copy_sides)
	{
		auto copy = std::make_unique<MapSide>();
		copy->copy(side);

		// Set relative sector
		for (unsigned b = 0; b < copy_sectors.size(); b++)
		{
			if (side->sector() == copy_sectors[b])
			{
				copy->setSector(sectors_[b].get());
				break;
			}
		}

		sides_.push_back(std::move(copy));
	}

	// Get vertices to copy (and determine midpoint)
	double             min_x = 9999999;
	double             max_x = -9999999;
	double             min_y = 9999999;
	double             max_y = -9999999;
	vector<MapVertex*> copy_verts;
	for (auto line : lines)
	{
		auto v1 = line->v1();
		auto v2 = line->v2();

		// Add vertices to copy list
		if (std::find(copy_verts.begin(), copy_verts.end(), v1) == copy_verts.end())
			copy_verts.push_back(v1);
		if (std::find(copy_verts.begin(), copy_verts.end(), v2) == copy_verts.end())
			copy_verts.push_back(v2);

		// Update min/max
		if (v1->xPos() < min_x)
			min_x = v1->xPos();
		if (v1->xPos() > max_x)
			max_x = v1->xPos();
		if (v1->yPos() < min_y)
			min_y = v1->yPos();
		if (v1->yPos() > max_y)
			max_y = v1->yPos();
		if (v2->xPos() < min_x)
			min_x = v2->xPos();
		if (v2->xPos() > max_x)
			max_x = v2->xPos();
		if (v2->yPos() < min_y)
			min_y = v2->yPos();
		if (v2->yPos() > max_y)
			max_y = v2->yPos();
	}

	// Determine midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	midpoint_    = { mid_x, mid_y };

	// Copy vertices
	for (auto& vertex : copy_verts)
	{
		auto copy = std::make_unique<MapVertex>(vertex->position() - midpoint_);
		copy->copy(vertex);
		vertices_.push_back(std::move(copy));
	}

	// Copy lines
	for (auto line : lines)
	{
		// Get relative sides
		MapSide* s1       = nullptr;
		MapSide* s2       = nullptr;
		bool     s1_found = false;
		bool     s2_found = !(line->s2());
		for (unsigned i = 0; i < copy_sides.size(); i++)
		{
			if (line->s1() == copy_sides[i])
			{
				s1       = sides_[i].get();
				s1_found = true;
			}
			if (line->s2() == copy_sides[i])
			{
				s2       = sides_[i].get();
				s2_found = true;
			}

			if (s1_found && s2_found)
				break;
		}

		// Get relative vertices
		MapVertex* v1 = nullptr;
		MapVertex* v2 = nullptr;
		for (unsigned i = 0; i < copy_verts.size(); i++)
		{
			if (line->v1() == copy_verts[i])
				v1 = vertices_[i].get();
			if (line->v2() == copy_verts[i])
				v2 = vertices_[i].get();

			if (v1 && v2)
				break;
		}

		// Copy line
		auto copy = std::make_unique<MapLine>(v1, v2, s1, s2);
		copy->copy(line);
		lines_.push_back(std::move(copy));
	}
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapArchClipboardItem::info() const
{
	return fmt::format(
		"{} Vertices, {} Lines, {} Sides and {} Sectors",
		vertices_.size(),
		lines_.size(),
		sides_.size(),
		sectors_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied architecture to [map] at [position]
// -----------------------------------------------------------------------------
vector<MapVertex*> MapArchClipboardItem::pasteToMap(SLADEMap* map, const Vec2d& position) const
{
	std::map<MapVertex*, MapVertex*> vertMap;
	std::map<MapSector*, MapSector*> sectMap;
	std::map<MapSide*, MapSide*>     sideMap;
	// Not used yet...
	// std::map<MapLine*, MapLine*> lineMap;

	// Add vertices
	vector<MapVertex*> new_verts;
	for (auto& vertex : vertices_)
	{
		new_verts.push_back(map->createVertex(position + vertex->position()));
		new_verts.back()->copy(vertex.get());
		vertMap[vertex.get()] = new_verts.back();
	}

	// Add sectors
	for (auto& sector : sectors_)
	{
		auto new_sector = map->createSector();
		new_sector->copy(sector.get());
		sectMap[sector.get()] = new_sector;
	}

	// Add sides
	// int first_new_side = map->nSides();
	for (auto& side : sides_)
	{
		// Get relative sector
		auto sector = findInMap(sectMap, side->sector());

		auto new_side = map->createSide(sector);
		new_side->copy(side.get());
		sideMap[side.get()] = new_side;
	}

	// Add lines
	// int first_new_line = map->nLines();
	for (auto& line : lines_)
	{
		// Get relative vertices
		auto v1 = findInMap(vertMap, line->v1());
		auto v2 = findInMap(vertMap, line->v2());

		if (!v1)
		{
			log::info(1, "no v1");
			continue;
		}
		if (!v2)
		{
			log::info(1, "no v2");
		}

		auto newline = map->createLine(v1, v2, true);
		newline->copy(line.get());

		// Set relative sides
		auto newS1 = findInMap(sideMap, line->s1());
		auto newS2 = findInMap(sideMap, line->s2());
		if (newS1)
			newline->setS1(newS1);
		if (newS2)
			newline->setS2(newS2);

		// Set important flags (needed when copying from Doom/Hexen format to UDMF)
		// Won't be needed when proper map format conversion stuff is implemented
		game::configuration().setLineBasicFlag("twosided", newline, map->currentFormat(), (newS1 && newS2));
		game::configuration().setLineBasicFlag("blocking", newline, map->currentFormat(), !newS2);
	}

	// TODO:
	// - Split lines
	// - Merge lines

	//// Fix sector references
	//// TODO: figure out what lines are 'outside' on copy, only fix said lines
	// for (unsigned a = first_new_line; a < map->nLines(); a++)
	//{
	//	MapLine* line = map->getLine(a);
	//	MapSector* sec1 = map->getLineSideSector(line, true);
	//	MapSector* sec2 = map->getLineSideSector(line, false);
	//	int i1 = -1;
	//	int i2 = -2;
	//	if (sec1) i1 = sec1->getIndex();
	//	if (sec2) i2 = sec2->getIndex();
	//	map->setLineSector(a, i1, true);
	//	map->setLineSector(a, i2, false);
	//}

	return new_verts;
}

// -----------------------------------------------------------------------------
// Adds all copied lines to [list]
// -----------------------------------------------------------------------------
void MapArchClipboardItem::putLines(vector<MapLine*>& list) const
{
	for (auto& line : lines_)
		list.push_back(line.get());
}


// -----------------------------------------------------------------------------
//
// MapThingsClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Copies [things]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::addThings(const vector<MapThing*>& things)
{
	// Copy things
	double min_x = 99999999;
	double min_y = 99999999;
	double max_x = -99999999;
	double max_y = -99999999;
	for (auto& thing : things)
	{
		auto copy_thing = std::make_unique<MapThing>();
		copy_thing->copy(thing);
		things_.push_back(std::move(copy_thing));

		if (thing->xPos() < min_x)
			min_x = thing->xPos();
		if (thing->yPos() < min_y)
			min_y = thing->yPos();
		if (thing->xPos() > max_x)
			max_x = thing->xPos();
		if (thing->yPos() > max_y)
			max_y = thing->yPos();
	}

	// Get midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	midpoint_    = { mid_x, mid_y };

	// Adjust thing positions
	for (auto& thing : things_)
		thing->move(thing->position() - midpoint_);
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapThingsClipboardItem::info() const
{
	return fmt::format("{} Things", things_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied things to [map] at [position]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::pasteToMap(SLADEMap* map, const Vec2d& position) const
{
	for (auto& thing : things_)
	{
		auto newthing = map->createThing({ 0., 0. });
		newthing->copy(thing.get());
		newthing->move(position + thing->position());
	}
}

// -----------------------------------------------------------------------------
// Adds all copied things to [list]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::putThings(vector<MapThing*>& list) const
{
	for (auto& thing : things_)
		list.push_back(thing.get());
}
