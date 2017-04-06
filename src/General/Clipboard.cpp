
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Clipboard.cpp
 * Description: The SLADE Clipboard implementation
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
#include "App.h"
#include "Clipboard.h"
#include "Graphics/CTexture/CTexture.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/SectorBuilder.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
Clipboard* Clipboard::instance = NULL;


/*******************************************************************
 * CLIPBOARDITEM CLASS FUNCTIONS
 *******************************************************************/

/* ClipboardItem::ClipboardItem
 * ClipboardItem class constructor
 *******************************************************************/
ClipboardItem::ClipboardItem(int type)
{
	this->type = type;
}

/* ClipboardItem::~ClipboardItem
 * ClipboardItem class destructor
 *******************************************************************/
ClipboardItem::~ClipboardItem()
{
}


/*******************************************************************
 * ENTRYTREECLIPBOARDITEM CLASS FUNCTIONS
 *******************************************************************/

/* EntryTreeClipboardItem::EntryTreeClipboardItem
 * EntryTreeClipboardItem class constructor
 *******************************************************************/
EntryTreeClipboardItem::EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs)
	: ClipboardItem(CLIPBOARD_ENTRY_TREE)
{
	// Create tree
	tree = new ArchiveTreeNode();

	// Copy entries
	for (unsigned a = 0; a < entries.size(); a++)
		tree->addEntry(new ArchiveEntry(*(entries[a])));

	// Copy entries to system clipboard
	// (exports them as temp files and adds the paths to the clipboard)
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->Clear();
		wxFileDataObject* file = new wxFileDataObject();
		string tmp_directory = App::path("", App::Dir::Temp); // cache temp directory
		string file_dot = ".";
		for (unsigned a = 0; a < entries.size(); a++)
		{
			// Export to file
			string filename = tmp_directory + entries[a]->getName(true) + file_dot + entries[a]->getType()->getExtension();
			//string filename = entries[a]->getName(true) + "." + entries[a]->getType()->getExtension();
			//filename = App::path(filename, App::Dir::Temp);
			entries[a]->exportFile(filename);

			// Add to clipboard
			file->AddFile(filename);
		}
		wxTheClipboard->AddData(file);
		wxTheClipboard->Close();
	}

	// Copy dirs
	for (unsigned a = 0; a < dirs.size(); a++)
		tree->addChild(dirs[a]->clone());
}

/* EntryTreeClipboardItem::~EntryTreeClipboardItem
 * EntryTreeClipboardItem class destructor
 *******************************************************************/
EntryTreeClipboardItem::~EntryTreeClipboardItem()
{
	if (tree)
		delete tree;
}


/*******************************************************************
 * TEXTURECLIPBOARDITEM CLASS FUNCTIONS
 *******************************************************************/

/* TextureClipboardItem::TextureClipboardItem
 * TextureClipboardItem class constructor
 *******************************************************************/
TextureClipboardItem::TextureClipboardItem(CTexture* texture, Archive* parent) : ClipboardItem(CLIPBOARD_COMPOSITE_TEXTURE)
{
	// Create/copy texture
	this->texture = new CTexture();
	this->texture->copyTexture(texture);

	// Copy patch entries if possible
	for (unsigned a = 0; a < texture->nPatches(); a++)
	{
		ArchiveEntry* entry = texture->getPatch(a)->getPatchEntry(parent);

		// FIXME/TODO: Do something to handle patches that are defined
		// in TEXTURES rather than a discrete entry!
		if (entry == NULL)
			continue;

		// Don't copy patch if it has been already
		bool there = false;
		for (unsigned b = 0; b < patch_entries.size(); b++)
		{
			if (patch_entries[b]->getName() == entry->getName())
			{
				there = true;
				break;
			}
		}
		if (there) continue;

		// Copy patch entry
		if (entry)
			patch_entries.push_back(new ArchiveEntry(*entry));
	}
}

/* TextureClipboardItem::~TextureClipboardItem
 * TextureClipboardItem class destructor
 *******************************************************************/
TextureClipboardItem::~TextureClipboardItem()
{
	// Clean up
	delete texture;
	for (unsigned a = 0; a < patch_entries.size(); a++)
	{
		if (patch_entries[a])
			delete patch_entries[a];
	}
}

/* TextureClipboardItem::getPatchEntry
 * Returns the entry copy for the patch at [index] in the texture
 *******************************************************************/
