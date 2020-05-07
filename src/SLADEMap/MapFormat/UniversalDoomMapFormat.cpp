
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    UniversalDoomMapFormat.cpp
// Description: MapFormatHandler specialization to handle UDMF maps
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
#include "UniversalDoomMapFormat.h"
#include "App.h"
#include "Game/Configuration.h"
#include "General/UI.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectCollection.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// UniversalDoomMapFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads the given UDMF-format [map], populating [map_data]
// -----------------------------------------------------------------------------
bool UniversalDoomMapFormat::readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props)
{
	auto m_head = map.head.lock();
	if (!m_head)
		return false;

	// Get TEXTMAP entry (will always be after the 'head' entry)
	auto textmap = m_head->nextEntry();
	if (!textmap)
		return false;

	// --- Parse UDMF text ---
	ui::setSplashProgressMessage("Parsing TEXTMAP");
	ui::setSplashProgress(-100.0f);
	Parser parser;
	if (!parser.parseText(textmap->data()))
		return false;

	// --- Process parsed data ---

	// First we have to sort the definition blocks by type so they can
	// be created in the correct order (verts->sides->lines->sectors->things),
	// even if they aren't defined in that order.
	// Unknown definitions are also kept, just in case
	ui::setSplashProgressMessage("Sorting definitions");
	auto                   root = parser.parseTreeRoot();
	vector<ParseTreeNode*> defs_vertices;
	vector<ParseTreeNode*> defs_lines;
	vector<ParseTreeNode*> defs_sides;
	vector<ParseTreeNode*> defs_sectors;
	vector<ParseTreeNode*> defs_things;
	vector<ParseTreeNode*> defs_other;
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		ui::setSplashProgress((float)a / root->nChildren());

		auto node = root->childPTN(a);

		// Vertex definition
		if (strutil::equalCI(node->name(), "vertex"))
			defs_vertices.push_back(node);

		// Line definition
		else if (strutil::equalCI(node->name(), "linedef"))
			defs_lines.push_back(node);

		// Side definition
		else if (strutil::equalCI(node->name(), "sidedef"))
			defs_sides.push_back(node);

		// Sector definition
		else if (strutil::equalCI(node->name(), "sector"))
			defs_sectors.push_back(node);

		// Thing definition
		else if (strutil::equalCI(node->name(), "thing"))
			defs_things.push_back(node);

		// Namespace
		else if (strutil::equalCI(node->name(), "namespace"))
			udmf_namespace_ = node->stringValue();

		// Unknown
		else
			defs_other.push_back(node);
	}

	// Now create map structures from parsed data, in the right order

	// Create vertices from parsed data
	ui::setSplashProgressMessage("Reading Vertices");
	for (unsigned a = 0; a < defs_vertices.size(); a++)
	{
		ui::setSplashProgress(((float)a / defs_vertices.size()) * 0.2f);

		auto vertex = createVertex(defs_vertices[a]);
		if (!vertex)
		{
			log::warning("Invalid UDMF vertex definition {}, not added", a);
			continue;
		}

		map_data.addVertex(std::move(vertex));
	}

	// Create sectors from parsed data
	ui::setSplashProgressMessage("Reading Sectors");
	for (unsigned a = 0; a < defs_sectors.size(); a++)
	{
		ui::setSplashProgress(0.2f + ((float)a / defs_sectors.size()) * 0.2f);

		auto sector = createSector(defs_sectors[a]);
		if (!sector)
		{
			log::warning("Invalid UDMF sector definition {}, not added", a);
			continue;
		}

		map_data.addSector(std::move(sector));
	}

	// Create sides from parsed data
	ui::setSplashProgressMessage("Reading Sides");
	for (unsigned a = 0; a < defs_sides.size(); a++)
	{
		ui::setSplashProgress(0.4f + ((float)a / defs_sides.size()) * 0.2f);

		auto side = createSide(defs_sides[a], map_data);
		if (!side)
		{
			log::warning("Invalid UDMF side definition {}, not added", a);
			continue;
		}

		map_data.addSide(std::move(side));
	}

	// Create lines from parsed data
	ui::setSplashProgressMessage("Reading Lines");
	for (unsigned a = 0; a < defs_lines.size(); a++)
	{
		ui::setSplashProgress(0.6f + ((float)a / defs_lines.size()) * 0.2f);

		auto line = createLine(defs_lines[a], map_data);
		if (!line)
		{
			log::warning("Invalid UDMF line definition {}, not added", a);
			continue;
		}

		map_data.addLine(std::move(line));
	}

	// Create things from parsed data
	ui::setSplashProgressMessage("Reading Things");
	for (unsigned a = 0; a < defs_things.size(); a++)
	{
		ui::setSplashProgress(0.8f + ((float)a / defs_things.size()) * 0.2f);

		auto thing = createThing(defs_things[a]);
		if (!thing)
		{
			log::warning("Invalid UDMF thing definition {}, not added", a);
			continue;
		}

		map_data.addThing(std::move(thing));
	}

	// Keep map-scope values
	for (auto node : defs_other)
	{
		if (node->nValues() > 0)
			map_extra_props[node->name()] = node->value();

		// TODO: Unknown blocks
	}

	ui::setSplashProgressMessage("Init map data");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the given [map_data] to UDMF format, returning the list of entries
