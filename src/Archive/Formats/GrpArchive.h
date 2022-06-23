#pragma once

#include "Archive/Archive.h"

namespace slade
{
class GrpArchive : public TreelessArchive
{
public:
	GrpArchive() : TreelessArchive("grp") {}
	~GrpArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isGrpArchive(const MemChunk& mc);
	static bool isGrpArchive(const string& filename);
};
} // namespace slade
