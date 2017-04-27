
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapPreviewCanvas.cpp
 * Description: OpenGL Canvas that shows a basic map preview, can
 *              also save the preview to an image
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
#include "MapPreviewCanvas.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "General/ColourConfiguration.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapThing.h"
#include "MapEditor/SLADEMap/MapVertex.h"
#include "OpenGL/GLTexture.h"
#include "Utility/Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Float, map_image_thickness, 1.5, CVAR_SAVE)
CVAR(Bool, map_view_things, true, CVAR_SAVE)


/*******************************************************************
 * MAPPREVIEWCANVAS CLASS FUNCTIONS
 *******************************************************************/

/* MapPreviewCanvas::MapPreviewCanvas
 * MapPreviewCanvas class constructor
 *******************************************************************/
MapPreviewCanvas::MapPreviewCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	zoom = 1;
	offset_x = 0;
	offset_y = 0;
	temp_archive = NULL;
	tex_thing = NULL;
	tex_loaded = false;
	n_sides = 0;
	n_sectors = 0;
}

/* MapPreviewCanvas::~MapPreviewCanvas
 * MapPreviewCanvas class destructor
 *******************************************************************/
MapPreviewCanvas::~MapPreviewCanvas()
{
	if (tex_thing) delete tex_thing;
}

/* MapPreviewCanvas::addVertex
 * Adds a vertex to the map data
 *******************************************************************/
void MapPreviewCanvas::addVertex(double x, double y)
{
	verts.push_back(mep_vertex_t(x, y));
}

/* MapPreviewCanvas::addLine
 * Adds a line to the map data
 *******************************************************************/
void MapPreviewCanvas::addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro)
{
	mep_line_t line(v1, v2);
	line.twosided = twosided;
	line.special = special;
	line.macro = macro;
	lines.push_back(line);
}

/* MapPreviewCanvas::addThing
 * Adds a thing to the map data
 *******************************************************************/
void MapPreviewCanvas::addThing(double x, double y)
{
	mep_thing_t thing;
	thing.x = x;
	thing.y = y;
	things.push_back(thing);
}

/* MapPreviewCanvas::openMap
 * Opens a map from a mapdesc_t
 *******************************************************************/
