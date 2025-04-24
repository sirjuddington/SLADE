
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapPreviewData.cpp
// Description: Struct for holding data for map preview canvases and related
//              functions
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
#include "MapPreviewData.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/MapDesc.h"
#include "ColourConfiguration.h"
#include "SLADEMap/MapFormat/Doom32XMapFormat.h"
#include "SLADEMap/MapFormat/Doom64MapFormat.h"
#include "SLADEMap/MapFormat/HexenMapFormat.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, map_image_thickness, 1.5, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Adds a vertex at [x,y] to map preview [data]
// -----------------------------------------------------------------------------
void addVertex(MapPreviewData& data, double x, double y)
{
	data.vertices.emplace_back(x, y);
	data.bounds.extend(x, y);
}

// -----------------------------------------------------------------------------
// Adds a thing at [x,y] to map preview [data]
// -----------------------------------------------------------------------------
void addThing(MapPreviewData& data, double x, double y)
{
	data.things.emplace_back(x, y);
	data.bounds.extend(x, y);
}

// -----------------------------------------------------------------------------
// Reads non-UDMF vertex data
// -----------------------------------------------------------------------------
bool readVertices(MapPreviewData& data, ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
{
	// Find VERTEXES entry
	ArchiveEntry* vertexes = nullptr;
	while (map_head)
	{
		// Check entry type
		if (map_head->type() == EntryType::fromId("map_vertexes"))
		{
			vertexes = map_head;
			break;
		}

		// Exit loop if we've reached the end of the map entries
		if (map_head == map_end)
			break;
		else
			map_head = map_head->nextEntry();
	}

	// Can't open a map without vertices
	if (!vertexes)
		return false;

	// Read vertex data
	auto& mc = vertexes->data();
	mc.seek(0, SEEK_SET);

	if (map_format == MapFormat::Doom64)
	{
		Doom64MapFormat::Vertex v;
		while (true)
		{
			// Read vertex
			if (!mc.read(&v, 8))
				break;

			// Add vertex
			addVertex(data, static_cast<double>(v.x) / 65536, static_cast<double>(v.y) / 65536);
		}
	}
	else if (map_format == MapFormat::Doom32X)
	{
		Doom32XMapFormat::Vertex32BE v;
		while (true)
		{
			// Read vertex
			if (!mc.read(&v, 8))
				break;

			// Add vertex
			addVertex(
				data,
				static_cast<double>(wxINT32_SWAP_ON_LE(v.x)) / 65536,
				static_cast<double>(wxINT32_SWAP_ON_LE(v.y)) / 65536);
		}
	}
	else
	{
		DoomMapFormat::Vertex v;
		while (true)
		{
			// Read vertex
			if (!mc.read(&v, 4))
				break;

			// Add vertex
			addVertex(data, v.x, v.y);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF line data
// -----------------------------------------------------------------------------
bool readLines(MapPreviewData& data, ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
{
	// Find LINEDEFS entry
	ArchiveEntry* linedefs = nullptr;
	while (map_head)
	{
		// Check entry type
		if (map_head->type() == EntryType::fromId("map_linedefs"))
		{
			linedefs = map_head;
			break;
		}

		// Exit loop if we've reached the end of the map entries
		if (map_head == map_end)
			break;
		else
			map_head = map_head->nextEntry();
	}

	// Can't open a map without linedefs
	if (!linedefs)
		return false;

	// Read line data
	auto& mc = linedefs->data();
	mc.seek(0, SEEK_SET);
	if (map_format == MapFormat::Doom || map_format == MapFormat::Doom32X)
	{
		while (true)
		{
			// Read line
			DoomMapFormat::LineDef l;
			if (!mc.read(&l, sizeof(DoomMapFormat::LineDef)))
				break;

			// Check properties
			bool special  = false;
			bool twosided = false;
			if (l.side2 != 0xFFFF)
				twosided = true;
			if (l.type > 0)
				special = true;

			// Add line
			data.lines.emplace_back(l.vertex1, l.vertex2, twosided, special);
		}
	}
	else if (map_format == MapFormat::Doom64)
	{
		while (true)
		{
			// Read line
			Doom64MapFormat::LineDef l;
			if (!mc.read(&l, sizeof(Doom64MapFormat::LineDef)))
				break;

			// Check properties
			bool macro    = false;
			bool special  = false;
			bool twosided = false;
			if (l.side2 != 0xFFFF)
				twosided = true;
			if (l.type > 0)
			{
				if (l.type & 0x100)
					macro = true;
				else
					special = true;
			}

			// Add line
			data.lines.emplace_back(l.vertex1, l.vertex2, twosided, special, macro);
		}
	}
	else if (map_format == MapFormat::Hexen)
	{
		while (true)
		{
			// Read line
			HexenMapFormat::LineDef l;
			if (!mc.read(&l, sizeof(HexenMapFormat::LineDef)))
				break;

			// Check properties
			bool special  = false;
			bool twosided = false;
			if (l.side2 != 0xFFFF)
				twosided = true;
			if (l.type > 0)
				special = true;

			// Add line
			data.lines.emplace_back(l.vertex1, l.vertex2, twosided, special);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF thing data
// -----------------------------------------------------------------------------
bool readThings(MapPreviewData& data, ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
{
	// Find THINGS entry
	ArchiveEntry* things = nullptr;
	while (map_head)
	{
		// Check entry type
		if (map_head->type() == EntryType::fromId("map_things"))
		{
			things = map_head;
			break;
		}

		// Exit loop if we've reached the end of the map entries
		if (map_head == map_end)
			break;
		else
			map_head = map_head->nextEntry();
	}

	// No things
	if (!things)
		return false;

	// Read things data
	if (map_format == MapFormat::Doom || map_format == MapFormat::Doom32X)
	{
		auto     thng_data = reinterpret_cast<const DoomMapFormat::Thing*>(things->rawData());
		unsigned nt        = things->size() / sizeof(DoomMapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(data, thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Doom64)
	{
		auto     thng_data = reinterpret_cast<const Doom64MapFormat::Thing*>(things->rawData());
		unsigned nt        = things->size() / sizeof(Doom64MapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(data, thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Hexen)
	{
		auto     thng_data = reinterpret_cast<const HexenMapFormat::Thing*>(things->rawData());
		unsigned nt        = things->size() / sizeof(HexenMapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(data, thng_data[a].x, thng_data[a].y);
	}

	return true;
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapPreviewData struct functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Opens a map from a MapDesc
// -----------------------------------------------------------------------------
bool MapPreviewData::openMap(MapDesc map)
{
	auto m_head = map.head.lock();
	if (!m_head)
		return false;

	// All errors = invalid map
	global::error = "Invalid map";

	// Init
	std::unique_ptr<Archive> temp_archive;

	// Check if this map is a pk3 map
	bool map_archive = false;
	if (map.archive)
	{
		map_archive = true;

		// Attempt to open entry as wad archive
		temp_archive = std::make_unique<Archive>(ArchiveFormat::Wad);
		if (!temp_archive->open(m_head->data()))
		{
			return false;
		}

		// Detect maps
		auto maps = temp_archive->detectMaps();

		// Set map if there are any in the archive
		if (!maps.empty())
			map = maps[0];
		else
			return false;

		m_head = maps[0].head.lock();
	}

	// Parse UDMF map
	if (map.format == MapFormat::UDMF)
	{
		ArchiveEntry* udmfdata  = nullptr;
		auto          archive   = m_head->parent();
		auto          index     = archive->entryIndex(m_head.get());
		auto          end_index = archive->entryIndex(map.end.lock().get());
		while (index <= end_index)
		{
			// Check entry type
			auto entry = archive->entryAt(index);
			if (entry->type() == EntryType::fromId("udmf_textmap"))
			{
				udmfdata = entry;
				break;
			}
			++index;
		}
		if (udmfdata == nullptr)
			return false;

		// Start parsing
		Tokenizer tz;
		tz.openMem(udmfdata->data(), m_head->name());
		size_t vertcounter = 0, linecounter = 0, thingcounter = 0;
		while (!tz.atEnd())
		{
			// Namespace
			if (tz.checkNC("namespace"))
				tz.advUntil(";");

			// Sidedef
			else if (tz.checkNC("sidedef"))
			{
				// Just increase count
				n_sides++;
				tz.advUntil("}");
			}

			// Sector
			else if (tz.checkNC("sector"))
			{
				// Just increase count
				n_sectors++;
				tz.advUntil("}");
			}

			// Vertex
			else if (tz.checkNC("vertex"))
			{
				// Get X and Y properties
				bool   gotx = false;
				bool   goty = false;
				double x    = 0.;
				double y    = 0.;

				tz.adv(2); // skip {

				while (!tz.check("}"))
				{
					if (tz.checkNC("x") || tz.checkNC("y"))
					{
						if (!tz.checkNext("="))
						{
							log::error("Bad syntax for vertex {} in UDMF map data", vertcounter);
							return false;
						}

						if (tz.checkNC("x"))
						{
							tz.adv(2);
							x    = tz.current().asFloat();
							gotx = true;
						}
						else
						{
							tz.adv(2);
							y    = tz.current().asFloat();
							goty = true;
						}
					}

					tz.advUntil(";");
					tz.adv();
				}

				if (gotx && goty)
					addVertex(*this, x, y);
				else
				{
					log::error("Wrong vertex {} in UDMF map data", vertcounter);
					return false;
				}

				vertcounter++;
			}

			// Thing
			else if (tz.checkNC("thing"))
			{
				// Get X and Y properties
				bool   gotx = false;
				bool   goty = false;
				double x    = 0.;
				double y    = 0.;

				tz.adv(2); // skip {

				while (!tz.check("}"))
				{
					if (tz.checkNC("x") || tz.checkNC("y"))
					{
						if (!tz.checkNext("="))
						{
							log::error("Bad syntax for thing {} in UDMF map data", thingcounter);
							return false;
						}

						if (tz.checkNC("x"))
						{
							tz.adv(2);
							x    = tz.current().asFloat();
							gotx = true;
						}
						else
						{
							tz.adv(2);
							y    = tz.current().asFloat();
							goty = true;
						}
					}

					tz.advUntil(";");
					tz.adv();
				}

				if (gotx && goty)
					addThing(*this, x, y);
				else
				{
					log::error("Wrong thing {} in UDMF map data", thingcounter);
					return false;
				}

				thingcounter++;
			}

			// Linedef
			else if (tz.checkNC("linedef"))
			{
				bool     special  = false;
				bool     twosided = false;
				bool     gotv1 = false, gotv2 = false;
				unsigned v1 = 0, v2 = 0;

				tz.adv(2); // skip {

				while (!tz.check("}"))
				{
					if (tz.checkNC("v1") || tz.checkNC("v2"))
					{
						if (!tz.checkNext("="))
						{
							log::error("Bad syntax for linedef {} in UDMF map data", linecounter);
							return false;
						}

						if (tz.checkNC("v1"))
						{
							tz.adv(2);
							v1    = tz.current().asInt();
							gotv1 = true;
						}
						else
						{
							tz.adv(2);
							v2    = tz.current().asInt();
							gotv2 = true;
						}
					}
					else if (tz.checkNC("special"))
						special = true;
					else if (tz.checkNC("sideback"))
						twosided = true;

					tz.advUntil(";");
					tz.adv();
				}

				if (gotv1 && gotv2)
					lines.emplace_back(v1, v2, twosided, special);
				else
				{
					log::error("Wrong line {} in UDMF map data", linecounter);
					return false;
				}

				linecounter++;
			}

			tz.adv();
		}
	}

	// Non-UDMF map
	if (map.format != MapFormat::UDMF)
	{
		auto m_head = map.head.lock().get();
		auto m_end  = map.end.lock().get();

		// Read vertices (required)
		if (!readVertices(*this, m_head, m_end, map.format))
			return false;

		// Read linedefs (required)
		if (!readLines(*this, m_head, m_end, map.format))
			return false;

		// Read things
		if (map.format != MapFormat::UDMF)
			readThings(*this, m_head, m_end, map.format);

		// Read sides & sectors (count only)
		ArchiveEntry* sidedefs = nullptr;
		ArchiveEntry* sectors  = nullptr;
		while (m_head)
		{
			// Check entry type
			if (m_head->type() == EntryType::fromId("map_sidedefs"))
				sidedefs = m_head;
			if (m_head->type() == EntryType::fromId("map_sectors"))
				sectors = m_head;

			// Exit loop if we've reached the end of the map entries
			if (m_head == m_end)
				break;
			else
				m_head = m_head->nextEntry();
		}
		if (sidedefs && sectors)
		{
			// Doom64 map
			if (map.format != MapFormat::Doom64)
			{
				n_sides   = sidedefs->size() / 30;
				n_sectors = sectors->size() / 26;
			}

			// Doom/Hexen map
			else
			{
				n_sides   = sidedefs->size() / 12;
				n_sectors = sectors->size() / 16;
			}
		}
	}

	// Clean up
	if (map_archive)
		temp_archive->close();

	updated_time = app::runTimer();

	return true;
}

// -----------------------------------------------------------------------------
// Clears all map preview data
// -----------------------------------------------------------------------------
void MapPreviewData::clear()
{
	vertices.clear();
	lines.clear();
	things.clear();
	bounds.reset();
	n_sides      = 0;
	n_sectors    = 0;
	updated_time = app::runTimer();
}

// -----------------------------------------------------------------------------
// Saves a png image of the map preview [data] to [filename].
// [width] and [height] can either be absolute values (positive) or multiples
// of map size (negative), eg. -5 width is mapwidth/5
// -----------------------------------------------------------------------------
bool slade::createMapImage(const MapPreviewData& data, const string& filename, int width, int height)
{
	// Determine width/height of image
	if (width == 0)
		width = -5;
	if (height == 0)
		height = -5;
	if (width < 0)
		width = data.bounds.width() / abs(width);
	if (height < 0)
		height = data.bounds.height() / abs(height);

	// Setup colours
	auto col_save_background   = colourconfig::colour("map_image_background");
	auto col_save_line_1s      = colourconfig::colour("map_image_line_1s");
	auto col_save_line_2s      = colourconfig::colour("map_image_line_2s");
	auto col_save_line_special = colourconfig::colour("map_image_line_special");
	auto col_save_line_macro   = colourconfig::colour("map_image_line_macro");

	// Determine mid / scale
	auto mid_x   = width / 2;
	auto mid_y   = height / 2;
	auto map_mid = data.bounds.mid();
	auto scale_x = static_cast<double>(width) / data.bounds.width();
	auto scale_y = static_cast<double>(height) / data.bounds.height();
	auto scale   = glm::min(scale_x, scale_y) * 0.95;

	// Create image / wxGraphicsContext
	auto image = wxImage(width, height);
	image.SetMaskColour(0, 0, 0);
	image.InitAlpha();
	auto gc = wxGraphicsContext::Create(image);

	// Background
	gc->SetBrush(wxBrush(col_save_background));
	gc->DrawRectangle(0, 0, width, height);

	// Build lines to draw via wxGraphicsContext
	vector<wxPoint2DDouble> lines_1s_v1;
	vector<wxPoint2DDouble> lines_1s_v2;
	vector<wxPoint2DDouble> lines_2s_v1;
	vector<wxPoint2DDouble> lines_2s_v2;
	vector<wxPoint2DDouble> lines_special_v1;
	vector<wxPoint2DDouble> lines_special_v2;
	vector<wxPoint2DDouble> lines_macro_v1;
	vector<wxPoint2DDouble> lines_macro_v2;
	for (auto& line : data.lines)
	{
		auto& v1 = data.vertices[line.v1];
		auto& v2 = data.vertices[line.v2];

		// Transform vertex positions
		auto x1 = mid_x + (v1.x - map_mid.x) * scale;
		auto y1 = mid_y - (v1.y - map_mid.y) * scale;
		auto x2 = mid_x + (v2.x - map_mid.x) * scale;
		auto y2 = mid_y - (v2.y - map_mid.y) * scale;

		if (line.twosided)
		{
			lines_2s_v1.emplace_back(x1, y1);
			lines_2s_v2.emplace_back(x2, y2);
		}
		else if (line.special)
		{
			lines_special_v1.emplace_back(x1, y1);
			lines_special_v2.emplace_back(x2, y2);
		}
		else if (line.macro)
		{
			lines_macro_v1.emplace_back(x1, y1);
			lines_macro_v2.emplace_back(x2, y2);
		}
		else
		{
			lines_1s_v1.emplace_back(x1, y1);
			lines_1s_v2.emplace_back(x2, y2);
		}
	}

	double line_width = map_image_thickness + 0.01;
	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
	gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

	// 2-Sided lines
	if (!lines_2s_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(col_save_line_2s, line_width)));
		gc->StrokeLines(lines_2s_v1.size(), lines_2s_v1.data(), lines_2s_v2.data());
	}

	// 1-Sided lines
	if (!lines_1s_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(col_save_line_1s, line_width)));
		gc->StrokeLines(lines_1s_v1.size(), lines_1s_v1.data(), lines_1s_v2.data());
	}

	// Macro lines
	if (!lines_macro_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(col_save_line_macro, line_width)));
		gc->StrokeLines(lines_macro_v1.size(), lines_macro_v1.data(), lines_macro_v2.data());
	}

	// Special lines
	if (!lines_special_v1.empty())
	{
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(col_save_line_special, line_width)));
		gc->StrokeLines(lines_special_v1.size(), lines_special_v1.data(), lines_special_v2.data());
	}

	gc->Flush();
	return image.SaveFile(wxString::FromUTF8(filename), wxBITMAP_TYPE_PNG);
}
