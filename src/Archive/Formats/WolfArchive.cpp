
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WolfArchive.cpp
// Description: WolfArchive, archive class to handle Wolf 3D data.
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
#include "WolfArchive.h"
#include "General/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"



// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns the full path of a given file with the correct casing for the
// filename. On Windws systems, filenames are case-insensitive, so the source
// filename is returned as-is. On other systems, we instead take only the path
// (assumed to be correct, since we got it from attempting to open a file that
// does exist) and then we iterate through all of the directory's files until we
// find the first one whose name matches.
// -----------------------------------------------------------------------------
string findFileCasing(const StrUtil::Path& filename)
{
#ifdef _WIN32
	return string{ filename.fileName() };
#else
	string path{ filename.path() };
	wxDir  dir(path);
	if (!dir.IsOpened())
	{
		Log::error("No directory at path {}. This shouldn't happen.", path);
		return "";
	}

	wxString found;
	bool     cont = dir.GetFirst(&found);
	while (cont)
	{
		if (StrUtil::equalCI(WxUtils::strToView(found), filename.fileName()))
			return (dir.GetNameWithSep() + found).ToStdString();
		cont = dir.GetNext(&found);
	}

	return "";
#endif
}

// -----------------------------------------------------------------------------
// Returns a Wolf constant depending on the size of the archive.
// Anyone who finds that the Doom source code is hacky should take a look at how
// Wolf3D was coded. It's a wonder it works at all.
// -----------------------------------------------------------------------------
enum WolfConstants
{
	NUMTILE8,
	STARTPICS,
	STARTPICM,
	STARTSPRITES,
	STARTTILE8 = STARTPICM,
	STARTPAL,
	ENDPAL,
	TITLE1PIC,
	TITLE2PIC,
	ENDSCREEN1PIC,
	ENDSCREEN9PIC,
	IDGUYS1PIC,
	IDGUYS2PIC,
	PLACEHOLDER,
};
#define return7(a, b, c, d, e, f, g, h) \
	switch (a)                          \
	{                                   \
	case 0: return b;                   \
	case 1: return c;                   \
	case 2: return d;                   \
	case 3: return e;                   \
	case 4: return f;                   \
	case 5: return g;                   \
	case 6: return h;                   \
	}
size_t WolfConstant(int name, size_t numlumps)
{
	int game = 0; // 0: wolf shareware V, 1: wolf shareware E, 2: wolf shareware ?
				  // 3: wolf full V, 4: wolf fulll E,
				  // 5: spear demo V, 6: spear full V
				  // There's also a GFXV_SDM, but with the same numlumps as GFXE_WL6 so screw it.
	switch (numlumps)
	{
	case 133: game = 5; break; // GFXV_SDM
	case 149: game = 3; break; // GFXV_WL6
	case 156: game = 2; break; // It's the version I have but it's not in the Wolf source code...
	case 169: game = 6; break; // GFXV_SOD
	case 414: game = 4; break; // GFXE_WL6: Just a mess of chunks without anything usable
	case 556: game = 0; break; // GFXV_WL1
	case 558: game = 1; break; // GFXE_WL1
	default: return 0;
	}
	switch (name) //	  VW1, EW1, ?W1, VW6, EW6, SDM, SOD
	{
	case STARTPICS: return7(game, 3, 3, 3, 3, 0, 3, 3) break;
	case STARTPICM: return7(game, 139, 142, 147, 135, 0, 128, 150) break;
	case NUMTILE8: return7(game, 72, 72, 72, 72, 0, 72, 72) break;
	case STARTPAL: return7(game, 0, 0, 0, 0, 0, 131, 153) break;
	case ENDPAL: return7(game, 0, 0, 0, 0, 0, 131, 163) break;
	case TITLE1PIC: return7(game, 0, 0, 0, 0, 0, 74, 79) break;
	case TITLE2PIC: return7(game, 0, 0, 0, 0, 0, 75, 80) break;
	case ENDSCREEN1PIC: return7(game, 0, 0, 0, 0, 0, 0, 81) break;
	case ENDSCREEN9PIC: return7(game, 0, 0, 0, 0, 0, 0, 89) break;
	case IDGUYS1PIC: return7(game, 0, 0, 0, 0, 0, 0, 93) break;
	case IDGUYS2PIC: return7(game, 0, 0, 0, 0, 0, 0, 94) break;
	}
	return 0;
}

