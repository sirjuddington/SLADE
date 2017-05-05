
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    WolfArchive.cpp
 * Description: WolfArchive, archive class to handle Wolf 3D data.
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
#include "WolfArchive.h"
#include "General/UI.h"

/*******************************************************************
 * ADDITIONAL FUNCTIONS
 *******************************************************************/

/* findFileCasing
 * Returns the full path of a given file with the correct casing for
 * the filename. On Windws systems, filenames are case-insensitive,
 * so the source filename is returned as-is. On other systems, we
 * instead take only the path (assumed to be correct, since we got
 * it from attempting to open a file that does exist) and then we
 * iterate through all of the directory's files until we find the
 * first one whose name matches.
 *******************************************************************/
string findFileCasing(wxFileName filename)
{
#ifdef _WIN32
	return filename.GetFullPath();
#else
	string path = filename.GetPath();
	wxDir dir(path);
	if (!dir.IsOpened())
	{
		LOG_MESSAGE(1, "Error: No directory at path %s. This shouldn't happen.");
		return "";
	}

	string found;
	bool cont = dir.GetFirst(&found);
	while (cont)
	{
		if (S_CMPNOCASE(found, filename.GetFullName()))
			return (dir.GetNameWithSep() + found);
		cont = dir.GetNext(&found);
	}

	return "";
#endif
}

/* WolfConstant
 * Returns a Wolf constant depending on the size of the archive.
 * Anyone who finds that the Doom source code is hacky should take
 * a look at how Wolf3D was coded. It's a wonder it works at all.
 *******************************************************************/
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
#define return7(a, b, c, d, e, f, g, h) switch(a) { case 0: return b; case 1: return c; case 2: return d; case 3: return e; case 4: return f; case 5: return g; case 6: return h; }
size_t WolfConstant(int name, size_t numlumps)
{
	int game = 0;	// 0: wolf shareware V, 1: wolf shareware E, 2: wolf shareware ?
					// 3: wolf full V, 4: wolf fulll E,
					// 5: spear demo V, 6: spear full V
					// There's also a GFXV_SDM, but with the same numlumps as GFXE_WL6 so screw it.
	switch (numlumps)
	{
	case 133: game = 5; break;	// GFXV_SDM
	case 149: game = 3; break;	// GFXV_WL6
	case 156: game = 2; break;	// It's the version I have but it's not in the Wolf source code...
	case 169: game = 6; break;	// GFXV_SOD
	case 414: game = 4; break;	// GFXE_WL6: Just a mess of chunks without anything usable
	case 556: game = 0; break;	// GFXV_WL1
	case 558: game = 1;	break;	// GFXE_WL1
	default: return 0;
	}
	switch (name)  					//	  VW1, EW1, ?W1, VW6, EW6, SDM, SOD
	{
	case STARTPICS:		return7(game,   3,   3,   3,   3,   0,   3,   3) break;
	case STARTPICM:		return7(game, 139, 142, 147, 135,   0, 128, 150) break;
	case NUMTILE8:		return7(game,  72,  72,  72,  72,   0,  72,  72) break;
	case STARTPAL:		return7(game,   0,   0,   0,   0,   0, 131, 153) break;
	case ENDPAL:		return7(game,   0,   0,   0,   0,   0, 131, 163) break;
	case TITLE1PIC:		return7(game,   0,   0,   0,   0,   0,  74,  79) break;
	case TITLE2PIC:		return7(game,   0,   0,   0,   0,   0,  75,  80) break;
	case ENDSCREEN1PIC:	return7(game,   0,   0,   0,   0,   0,   0,  81) break;
	case ENDSCREEN9PIC:	return7(game,   0,   0,   0,   0,   0,   0,  89) break;
	case IDGUYS1PIC:	return7(game,   0,   0,   0,   0,   0,   0,  93) break;
	case IDGUYS2PIC:	return7(game,   0,   0,   0,   0,   0,   0,  94) break;
	}
	return 0;
}

