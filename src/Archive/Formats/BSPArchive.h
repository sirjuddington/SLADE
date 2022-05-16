#pragma once

#include "Archive/Archive.h"

namespace slade
{
class BSPArchive : public Archive
{
public:
	BSPArchive() : Archive("bsp") {}
	~BSPArchive() override = default;

	// Opening/writing
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isBSPArchive(MemChunk& mc);
	static bool isBSPArchive(const string& filename);
};
} // namespace slade