// -----------------------------------------------------------------------------
// Looks for the string naming the song towards the end of the file.
// Returns an empty string if nothing is found.
// -----------------------------------------------------------------------------
string searchIMFName(MemChunk& mc)
{
	char tmp[17];
	char tmp2[65];
	tmp[16]  = 0;
	tmp2[64] = 0;

	string ret      = "";
	string fullname = "";
	if (mc.size() >= 88u)
	{
		uint16_t nameOffset = mc.readL16(0) + 4u;
		// Shareware stubs
		if (nameOffset == 4)
		{
			memcpy(tmp, &mc[2], 16);
			ret = tmp;

			memcpy(tmp2, &mc[18], 64);
			fullname = tmp2;
		}
		else if (mc.size() > nameOffset + 80u)
		{
			memcpy(tmp, &mc[nameOffset], 16);
			ret = tmp;

			memcpy(tmp2, &mc[nameOffset + 16], 64);
			fullname = tmp2;
		}

		if (ret.empty() || ret.length() > 12 || !StrUtil::endsWith(fullname, "IMF"))
			return "";
	}
	return ret;
}

// -----------------------------------------------------------------------------
// Adds height and width information to a picture. Needed because Wolf3D is just
// that much of a horrible hacky mess.
// -----------------------------------------------------------------------------
void addWolfPicHeader(ArchiveEntry* entry, uint16_t width, uint16_t height)
{
	if (!entry)
		return;

	auto& mc = entry->data();
	if (mc.size() == 0)
		return;

	uint32_t newsize = mc.size() + 4;
	uint8_t* newdata = new uint8_t[newsize];

	newdata[0] = (uint8_t)(width & 0xFF);
	newdata[1] = (uint8_t)(width >> 8);
	newdata[2] = (uint8_t)(height & 0xFF);
	newdata[3] = (uint8_t)(height >> 8);

	for (size_t i = 0; 4 + i < newsize; ++i)
	{
		newdata[4 + i] = mc[i];
	}
	// mc.clear();
	entry->importMem(newdata, newsize);
	delete[] newdata;
}

// -----------------------------------------------------------------------------
// Automatizes this: http://zdoom.org/wiki/Using_OPL_music_in_ZDoom
// -----------------------------------------------------------------------------
void addIMFHeader(ArchiveEntry* entry)
{
	if (!entry)
		return;

	auto& mc = entry->data();
	if (mc.size() == 0)
		return;

	uint32_t newsize = mc.size() + 9;
	uint8_t  start   = 0;
	if (mc[0] | mc[1])
	{
		// non-zero start
		newsize += 2;
		start = 2;
	}
	else
		newsize += 4;

	uint8_t* newdata = new uint8_t[newsize];
	newdata[0]       = 'A';
	newdata[1]       = 'D';
	newdata[2]       = 'L';
	newdata[3]       = 'I';
	newdata[4]       = 'B';
	newdata[5]       = 1;
	newdata[6]       = 0;
	newdata[7]       = 0;
	newdata[8]       = 1;
	if (mc[0] | mc[1])
	{
		newdata[9]  = mc[0];
		newdata[10] = mc[1];
		newdata[11] = 0;
		newdata[12] = 0;
	}
	else
	{
		newdata[9]  = 0;
		newdata[10] = 0;
		newdata[11] = 0;
		newdata[12] = 0;
	}
	for (size_t i = 0; ((i + start < mc.size()) && (13 + i < newsize)); ++i)
	{
		newdata[13 + i] = mc[i + start];
	}
	// mc.clear();
	entry->importMem(newdata, newsize);
	delete[] newdata;
}

