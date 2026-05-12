
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WolfArchiveHandler.cpp
// Description: ArchiveFormatHandler for Wolf3D data
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
#include "WolfArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "UI/UI.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#ifndef _WIN32
#include "UI/WxUtils.h"
#endif

using namespace slade;



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
string findFileCasing(const strutil::Path& filename)
{
#ifdef _WIN32
	return string{ filename.fileName() };
#else
	string path{ filename.path() };
	wxDir  dir(path);
	if (!dir.IsOpened())
	{
		log::error("No directory at path {}. This shouldn't happen.", path);
		return "";
	}

	wxString found;
	bool     cont = dir.GetFirst(&found);
	while (cont)
	{
		if (strutil::equalCI(found.utf8_string(), filename.fileName()))
			return (dir.GetNameWithSep() + found).utf8_string();
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
	default:  return 0;
	}
	switch (name) //	  VW1, EW1, ?W1, VW6, EW6, SDM, SOD
	{
	case STARTPICS:     return7(game, 3, 3, 3, 3, 0, 3, 3) break;
	case STARTPICM:     return7(game, 139, 142, 147, 135, 0, 128, 150) break;
	case NUMTILE8:      return7(game, 72, 72, 72, 72, 0, 72, 72) break;
	case STARTPAL:      return7(game, 0, 0, 0, 0, 0, 131, 153) break;
	case ENDPAL:        return7(game, 0, 0, 0, 0, 0, 131, 163) break;
	case TITLE1PIC:     return7(game, 0, 0, 0, 0, 0, 74, 79) break;
	case TITLE2PIC:     return7(game, 0, 0, 0, 0, 0, 75, 80) break;
	case ENDSCREEN1PIC: return7(game, 0, 0, 0, 0, 0, 0, 81) break;
	case ENDSCREEN9PIC: return7(game, 0, 0, 0, 0, 0, 0, 89) break;
	case IDGUYS1PIC:    return7(game, 0, 0, 0, 0, 0, 0, 93) break;
	case IDGUYS2PIC:    return7(game, 0, 0, 0, 0, 0, 0, 94) break;
	}
	return 0;
}

// -----------------------------------------------------------------------------
// Looks for the string naming the song towards the end of the file.
// Returns an empty string if nothing is found.
// -----------------------------------------------------------------------------
string searchIMFName(const MemChunk& mc)
{
	char tmp[17];
	char tmp2[65];
	tmp[16]  = 0;
	tmp2[64] = 0;

	string ret;
	string fullname;
	if (mc.size() >= 88u)
	{
		uint16_t nameOffset = mc.readL16(0) + 4u;
		// Shareware stubs
		if (nameOffset == 4)
		{
			memcpy(tmp, mc.data() + 2, 16);
			ret = tmp;

			memcpy(tmp2, mc.data() + 18, 64);
			fullname = tmp2;
		}
		else if (mc.size() > nameOffset + 80u)
		{
			memcpy(tmp, mc.data() + nameOffset, 16);
			ret = tmp;

			memcpy(tmp2, mc.data() + nameOffset + 16, 64);
			fullname = tmp2;
		}

		if (ret.empty() || ret.length() > 12 || !strutil::endsWith(fullname, "IMF"))
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

	newdata[0] = static_cast<uint8_t>(width & 0xFF);
	newdata[1] = static_cast<uint8_t>(width >> 8);
	newdata[2] = static_cast<uint8_t>(height & 0xFF);
	newdata[3] = static_cast<uint8_t>(height >> 8);

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
		expanded = *reinterpret_cast<const uint32_t*>(source);
		source += 4; // skip over length
	}

	if (expanded == 0 || expanded > 65000)
	{
		log::error("ExpandWolfGraphLump: invalid expanded size in entry {}", lumpnum);
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
		{
			val  = *source++;
			mask = 1;
		}
		else
			mask <<= 1;

		if (nodeval < 256)
		{
			*dest++ = static_cast<uint8_t>(nodeval);
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
			log::warning("ExpandWolfGraphLump: nodeval is out of control ({}) in entry {}", nodeval, lumpnum);
	}

	entry->importMem(start, expanded);
	delete[] start;
}
} // namespace


