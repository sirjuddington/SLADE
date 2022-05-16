#pragma once

#include "Archive/Archive.h"

namespace slade
{
class SiNArchive : public Archive
{
public:
	SiNArchive() : Archive("sin") {}
	~SiNArchive() override = default;

	// Opening/writing
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isSiNArchive(MemChunk& mc);
	static bool isSiNArchive(const string& filename);
};
} // namespace slade
