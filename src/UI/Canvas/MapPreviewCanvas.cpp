
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapPreviewCanvas.cpp
// Description: OpenGL Canvas that shows a basic map preview, can also save the
//              preview to an image
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
#include "MapPreviewCanvas.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "General/ColourConfiguration.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/MapFormat/Doom64MapFormat.h"
#include "SLADEMap/MapFormat/DoomMapFormat.h"
#include "SLADEMap/MapFormat/HexenMapFormat.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, map_image_thickness, 1.5, CVar::Flag::Save)
CVAR(Bool, map_view_things, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapPreviewCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds a vertex to the map data
// -----------------------------------------------------------------------------
void MapPreviewCanvas::addVertex(double x, double y)
{
	verts_.emplace_back(x, y);
}

// -----------------------------------------------------------------------------
// Adds a line to the map data
// -----------------------------------------------------------------------------
void MapPreviewCanvas::addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro)
{
	lines_.emplace_back(v1, v2, twosided, special, macro);
}

// -----------------------------------------------------------------------------
// Adds a thing to the map data
// -----------------------------------------------------------------------------
void MapPreviewCanvas::addThing(double x, double y)
{
	things_.emplace_back(x, y);
}

// -----------------------------------------------------------------------------
// Opens a map from a mapdesc_t
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::openMap(Archive::MapDesc map)
{
	auto m_head = map.head.lock();
	if (!m_head)
		return false;

	// All errors = invalid map
	Global::error = "Invalid map";

	// Check if this map is a pk3 map
	bool map_archive = false;
	if (map.archive)
	{
		map_archive = true;

		// Attempt to open entry as wad archive
		temp_archive_ = std::make_unique<WadArchive>();
		if (!temp_archive_->open(m_head->data()))
		{
			temp_archive_.reset();
			return false;
		}

		// Detect maps
		auto maps = temp_archive_->detectMaps();

		// Set map if there are any in the archive
		if (!maps.empty())
			map = maps[0];
		else
			return false;
	}

	// Parse UDMF map
	if (map.format == MapFormat::UDMF)
	{
		ArchiveEntry* udmfdata = nullptr;
		auto          archive   = m_head->parent();
		auto          index    = archive->entryIndex(m_head.get());
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

		// Get first token
		wxString token       = tz.getToken();
		size_t   vertcounter = 0, linecounter = 0, thingcounter = 0;
		while (!token.IsEmpty())
		{
			if (!token.CmpNoCase("namespace"))
			{
				//  skip till we reach the ';'
				do
				{
					token = tz.getToken();
				} while (token.Cmp(";"));
			}
			else if (!token.CmpNoCase("vertex"))
			{
				// Get X and Y properties
				bool   gotx = false;
				bool   goty = false;
				double x    = 0.;
				double y    = 0.;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("x") || !token.CmpNoCase("y"))
					{
						bool isx = !token.CmpNoCase("x");
						token    = tz.getToken();
						if (token.Cmp("="))
						{
							Log::error(wxString::Format("Bad syntax for vertex %i in UDMF map data", vertcounter));
							return false;
						}
						if (isx)
							x = tz.getDouble(), gotx = true;
						else
							y = tz.getDouble(), goty = true;
						// skip to end of declaration after each key
						do
						{
							token = tz.getToken();
						} while (token.Cmp(";"));
					}
				} while (token.Cmp("}"));
				if (gotx && goty)
					addVertex(x, y);
				else
				{
					Log::error(wxString::Format("Wrong vertex %i in UDMF map data", vertcounter));
					return false;
				}
				vertcounter++;
			}
			else if (!token.CmpNoCase("linedef"))
			{
				bool   special  = false;
				bool   twosided = false;
				bool   gotv1 = false, gotv2 = false;
				size_t v1 = 0, v2 = 0;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("v1") || !token.CmpNoCase("v2"))
					{
						bool isv1 = !token.CmpNoCase("v1");
						token     = tz.getToken();
						if (token.Cmp("="))
						{
							Log::error(wxString::Format("Bad syntax for linedef %i in UDMF map data", linecounter));
							return false;
						}
						if (isv1)
							v1 = tz.getInteger(), gotv1 = true;
						else
							v2 = tz.getInteger(), gotv2 = true;
						// skip to end of declaration after each key
						do
						{
							token = tz.getToken();
						} while (token.Cmp(";"));
					}
					else if (!token.CmpNoCase("special"))
					{
						special = true;
						// skip to end of declaration after each key
						do
						{
							token = tz.getToken();
						} while (token.Cmp(";"));
					}
					else if (!token.CmpNoCase("sideback"))
					{
						twosided = true;
						// skip to end of declaration after each key
						do
						{
							token = tz.getToken();
						} while (token.Cmp(";"));
					}
				} while (token.Cmp("}"));
				if (gotv1 && gotv2)
					addLine(v1, v2, twosided, special);
				else
				{
					Log::error(wxString::Format("Wrong line %i in UDMF map data", linecounter));
					return false;
				}
				linecounter++;
			}
			else if (S_CMPNOCASE(token, "thing"))
			{
				// Get X and Y properties
				bool   gotx = false;
				bool   goty = false;
				double x    = 0.;
				double y    = 0.;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("x") || !token.CmpNoCase("y"))
					{
						bool isx = !token.CmpNoCase("x");
						token    = tz.getToken();
						if (token.Cmp("="))
						{
							Log::error(wxString::Format("Bad syntax for thing %i in UDMF map data", vertcounter));
							return false;
						}
						if (isx)
							x = tz.getDouble(), gotx = true;
						else
							y = tz.getDouble(), goty = true;
						// skip to end of declaration after each key
						do
						{
							token = tz.getToken();
						} while (token.Cmp(";"));
					}
				} while (token.Cmp("}"));
				if (gotx && goty)
					addThing(x, y);
				else
				{
					Log::error(wxString::Format("Wrong thing %i in UDMF map data", vertcounter));
					return false;
				}
				vertcounter++;
			}
			else
			{
				// Check for side or sector definition (increase counts)
				if (S_CMPNOCASE(token, "sidedef"))
					n_sides_++;
				else if (S_CMPNOCASE(token, "sector"))
					n_sectors_++;

				// map preview ignores sidedefs, sectors, comments,
				// unknown fields, etc. so skip to end of block
				do
				{
					token = tz.getToken();
				} while (token.Cmp("}") && !token.empty());
			}
			// Iterate to next token
			token = tz.getToken();
		}
	}

	// Non-UDMF map
	if (map.format != MapFormat::UDMF)
	{
		auto m_head = map.head.lock().get();
		auto m_end  = map.end.lock().get();

		// Read vertices (required)
		if (!readVertices(m_head, m_end, map.format))
			return false;

		// Read linedefs (required)
		if (!readLines(m_head, m_end, map.format))
			return false;

		// Read things
		if (map.format != MapFormat::UDMF)
			readThings(m_head, m_end, map.format);

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
				n_sides_   = sidedefs->size() / 30;
				n_sectors_ = sectors->size() / 26;
			}

			// Doom/Hexen map
			else
			{
				n_sides_   = sidedefs->size() / 12;
				n_sectors_ = sectors->size() / 16;
			}
		}
	}

	// Clean up
	if (map_archive)
	{
		temp_archive_->close();
		temp_archive_ = nullptr;
	}

	// Refresh map
	Refresh();

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF vertex data
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::readVertices(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format)
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
			addVertex((double)v.x / 65536, (double)v.y / 65536);
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
			addVertex((double)v.x, (double)v.y);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF line data
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::readLines(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format)
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
	if (map_format == MapFormat::Doom)
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
			addLine(l.vertex1, l.vertex2, twosided, special);
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
			addLine(l.vertex1, l.vertex2, twosided, special, macro);
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
			addLine(l.vertex1, l.vertex2, twosided, special);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF thing data
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::readThings(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format)
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
	if (map_format == MapFormat::Doom)
	{
		auto     thng_data = (DoomMapFormat::Thing*)things->rawData(true);
		unsigned nt        = things->size() / sizeof(DoomMapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Doom64)
	{
		auto     thng_data = (Doom64MapFormat::Thing*)things->rawData(true);
		unsigned nt        = things->size() / sizeof(Doom64MapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Hexen)
	{
		auto     thng_data = (HexenMapFormat::Thing*)things->rawData(true);
		unsigned nt        = things->size() / sizeof(HexenMapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Clears map data
// -----------------------------------------------------------------------------
void MapPreviewCanvas::clearMap()
{
	verts_.clear();
	lines_.clear();
	things_.clear();
	n_sides_   = 0;
	n_sectors_ = 0;
}

// -----------------------------------------------------------------------------
// Adjusts zoom and offset to show the whole map
// -----------------------------------------------------------------------------
void MapPreviewCanvas::showMap()
{
	// Find extents of map
	Vertex m_min(999999.0, 999999.0);
	Vertex m_max(-999999.0, -999999.0);
	for (auto& vert : verts_)
	{
		if (vert.x < m_min.x)
			m_min.x = vert.x;
		if (vert.x > m_max.x)
			m_max.x = vert.x;
		if (vert.y < m_min.y)
			m_min.y = vert.y;
		if (vert.y > m_max.y)
			m_max.y = vert.y;
	}

	// Offset to center of map
	double width  = m_max.x - m_min.x;
	double height = m_max.y - m_min.y;
	offset_       = { m_min.x + (width * 0.5), m_min.y + (height * 0.5) };

	// Zoom to fit whole map
	double x_scale = ((double)GetClientSize().x) / width;
	double y_scale = ((double)GetClientSize().y) / height;
	zoom_          = std::min<double>(x_scale, y_scale);
	zoom_ *= 0.95;
}

// -----------------------------------------------------------------------------
// Draws the map
// -----------------------------------------------------------------------------
void MapPreviewCanvas::draw()
{
	// Setup colours
	auto col_view_background   = ColourConfiguration::colour("map_view_background");
	auto col_view_line_1s      = ColourConfiguration::colour("map_view_line_1s");
	auto col_view_line_2s      = ColourConfiguration::colour("map_view_line_2s");
	auto col_view_line_special = ColourConfiguration::colour("map_view_line_special");
	auto col_view_line_macro   = ColourConfiguration::colour("map_view_line_macro");
	auto col_view_thing        = ColourConfiguration::colour("map_view_thing");

	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, 0, GetSize().y, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(
		((double)col_view_background.r) / 255.f,
		((double)col_view_background.g) / 255.f,
		((double)col_view_background.b) / 255.f,
		((double)col_view_background.a) / 255.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Zoom/offset to show full map
	showMap();

	// Translate to middle of canvas
	glTranslated(GetSize().x * 0.5, GetSize().y * 0.5, 0);

	// Zoom
	glScaled(zoom_, zoom_, 1);

	// Translate to offset
	glTranslated(-offset_.x, -offset_.y, 0);

	// Setup drawing
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLineWidth(1.5f);
	glEnable(GL_LINE_SMOOTH);

	// Draw lines
	for (auto& line : lines_)
	{
		// Check ends
		if (line.v1 >= verts_.size() || line.v2 >= verts_.size())
			continue;

		// Get vertices
		auto v1 = verts_[line.v1];
		auto v2 = verts_[line.v2];

		// Set colour
		if (line.special)
			OpenGL::setColour(col_view_line_special);
		else if (line.macro)
			OpenGL::setColour(col_view_line_macro);
		else if (line.twosided)
			OpenGL::setColour(col_view_line_2s);
		else
			OpenGL::setColour(col_view_line_1s);

		// Draw line
		glBegin(GL_LINES);
		glVertex2d(v1.x, v1.y);
		glVertex2d(v2.x, v2.y);
		glEnd();
	}

	// Load thing texture if needed
	if (!tex_loaded_)
	{
		// Load thing texture
		SImage image;
		auto   entry = App::archiveManager().programResourceArchive()->entryAtPath("images/thing/normal_n.png");
		if (entry)
		{
			image.open(entry->data());
			tex_thing_ = OpenGL::Texture::createFromImage(image, nullptr, OpenGL::TexFilter::Mipmap);
		}
		else
			tex_thing_ = 0;

		tex_loaded_ = true;
	}

	// Draw things
	if (map_view_things)
	{
		OpenGL::setColour(col_view_thing);
		if (tex_thing_)
		{
			double radius = 20;
			glEnable(GL_TEXTURE_2D);
			OpenGL::Texture::bind(tex_thing_);
			for (auto& thing : things_)
			{
				glPushMatrix();
				glTranslated(thing.x, thing.y, 0);
				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);
				glVertex2d(-radius, -radius);
				glTexCoord2f(0.0f, 1.0f);
				glVertex2d(-radius, radius);
				glTexCoord2f(1.0f, 1.0f);
				glVertex2d(radius, radius);
				glTexCoord2f(1.0f, 0.0f);
				glVertex2d(radius, -radius);
				glEnd();
				glPopMatrix();
			}
		}
		else
		{
			glEnable(GL_POINT_SMOOTH);
			glPointSize(8.0f);
			glBegin(GL_POINTS);
			for (auto& thing : things_)
				glVertex2d(thing.x, thing.y);
			glEnd();
		}
	}

	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// -----------------------------------------------------------------------------
// Draws the map in an image
// TODO: Factorize code with normal draw() and showMap() functions.
// TODO: Find a way to generate an arbitrary-sized image through tiled rendering
// -----------------------------------------------------------------------------
void MapPreviewCanvas::createImage(ArchiveEntry& ae, int width, int height)
{
	// Find extents of map
	Vertex m_min(999999.0, 999999.0);
	Vertex m_max(-999999.0, -999999.0);
	for (auto& vert : verts_)
	{
		if (vert.x < m_min.x)
			m_min.x = vert.x;
		if (vert.x > m_max.x)
			m_max.x = vert.x;
		if (vert.y < m_min.y)
			m_min.y = vert.y;
		if (vert.y > m_max.y)
			m_max.y = vert.y;
	}
	double mapwidth  = m_max.x - m_min.x;
	double mapheight = m_max.y - m_min.y;

	if (width == 0)
		width = -5;
	if (height == 0)
		height = -5;
	if (width < 0)
		width = mapwidth / abs(width);
	if (height < 0)
		height = mapheight / abs(height);

	// Setup colours
	auto col_save_background   = ColourConfiguration::colour("map_image_background");
	auto col_save_line_1s      = ColourConfiguration::colour("map_image_line_1s");
	auto col_save_line_2s      = ColourConfiguration::colour("map_image_line_2s");
	auto col_save_line_special = ColourConfiguration::colour("map_image_line_special");
	auto col_save_line_macro   = ColourConfiguration::colour("map_image_line_macro");

	// Setup OpenGL rigmarole
	GLuint tex_id, fbo_id;
	if (GLEW_ARB_framebuffer_object)
	{
		glGenTextures(1, &tex_id);
		OpenGL::Texture::bind(tex_id);
		// We don't use mipmaps, but OpenGL will refuse to attach
		// the texture to the framebuffer if they are not present
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		OpenGL::Texture::bind(0);
		glGenFramebuffersEXT(1, &fbo_id);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_id);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex_id, 0);
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}

	glViewport(0, 0, width, height);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(
		((double)col_save_background.r) / 255.f,
		((double)col_save_background.g) / 255.f,
		((double)col_save_background.b) / 255.f,
		((double)col_save_background.a) / 255.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Zoom/offset to show full map
	// Offset to center of map
	offset_ = { m_min.x + (mapwidth * 0.5), m_min.y + (mapheight * 0.5) };

	// Zoom to fit whole map
	double x_scale = ((double)width) / mapwidth;
	double y_scale = ((double)height) / mapheight;
	zoom_          = std::min<double>(x_scale, y_scale);
	zoom_ *= 0.95;

	// Translate to middle of canvas
	glTranslated(width >> 1, height >> 1, 0);

	// Zoom
	glScaled(zoom_, zoom_, 1);

	// Translate to offset
	glTranslated(-offset_.x, -offset_.y, 0);

	// Setup drawing
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLineWidth(map_image_thickness);
	glEnable(GL_LINE_SMOOTH);

	// Draw 2s lines
	for (auto& line : lines_)
	{
		if (!line.twosided)
			continue;

		// Check ends
		if (line.v1 >= verts_.size() || line.v2 >= verts_.size())
			continue;

		// Get vertices
		auto v1 = verts_[line.v1];
		auto v2 = verts_[line.v2];

		// Set colour
		if (line.special)
			OpenGL::setColour(col_save_line_special);
		else if (line.macro)
			OpenGL::setColour(col_save_line_macro);
		else if (line.twosided)
			OpenGL::setColour(col_save_line_2s);
		else
			OpenGL::setColour(col_save_line_1s);

		// Draw line
		glBegin(GL_LINES);
		glVertex2d(v1.x, v1.y);
		glVertex2d(v2.x, v2.y);
		glEnd();
	}

	// Draw 1s lines
	for (auto& line : lines_)
	{
		if (line.twosided)
			continue;

		// Check ends
		if (line.v1 >= verts_.size() || line.v2 >= verts_.size())
			continue;

		// Get vertices
		auto v1 = verts_[line.v1];
		auto v2 = verts_[line.v2];

		// Set colour
		if (line.special)
			OpenGL::setColour(col_save_line_special);
		else if (line.macro)
			OpenGL::setColour(col_save_line_macro);
		else
			OpenGL::setColour(col_save_line_1s);

		// Draw line
		glBegin(GL_LINES);
		glVertex2d(v1.x, v1.y);
		glVertex2d(v2.x, v2.y);
		glEnd();
	}

	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	vector<uint8_t> buffer(width * height * 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

	if (GLEW_ARB_framebuffer_object)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteTextures(1, &tex_id);
		glDeleteFramebuffersEXT(1, &fbo_id);
	}
	SImage img;
	img.setImageData(buffer, width, height, SImage::Type::RGBA);
	img.mirror(true);
	MemChunk mc;
	SIFormat::getFormat("png")->saveImage(img, mc);
	ae.importMemChunk(mc);
}

// -----------------------------------------------------------------------------
// Returns the number of (attached) vertices in the map
// -----------------------------------------------------------------------------
unsigned MapPreviewCanvas::nVertices()
{
	// Get list of used vertices
	vector<bool> v_used;
	for (unsigned a = 0; a < verts_.size(); a++)
		v_used.push_back(false);
	for (auto& line : lines_)
	{
		v_used[line.v1] = true;
		v_used[line.v2] = true;
	}

	// Get count of used vertices
	unsigned count = 0;
	for (auto&& a : v_used)
	{
		if (a)
			count++;
	}

	return count;
}

// -----------------------------------------------------------------------------
// Returns the width (in map units) of the map
// -----------------------------------------------------------------------------
unsigned MapPreviewCanvas::width()
{
	int min_x = wxINT32_MAX;
	int max_x = wxINT32_MIN;

	for (auto& vertex : verts_)
	{
		if (vertex.x < min_x)
			min_x = vertex.x;
		if (vertex.x > max_x)
			max_x = vertex.x;
	}

	return max_x - min_x;
}

// -----------------------------------------------------------------------------
// Returns the height (in map units) of the map
// -----------------------------------------------------------------------------
unsigned MapPreviewCanvas::height()
{
	int min_y = wxINT32_MAX;
	int max_y = wxINT32_MIN;

	for (auto& vertex : verts_)
	{
		if (vertex.y < min_y)
			min_y = vertex.y;
		if (vertex.y > max_y)
			max_y = vertex.y;
	}

	return max_y - min_y;
}