bool MapPreviewCanvas::openMap(Archive::mapdesc_t map)
{
	// All errors = invalid map
	Global::error = "Invalid map";

	// Check if this map is a pk3 map
	bool map_archive = false;
	if (map.archive)
	{
		map_archive = true;

		// Attempt to open entry as wad archive
		temp_archive = new WadArchive();
		if (!temp_archive->open(map.head))
		{
			delete temp_archive;
			return false;
		}

		// Detect maps
		vector<Archive::mapdesc_t> maps = temp_archive->detectMaps();

		// Set map if there are any in the archive
		if (maps.size() > 0)
			map = maps[0];
		else
			return false;
	}

	// Parse UDMF map
	if (map.format == MAP_UDMF)
	{
		ArchiveEntry* udmfdata = NULL;
		for (ArchiveEntry* mapentry = map.head; mapentry != map.end; mapentry = mapentry->nextEntry())
		{
			// Check entry type
			if (mapentry->getType() == EntryType::getType("udmf_textmap"))
			{
				udmfdata = mapentry;
				break;
			}
		}
		if (udmfdata == NULL)
			return false;

		// Start parsing
		Tokenizer tz;
		tz.openMem(udmfdata->getData(), udmfdata->getSize(), map.head->getName());

		// Get first token
		string token = tz.getToken();
		size_t vertcounter = 0, linecounter = 0, thingcounter = 0;
		while (!token.IsEmpty())
		{
			if (!token.CmpNoCase("namespace"))
			{
				//  skip till we reach the ';'
				do { token = tz.getToken(); }
				while (token.Cmp(";"));
			}
			else if (!token.CmpNoCase("vertex"))
			{
				// Get X and Y properties
				bool gotx = false;
				bool goty = false;
				double x = 0.;
				double y = 0.;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("x") || !token.CmpNoCase("y"))
					{
						bool isx = !token.CmpNoCase("x");
						token = tz.getToken();
						if (token.Cmp("="))
						{
							LOG_MESSAGE(1, "Bad syntax for vertex %i in UDMF map data", vertcounter);
							return false;
						}
						if (isx) x = tz.getDouble(), gotx = true;
						else y = tz.getDouble(), goty = true;
						// skip to end of declaration after each key
						do { token = tz.getToken(); }
						while (token.Cmp(";"));
					}
				}
				while (token.Cmp("}"));
				if (gotx && goty)
					addVertex(x, y);
				else
				{
					LOG_MESSAGE(1, "Wrong vertex %i in UDMF map data", vertcounter);
					return false;
				}
				vertcounter++;
			}
			else if (!token.CmpNoCase("linedef"))
			{
				bool special = false;
				bool twosided = false;
				bool gotv1 = false, gotv2 = false;
				size_t v1 = 0, v2 = 0;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("v1") || !token.CmpNoCase("v2"))
					{
						bool isv1 = !token.CmpNoCase("v1");
						token = tz.getToken();
						if (token.Cmp("="))
						{
							LOG_MESSAGE(1, "Bad syntax for linedef %i in UDMF map data", linecounter);
							return false;
						}
						if (isv1) v1 = tz.getInteger(), gotv1 = true;
						else v2 = tz.getInteger(), gotv2 = true;
						// skip to end of declaration after each key
						do { token = tz.getToken(); }
						while (token.Cmp(";"));
					}
					else if (!token.CmpNoCase("special"))
					{
						special = true;
						// skip to end of declaration after each key
						do { token = tz.getToken(); }
						while (token.Cmp(";"));
					}
					else if (!token.CmpNoCase("sideback"))
					{
						twosided = true;
						// skip to end of declaration after each key
						do { token = tz.getToken(); }
						while (token.Cmp(";"));
					}
				}
				while (token.Cmp("}"));
				if (gotv1 && gotv2)
					addLine(v1, v2, twosided, special);
				else
				{
					LOG_MESSAGE(1, "Wrong line %i in UDMF map data", linecounter);
					return false;
				}
				linecounter++;
			}
			else if (S_CMPNOCASE(token, "thing"))
			{
				// Get X and Y properties
				bool gotx = false;
				bool goty = false;
				double x = 0.;
				double y = 0.;
				do
				{
					token = tz.getToken();
					if (!token.CmpNoCase("x") || !token.CmpNoCase("y"))
					{
						bool isx = !token.CmpNoCase("x");
						token = tz.getToken();
						if (token.Cmp("="))
						{
							LOG_MESSAGE(1, "Bad syntax for thing %i in UDMF map data", vertcounter);
							return false;
						}
						if (isx) x = tz.getDouble(), gotx = true;
						else y = tz.getDouble(), goty = true;
						// skip to end of declaration after each key
						do { token = tz.getToken(); } while (token.Cmp(";"));
					}
				} while (token.Cmp("}"));
				if (gotx && goty)
					addThing(x, y);
				else
				{
					LOG_MESSAGE(1, "Wrong thing %i in UDMF map data", vertcounter);
					return false;
				}
				vertcounter++;
			}
			else
			{
				// Check for side or sector definition (increase counts)
				if (S_CMPNOCASE(token, "sidedef"))
					n_sides++;
				else if (S_CMPNOCASE(token, "sector"))
					n_sectors++;

				// map preview ignores sidedefs, sectors, comments,
				// unknown fields, etc. so skip to end of block
				do { token = tz.getToken(); }
				while (token.Cmp("}"));
			}
			// Iterate to next token
			token = tz.getToken();
		}
	}

	// Non-UDMF map
	if (map.format != MAP_UDMF)
	{
		// Read vertices (required)
		if (!readVertices(map.head, map.end, map.format))
			return false;

		// Read linedefs (required)
		if (!readLines(map.head, map.end, map.format))
			return false;

		// Read things
		if (map.format != MAP_UDMF)
			readThings(map.head, map.end, map.format);

		// Read sides & sectors (count only)
		ArchiveEntry* sidedefs = NULL;
		ArchiveEntry* sectors = NULL;
		while (map.head)
		{
			// Check entry type
			if (map.head->getType() == EntryType::getType("map_sidedefs"))
				sidedefs = map.head;
			if (map.head->getType() == EntryType::getType("map_sectors"))
				sectors = map.head;

			// Exit loop if we've reached the end of the map entries
			if (map.head == map.end)
				break;
			else
				map.head = map.head->nextEntry();
		}
		if (sidedefs && sectors)
		{
			// Doom64 map
			if (map.format != MAP_DOOM64)
			{
				n_sides = sidedefs->getSize() / 30;
				n_sectors = sectors->getSize() / 26;
			}

			// Doom/Hexen map
			else
			{
				n_sides = sidedefs->getSize() / 12;
				n_sectors = sectors->getSize() / 16;
			}
		}
	}

	// Clean up
	if (map_archive)
	{
		temp_archive->close();
		delete temp_archive;
		temp_archive = NULL;
	}

	// Refresh map
	Refresh();

	return true;
}