// -----------------------------------------------------------------------------
// Needed to read VGAGRAPH content.
// Adapted from Wolf3D code, but with dead code removed from it.
// -----------------------------------------------------------------------------
struct HuffNode
{
	uint16_t bit0, bit1; // 0-255 is a character, > is a pointer to a node
};
void expandWolfGraphLump(ArchiveEntry* entry, size_t lumpnum, size_t numlumps, HuffNode* hufftable)
{
	if (!entry || entry->size() == 0)
		return;

	size_t         expanded; // expanded size
	const uint8_t* source = entry->rawData();


	if (lumpnum == WolfConstant(STARTTILE8, numlumps))
		expanded = 64 * WolfConstant(NUMTILE8, numlumps);
	else
	{
		expanded = *(uint32_t*)source;
		source += 4; // skip over length
	}

	if (expanded == 0 || expanded > 65000)
	{
		Log::error("ExpandWolfGraphLump: invalid expanded size in entry {}", lumpnum);
		return;
	}

	uint8_t*  dest = new uint8_t[expanded];
	uint8_t * end, *start;
	HuffNode *headptr, *huffptr;

	headptr = hufftable + 254; // head node is always node 254

	size_t written = 0;

	end   = dest + expanded;
	start = dest;

	uint8_t  val  = *source++;
	uint8_t  mask = 1;
	uint16_t nodeval;
	huffptr = headptr;
	while (true)
	{
		if (!(val & mask))
			nodeval = huffptr->bit0;
		else
			nodeval = huffptr->bit1;
		if (mask == 0x80)
			val = *source++, mask = 1;
		else
			mask <<= 1;

		if (nodeval < 256)
		{
			*dest++ = (uint8_t)nodeval;
			written++;
			huffptr = headptr;
			if (dest >= end)
				break;
		}
		else if (nodeval < 512)
		{
			huffptr = hufftable + (nodeval - 256);
		}
		else
			Log::warning("ExpandWolfGraphLump: nodeval is out of control ({}) in entry {}", nodeval, lumpnum);
	}

	entry->importMem(start, expanded);
	delete[] start;
}
} // namespace


// -----------------------------------------------------------------------------
//
// WolfArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Gets a lump entry's offset
// Returns the lump entry's offset, or zero if it doesn't exist
// -----------------------------------------------------------------------------
uint32_t WolfArchive::getEntryOffset(ArchiveEntry* entry) const
{
	return uint32_t((int)entry->exProp("Offset"));
}

