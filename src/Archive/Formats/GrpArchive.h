#pragma once

#include "Archive/Archive.h"

class GrpArchive : public TreelessArchive
{
public:
	GrpArchive() : TreelessArchive("grp") {}
	~GrpArchive() = default;

	// GRP specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isGrpArchive(MemChunk& mc);
	static bool isGrpArchive(const string& filename);
};