/* MapPreviewCanvas::readVertices
 * Reads non-UDMF vertex data
 *******************************************************************/
bool MapPreviewCanvas::readVertices(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format)
{
	// Find VERTEXES entry
	ArchiveEntry* vertexes = NULL;
	while (map_head)
	{
		// Check entry type
		if (map_head->getType() == EntryType::getType("map_vertexes"))
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
	MemChunk& mc = vertexes->getMCData();
	mc.seek(0, SEEK_SET);

	if (map_format == MAP_DOOM64)
	{
		doom64vertex_t v;
		while (1)
		{
			// Read vertex
			if (!mc.read(&v, 8))
				break;

			// Add vertex
			addVertex((double)v.x/65536, (double)v.y/65536);
		}
	}
	else
	{
		doomvertex_t v;
		while (1)
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

/* MapPreviewCanvas::readLines
 * Reads non-UDMF line data
 *******************************************************************/
bool MapPreviewCanvas::readLines(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format)
{
	// Find LINEDEFS entry
	ArchiveEntry* linedefs = NULL;
	while (map_head)
	{
		// Check entry type
		if (map_head->getType() == EntryType::getType("map_linedefs"))
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
	MemChunk& mc = linedefs->getMCData();
	mc.seek(0, SEEK_SET);
	if (map_format == MAP_DOOM)
	{
		while (1)
		{
			// Read line
			doomline_t l;
			if (!mc.read(&l, sizeof(doomline_t)))
				break;

			// Check properties
			bool special = false;
			bool twosided = false;
			if (l.side2 != 0xFFFF)
				twosided = true;
			if (l.type > 0)
				special = true;

			// Add line
			addLine(l.vertex1, l.vertex2, twosided, special);
		}
	}
	else if (map_format == MAP_DOOM64)
	{
		while (1)
		{
			// Read line
			doom64line_t l;
			if (!mc.read(&l, sizeof(doom64line_t)))
				break;

			// Check properties
			bool macro = false;
			bool special = false;
			bool twosided = false;
			if (l.side2  != 0xFFFF)
				twosided = true;
			if (l.type > 0)
			{
				if (l.type & 0x100)
					macro = true;
				else special = true;
			}

			// Add line
			addLine(l.vertex1, l.vertex2, twosided, special, macro);
		}
	}
	else if (map_format == MAP_HEXEN)
	{
		while (1)
		{
			// Read line
			hexenline_t l;
			if (!mc.read(&l, sizeof(hexenline_t)))
				break;

			// Check properties
			bool special = false;
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

/* MapPreviewCanvas::readThings
 * Reads non-UDMF thing data
 *******************************************************************/
bool MapPreviewCanvas::readThings(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format)
{
	// Find THINGS entry
	ArchiveEntry* things = NULL;
	while (map_head)
	{
		// Check entry type
		if (map_head->getType() == EntryType::getType("map_things"))
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
	if (map_format == MAP_DOOM)
	{
		doomthing_t* thng_data = (doomthing_t*)things->getData(true);
		unsigned nt = things->getSize() / sizeof(doomthing_t);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MAP_DOOM64)
	{
		doom64thing_t* thng_data = (doom64thing_t*)things->getData(true);
		unsigned nt = things->getSize() / sizeof(doom64thing_t);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}
	else if (map_format == MAP_HEXEN)
	{
		hexenthing_t* thng_data = (hexenthing_t*)things->getData(true);
		unsigned nt = things->getSize() / sizeof(hexenthing_t);
		for (size_t a = 0; a < nt; a++)
			addThing(thng_data[a].x, thng_data[a].y);
	}

	return true;
}

/* MapPreviewCanvas::clearMap
 * Clears map data
 *******************************************************************/
void MapPreviewCanvas::clearMap()
{
	verts.clear();
	lines.clear();
	things.clear();
}

/* MapPreviewCanvas::showMap
 * Adjusts zoom and offset to show the whole map
 *******************************************************************/
void MapPreviewCanvas::showMap()
{
	// Find extents of map
	mep_vertex_t m_min(999999.0, 999999.0);
	mep_vertex_t m_max(-999999.0, -999999.0);
	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (verts[a].x < m_min.x)
			m_min.x = verts[a].x;
		if (verts[a].x > m_max.x)
			m_max.x = verts[a].x;
		if (verts[a].y < m_min.y)
			m_min.y = verts[a].y;
		if (verts[a].y > m_max.y)
			m_max.y = verts[a].y;
	}

	// Offset to center of map
	double width = m_max.x - m_min.x;
	double height = m_max.y - m_min.y;
	offset_x = m_min.x + (width * 0.5);
	offset_y = m_min.y + (height * 0.5);

	// Zoom to fit whole map
	double x_scale = ((double)GetClientSize().x) / width;
	double y_scale = ((double)GetClientSize().y) / height;
	zoom = MIN(x_scale, y_scale);
	zoom *= 0.95;
}

/* MapPreviewCanvas::draw
 * Draws the map
 *******************************************************************/
void MapPreviewCanvas::draw()
{
	// Setup colours
	rgba_t col_view_background = ColourConfiguration::getColour("map_view_background");
	rgba_t col_view_line_1s = ColourConfiguration::getColour("map_view_line_1s");
	rgba_t col_view_line_2s = ColourConfiguration::getColour("map_view_line_2s");
	rgba_t col_view_line_special = ColourConfiguration::getColour("map_view_line_special");
	rgba_t col_view_line_macro = ColourConfiguration::getColour("map_view_line_macro");
	rgba_t col_view_thing = ColourConfiguration::getColour("map_view_thing");

	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, 0, GetSize().y, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(((double)col_view_background.r)/255.f, ((double)col_view_background.g)/255.f,
	             ((double)col_view_background.b)/255.f, ((double)col_view_background.a)/255.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Zoom/offset to show full map
	showMap();

	// Translate to middle of canvas
	glTranslated(GetSize().x * 0.5, GetSize().y * 0.5, 0);

	// Zoom
	glScaled(zoom, zoom, 1);

	// Translate to offset
	glTranslated(-offset_x, -offset_y, 0);

	// Setup drawing
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLineWidth(1.5f);
	glEnable(GL_LINE_SMOOTH);

	// Draw lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		mep_line_t line = lines[a];

		// Check ends
		if (line.v1 >= verts.size() || line.v2 >= verts.size())
			continue;

		// Get vertices
		mep_vertex_t v1 = verts[lines[a].v1];
		mep_vertex_t v2 = verts[lines[a].v2];

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
	if (!tex_loaded)
	{
		// Load thing texture
		SImage image;
		ArchiveEntry* entry = theArchiveManager->programResourceArchive()->entryAtPath("images/thing/normal_n.png");
		if (entry)
		{
			image.open(entry->getMCData());
			tex_thing = new GLTexture(false);
			tex_thing->setFilter(GLTexture::MIPMAP);
			tex_thing->loadImage(&image);
		}
		else
			tex_thing = NULL;

		tex_loaded = true;
	}

	// Draw things
	if (map_view_things)
	{
		OpenGL::setColour(col_view_thing);
		if (tex_thing)
		{
			double radius = 20;
			glEnable(GL_TEXTURE_2D);
			tex_thing->bind();
			for (unsigned a = 0; a < things.size(); a++)
			{
				glPushMatrix();
				glTranslated(things[a].x, things[a].y, 0);
				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);	glVertex2d(-radius, -radius);
				glTexCoord2f(0.0f, 1.0f);	glVertex2d(-radius, radius);
				glTexCoord2f(1.0f, 1.0f);	glVertex2d(radius, radius);
				glTexCoord2f(1.0f, 0.0f);	glVertex2d(radius, -radius);
				glEnd();
				glPopMatrix();
			}
		}
		else
		{
			glEnable(GL_POINT_SMOOTH);
			glPointSize(8.0f);
			glBegin(GL_POINTS);
			for (unsigned a = 0; a < things.size(); a++)
				glVertex2d(things[a].x, things[a].y);
			glEnd();
		}
	}

	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


/* MapPreviewCanvas::createImage
 * Draws the map in an image
 * TODO: Factorize code with normal draw() and showMap() functions.
 * TODO: Find a way to generate an arbitrary-sized image through
 * tiled rendering.
 *******************************************************************/
void MapPreviewCanvas::createImage(ArchiveEntry& ae, int width, int height)
{
	// Find extents of map
	mep_vertex_t m_min(999999.0, 999999.0);
	mep_vertex_t m_max(-999999.0, -999999.0);
	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (verts[a].x < m_min.x)
			m_min.x = verts[a].x;
		if (verts[a].x > m_max.x)
			m_max.x = verts[a].x;
		if (verts[a].y < m_min.y)
			m_min.y = verts[a].y;
		if (verts[a].y > m_max.y)
			m_max.y = verts[a].y;
	}
	double mapwidth = m_max.x - m_min.x;
	double mapheight = m_max.y - m_min.y;

	if (width == 0) width = -5;
	if (height == 0) height = -5;
	if (width < 0)
		width = mapwidth / abs(width);
	if (height < 0)
		height = mapheight / abs(height);

	// Setup colours
	rgba_t col_save_background = ColourConfiguration::getColour("map_image_background");
	rgba_t col_save_line_1s = ColourConfiguration::getColour("map_image_line_1s");
	rgba_t col_save_line_2s = ColourConfiguration::getColour("map_image_line_2s");
	rgba_t col_save_line_special = ColourConfiguration::getColour("map_image_line_special");
	rgba_t col_save_line_macro = ColourConfiguration::getColour("map_image_line_macro");

	// Setup OpenGL rigmarole
	GLuint texID, fboID;
	if (GLEW_ARB_framebuffer_object)
	{
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		// We don't use mipmaps, but OpenGL will refuse to attach
		// the texture to the framebuffer if they are not present
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glGenFramebuffersEXT(1, &fboID);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texID, 0);
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
	glClearColor(((double)col_save_background.r)/255.f, ((double)col_save_background.g)/255.f,
	             ((double)col_save_background.b)/255.f, ((double)col_save_background.a)/255.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Zoom/offset to show full map
	// Offset to center of map
	offset_x = m_min.x + (mapwidth * 0.5);
	offset_y = m_min.y + (mapheight * 0.5);

	// Zoom to fit whole map
	double x_scale = ((double)width) / mapwidth;
	double y_scale = ((double)height) / mapheight;
	zoom = MIN(x_scale, y_scale);
	zoom *= 0.95;

	// Translate to middle of canvas
	glTranslated(width>>1, height>>1, 0);

	// Zoom
	glScaled(zoom, zoom, 1);

	// Translate to offset
	glTranslated(-offset_x, -offset_y, 0);

	// Setup drawing
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLineWidth(map_image_thickness);
	glEnable(GL_LINE_SMOOTH);

	// Draw 2s lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		mep_line_t line = lines[a];

		if (!line.twosided)
			continue;

		// Check ends
		if (line.v1 >= verts.size() || line.v2 >= verts.size())
			continue;

		// Get vertices
		mep_vertex_t v1 = verts[lines[a].v1];
		mep_vertex_t v2 = verts[lines[a].v2];

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
	for (unsigned a = 0; a < lines.size(); a++)
	{
		mep_line_t line = lines[a];

		if (line.twosided)
			continue;

		// Check ends
		if (line.v1 >= verts.size() || line.v2 >= verts.size())
			continue;

		// Get vertices
		mep_vertex_t v1 = verts[lines[a].v1];
		mep_vertex_t v2 = verts[lines[a].v2];

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

	uint8_t* ImageBuffer = new uint8_t[width * height * 4];
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ImageBuffer);

	if (GLEW_ARB_framebuffer_object)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteTextures( 1, &texID );
		glDeleteFramebuffersEXT( 1, &fboID );
	}
	SImage img;
	img.setImageData(ImageBuffer, width, height, RGBA);
	img.mirror(true);
	MemChunk mc;
	SIFormat::getFormat("png")->saveImage(img, mc);
	ae.importMemChunk(mc);
}

/* MapPreviewCanvas::nVertices
 * Returns the number of (attached) vertices in the map
 *******************************************************************/
unsigned MapPreviewCanvas::nVertices()
{
	// Get list of used vertices
	vector<bool> v_used;
	for (unsigned a = 0; a < verts.size(); a++)
		v_used.push_back(false);
	for (unsigned a = 0; a < lines.size(); a++)
	{
		v_used[lines[a].v1] = true;
		v_used[lines[a].v2] = true;
	}

	// Get count of used vertices
	unsigned count = 0;
	for (unsigned a = 0; a < v_used.size(); a++)
	{
		if (v_used[a])
			count++;
	}

	return count;
}

/* MapPreviewCanvas::nSides
 * Returns the number of sides in the map
 *******************************************************************/
unsigned MapPreviewCanvas::nSides()
{
	return n_sides;
}

/* MapPreviewCanvas::nLines
 * Returns the number of lines in the map
 *******************************************************************/
unsigned MapPreviewCanvas::nLines()
{
	return lines.size();
}

/* MapPreviewCanvas::nSectors
 * Returns the number of sectors in the map
 *******************************************************************/
unsigned MapPreviewCanvas::nSectors()
{
	return n_sectors;
}

/* MapPreviewCanvas::nThings
 * Returns the number of things in the map
 *******************************************************************/
unsigned MapPreviewCanvas::nThings()
{
	return things.size();
}

/* MapPreviewCanvas::getWidth
 * Returns the width (in map units) of the map
 *******************************************************************/
unsigned MapPreviewCanvas::getWidth()
{
	int min_x = wxINT32_MAX;
	int max_x = wxINT32_MIN;

	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (verts[a].x < min_x)
			min_x = verts[a].x;
		if (verts[a].x > max_x)
			max_x = verts[a].x;
	}

	return max_x - min_x;
}

/* MapPreviewCanvas::getHeight
 * Returns the height (in map units) of the map
 *******************************************************************/
unsigned MapPreviewCanvas::getHeight()
{
	int min_y = wxINT32_MAX;
	int max_y = wxINT32_MIN;

	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (verts[a].y < min_y)
			min_y = verts[a].y;
		if (verts[a].y > max_y)
			max_y = verts[a].y;
	}

	return max_y - min_y;
}