// making up the map
// -----------------------------------------------------------------------------
vector<unique_ptr<ArchiveEntry>> UniversalDoomMapFormat::writeMap(
	const MapObjectCollection& map_data,
	const PropertyList&        map_extra_props)
{
	vector<unique_ptr<ArchiveEntry>> entries;
	entries.push_back(std::make_unique<ArchiveEntry>("TEXTMAP"));

	// Open temp text file
	wxFile tempfile(app::path("sladetemp.txt", app::Dir::Temp), wxFile::write);

	// Write map namespace
	tempfile.Write("// Written by SLADE3\n");
	tempfile.Write(fmt::format("namespace=\"{}\";\n", udmf_namespace_));

	// Write map-scope props
	tempfile.Write(map_extra_props.toString(true));
	tempfile.Write("\n");

	// sf::Clock clock;

	// Locale for float number format
	setlocale(LC_NUMERIC, "C");

	// Write things
	string object_def;
	for (const auto& thing : map_data.things())
	{
		// Cleanup properties
		if (!thing->props().isEmpty())
		{
			thing->props().removeProperty("flags");
			game::configuration().cleanObjectUDMFProps(thing);
		}

		thing->writeUDMF(object_def);
		tempfile.Write(object_def);
	}
	// log::info(1, "Writing things took %dms", clock.getElapsedTime().asMilliseconds());

	// Write lines
	// clock.restart();
	for (const auto& line : map_data.lines())
	{
		// Cleanup properties
		if (!line->props().isEmpty())
		{
			line->props().removeProperty("flags");
			game::configuration().cleanObjectUDMFProps(line);
		}

		line->writeUDMF(object_def);
		tempfile.Write(object_def);
	}
	// log::info(1, "Writing lines took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sides
	// clock.restart();
	for (const auto& side : map_data.sides())
	{
		// Cleanup properties
		if (!side->props().isEmpty())
			game::configuration().cleanObjectUDMFProps(side);

		side->writeUDMF(object_def);
		tempfile.Write(object_def);
	}
	// log::info(1, "Writing sides took %dms", clock.getElapsedTime().asMilliseconds());

	// Write vertices
	// clock.restart();
	for (const auto& vertex : map_data.vertices())
	{
		// Cleanup properties
		if (!vertex->props().isEmpty())
			game::configuration().cleanObjectUDMFProps(vertex);

		vertex->writeUDMF(object_def);
		tempfile.Write(object_def);
	}
	// log::info(1, "Writing vertices took %dms", clock.getElapsedTime().asMilliseconds());

	// Write sectors
	// clock.restart();
	for (const auto& sector : map_data.sectors())
	{
		// Cleanup properties
		if (!sector->props().isEmpty())
			game::configuration().cleanObjectUDMFProps(sector);

		sector->writeUDMF(object_def);
		tempfile.Write(object_def);
	}
	// log::info(1, "Writing sectors took %dms", clock.getElapsedTime().asMilliseconds());

	// Close file
	tempfile.Close();

	// Load file to entry
	entries[0]->importFile(app::path("sladetemp.txt", app::Dir::Temp));

	return entries;
}