/* searchIMFName
 * Looks for the string naming the song towards the end of the file.
 * Returns an empty string if nothing is found.
 *******************************************************************/
string searchIMFName(MemChunk& mc)
{
	char tmp[17];
	char tmp2[65];
	tmp[16] = 0;
	tmp2[64] = 0;

	string ret = "";
	string fullname = "";
	if (mc.getSize() >= 88u)
	{
		uint16_t nameOffset = READ_L16(mc, 0)+4u;
		// Shareware stubs
		if (nameOffset == 4)
		{
			memcpy(tmp, &mc[2], 16);
			ret = tmp;

			memcpy(tmp2, &mc[18], 64);
			fullname = tmp2;
		}
		else if (mc.getSize() > nameOffset+80u)
		{
			memcpy(tmp, &mc[nameOffset], 16);
			ret = tmp;

			memcpy(tmp2, &mc[nameOffset + 16], 64);
			fullname = tmp2;
		}

		if (ret.IsEmpty() || ret.length() > 12 || !fullname.EndsWith("IMF"))
			return "";
	}
	return ret;
}

/* addWolfPicHeader
 * Adds height and width information to a picture. Needed because
 * Wolfenstein 3-D is just that much of a horrible hacky mess.
 *******************************************************************/
void addWolfPicHeader(ArchiveEntry* entry, uint16_t width, uint16_t height)
{
	if (!entry)
		return;

	MemChunk& mc = entry->getMCData();
	if (mc.getSize() == 0)
		return;

	uint32_t newsize = mc.getSize() + 4;
	uint8_t* newdata = new uint8_t[newsize];

	newdata[0] = (uint8_t) (width & 0xFF);
	newdata[1] = (uint8_t) (width>>8);
	newdata[2] = (uint8_t) (height & 0xFF);
	newdata[3] = (uint8_t) (height>>8);

	for (size_t i = 0; 4 + i < newsize; ++i)
	{
		newdata[4 + i] = mc[i];
	}
	//mc.clear();
	entry->importMem(newdata, newsize);
	delete[] newdata;
}

/* addIMFHeader
 * Automatizes this: http://zdoom.org/wiki/Using_OPL_music_in_ZDoom
 *******************************************************************/
void addIMFHeader(ArchiveEntry* entry)
{
	if (!entry)
		return;

	MemChunk& mc = entry->getMCData();
	if (mc.getSize() == 0)
		return;

	uint32_t newsize = mc.getSize() + 9;
	uint8_t start = 0;
	if (mc[0] | mc[1])
	{
		// non-zero start
		newsize += 2;
		start = 2;
	}
	else newsize += 4;

	uint8_t* newdata = new uint8_t[newsize];
	newdata[0] = 'A'; newdata[1] = 'D'; newdata[2] = 'L'; newdata[3] = 'I'; newdata[4] = 'B';
	newdata[5] = 1; newdata[6] = 0; newdata[7] = 0; newdata[8] = 1;
	if (mc[0] | mc[1])
	{
		newdata[9] = mc[0]; newdata[10] = mc[1]; newdata[11] = 0; newdata[12] = 0;
	}
	else
	{
		newdata[9] = 0; newdata[10] = 0; newdata[11] = 0; newdata[12] = 0;
	}
	for (size_t i = 0; ((i + start < mc.getSize()) && (13 + i < newsize)); ++i)
	{
		newdata[13 + i] = mc[i+start];
	}
	//mc.clear();
	entry->importMem(newdata, newsize);
	delete[] newdata;
}

/* ExpandWolfGraphLump
 * Needed to read VGAGRAPH content. Adapted from Wolf3D code, but
 * with dead code removed from it.
 *******************************************************************/
