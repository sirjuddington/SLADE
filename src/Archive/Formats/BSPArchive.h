#pragma once

#include "Archive/Archive.h"

namespace slade
{
class BSPArchive : public Archive
{
public:
	BSPArchive() : Archive("bsp") {}
	~BSPArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool     loadEntryData(ArchiveEntry* entry) override;
	uint32_t entryOffset(ArchiveEntry* entry);

	// Static functions
	static bool isBSPArchive(MemChunk& mc);
	static bool isBSPArchive(const string& filename);
};
} // namespace slade