// -----------------------------------------------------------------------------
//
// WolfArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads a Wolf format file from disk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::open(Archive& archive, string_view filename)
{
	// Find wolf archive type
	strutil::Path fn1(filename);
	string        fn1_name = strutil::upper(fn1.fileName(false));
	bool          opened;
	if (fn1_name == "MAPHEAD" || fn1_name == "GAMEMAPS" || fn1_name == "MAPTEMP")
	{
		// MAPHEAD can be paried with either a GAMEMAPS (Carmack,RLEW) or MAPTEMP (RLEW)
		strutil::Path fn2(fn1);
		if (fn1_name == "MAPHEAD")
		{
			fn2.setFileName("GAMEMAPS");
			if (!fileutil::fileExists(fn2.fullPath()))
				fn2.setFileName("MAPTEMP");
		}
		else
		{
			fn1.setFileName("MAPHEAD");
		}
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openMaps(archive, head, data);
	}
	else if (fn1_name == "AUDIOHED" || fn1_name == "AUDIOT")
	{
		strutil::Path fn2(fn1);
		fn1.setFileName("AUDIOHED");
		fn2.setFileName("AUDIOT");
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openAudio(archive, head, data);
	}
	else if (fn1_name == "VGAHEAD" || fn1_name == "VGAGRAPH" || fn1_name == "VGADICT")
	{
		strutil::Path fn2(fn1);
		strutil::Path fn3(fn1);
		fn1.setFileName("VGAHEAD");
		fn2.setFileName("VGAGRAPH");
		fn3.setFileName("VGADICT");
		MemChunk data, head, dict;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		dict.importFile(findFileCasing(fn3));
		opened = openGraph(archive, head, data, dict);
	}
	else
	{
		// Read the file into a MemChunk
		MemChunk mc;
		if (!mc.importFile(filename))
		{
			global::error = "Unable to open file. Make sure it isn't in use by another program.";
			return false;
		}
		// Load from MemChunk
		opened = open(archive, mc);
	}

	return opened;
}

