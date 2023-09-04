
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "General/ColourConfiguration.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/PointSpriteBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "SLADEMap/MapFormat/Doom32XMapFormat.h"
#include "SLADEMap/MapFormat/Doom64MapFormat.h"
#include "SLADEMap/MapFormat/DoomMapFormat.h"
#include "SLADEMap/MapFormat/HexenMapFormat.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "Utility/Tokenizer.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;


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


MapPreviewCanvas::MapPreviewCanvas(wxWindow* parent, bool allow_zoom, bool allow_pan) : GLCanvas(parent)
{
	// Centered view
	setView({ true, true, false });

	// Mousewheel zoom
	if (allow_zoom)
		setupMousewheelZoom();

	// View Panning
	if (allow_pan)
		setupMousePanning();
}


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
	global::error = "Invalid map";

	// Init
	std::unique_ptr<Archive> temp_archive;

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
				n_sides_++;
				tz.advUntil("}");
			}

			// Sector
			else if (tz.checkNC("sector"))
			{
				// Just increase count
				n_sectors_++;
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
							log::error(wxString::Format("Bad syntax for vertex %i in UDMF map data", vertcounter));
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
					addVertex(x, y);
				else
				{
					log::error(wxString::Format("Wrong vertex %i in UDMF map data", vertcounter));
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
							log::error(wxString::Format("Bad syntax for thing %i in UDMF map data", thingcounter));
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
					addThing(x, y);
				else
				{
					log::error(wxString::Format("Wrong thing %i in UDMF map data", thingcounter));
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
							log::error(wxString::Format("Bad syntax for linedef %i in UDMF map data", linecounter));
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
					addLine(v1, v2, twosided, special);
				else
				{
					log::error(wxString::Format("Wrong line %i in UDMF map data", linecounter));
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

	view_init_ = false;

	// Refresh map
	Refresh();

	return true;
}

// -----------------------------------------------------------------------------
// Reads non-UDMF vertex data
// -----------------------------------------------------------------------------
bool MapPreviewCanvas::readVertices(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
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
	else if (map_format == MapFormat::Doom32X)
	{
		Doom32XMapFormat::Vertex32BE v;
		while (true)
		{
			// Read vertex
			if (!mc.read(&v, 8))
				break;

			// Add vertex
			addVertex((double)wxINT32_SWAP_ON_LE(v.x) / 65536, (double)wxINT32_SWAP_ON_LE(v.y) / 65536);
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
bool MapPreviewCanvas::readLines(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
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
bool MapPreviewCanvas::readThings(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format)
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
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Doom64)
	{
		auto     thng_data = reinterpret_cast<const Doom64MapFormat::Thing*>(things->rawData());
		unsigned nt        = things->size() / sizeof(Doom64MapFormat::Thing);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MapFormat::Hexen)
	{
		auto     thng_data = reinterpret_cast<const HexenMapFormat::Thing*>(things->rawData());
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
	if (vb_lines_)
		vb_lines_->clear();
	if (psb_things_)
		psb_things_->buffer().clear();
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

	// Fit in view
	view_.fitTo(BBox{ m_min.x, m_min.y, m_max.x, m_max.y }, 1.1);
	view_.zoom(0.95);
}

// -----------------------------------------------------------------------------
// Draws the map
// -----------------------------------------------------------------------------
void MapPreviewCanvas::draw()
{
	setBackground(BGStyle::Colour, colourconfig::colour("map_view_background"));

	// Update buffer if needed
	if (!vb_lines_ || vb_lines_->empty())
		updateLinesBuffer();

	// Zoom/offset to show full map
	if (!view_init_)
	{
		showMap();
		view_init_ = true;
	}

	// Setup drawing
	auto& shader = gl::LineBuffer::shader();
	view_.setupShader(shader);

	// Draw lines
	vb_lines_->draw();

	// Draw things
	if (map_view_things)
	{
		// Update buffer if needed
		if (!psb_things_ || psb_things_->buffer().size() == 0)
			updateThingsBuffer();

		// Draw things
		psb_things_->setPointRadius(20.0f);
		psb_things_->setColour(colourconfig::colour("map_view_thing").asVec4());
		psb_things_->draw(gl::PointSpriteType::Circle, &view_);
	}
}


// -----------------------------------------------------------------------------
// Draws the map in an image
// TODO: Factorize code with normal draw() and showMap() functions.
// TODO: Find a way to generate an arbitrary-sized image through tiled rendering
// -----------------------------------------------------------------------------
void MapPreviewCanvas::createImage(ArchiveEntry& ae, int width, int height) const
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
	auto col_save_background   = colourconfig::colour("map_image_background");
	auto col_save_line_1s      = colourconfig::colour("map_image_line_1s");
	auto col_save_line_2s      = colourconfig::colour("map_image_line_2s");
	auto col_save_line_special = colourconfig::colour("map_image_line_special");
	auto col_save_line_macro   = colourconfig::colour("map_image_line_macro");

	// Setup OpenGL rigmarole
	GLuint tex_id, fbo_id;
	if (gl::fboSupport())
	{
		glGenTextures(1, &tex_id);
		gl::Texture::bind(tex_id);
		// We don't use mipmaps, but OpenGL will refuse to attach
		// the texture to the framebuffer if they are not present
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		gl::Texture::bind(0);
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
	if (gl::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Zoom/offset to show full map
	// Offset to center of map
	Vec2d offset = { m_min.x + (mapwidth * 0.5), m_min.y + (mapheight * 0.5) };

	// Zoom to fit whole map
	double x_scale = ((double)width) / mapwidth;
	double y_scale = ((double)height) / mapheight;
	double zoom    = std::min<double>(x_scale, y_scale);
	zoom *= 0.95;

	// Translate to middle of canvas
	glTranslated(width >> 1, height >> 1, 0);

	// Zoom
	glScaled(zoom, zoom, 1);

	// Translate to offset
	glTranslated(-offset.x, -offset.y, 0);

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
			gl::setColour(col_save_line_special);
		else if (line.macro)
			gl::setColour(col_save_line_macro);
		else if (line.twosided)
			gl::setColour(col_save_line_2s);
		else
			gl::setColour(col_save_line_1s);

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
			gl::setColour(col_save_line_special);
		else if (line.macro)
			gl::setColour(col_save_line_macro);
		else
			gl::setColour(col_save_line_1s);

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

	if (gl::fboSupport())
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
unsigned MapPreviewCanvas::nVertices() const
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
unsigned MapPreviewCanvas::width() const
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
unsigned MapPreviewCanvas::height() const
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

// -----------------------------------------------------------------------------
// Updates the lines vertex buffer
// -----------------------------------------------------------------------------
void MapPreviewCanvas::updateLinesBuffer()
{
	// Setup colours
	auto col_view_line_1s      = colourconfig::colour("map_view_line_1s").asVec4();
	auto col_view_line_2s      = colourconfig::colour("map_view_line_2s").asVec4();
	auto col_view_line_special = colourconfig::colour("map_view_line_special").asVec4();
	auto col_view_line_macro   = colourconfig::colour("map_view_line_macro").asVec4();

	if (!vb_lines_)
		vb_lines_ = std::make_unique<gl::LineBuffer>();
	else
		vb_lines_->clear();

	gl::LineBuffer::Line lb_line;
	for (auto& line : lines_)
	{
		// Check ends
		if (line.v1 >= verts_.size() || line.v2 >= verts_.size())
			continue;

		// Set colour
		if (line.special)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_special;
		else if (line.macro)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_macro;
		else if (line.twosided)
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_2s;
		else
			lb_line.v1_colour = lb_line.v2_colour = col_view_line_1s;

		// Add to buffer
		lb_line.v1_pos_width = { verts_[line.v1].x, verts_[line.v1].y, 0.0f, line.twosided ? 1.5f : 2.0f };
		lb_line.v2_pos_width = { verts_[line.v2].x, verts_[line.v2].y, 0.0f, line.twosided ? 1.5f : 2.0f };
		vb_lines_->add(lb_line);
	}
}

// -----------------------------------------------------------------------------
// Updates the things vertex buffer
// -----------------------------------------------------------------------------
void MapPreviewCanvas::updateThingsBuffer()
{
	if (!psb_things_)
		psb_things_ = std::make_unique<gl::PointSpriteBuffer>();
	else
		psb_things_->buffer().clear();

	auto col = glm::vec4(1.0f);
	auto tc  = glm::vec2();
	for (auto& thing : things_)
		psb_things_->add({ { thing.x, thing.y }, col, tc });
	psb_things_->upload();
}