ArchiveEntry* TextureClipboardItem::getPatchEntry(string patch)
{
	// Find copied patch entry with matching name
	for (unsigned a = 0; a < patch_entries.size(); a++)
	{
		if (S_CMPNOCASE(patch_entries[a]->getName(true).Truncate(8), patch))
			return patch_entries[a];
	}

	// Not found
	return NULL;
}


/*******************************************************************
 * MAPARCHCLIPBOARDITEM CLASS FUNCTIONS
 *******************************************************************/

/* MapArchClipboardItem::MapArchClipboardItem
 * MapArchClipboardItem class constructor
 *******************************************************************/
MapArchClipboardItem::MapArchClipboardItem() : ClipboardItem(CLIPBOARD_MAP_ARCH)
{
}

/* MapArchClipboardItem::~MapArchClipboardItem
 * MapArchClipboardItem class destructor
 *******************************************************************/
MapArchClipboardItem::~MapArchClipboardItem()
{
	for (unsigned a = 0; a < vertices.size(); a++)
		delete vertices[a];
	for (unsigned a = 0; a < lines.size(); a++)
		delete lines[a];
	for (unsigned a = 0; a < sectors.size(); a++)
		delete sectors[a];
}

/* MapArchClipboardItem::addLines
 * Copies [lines] and all related map structures
 *******************************************************************/
void MapArchClipboardItem::addLines(vector<MapLine*> lines)
{
	// Get sectors and sides to copy
	vector<MapSector*> copy_sectors;
	vector<MapSide*> copy_sides;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		MapSide* s1 = lines[a]->s1();
		MapSide* s2 = lines[a]->s2();

		// Front side
		if (s1)
		{
			copy_sides.push_back(s1);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s1->getSector()) == copy_sectors.end())
				copy_sectors.push_back(s1->getSector());
		}

		// Back side
		if (s2)
		{
			copy_sides.push_back(s2);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s2->getSector()) == copy_sectors.end())
				copy_sectors.push_back(s2->getSector());
		}
	}

	// Copy sectors
	for (unsigned a = 0; a < copy_sectors.size(); a++)
	{
		MapSector* copy = new MapSector(NULL);
		copy->copy(copy_sectors[a]);
		sectors.push_back(copy);
	}

	// Copy sides
	for (unsigned a = 0; a < copy_sides.size(); a++)
	{
		MapSide* copy = new MapSide();
		copy->copy(copy_sides[a]);

		// Set relative sector
		for (unsigned b = 0; b < copy_sectors.size(); b++)
		{
			if (copy_sides[a]->getSector() == copy_sectors[b])
			{
				copy->setSector(sectors[b]);
				break;
			}
		}

		sides.push_back(copy);
	}

	// Get vertices to copy (and determine midpoint)
	double min_x = 9999999;
	double max_x = -9999999;
	double min_y = 9999999;
	double max_y = -9999999;
	vector<MapVertex*> copy_verts;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		MapVertex* v1 = lines[a]->v1();
		MapVertex* v2 = lines[a]->v2();

		// Add vertices to copy list
		if (std::find(copy_verts.begin(), copy_verts.end(), v1) == copy_verts.end())
			copy_verts.push_back(v1);
		if (std::find(copy_verts.begin(), copy_verts.end(), v2) == copy_verts.end())
			copy_verts.push_back(v2);

		// Update min/max
		if (v1->xPos() < min_x) min_x = v1->xPos();
		if (v1->xPos() > max_x) max_x = v1->xPos();
		if (v1->yPos() < min_y) min_y = v1->yPos();
		if (v1->yPos() > max_y) max_y = v1->yPos();
		if (v2->xPos() < min_x) min_x = v2->xPos();
		if (v2->xPos() > max_x) max_x = v2->xPos();
		if (v2->yPos() < min_y) min_y = v2->yPos();
		if (v2->yPos() > max_y) max_y = v2->yPos();
	}

	// Determine midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	this->midpoint.set(mid_x, mid_y);

	// Copy vertices
	for (unsigned a = 0; a < copy_verts.size(); a++)
	{
		MapVertex* copy = new MapVertex(copy_verts[a]->xPos() - mid_x, copy_verts[a]->yPos() - mid_y);
		copy->copy(copy_verts[a]);
		vertices.push_back(copy);
	}

	// Copy lines
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Get relative sides
		MapSide* s1 = NULL;
		MapSide* s2 = NULL;
		bool s1_found = false;
		bool s2_found = !(lines[a]->s2());
		for (unsigned b = 0; b < copy_sides.size(); b++)
		{
			if (lines[a]->s1() == copy_sides[b])
			{
				s1 = sides[b];
				s1_found = true;
			}
			if (lines[a]->s2() == copy_sides[b])
			{
				s2 = sides[b];
				s2_found = true;
			}

			if (s1_found && s2_found)
				break;
		}

		// Get relative vertices
		MapVertex* v1 = NULL;
		MapVertex* v2 = NULL;
		for (unsigned b = 0; b < copy_verts.size(); b++)
		{
			if (lines[a]->v1() == copy_verts[b])
				v1 = vertices[b];
			if (lines[a]->v2() == copy_verts[b])
				v2 = vertices[b];

			if (v1 && v2)
				break;
		}

		// Copy line
		MapLine* copy = new MapLine(v1, v2, s1, s2);
		copy->copy(lines[a]);
		this->lines.push_back(copy);
	}
}

