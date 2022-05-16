#pragma once

#include "Archive/Archive.h"

namespace slade
{
class LfdArchive : public TreelessArchive
{
public:
	LfdArchive() : TreelessArchive("lfd") {}
	~LfdArchive() override = default;

	// Opening/writing
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isLfdArchive(MemChunk& mc);
	static bool isLfdArchive(const string& filename);
};
} // namespace slade
