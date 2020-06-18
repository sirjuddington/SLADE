#pragma once

#include "Archive/Archive.h"

namespace slade
{
class TarArchive : public Archive
{
public:
	TarArchive() : Archive("tar") {}
	~TarArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isTarArchive(MemChunk& mc);
	static bool isTarArchive(const string& filename);
};
} // namespace slade
