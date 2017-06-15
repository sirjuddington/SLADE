
#ifndef __WAD2_ARCHIVE_H__
#define __WAD2_ARCHIVE_H__

#include "Archive/Archive.h"

// From http://www.gamers.org/dEngine/quake/spec/quake-spec31.html#CWADF
struct Wad2Entry
{
	int32_t offset;	// Position of the entry in WAD
	int32_t dsize;	// Size of the entry in WAD file
	int32_t size;	// Size of the entry in memory
	char type;		// type of entry
	char cmprs;		// Compression. 0 if none.
	int16_t dummy;	// Not used
	char name[16];	// 1 to 16 characters, '\0'-padded
};

class Wad2Archive : public TreelessArchive
{
public:
	Wad2Archive();
	~Wad2Archive();

	// Opening/writing
	bool	open(MemChunk& mc) override;						// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true) override;	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry*	addEntry(
						ArchiveEntry* entry,
						unsigned position = 0xFFFFFFFF,
						ArchiveTreeNode* dir = nullptr,
						bool copy = false
					) override;

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name) override;

	// Static functions
	static bool isWad2Archive(MemChunk& mc);
	static bool isWad2Archive(string filename);

private:
	bool	wad3_;
};

#endif//__WAD2_ARCHIVE_H__