struct huffnode
{
	uint16_t bit0, bit1;	// 0-255 is a character, > is a pointer to a node
};
void ExpandWolfGraphLump (ArchiveEntry* entry, size_t lumpnum, size_t numlumps, huffnode* hufftable)
{
	if (!entry || entry->getSize() == 0)
		return;

	size_t expanded; // expanded size
	const uint8_t* source = entry->getData();


	if (lumpnum == WolfConstant(STARTTILE8, numlumps))
		expanded = 64 * WolfConstant(NUMTILE8, numlumps);
	else
	{
		expanded = *(uint32_t*)source;
		source += 4;			// skip over length
	}

	if (expanded == 0 || expanded > 65000)
	{
		LOG_MESSAGE(1, "ExpandWolfGraphLump: invalid expanded size in entry %d", lumpnum);
		return;
	}

	uint8_t* dest = new uint8_t[expanded];
	uint8_t* end, *start;
	huffnode* headptr, *huffptr;

	headptr = hufftable+254;        // head node is always node 254

	size_t written = 0;

	end = dest+expanded;
	start = dest;

	uint8_t val = *source++;
	uint8_t mask = 1;
	uint16_t nodeval = 0;
	huffptr = headptr;
	while(1)
	{
		if(!(val & mask))		nodeval = huffptr->bit0;
		else					nodeval = huffptr->bit1;
		if(mask==0x80)			val = *source++, mask = 1;
		else					mask <<= 1;

		if(nodeval<256)
		{
			*dest++ = (uint8_t) nodeval;
			written++;
			huffptr = headptr;
			if(dest>=end) break;
		}
		else if (nodeval < 512)
		{
			huffptr = hufftable + (nodeval - 256);
		}
		else LOG_MESSAGE(1, "ExpandWolfGraphLump: nodeval is out of control (%d) in entry %d", nodeval, lumpnum);
	}

	entry->importMem(start, expanded);
	delete[] start;
}

/*******************************************************************
 * VARIABLES
 *******************************************************************/

/*******************************************************************
 * WOLFARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* WolfArchive::WolfArchive
 * WolfArchive class constructor
 *******************************************************************/
WolfArchive::WolfArchive()
	: TreelessArchive(ARCHIVE_WOLF)
{
}

/* WolfArchive::~WolfArchive
 * WolfArchive class destructor
 *******************************************************************/
WolfArchive::~WolfArchive()
{
}

/* WolfArchive::getEntryOffset
 * Gets a lump entry's offset
 * Returns the lump entry's offset, or zero if it doesn't exist
 *******************************************************************/
uint32_t WolfArchive::getEntryOffset(ArchiveEntry* entry)
{
	return uint32_t((int)entry->exProp("Offset"));
}

/* WolfArchive::setEntryOffset
 * Sets a lump entry's offset
 *******************************************************************/
void WolfArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	entry->exProp("Offset") = (int)offset;
}


/* WolfArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string WolfArchive::getFileExtensionString()
{
	return "Wolfenstein 3D Files (*.wl1; *.wl6; *.sod; *.sd?)|*.wl1;*.wl6;*.sod;*.sd1;*.sd2;*.sd3";
}

/* WolfArchive::getFormat
 * Gives the "archive_dat" string
 *******************************************************************/
string WolfArchive::getFormat()
{
	return "archive_wolf";
}