// -----------------------------------------------------------------------------
// Creates and returns a vertex from parsed UDMF definition [def]
// -----------------------------------------------------------------------------
unique_ptr<MapVertex> UniversalDoomMapFormat::createVertex(ParseTreeNode* def) const
{
	// Check for required properties
	auto prop_x = def->childPTN("x");
	auto prop_y = def->childPTN("y");
	if (!prop_x || !prop_y)
		return nullptr;

	// Create vertex
	return std::make_unique<MapVertex>(Vec2d{ prop_x->floatValue(), prop_y->floatValue() }, def);
}

// -----------------------------------------------------------------------------
// Creates and returns a sector from parsed UDMF definition [def]
// -----------------------------------------------------------------------------
unique_ptr<MapSector> UniversalDoomMapFormat::createSector(ParseTreeNode* def) const
{
	// Check for required properties
	auto prop_ftex = def->childPTN("texturefloor");
	auto prop_ctex = def->childPTN("textureceiling");
	if (!prop_ftex || !prop_ctex)
		return nullptr;

	// Create sector
	return std::make_unique<MapSector>(prop_ftex->stringValue(), prop_ctex->stringValue(), def);
}

// -----------------------------------------------------------------------------
// Creates and returns a side from parsed UDMF definition [def]
// -----------------------------------------------------------------------------
unique_ptr<MapSide> UniversalDoomMapFormat::createSide(ParseTreeNode* def, const MapObjectCollection& map_data) const
{
	// Check for required properties
	auto prop_sector = def->childPTN("sector");
	if (!prop_sector)
		return nullptr;

	// Check sector exists
	auto sector = map_data.sectors().at(prop_sector->intValue());
	if (!sector)
		return nullptr;

	// Create side
	return std::make_unique<MapSide>(sector, def);
}

// -----------------------------------------------------------------------------
// Creates and returns a line from parsed UDMF definition [def]
// -----------------------------------------------------------------------------
unique_ptr<MapLine> UniversalDoomMapFormat::createLine(ParseTreeNode* def, const MapObjectCollection& map_data) const
{
	// Check for required properties
	auto prop_v1 = def->childPTN(MapLine::PROP_V1);
	auto prop_v2 = def->childPTN(MapLine::PROP_V2);
	auto prop_s1 = def->childPTN(MapLine::PROP_S1);
	auto prop_s2 = def->childPTN(MapLine::PROP_S2);
	if (!prop_v1 || !prop_v2 || !prop_s1)
		return nullptr;

	// Check vertices
	auto v1 = map_data.vertices().at(prop_v1->intValue());
	auto v2 = map_data.vertices().at(prop_v2->intValue());
	if (!v1 || !v2)
		return nullptr;

	// Get sides
	auto s1 = map_data.sides().at(prop_s1->intValue());
	auto s2 = prop_s2 ? map_data.sides().at(prop_s2->intValue()) : nullptr;

	// Create line
	return std::make_unique<MapLine>(v1, v2, s1, s2, def);
}

// -----------------------------------------------------------------------------
// Creates and returns a thing from parsed UDMF definition [def]
// -----------------------------------------------------------------------------
unique_ptr<MapThing> UniversalDoomMapFormat::createThing(ParseTreeNode* def) const
{
	// Check for required properties
	auto prop_x    = def->childPTN(MapThing::PROP_X);
	auto prop_y    = def->childPTN(MapThing::PROP_Y);
	auto prop_type = def->childPTN(MapThing::PROP_TYPE);
	if (!prop_x || !prop_y || !prop_type)
		return nullptr;

	// Create thing
	return std::make_unique<MapThing>(
		Vec3d{ prop_x->floatValue(), prop_y->floatValue(), 0. }, prop_type->intValue(), def);
}
