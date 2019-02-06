#pragma once

#include "Archive/Archive.h"

class RffArchive : public TreelessArchive
{
public:
	RffArchive() : TreelessArchive("rff") {}
	~RffArchive() = default;

	// RFF specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isRffArchive(MemChunk& mc);
	static bool isRffArchive(const wxString& filename);
};