/* WolfArchive::open
 * Reads a Wolf format file from disk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WolfArchive::open(string filename)
{
	// Find wolf archive type
	wxFileName fn1(filename);
	bool opened = false;
	if (fn1.GetName().MakeUpper() == "MAPHEAD" || fn1.GetName().MakeUpper() == "GAMEMAPS" || fn1.GetName().MakeUpper() == "MAPTEMP")
	{
		// MAPHEAD can be paried with either a GAMEMAPS (Carmack,RLEW) or MAPTEMP (RLEW)
		wxFileName fn2(fn1);
		if (fn1.GetName().MakeUpper() == "MAPHEAD")
		{
			fn2.SetName("GAMEMAPS");
			if (!wxFile::Exists(fn2.GetFullPath()))
				fn2.SetName("MAPTEMP");
		}
		else
		{
			fn1.SetName("MAPHEAD");
		}
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openMaps(head, data);
	}
	else if (fn1.GetName().MakeUpper() == "AUDIOHED" || fn1.GetName().MakeUpper() == "AUDIOT")
	{
		wxFileName fn2(fn1);
		fn1.SetName("AUDIOHED");
		fn2.SetName("AUDIOT");
		MemChunk data, head;
		head.importFile(findFileCasing(fn1));
		data.importFile(findFileCasing(fn2));
		opened = openAudio(head, data);
	}
	else if (fn1.GetName().MakeUpper() == "VGAHEAD" || fn1.GetName().MakeUpper() == "VGAGRAPH" || fn1.GetName().MakeUpper() == "VGADICT")
	{
		wxFileName fn2(fn1);
		wxFileName fn3(fn1);
		fn1.SetName("VGAHEAD");
		fn2.SetName("VGAGRAPH");
		fn3.SetName("VGADICT");
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
		this->filename = filename;
		this->on_disk = true;

		return true;
	}
	else
		return false;
}

/* WolfArchive::open
 * Reads VSWAP Wolf format data from a MemChunk.
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WolfArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read Wolf header
	mc.seek(0, SEEK_SET);
	uint16_t num_chunks, num_lumps;
	mc.read(&num_chunks, 2);	// Number of chunks
	mc.read(&spritestart, 2);	// First sprite
	mc.read(&soundstart, 2);	// First sound
	num_chunks	= wxINT16_SWAP_ON_BE(num_chunks);
	spritestart	= wxINT16_SWAP_ON_BE(spritestart);
	soundstart	= wxINT16_SWAP_ON_BE(soundstart);
	num_lumps	= num_chunks;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	WolfHandle* pages = new WolfHandle[num_lumps];
	for (uint32_t d = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		UI::setSplashProgress((float)d / (float)num_chunks*2.0f);

		// Read offset info
		uint32_t offset = 0;
		mc.read(&offset,	4);		// Offset
		pages[d].offset = wxINT32_SWAP_ON_BE(offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (pages[d].offset != 0 && pages[d].offset < (unsigned)((num_lumps + 1) * 6))
		{
			delete[] pages;
			LOG_MESSAGE(1, "WolfArchive::open: Wolf archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt ";
			setMuted(false);
			return false;
		}
	}

	// Then read the sizes
	for (uint32_t d = 0, l = 0; d < num_chunks; d++)
	{
		// Update splash window progress
		UI::setSplashProgress((float)(d + num_chunks) / (float)num_chunks*2.0f);

		// Read size info
		uint16_t size = 0;
		mc.read(&size,	2);		// Size
		size	= wxINT16_SWAP_ON_BE(size);

		// Wolf chunks have no names, so just give them a number
		string name;
		if (d < spritestart)		name = S_FMT("WAL%05d", l);
		else if (d < soundstart)	name = S_FMT("SPR%05d", l - spritestart);
		else						name = S_FMT("SND%05d", l - soundstart);

		++l;

		// Shareware versions do not include all lumps,
		// no need to bother with fakes
		if (pages[d].offset > 0)
		{

			// Digitized sounds can be made of multiple pages
			size_t e = d;
			if (d >= soundstart && size == 4096)
			{
				size_t fullsize = 4096;
				do
				{
					mc.read(&size, 2);
					size = wxINT16_SWAP_ON_BE(size);
					fullsize += size;
					++e;
				}
				while (size == 4096);
				size = fullsize;
			}

			// Create & setup lump
			ArchiveEntry* nlump = new ArchiveEntry(name, size);
			nlump->setLoaded(false);
			nlump->exProp("Offset") = (int)pages[d].offset;
			nlump->setState(0);

			d = e;

			// Add to entry list
			getRoot()->addEntry(nlump);

			// If the lump data goes past the end of file,
			// the data file is invalid
			if (getEntryOffset(nlump) + size > mc.getSize())
			{
				delete[] pages;
				LOG_MESSAGE(1, "WolfArchive::open: Wolf archive is invalid or corrupt");
				Global::error = "Archive is invalid and/or corrupt";
				setMuted(false);
				return false;
			}
		}

	}
	// Cleanup
	delete[] pages;

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* WolfArchive::openAudio
 * Reads Wolf AUDIOT/AUDIOHEAD format data from a MemChunk.
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WolfArchive::openAudio(MemChunk& head, MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.getSize()>>2) - 1;
	spritestart	= soundstart = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	const uint32_t* offsets = (const uint32_t*) head.getData();
	MemChunk edata;

	// First try to determine where data type changes
	enum
	{
		SegmentPCSpeaker,
		SegmentAdLib,
		SegmentDigital,
		SegmentMusic
	};
	int currentSeg = SegmentPCSpeaker;
	static const char* const segPrefix[4] = {"PCS", "ADL", "SND", "MUS"};
	unsigned int segEnds[4] = {0,0,0,num_lumps};
	bool stripTags = true;
	// Method 1: Look for !ID! tags
	for (uint32_t d = 0; d < num_lumps && currentSeg != SegmentMusic; d++)
	{
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size = wxINT32_SWAP_ON_BE(offsets[d+1]) - offset;

		// Read entry data if it isn't zero-sized
		if (size >= 4)
		{
			data.exportMemChunk(edata, offset, size);

			if (strncmp((const char*)&edata[size-4], "!ID!", 4) == 0)
				segEnds[currentSeg++] = d;
		}
	}

	if(currentSeg != SegmentMusic)
	{
		// Method 2: Heuristics - Find music and then assume there are the same number PC, Adlib, and Digital
		stripTags = false;
		uint32_t d = num_lumps;
		while(d-- > 3)
		{
			uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
			uint32_t size = wxINT32_SWAP_ON_BE(offsets[d+1]) - offset;

			if(size <= 4)
				break;

			// Look to see if we have an IMF
			data.exportMemChunk(edata, offset, size);

			string name = searchIMFName(edata);
			if(name == "")
				break;
		}
		segEnds[SegmentDigital] = d;
		segEnds[SegmentPCSpeaker] = d/3;
		segEnds[SegmentAdLib] = segEnds[SegmentPCSpeaker]*2;
	}

	// Now we can actually process the chunks
	currentSeg = 0;
	uint32_t dOfs = 0; // So that each segment starts counting at 0
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size = wxINT32_SWAP_ON_BE(offsets[d+1]) - offset;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.getSize())
		{
			LOG_MESSAGE(1, "WolfArchive::openAudio: Wolf archive is invalid or corrupt");
			Global::error = S_FMT("Archive is invalid and/or corrupt in entry %d", d);
			setMuted(false);
			return false;
		}

		// See if we need to remove !ID! tag from final chunk
		if (d == segEnds[currentSeg] && stripTags)
		{
			size -= 4;
		}
		else if(d == segEnds[currentSeg]+1)
		{
			dOfs = segEnds[currentSeg]+1;
			++currentSeg;
		}

		//
		// Read entry data if it isn't zero-sized
		if (size > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, offset, size);
		}

		// Wolf chunks have no names, so just give them a number
		string name = "";
		if (currentSeg == SegmentMusic)
			name = searchIMFName(edata);
		if (name == "")
			name = S_FMT("%s%05d", segPrefix[currentSeg], d-dOfs);

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;

		// Detect entry type
		if (size > 0) nlump->importMemChunk(edata);
		EntryType::detectEntryType(nlump);

		// Add to entry list
		nlump->setState(0);
		getRoot()->addEntry(nlump);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* WolfArchive::openMaps
 * Reads Wolf GAMEMAPS/MAPHEAD format data from a MemChunk.
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WolfArchive::openMaps(MemChunk& head, MemChunk& data)
{
	// Check data was given
	if (!head.hasData() || !data.hasData())
		return false;

	// Read Wolf header file
	uint32_t num_lumps = (head.getSize()- 2) >> 2;
	spritestart	= soundstart = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	const uint32_t* offsets = (const uint32_t*) (2 + head.getData());
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = wxINT32_SWAP_ON_BE(offsets[d]);
		uint32_t size = 38;

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.getSize())
		{
			LOG_MESSAGE(1, "WolfArchive::openMaps: Wolf archive is invalid or corrupt");
			Global::error = S_FMT("Archive is invalid and/or corrupt in entry %d", d);
			setMuted(false);
			return false;
		}

		if (offset == 0 && d > 0) continue;

		string name = "";
		for (size_t i = 0; i < 16; ++i)
		{
			name += data[offset + 22 + i];
			if (data[offset + 22 + i] == 0)
				break;
		}

		ArchiveEntry* nlump = new ArchiveEntry(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Add to entry list
		getRoot()->addEntry(nlump);

		// Add map planes to entry list
		uint32_t planeofs[3];
		uint16_t planelen[3];
		planeofs[0] = READ_L32(data, offset);
		planeofs[1] = READ_L32(data, offset + 4);
		planeofs[2] = READ_L32(data, offset + 8);
		planelen[0] = READ_L16(data, offset +12);
		planelen[1] = READ_L16(data, offset +14);
		planelen[2] = READ_L16(data, offset +16);
		for (int i = 0; i < 3; ++i)
		{
			name = S_FMT("PLANE%d", i);
			nlump = new ArchiveEntry(name, planelen[i]);
			nlump->setLoaded(false);
			nlump->exProp("Offset") = (int)planeofs[i];
			nlump->setState(0);
			getRoot()->addEntry(nlump);
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
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* WolfArchive::openGraph
 * Reads Wolf VGAGRAPH/VGAHEAD/VGADICT format data from a MemChunk.
 * Returns true if successful, false otherwise
 *******************************************************************/