// -----------------------------------------------------------------------------
// Sets a lump entry's offset
// -----------------------------------------------------------------------------
void WolfArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset) const
{
	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads a Wolf format file from disk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchive::open(string_view filename)
{
	// Find wolf archive type
	StrUtil::Path fn1(filename);
	string        fn1_name = StrUtil::upper(fn1.fileName(false));
	bool          opened;
	if (fn1_name == "MAPHEAD" || fn1_name == "GAMEMAPS" || fn1_name == "MAPTEMP")
	{
		// MAPHEAD can be paried with either a GAMEMAPS (Carmack,RLEW) or MAPTEMP (RLEW)
		StrUtil::Path fn2(fn1);
		if (fn1_name == "MAPHEAD")
		{
			fn2.setFileName("GAMEMAPS");
			if (!FileUtil::fileExists(fn2.fullPath()))
				fn2.setFileName("MAPTEMP");
		}
		else
		{
			fn1.setFileName("MAPHEAD");
		}
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openMaps(head, data);
	}
	else if (fn1_name == "AUDIOHED" || fn1_name == "AUDIOT")
	{
		StrUtil::Path fn2(fn1);
		fn1.setFileName("AUDIOHED");
		fn2.setFileName("AUDIOT");
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openAudio(head, data);
	}
	else if (fn1_name == "VGAHEAD" || fn1_name == "VGAGRAPH" || fn1_name == "VGADICT")
	{
		StrUtil::Path fn2(fn1);
		StrUtil::Path fn3(fn1);
		fn1.setFileName("VGAHEAD");
		fn2.setFileName("VGAGRAPH");
		fn3.setFileName("VGADICT");
		MemChunk data, head, dict;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		dict.importFile(findFileCasing(fn3));
		opened = openGraph(head, data, dict);
	}
	else
	{
		// Read the file into a MemChunk
		MemChunk mc;
		if (!mc.importFile(filename))
		{
			Global::error = "Unable to open file. Make sure it isn't in use by another program.";
			return false;
		}
		// Load from MemChunk
		opened = open(mc);
	}

	if (opened)
	{
		// Update variables
		this->filename_.assign(filename.data(), filename.size());
		this->on_disk_ = true;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Reads VSWAP Wolf format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read Wolf header
	mc.seek(0, SEEK_SET);
	uint16_t num_chunks, num_lumps;
	mc.read(&num_chunks, 2);   // Number of chunks
	mc.read(&spritestart_, 2); // First sprite
	mc.read(&soundstart_, 2);  // First sound
	num_chunks   = wxINT16_SWAP_ON_BE(num_chunks);
	spritestart_ = wxINT16_SWAP_ON_BE(spritestart_);
	soundstart_  = wxINT16_SWAP_ON_BE(soundstart_);
	num_lumps    = num_chunks;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	vector<WolfHandle> pages(num_lumps);
	for (uint32_t d = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		UI::setSplashProgress((float)d / (float)num_chunks * 2.0f);

		// Read offset info
		uint32_t offset = 0;
		mc.read(&offset, 4); // Offset
		pages[d].offset = wxINT32_SWAP_ON_BE(offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (pages[d].offset != 0 && pages[d].offset < (unsigned)((num_lumps + 1) * 6))
		{
			Log::error("WolfArchive::open: Wolf archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt ";
			setMuted(false);
			return false;
		}
	}

	// Then read the sizes
	for (uint32_t d = 0, l = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		UI::setSplashProgress((float)(d + num_chunks) / (float)num_chunks * 2.0f);

		// Read size info
		uint16_t size = 0;
		mc.read(&size, 2); // Size
		size = wxINT16_SWAP_ON_BE(size);

		// Wolf chunks have no names, so just give them a number
		string name;
		if (d < spritestart_)
			name = fmt::format("WAL{:05d}", l);
		else if (d < soundstart_)
			name = fmt::format("SPR{:05d}", l - spritestart_);
		else
			name = fmt::format("SND{:05d}", l - soundstart_);

		++l;

		// Shareware versions do not include all lumps,
		// no need to bother with fakes
		if (pages[d].offset > 0)
		{
			// Digitized sounds can be made of multiple pages
			size_t e = d;
			if (d >= soundstart_ && size == 4096)
			{
				size_t fullsize = 4096;
				do
				{
					mc.read(&size, 2);
					size = wxINT16_SWAP_ON_BE(size);
					fullsize += size;
					++e;
				} while (size == 4096);
				size = fullsize;
			}

			// Create & setup lump
			auto nlump = std::make_shared<ArchiveEntry>(name, size);
			nlump->setLoaded(false);
			nlump->exProp("Offset") = (int)pages[d].offset;
			nlump->setState(ArchiveEntry::State::Unmodified);

			d = e;

			// Add to entry list
			rootDir()->addEntry(nlump);

			// If the lump data goes past the end of file,
			// the data file is invalid
			if (getEntryOffset(nlump.get()) + size > mc.size())
			{
				Log::error("WolfArchive::open: Wolf archive is invalid or corrupt");
				Global::error = "Archive is invalid and/or corrupt";
				setMuted(false);
				return false;
			}
		}
	}

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->size());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf AUDIOT/AUDIOHEAD format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchive::openAudio(MemChunk& head, MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.size() >> 2) - 1;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	auto     offsets = (const uint32_t*)head.data();
	MemChunk edata;

	// First try to determine where data type changes
	enum
	{
		SegmentPCSpeaker,
		SegmentAdLib,
		SegmentDigital,
		SegmentMusic
	};
	int                      current_seg   = SegmentPCSpeaker;
	static const char* const SEG_PREFIX[4] = { "PCS", "ADL", "SND", "MUS" };
	unsigned int             seg_ends[4]   = { 0, 0, 0, num_lumps };
	bool                     strip_tags    = true;
	// Method 1: Look for !ID! tags
	for (uint32_t d = 0; d < num_lumps && current_seg != SegmentMusic; d++)
	{
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size   = wxINT32_SWAP_ON_BE(offsets[d + 1]) - offset;

		// Read entry data if it isn't zero-sized
		if (size >= 4)
		{
			data.exportMemChunk(edata, offset, size);

			if (strncmp((const char*)&edata[size - 4], "!ID!", 4) == 0)
				seg_ends[current_seg++] = d;
		}
	}

	if (current_seg != SegmentMusic)
	{
		// Method 2: Heuristics - Find music and then assume there are the same number PC, Adlib, and Digital
		strip_tags = false;
		uint32_t d = num_lumps;
		while (d-- > 3)
		{
			uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
			uint32_t size   = wxINT32_SWAP_ON_BE(offsets[d + 1]) - offset;

			if (size <= 4)
				break;

			// Look to see if we have an IMF
			data.exportMemChunk(edata, offset, size);

			auto name = searchIMFName(edata);
			if (name.empty())
				break;
		}
		seg_ends[SegmentDigital]   = d;
		seg_ends[SegmentPCSpeaker] = d / 3;
		seg_ends[SegmentAdLib]     = seg_ends[SegmentPCSpeaker] * 2;
	}

	// Now we can actually process the chunks
	current_seg    = 0;
	uint32_t d_ofs = 0; // So that each segment starts counting at 0
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size   = wxINT32_SWAP_ON_BE(offsets[d + 1]) - offset;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			Log::error("WolfArchive::openAudio: Wolf archive is invalid or corrupt");
			Global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
			setMuted(false);
			return false;
		}

		// See if we need to remove !ID! tag from final chunk
		if (d == seg_ends[current_seg] && strip_tags)
		{
			size -= 4;
		}
		else if (d == seg_ends[current_seg] + 1)
		{
			d_ofs = seg_ends[current_seg] + 1;
			++current_seg;
		}

		//
		// Read entry data if it isn't zero-sized
		if (size > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, offset, size);
		}

		// Wolf chunks have no names, so just give them a number
		string name;
		if (current_seg == SegmentMusic)
			name = searchIMFName(edata);
		if (name.empty())
			name = fmt::format("{}{:05d}", SEG_PREFIX[current_seg], d - d_ofs);

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;

		// Detect entry type
		if (size > 0)
			nlump->importMemChunk(edata);
		EntryType::detectEntryType(nlump.get());

		// Add to entry list
		nlump->setState(ArchiveEntry::State::Unmodified);
		rootDir()->addEntry(nlump);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf GAMEMAPS/MAPHEAD format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchive::openMaps(MemChunk& head, MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.size() - 2) >> 2;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	auto offsets = (const uint32_t*)(2 + head.data());
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size   = 38;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			Log::error("WolfArchive::openMaps: Wolf archive is invalid or corrupt");
			Global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
			setMuted(false);
			return false;
		}

		if (offset == 0 && d > 0)
			continue;

		string name;
		for (size_t i = 0; i < 16; ++i)
		{
			name += data[offset + 22 + i];
			if (data[offset + 22 + i] == 0)
				break;
		}

		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);

		// Add map planes to entry list
		uint32_t planeofs[3];
		uint16_t planelen[3];
		planeofs[0] = data.readL32(offset);
		planeofs[1] = data.readL32(offset + 4);
		planeofs[2] = data.readL32(offset + 8);
		planelen[0] = data.readL16(offset + 12);
		planelen[1] = data.readL16(offset + 14);
		planelen[2] = data.readL16(offset + 16);
		for (int i = 0; i < 3; ++i)
		{
			name        = fmt::format("PLANE{}", i);
			auto nlump2 = std::make_shared<ArchiveEntry>(name, planelen[i]);
			nlump2->setLoaded(false);
			nlump2->exProp("Offset") = (int)planeofs[i];
			nlump2->setState(ArchiveEntry::State::Unmodified);
			rootDir()->addEntry(nlump2);
		}
	}

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, getEntryOffset(entry), entry->size());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf VGAGRAPH/VGAHEAD/VGADICT format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
#define WC(a) WolfConstant(a, num_lumps)
bool WolfArchive::openGraph(MemChunk& head, MemChunk& data, MemChunk& dict)
{
	// Check data was given
	if (!head.hasData() || !data.hasData() || !dict.hasData())
		return false;

	if (dict.size() != 1024)
	{
		Global::error = fmt::format(
			"WolfArchive::openGraph: VGADICT is improperly sized ({} bytes instead of 1024)", dict.size());
		return false;
	}
	HuffNode nodes[256];
	memcpy(nodes, dict.data(), 1024);

	// Read Wolf header file
	uint32_t num_lumps = (head.size() / 3) - 1;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = head.readL24((d * 3));

		// Compute size from next offset
		uint32_t size = /*((d == num_lumps - 1) ? data.getSize() - offset :*/ (head.readL24(((d + 1) * 3)) - offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			Log::error("WolfArchive::openGraph: Wolf archive is invalid or corrupt");
			Global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
			setMuted(false);
			return false;
		}

		// Wolf chunks have no names, so just give them a number
		string name;
		if (d == 0)
			name = "INF";
		else if (d == 1 || d == 2)
			name = "FNT";
		else if (d >= WC(STARTPICS))
		{
			if (d >= WC(STARTPAL) && d <= WC(ENDPAL))
				name = "PAL";
			else if (d == WC(TITLE1PIC) || d == WC(TITLE2PIC))
				name = "TIT";
			else if (d == WC(IDGUYS1PIC) || d == WC(IDGUYS2PIC))
				name = "IDG";
			else if (d >= WC(ENDSCREEN1PIC) && d <= WC(ENDSCREEN9PIC))
				name = "END";
			else if (d < WC(STARTPICM))
				name = "PIC";
			else if (d == WC(STARTTILE8))
				name = "TIL";
			else
				name = "LMP";
		}
		else
			name = "LMP";
		name += fmt::format("{:05d}", d);


		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk        edata;
	const uint16_t* pictable = nullptr;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, getEntryOffset(entry), entry->size());
			entry->importMemChunk(edata);
		}
		expandWolfGraphLump(entry, a, num_lumps, nodes);

		// Store pictable information
		if (a == 0)
			pictable = (uint16_t*)entry->rawData();
		else if (a >= WC(STARTPICS) && a < WC(STARTPICM))
		{
			size_t i = (a - WC(STARTPICS)) << 1;
			addWolfPicHeader(entry, pictable[i], pictable[i + 1]);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory,
// and update namespaces if needed
// -----------------------------------------------------------------------------
ArchiveEntry* WolfArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return nullptr;

	// Check if read-only
	if (isReadOnly())
		return nullptr;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

// -----------------------------------------------------------------------------
// Since there are no namespaces, just give the hot potato to the other function
// and call it a day.
// -----------------------------------------------------------------------------
ArchiveEntry* WolfArchive::addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, nullptr, copy);
}

