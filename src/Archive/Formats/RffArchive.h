#pragma once

#include "Archive/Archive.h"

namespace slade
{
class RffArchive : public TreelessArchive
{
public:
	RffArchive() : TreelessArchive("rff") {}
	~RffArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isRffArchive(const MemChunk& mc);
	static bool isRffArchive(const string& filename);
};
} // namespace slade
