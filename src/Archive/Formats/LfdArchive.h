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
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isLfdArchive(const MemChunk& mc);
	static bool isLfdArchive(const string& filename);
};
} // namespace slade
