#pragma once

#include "Archive/Archive.h"

namespace slade
{
// From http://www.gamers.org/dEngine/quake/spec/quake-spec31.html#CWADF
struct Wad2Entry
{
	int32_t offset;   // Position of the entry in WAD
	int32_t dsize;    // Size of the entry in WAD file
	int32_t size;     // Size of the entry in memory
	char    type;     // type of entry
	char    cmprs;    // Compression. 0 if none.
	int16_t dummy;    // Not used
	char    name[16]; // 1 to 16 characters, '\0'-padded
};

class Wad2Archive : public TreelessArchive
{
public:
	Wad2Archive() : TreelessArchive("wad2") {}
	~Wad2Archive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isWad2Archive(MemChunk& mc);
	static bool isWad2Archive(const string& filename);

private:
	bool wad3_ = false;
};
} // namespace slade