// -----------------------------------------------------------------------------
// Reads VSWAP Wolf format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::open(Archive& archive, const MemChunk& mc)
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
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the offsets
	ui::setSplashProgressMessage("Reading Wolf archive data");
	vector<WolfHandle> pages(num_lumps);
	for (uint32_t d = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_chunks * 2);

		// Read offset info
		uint32_t offset = 0;
		mc.read(&offset, 4); // Offset
		pages[d].offset = wxINT32_SWAP_ON_BE(offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (pages[d].offset != 0 && pages[d].offset < static_cast<unsigned>((num_lumps + 1) * 6))
		{
			log::error("WolfArchiveHandler::open: Wolf archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt ";
			return false;
		}
	}

	// Then read the sizes
	for (uint32_t d = 0, l = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d + num_chunks, num_chunks * 2);

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

			// If the lump data goes past the end of file,
			// the data file is invalid
			if (pages[d].offset + size > mc.size())
			{
				log::error("WolfArchiveHandler::open: Wolf archive is invalid or corrupt");
				global::error = "Archive is invalid and/or corrupt";
				return false;
			}

			// Create & setup lump
			auto nlump = std::make_shared<ArchiveEntry>(name, size);
			nlump->setOffsetOnDisk(pages[d].offset);
			nlump->setSizeOnDisk();

			// Read entry data if it isn't zero-sized
			if (size > 0)
				nlump->importMemChunk(mc, pages[d].offset, size);

			nlump->setState(EntryState::Unmodified);

			d = e;

			// Add to entry list
			archive.rootDir()->addEntry(nlump);
		}
	}

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf AUDIOT/AUDIOHEAD format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::openAudio(Archive& archive, MemChunk& head, const MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.size() >> 2) - 1;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the offsets
	ui::setSplashProgressMessage("Reading Wolf archive data");
	auto     offsets = reinterpret_cast<const uint32_t*>(head.data());
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
		ui::setSplashProgress(d, num_lumps);

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size   = wxINT32_SWAP_ON_BE(offsets[d + 1]) - offset;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			log::error("WolfArchiveHandler::openAudio: Wolf archive is invalid or corrupt");
			global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
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
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk();

		// Detect entry type
		if (size > 0)
			nlump->importMemChunk(edata);
		EntryType::detectEntryType(*nlump);

		// Add to entry list
		nlump->setState(EntryState::Unmodified);
		archive.rootDir()->addEntry(nlump);
	}

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf GAMEMAPS/MAPHEAD format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::openMaps(Archive& archive, MemChunk& head, const MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.size() - 2) >> 2;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the offsets
	ui::setSplashProgressMessage("Reading Wolf archive data");
	auto offsets = reinterpret_cast<const uint32_t*>(2 + head.data());
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size   = 38;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			log::error("WolfArchiveHandler::openMaps: Wolf archive is invalid or corrupt");
			global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
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
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk();
		if (size > 0)
			nlump->importMemChunk(data, offset, size);
		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);

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
			nlump2->setOffsetOnDisk(planeofs[i]);
			nlump2->setSizeOnDisk();
			if (planelen[i] > 0)
				nlump2->importMemChunk(data, planeofs[i], planelen[i]);
			nlump2->setState(EntryState::Unmodified);
			archive.rootDir()->addEntry(nlump2);
		}
	}

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads Wolf VGAGRAPH/VGAHEAD/VGADICT format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
#define WC(a) WolfConstant(a, num_lumps)
bool WolfArchiveHandler::openGraph(Archive& archive, const MemChunk& head, const MemChunk& data, MemChunk& dict)
{
	// Check data was given
	if (!head.hasData() || !data.hasData() || !dict.hasData())
		return false;

	if (dict.size() != 1024)
	{
		global::error = fmt::format(
			"WolfArchiveHandler::openGraph: VGADICT is improperly sized ({} bytes instead of 1024)", dict.size());
		return false;
	}
	HuffNode nodes[256];
	memcpy(nodes, dict.data(), 1024);

	// Read Wolf header file
	uint32_t num_lumps = (head.size() / 3) - 1;
	spritestart_ = soundstart_ = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the offsets
	ui::setSplashProgressMessage("Reading Wolf archive data");
	const uint16_t* pictable = nullptr;
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read offset info
		uint32_t offset = head.readL24((d * 3));

		// Compute size from next offset
		uint32_t size = /*((d == num_lumps - 1) ? data.getSize() - offset :*/ (head.readL24(((d + 1) * 3)) - offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.size())
		{
			log::error("WolfArchiveHandler::openGraph: Wolf archive is invalid or corrupt");
			global::error = fmt::format("Archive is invalid and/or corrupt in entry {}", d);
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
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk();

		// Read entry data if it isn't zero-sized
		if (size > 0)
			nlump->importMemChunk(data, offset, size);
		expandWolfGraphLump(nlump.get(), d, num_lumps, nodes);

		// Store pictable information
		if (d == 0)
			pictable = reinterpret_cast<const uint16_t*>(nlump->rawData());
		else if (d >= WC(STARTPICS) && d < WC(STARTPICM))
		{
			size_t i = (d - WC(STARTPICS)) << 1;
			addWolfPicHeader(nlump.get(), pictable[i], pictable[i + 1]);
		}

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the dat archive to a MemChunk
// Returns true if successful, false otherwise [Not implemented]
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	return false;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Wolfenstein VSWAP archive
// -----------------------------------------------------------------------------
bool WolfArchiveHandler::isThisFormat(const MemChunk& mc)
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
bool WolfArchiveHandler::isThisFormat(const string& filename)
{
	// Find wolf archive type
	strutil::Path fn1(filename);
	string        fn1_name = strutil::upper(fn1.fileName(false));
	if (fn1_name == "MAPHEAD" || fn1_name == "GAMEMAPS" || fn1_name == "MAPTEMP")
	{
		strutil::Path fn2(fn1);
		fn1.setFileName("MAPHEAD");
		fn2.setFileName("GAMEMAPS");
		if (!(fileutil::fileExists(findFileCasing(fn1)) && fileutil::fileExists(findFileCasing(fn2))))
		{
			fn2.setFileName("MAPTEMP");
			return (fileutil::fileExists(findFileCasing(fn1)) && fileutil::fileExists(findFileCasing(fn2)));
		}
		return true;
	}
	else if (fn1_name == "AUDIOHED" || fn1_name == "AUDIOT")
	{
		strutil::Path fn2(fn1);
		fn1.setFileName("AUDIOHED");
		fn2.setFileName("AUDIOT");
		return (fileutil::fileExists(findFileCasing(fn1)) && fileutil::fileExists(findFileCasing(fn2)));
	}
	else if (fn1_name == "VGAHEAD" || fn1_name == "VGAGRAPH" || fn1_name == "VGADICT")
	{
		strutil::Path fn2(fn1);
		strutil::Path fn3(fn1);
		fn1.setFileName("VGAHEAD");
		fn2.setFileName("VGAGRAPH");
		fn3.setFileName("VGADICT");
		return (
			fileutil::fileExists(findFileCasing(fn1)) && fileutil::fileExists(findFileCasing(fn2))
			&& fileutil::fileExists(findFileCasing(fn3)));
	}

	// else we have to deal with a VSWAP archive, which is the only self-contained type

	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

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
#include "General/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(addimfheader, 0, true)
{
	auto entries = maineditor::currentEntrySelection();

	for (auto& entrie : entries)
		addIMFHeader(entrie);
}
