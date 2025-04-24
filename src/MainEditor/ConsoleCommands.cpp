
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ConsoleCommands.cpp
// Description: Various MainEditor-related console commands (usually working on
//              the currently open or selected entries)
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
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Console.h"
#include "General/Misc.h"
#include "MainEditor.h"
#include "UI/ArchivePanel.h"
#include "UI/EntryPanel/EntryPanel.h"
#include "Utility/Memory.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
void fixpngsrc(ArchiveEntry* entry)
{
	if (!entry)
		return;
	auto            source = entry->rawData();
	vector<uint8_t> data(entry->size());
	memcpy(data.data(), source, entry->size());

	// Last check that it's a PNG
	uint32_t header1 = memory::readB32(data.data(), 0);
	uint32_t header2 = memory::readB32(data.data(), 4);
	if (header1 != 0x89504E47 || header2 != 0x0D0A1A0A)
		return;

	// Loop through each chunk and recompute CRC
	uint32_t pointer      = 8;
	bool     neededchange = false;
	while (pointer < entry->size())
	{
		if (pointer + 12 > entry->size())
		{
			log::error("Entry {} cannot be repaired.", entry->name());
			return;
		}
		uint32_t chsz = memory::readB32(data.data(), pointer);
		if (pointer + 12 + chsz > entry->size())
		{
			log::error("Entry {} cannot be repaired.", entry->name());
			return;
		}
		uint32_t crc = misc::crc(data.data() + pointer + 4, 4 + chsz);
		if (crc != memory::readB32(data.data(), pointer + 8 + chsz))
		{
			log::error(
				"Chunk {:c}{:c}{:c}{:c} has bad CRC",
				data[pointer + 4],
				data[pointer + 5],
				data[pointer + 6],
				data[pointer + 7]);
			neededchange              = true;
			data[pointer + 8 + chsz]  = crc >> 24;
			data[pointer + 9 + chsz]  = (crc & 0x00ffffff) >> 16;
			data[pointer + 10 + chsz] = (crc & 0x0000ffff) >> 8;
			data[pointer + 11 + chsz] = (crc & 0x000000ff);
		}
		pointer += (chsz + 12);
	}
	// Import new data with fixed CRC
	if (neededchange)
	{
		entry->importMem(data.data(), entry->size());
	}
}

vector<ArchiveEntry*> searchEntries(string_view name)
{
	vector<ArchiveEntry*> entries;
	Archive*              archive = maineditor::currentArchive();
	ArchivePanel*         panel   = maineditor::currentArchivePanel();

	if (archive)
	{
		ArchiveSearchOptions options;
		options.search_subdirs = true;
		if (panel)
		{
			options.dir = panel->currentDir();
		}
		options.match_name = name;
		entries            = archive->findAll(options);
	}
	return entries;
}
} // namespace


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(palconv, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		meep->palConvert();
		meep->reloadCurrentPanel();
	}
}

