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
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isBSPArchive(const MemChunk& mc);
	static bool isBSPArchive(const string& filename);
};
} // namespace slade
