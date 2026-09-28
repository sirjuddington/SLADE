#pragma once

#include "Archive/Archive.h"

namespace slade
{
class SiNReloadedArchive : public Archive
{
public:
	SiNReloadedArchive() : Archive("sin") {}
	~SiNReloadedArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isSiNReloadedArchive(MemChunk& mc);
	static bool isSiNReloadedArchive(const string& filename);
};
} // namespace slade
