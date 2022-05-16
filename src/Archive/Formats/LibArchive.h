#pragma once

#include "Archive/Archive.h"

namespace slade
{
class LibArchive : public TreelessArchive
{
public:
	LibArchive() : TreelessArchive("lib") {}
	~LibArchive() override = default;

	// Opening/writing
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool     loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;
	unsigned numEntries() override { return rootDir()->numEntries(); }

	static bool isLibArchive(MemChunk& mc);
	static bool isLibArchive(const string& filename);
};
} // namespace slade