CONSOLE_COMMAND(palconv64, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal    = meep->currentEntry();
		auto&         source = pal->data();
		uint8_t*      dest   = new uint8_t[(pal->size() / 2) * 3];
		for (size_t i = 0; i < pal->size() / 2; ++i)
		{
			uint8_t  r, g, b;
			uint16_t col      = source.readB16(2 * i);
			r                 = (col & 0xF800) >> 8;
			g                 = (col & 0x07C0) >> 3;
			b                 = (col & 0x003E) << 2;
			dest[(3 * i) + 0] = r;
			dest[(3 * i) + 1] = g;
			dest[(3 * i) + 2] = b;
		}
		pal->importMem(dest, (pal->size() / 2) * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(palconvpsx, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal    = meep->currentEntry();
		auto&         source = pal->data();
		uint8_t*      dest   = new uint8_t[(pal->size() / 2) * 3];
		for (size_t i = 0; i < pal->size() / 2; ++i)
		{
			// A1 B5 G5 R5, LE
			uint8_t  a, r, g, b;
			uint16_t col      = source.readL16(2 * i);
			a                 = (col & 0x8000) >> 15;
			b                 = (col & 0x7C00) >> 10;
			g                 = (col & 0x03E0) >> 5;
			r                 = (col & 0x001F);
			r                 = (r << 3) | (r >> 2);
			g                 = (g << 3) | (g >> 2);
			b                 = (b << 3) | (b >> 2);
			dest[(3 * i) + 0] = r;
			dest[(3 * i) + 1] = g;
			dest[(3 * i) + 2] = b;
		}
		pal->importMem(dest, (pal->size() / 2) * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertex32x, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  v32x   = meep->currentEntry();
		const uint8_t* source = v32x->rawData();
		uint8_t*       dest   = new uint8_t[v32x->size() / 2];
		for (size_t i = 0; i < v32x->size() / 4; ++i)
		{
			dest[2 * i + 0] = source[4 * i + 1];
			dest[2 * i + 1] = source[4 * i + 0];
		}
		v32x->importMem(dest, v32x->size() / 2);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertexpsx, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  vpsx   = meep->currentEntry();
		const uint8_t* source = vpsx->rawData();
		uint8_t*       dest   = new uint8_t[vpsx->size() / 2];
		for (size_t i = 0; i < vpsx->size() / 4; ++i)
		{
			dest[2 * i + 0] = source[4 * i + 2];
			dest[2 * i + 1] = source[4 * i + 3];
		}
		vpsx->importMem(dest, vpsx->size() / 2);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(lightspsxtopalette, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  lights  = meep->currentEntry();
		const uint8_t* source  = lights->rawData();
		size_t         entries = lights->size() / 4;
		uint8_t*       dest    = new uint8_t[entries * 3];
		for (size_t i = 0; i < entries; ++i)
		{
			dest[3 * i + 0] = source[4 * i + 0];
			dest[3 * i + 1] = source[4 * i + 1];
			dest[3 * i + 2] = source[4 * i + 2];
		}
		lights->importMem(dest, entries * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(find, 1, true)
{
	vector<ArchiveEntry*> entries = searchEntries(args[0]);

	string message;
	size_t count = entries.size();
	if (count > 0)
	{
		for (size_t i = 0; i < count; ++i)
		{
			message += entries[i]->path(true) + "\n";
		}
	}
	log::info("Found {} entr{}", count, count == 1 ? "y" : "ies\n" + message);
}

CONSOLE_COMMAND(ren, 2, true)
{
	Archive*              archive = maineditor::currentArchive();
	vector<ArchiveEntry*> entries = searchEntries(args[0]);
	if (!entries.empty())
	{
		size_t count = 0;
		for (auto& entry : entries)
		{
			// Rename filter logic
			string newname = entry->name();
			for (unsigned c = 0; c < args[1].size(); c++)
			{
				// Check character
				if (args[1][c] == '*')
					continue; // Skip if *
				else
				{
					// First check that we aren't past the end of the name
					if (c >= newname.size())
					{
						// If we are, pad it with spaces
						while (newname.size() <= c)
							newname += " ";
					}

					// Replace character
					newname[c] = args[1][c];
				}
			}

			if (archive->renameEntry(entry, newname))
				++count;
		}
		log::info("Renamed {} entr{}", count, count == 1 ? "y" : "ies");
	}
}

CONSOLE_COMMAND(cd, 1, true)
{
	Archive*      current = maineditor::currentArchive();
	ArchivePanel* panel   = maineditor::currentArchivePanel();

	if (current && panel)
	{
		ArchiveDir* dir    = panel->currentDir();
		ArchiveDir* newdir = current->dirAtPath(args[0], dir);
		if (newdir == nullptr)
		{
			if (args[0] == "..")
				newdir = dir->parent().get();
			else if (args[0] == "/" || args[0] == "\\")
				newdir = current->rootDir().get();
		}

		if (newdir)
		{
			panel->openDir(ArchiveDir::getShared(newdir));
		}
		else
		{
			log::error("Error: Trying to open nonexistant directory {}", args[0]);
		}
	}
}

CONSOLE_COMMAND(fixpngcrc, 0, true)
{
	auto selection = maineditor::currentEntrySelection();
	if (selection.empty())
	{
		log::info(1, "No entry selected");
		return;
	}
	for (auto& entry : selection)
	{
		if (entry->type()->formatId() == "img_png")
			fixpngsrc(entry);
	}
}
