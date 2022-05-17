#pragma once

#include "Archive/Archive.h"

namespace slade
{
class GobArchive : public TreelessArchive
{
public:
	GobArchive() : TreelessArchive("gob") {}
	~GobArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc) override; // Open from MemChunk
	bool write(MemChunk& mc) override;      // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isGobArchive(const MemChunk& mc);
	static bool isGobArchive(const string& filename);
};
} // namespace slade