#define WC(a) WolfConstant(a, num_lumps)
bool WolfArchive::openGraph(MemChunk& head, MemChunk& data, MemChunk& dict)
{
	// Check data was given
	if (!head.hasData() || !data.hasData() || !dict.hasData())
		return false;

	if (dict.getSize() != 1024)
	{
		Global::error = S_FMT("WolfArchive::openGraph: VGADICT is improperly sized (%d bytes instead of 1024)", dict.getSize());
		return false;
	}
	huffnode nodes[256];
	memcpy(nodes, dict.getData(), 1024);

	// Read Wolf header file
	uint32_t num_lumps = (head.getSize() / 3) - 1;
	spritestart	= soundstart = num_lumps;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the offsets
	UI::setSplashProgressMessage("Reading Wolf archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read offset info
		uint32_t offset = READ_L24(head, (d * 3));

		// Compute size from next offset
		uint32_t size = /*((d == num_lumps - 1) ? data.getSize() - offset :*/(
					READ_L24(head, ((d+1) * 3)) - offset);

		// If the lump data goes before the end of the directory,
		// the data file is invalid
		if (offset + size > data.getSize())
		{
			LOG_MESSAGE(1, "WolfArchive::openGraph: Wolf archive is invalid or corrupt");
			Global::error = S_FMT("Archive is invalid and/or corrupt in entry %d", d);
			setMuted(false);
			return false;
		}

		// Wolf chunks have no names, so just give them a number
		string name;
		if (d == 0)														name = "INF";
		else if (d == 1 || d == 2)										name = "FNT";
		else if (d >= WC(STARTPICS))
		{
			if (d >= WC(STARTPAL) && d <= WC(ENDPAL))					name = "PAL";
			else if (d == WC(TITLE1PIC) || d == WC(TITLE2PIC))			name = "TIT";
			else if (d == WC(IDGUYS1PIC)|| d == WC(IDGUYS2PIC))			name = "IDG";
			else if (d >= WC(ENDSCREEN1PIC) && d <= WC(ENDSCREEN9PIC))	name = "END";
			else if (d < WC(STARTPICM)) 								name = "PIC";
			else if (d == WC(STARTTILE8))								name = "TIL";
			else														name = "LMP";
		}
		else name = "LMP";
		name += S_FMT("%05d", d);


		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Add to entry list
		getRoot()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	const uint16_t* pictable;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		//LOG_MESSAGE(1, "Entry %d/%d", a, numEntries());
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			data.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			entry->importMemChunk(edata);
		}
		ExpandWolfGraphLump(entry, a, num_lumps, nodes);

		// Store pictable information
		if (a == 0)
			pictable = (uint16_t*) entry->getData();
		else if (a >= WC(STARTPICS) && a < WC(STARTPICM))
		{
			size_t i = (a - WC(STARTPICS))<<1;
			addWolfPicHeader(entry, pictable[i], pictable[i+1]);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* WolfArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, and update namespaces if needed
 *******************************************************************/
ArchiveEntry* WolfArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return NULL;

	// Check if read-only
	if (isReadOnly())
		return NULL;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* WolfArchive::addEntry
 * Since there are no namespaces, just give the hot potato to the
 * other function and call it a day.
 *******************************************************************/
ArchiveEntry* WolfArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* WolfArchive::renameEntry
 * Wolf chunks have no names, so renaming is pointless.
 *******************************************************************/
bool WolfArchive::renameEntry(ArchiveEntry* entry, string name)
{
	return false;
}

/* WolfArchive::write
 * Writes the dat archive to a MemChunk
 * Returns true if successful, false otherwise [Not implemented]
 *******************************************************************/
bool WolfArchive::write(MemChunk& mc, bool update)
{
	return false;
}

/* WolfArchive::loadEntryData
 * Loads an entry's data from the datafile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WolfArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open wadfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "WolfArchive::loadEntryData: Failed to open datfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* WolfArchive::isWolfArchive
 * Checks if the given data is a valid Wolfenstein VSWAP archive
 *******************************************************************/
bool WolfArchive::isWolfArchive(MemChunk& mc)
{
	// Read Wolf header
	mc.seek(0, SEEK_SET);
	uint16_t num_lumps, sprites, sounds;
	mc.read(&num_lumps, 2);		// Size
	num_lumps	= wxINT16_SWAP_ON_BE(num_lumps);
	if (num_lumps == 0)
		return false;

	mc.read(&sprites, 2);		// Sprites start
	mc.read(&sounds, 2);		// Sounds start
	sprites		= wxINT16_SWAP_ON_BE(sprites);
	sounds		= wxINT16_SWAP_ON_BE(sounds);
	if (sprites > sounds)
		return false;

	// Read lump info
	uint32_t offset = 0;
	uint16_t size = 0;
	size_t totalsize = 6 * (num_lumps + 1);
	size_t pagesize = (totalsize / 512) + ((totalsize % 512) ? 1 : 0);
	size_t filesize = mc.getSize();
	if (filesize < totalsize)
		return false;

	WolfHandle* pages = new WolfHandle[num_lumps];
	uint32_t lastoffset = 0;
	for (size_t a = 0; a < num_lumps; ++a)
	{
		mc.read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);
		if (offset < lastoffset || offset % 512)
		{
			delete[] pages;
			return false;
		}
		lastoffset = offset;
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
		{
			delete[] pages;
			return false;
		}
		lastsize = size;
	}
	delete[] pages;
	return ((pagesize * 512) <= filesize || filesize >= lastoffset + lastsize);
}

/* WolfArchive::isWolfArchive
 * Checks if the file at [filename] is a valid Wolfenstein VSWAP archive
 *******************************************************************/
bool WolfArchive::isWolfArchive(string filename)
{
	// Find wolf archive type
	wxFileName fn1(filename);
	if (fn1.GetName().MakeUpper() == "MAPHEAD" || fn1.GetName().MakeUpper() == "GAMEMAPS" || fn1.GetName().MakeUpper() == "MAPTEMP")
	{
		wxFileName fn2(fn1);
		fn1.SetName("MAPHEAD");
		fn2.SetName("GAMEMAPS");
		if (!(wxFile::Exists(findFileCasing(fn1)) && wxFile::Exists(findFileCasing(fn2))))
		{
			fn2.SetName("MAPTEMP");
			return (wxFile::Exists(findFileCasing(fn1)) && wxFile::Exists(findFileCasing(fn2)));
		}
		return true;
	}
	else if (fn1.GetName().MakeUpper() == "AUDIOHED" || fn1.GetName().MakeUpper() == "AUDIOT")
	{
		wxFileName fn2(fn1);
		fn1.SetName("AUDIOHED");
		fn2.SetName("AUDIOT");
		return (wxFile::Exists(findFileCasing(fn1)) && wxFile::Exists(findFileCasing(fn2)));
	}
	else if (fn1.GetName().MakeUpper() == "VGAHEAD" || fn1.GetName().MakeUpper() == "VGAGRAPH"
			 || fn1.GetName().MakeUpper() == "VGADICT")
	{
		wxFileName fn2(fn1);
		wxFileName fn3(fn1);
		fn1.SetName("VGAHEAD");
		fn2.SetName("VGAGRAPH");
		fn3.SetName("VGADICT");
		return (wxFile::Exists(findFileCasing(fn1)) && wxFile::Exists(findFileCasing(fn2))
				&& wxFile::Exists(findFileCasing(fn3)));
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

	file.Read(&num_lumps, 2);	// Size
	num_lumps	= wxINT16_SWAP_ON_BE(num_lumps);
	if (num_lumps == 0)
		return false;

	file.Read(&sprites, 2);		// Sprites start
	file.Read(&sounds, 2);		// Sounds start
	sprites		= wxINT16_SWAP_ON_BE(sprites);
	sounds		= wxINT16_SWAP_ON_BE(sounds);
	if (sprites > sounds)
		return false;

	// Read lump info
	uint32_t offset = 0;
	uint16_t size = 0;
	size_t totalsize = 6 * (num_lumps + 1);
	size_t pagesize = (totalsize / 512) + ((totalsize % 512) ? 1 : 0);
	size_t filesize = file.Length();
	if (filesize < totalsize)
		return false;

	WolfHandle* pages = new WolfHandle[num_lumps];
	uint32_t lastoffset = 0;
	for (size_t a = 0; a < num_lumps; ++a)
	{
		file.Read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);
		if (offset == 0)
		{
			// Empty entry (we're opening a shareware/demo archive)
		}
		else if (offset < lastoffset || offset % 512)
		{
			delete[] pages;
			return false;
		}
		if (offset > 0)
			lastoffset = offset;
		pages[a].offset = offset;
	}
	uint16_t lastsize = 0;
	lastoffset = pages[0].offset;
	for (size_t b = 0; b < num_lumps; ++b)
	{
		file.Read(&size, 2);
		if (pages[b].offset > 0)
		{
			size = wxINT16_SWAP_ON_BE(size);
			pagesize += (size / 512) + ((size % 512) ? 1 : 0);
			pages[b].size = size;
			if (b > 0 && lastoffset + lastsize > pages[b].offset)
			{
				delete[] pages;
				return false;
			}
			lastoffset = pages[b].offset;
			lastsize = size;
		}
		else pages[b].size = 0;
	}
	delete[] pages;
	return ((pagesize * 512) <= filesize || filesize >= lastoffset + lastsize);
}

/*******************************************************************
 * EXTRA CONSOLE COMMANDS
 *******************************************************************/
#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(addimfheader, 0, true)
{
	vector<ArchiveEntry*> entries = MainEditor::currentEntrySelection();

	for (size_t i = 0; i < entries.size(); ++i)
		addIMFHeader(entries[i]);
}