/* MapArchClipboardItem::getInfo
 * Returns a string with info on what items are copied
 *******************************************************************/
string MapArchClipboardItem::getInfo()
{
	return S_FMT("%lu Vertices, %lu Lines, %lu Sides and %lu Sectors",
	             vertices.size(), lines.size(), sides.size(), sectors.size());
}

/* MapArchClipboardItem::pasteToMap
 * Pastes copied architecture to [map] at [position]
 *******************************************************************/
vector<MapVertex*> MapArchClipboardItem::pasteToMap(SLADEMap* map, fpoint2_t position)
{
	std::map<MapVertex*, MapVertex*> vertMap;
	std::map<MapSector*, MapSector*> sectMap;
	std::map<MapSide*, MapSide*> sideMap;
	// Not used yet...
	// std::map<MapLine*, MapLine*> lineMap;

	// Add vertices
	vector<MapVertex*> new_verts;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		new_verts.push_back(map->createVertex(position.x + vertices[a]->xPos(), position.y + vertices[a]->yPos()));
		new_verts.back()->copy(vertices[a]);
		vertMap[vertices[a]] = new_verts.back();
	}

	// Add sectors
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		MapSector* new_sector = map->createSector();
		new_sector->copy(sectors[a]);
		sectMap[sectors[a]] = new_sector;
	}

	// Add sides
	int first_new_side = map->nSides();
	for (unsigned a = 0; a < sides.size(); a++)
	{
		// Get relative sector
		MapSector* sector = findInMap(sectMap, sides[a]->getSector());

		MapSide* new_side = map->createSide(sector);
		new_side->copy(sides[a]);
		sideMap[sides[a]] = new_side;
	}

	// Add lines
	int first_new_line = map->nLines();
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// Get relative vertices
		MapVertex* v1 = findInMap(vertMap, lines[a]->v1());
		MapVertex* v2 = findInMap(vertMap, lines[a]->v2());

		if (!v1)
		{
			LOG_MESSAGE(1, "no v1");
			continue;
		}
		if (!v2)
		{
			LOG_MESSAGE(1, "no v2");
		}

		MapLine* newline = map->createLine(v1, v2, true);
		newline->copy(lines[a]);

		// Set relative sides
		MapSide* newS1 = findInMap(sideMap, lines[a]->s1());
		MapSide* newS2 = findInMap(sideMap, lines[a]->s2());
		if (newS1)
			newline->setS1(newS1);
		if (newS2)
			newline->setS2(newS2);

		// Set important flags (needed when copying from Doom/Hexen format to UDMF)
		// Won't be needed when proper map format conversion stuff is implemented
		theGameConfiguration->setLineBasicFlag(
			"twosided",
			newline,
			map->currentFormat(),
			(newS1 && newS2)
		);
		theGameConfiguration->setLineBasicFlag(
			"blocking",
			newline,
			map->currentFormat(),
			!newS2
		);
	}

	// TODO:
	// - Split lines
	// - Merge lines

	//// Fix sector references
	//// TODO: figure out what lines are 'outside' on copy, only fix said lines
	//for (unsigned a = first_new_line; a < map->nLines(); a++)
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

/* MapArchClipboardItem::getLines
 * Adds all copied lines to [list]
 *******************************************************************/
void MapArchClipboardItem::getLines(vector<MapLine*>& list)
{
	for (unsigned a = 0; a < lines.size(); a++)
		list.push_back(lines[a]);
}