// -----------------------------------------------------------------------------
// Wolf chunks have no names, so renaming is pointless.
// -----------------------------------------------------------------------------
bool WolfArchive::renameEntry(ArchiveEntry* entry, string_view name)
{
	return false;
}

// -----------------------------------------------------------------------------
// Writes the dat archive to a MemChunk
// Returns true if successful, false otherwise [Not implemented]
// -----------------------------------------------------------------------------
bool WolfArchive::write(MemChunk& mc, bool update)
{
	return false;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the datafile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open wadfile
	wxFile file(filename_);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		Log::error("WolfArchive::loadEntryData: Failed to open datfile {}", filename_);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Wolfenstein VSWAP archive
// -----------------------------------------------------------------------------
bool WolfArchive::isWolfArchive(MemChunk& mc)
{
	// Read Wolf header
	mc.seek(0, SEEK_SET);
	uint16_t num_lumps, sprites, sounds;
	mc.read(&num_lumps, 2); // Size
	num_lumps = wxINT16_SWAP_ON_BE(num_lumps);
	if (num_lumps == 0)
		return false;

	mc.read(&sprites, 2); // Sprites start
	mc.read(&sounds, 2);  // Sounds start
	sprites = wxINT16_SWAP_ON_BE(sprites);
	sounds  = wxINT16_SWAP_ON_BE(sounds);
	if (sprites > sounds)
		return false;

	// Read lump info
	uint32_t offset    = 0;
	uint16_t size      = 0;
	size_t   totalsize = 6 * (num_lumps + 1);
	size_t   pagesize  = (totalsize / 512) + ((totalsize % 512) ? 1 : 0);
	size_t   filesize  = mc.size();
	if (filesize < totalsize)
		return false;

	vector<WolfHandle> pages(num_lumps);
	uint32_t           lastoffset = 0;
	for (size_t a = 0; a < num_lumps; ++a)
	{
		mc.read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);
		if (offset < lastoffset || offset % 512)
			return false;
		lastoffset      = offset;
		pages[a].offset = offset;
	}
	uint16_t lastsize = 0;
	for (size_t b = 0; b < num_lumps; ++b)
	{
		mc.read(&size, 2);
		size = wxINT16_SWAP_ON_BE(size);
		pagesize += (size / 512) + ((size % 512) ? 1 : 0);
		pages[b].size = size;
		if (b > 0 && (pages[b - 1].offset + pages[b - 1].size) > pages[b].offset)
			return false;
		lastsize = size;
	}
	return ((pagesize * 512) <= filesize || filesize >= lastoffset + lastsize);
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Wolfenstein VSWAP archive
// -----------------------------------------------------------------------------
bool WolfArchive::isWolfArchive(const string& filename)
{
	// Find wolf archive type
	StrUtil::Path fn1(filename);
	string        fn1_name = StrUtil::upper(fn1.fileName(false));
	if (fn1_name == "MAPHEAD" || fn1_name == "GAMEMAPS" || fn1_name == "MAPTEMP")
	{
		StrUtil::Path fn2(fn1);
		fn1.setFileName("MAPHEAD");
		fn2.setFileName("GAMEMAPS");
		if (!(FileUtil::fileExists(findFileCasing(fn1)) && FileUtil::fileExists(findFileCasing(fn2))))
		{
			fn2.setFileName("MAPTEMP");
			return (FileUtil::fileExists(findFileCasing(fn1)) && FileUtil::fileExists(findFileCasing(fn2)));
		}
		return true;
	}
	else if (fn1_name == "AUDIOHED" || fn1_name == "AUDIOT")
	{
		StrUtil::Path fn2(fn1);
		fn1.setFileName("AUDIOHED");
		fn2.setFileName("AUDIOT");
		return (FileUtil::fileExists(findFileCasing(fn1)) && FileUtil::fileExists(findFileCasing(fn2)));
	}
	else if (fn1_name == "VGAHEAD" || fn1_name == "VGAGRAPH" || fn1_name == "VGADICT")
	{
		StrUtil::Path fn2(fn1);
		StrUtil::Path fn3(fn1);
		fn1.setFileName("VGAHEAD");
		fn2.setFileName("VGAGRAPH");
		fn3.setFileName("VGADICT");
		return (
			FileUtil::fileExists(findFileCasing(fn1)) && FileUtil::fileExists(findFileCasing(fn2))
			&& FileUtil::fileExists(findFileCasing(fn3)));
	}

	// else we have to deal with a VSWAP archive, which is the only self-contained type

	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Read Wolf header
	file.Seek(0, wxFromStart);
	uint16_t num_lumps, sprites, sounds;

	file.Read(&num_lumps, 2); // Size
	num_lumps = wxINT16_SWAP_ON_BE(num_lumps);
	if (num_lumps == 0)
		return false;

	file.Read(&sprites, 2); // Sprites start
	file.Read(&sounds, 2);  // Sounds start
	sprites = wxINT16_SWAP_ON_BE(sprites);
	sounds  = wxINT16_SWAP_ON_BE(sounds);
	if (sprites > sounds)
		return false;

	// Read lump info
	uint32_t offset    = 0;
	uint16_t size      = 0;
	size_t   totalsize = 6 * (num_lumps + 1);
	size_t   pagesize  = (totalsize / 512) + ((totalsize % 512) ? 1 : 0);
	size_t   filesize  = file.Length();
	if (filesize < totalsize)
		return false;

	vector<WolfHandle> pages(num_lumps);
	uint32_t           lastoffset = 0;
	for (size_t a = 0; a < num_lumps; ++a)
	{
		file.Read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);
		if (offset == 0)
		{
			// Empty entry (we're opening a shareware/demo archive)
		}
		else if (offset < lastoffset || offset % 512)
			return false;
		if (offset > 0)
			lastoffset = offset;
		pages[a].offset = offset;
	}
	uint16_t lastsize = 0;
	lastoffset        = pages[0].offset;
	for (size_t b = 0; b < num_lumps; ++b)
	{
		file.Read(&size, 2);
		if (pages[b].offset > 0)
		{
			size = wxINT16_SWAP_ON_BE(size);
			pagesize += (size / 512) + ((size % 512) ? 1 : 0);
			pages[b].size = size;
			if (b > 0 && lastoffset + lastsize > pages[b].offset)
				return false;
			lastoffset = pages[b].offset;
			lastsize   = size;
		}
		else
			pages[b].size = 0;
	}
	return ((pagesize * 512) <= filesize || filesize >= lastoffset + lastsize);
}

// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------
#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(addimfheader, 0, true)
{
	auto entries = MainEditor::currentEntrySelection();

	for (auto& entrie : entries)
		addIMFHeader(entrie);
}
