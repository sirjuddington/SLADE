#pragma once

#include "Archive/Archive.h"

namespace slade
{
class PakArchive : public Archive
{
public:
	PakArchive() : Archive("pak") {}
	~PakArchive() override = default;

	// Opening/writing
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isPakArchive(MemChunk& mc);
	static bool isPakArchive(const string& filename);
};
} // namespace slade