/*******************************************************************
 * MAPTHINGSCLIPBOARDITEM CLASS FUNCTIONS
 *******************************************************************/

/* MapThingsClipboardItem::MapThingsClipboardItem
 * MapThingsClipboardItem class constructor
 *******************************************************************/
MapThingsClipboardItem::MapThingsClipboardItem() : ClipboardItem(CLIPBOARD_MAP_THINGS)
{
}

/* MapThingsClipboardItem::~MapThingsClipboardItem
 * MapThingsClipboardItem class destructor
 *******************************************************************/
MapThingsClipboardItem::~MapThingsClipboardItem()
{
	for (unsigned a = 0; a < things.size(); a++)
		delete things[a];
}

/* MapThingsClipboardItem::addThings
 * Copies [things]
 *******************************************************************/
void MapThingsClipboardItem::addThings(vector<MapThing*>& things)
{
	// Copy things
	double min_x = 99999999;
	double min_y = 99999999;
	double max_x = -99999999;
	double max_y = -99999999;
	for (unsigned a = 0; a < things.size(); a++)
	{
		MapThing* copy_thing = new MapThing();
		copy_thing->copy(things[a]);
		this->things.push_back(copy_thing);

		if (things[a]->xPos() < min_x) min_x = things[a]->xPos();
		if (things[a]->yPos() < min_y) min_y = things[a]->yPos();
		if (things[a]->xPos() > max_x) max_x = things[a]->xPos();
		if (things[a]->yPos() > max_y) max_y = things[a]->yPos();
	}

	// Get midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	this->midpoint.set(mid_x, mid_y);

	// Adjust thing positions
	for (unsigned a = 0; a < this->things.size(); a++)
	{
		MapThing* thing = this->things[a];
		thing->setPos(thing->xPos() - mid_x, thing->yPos() - mid_y);
	}
}

/* MapThingsClipboardItem::getInfo
 * Returns a string with info on what items are copied
 *******************************************************************/
string MapThingsClipboardItem::getInfo()
{
	return S_FMT("%lu Things", things.size());
}

/* MapThingsClipboardItem::pasteToMap
 * Pastes copied things to [map] at [position]
 *******************************************************************/
void MapThingsClipboardItem::pasteToMap(SLADEMap* map, fpoint2_t position)
{
	for (unsigned a = 0; a < things.size(); a++)
	{
		MapThing* newthing = map->createThing(0, 0);
		newthing->copy(things[a]);
		newthing->setPos(position.x + things[a]->xPos(), position.y + things[a]->yPos());
	}
}

/* MapThingsClipboardItem::getLines
 * Adds all copied things to [list]
 *******************************************************************/
void MapThingsClipboardItem::getThings(vector<MapThing*>& list)
{
	for (unsigned a = 0; a < things.size(); a++)
	{
		list.push_back(things[a]);
	}
}


/*******************************************************************
 * CLIPBOARD CLASS FUNCTIONS
 *******************************************************************/

/* Clipboard::Clipboard
 * Clipboard class constructor
 *******************************************************************/
Clipboard::Clipboard()
{
}

/* Clipboard::~Clipboard
 * Clipboard class destructor
 *******************************************************************/
Clipboard::~Clipboard()
{
	for (uint32_t a = 0; a < items.size(); a++)
		delete items[a];
}

/* Clipboard::getItem
 * Returns the item at index or NULL if index is out of bounds
 *******************************************************************/
ClipboardItem* Clipboard::getItem(uint32_t index)
{
	if (index >= items.size())
		return NULL;
	else
		return items[index];
}

/* Clipboard::clear
 * Clears all clipboard items
 *******************************************************************/
void Clipboard::clear()
{
	for (uint32_t a = 0; a < items.size(); a++)
		delete items[a];

	items.clear();
}

/* Clipboard::addItem
 * Adds an item to the clipboard. Returns false if item is invalid
 *******************************************************************/
bool Clipboard::addItem(ClipboardItem* item)
{
	// Clear current clipboard contents
	clear();

	if (!item)
		return false;

	items.push_back(item);
	return true;
}

/* Clipboard::addItems
 * Adds multiple items to the clipboard
 *******************************************************************/
bool Clipboard::addItems(vector<ClipboardItem*>& items)
{
	// Clear current clipboard contents
	clear();

	for (unsigned a = 0; a < items.size(); a++)
	{
		if (items[a])
			this->items.push_back(items[a]);
	}

	return true;
}
