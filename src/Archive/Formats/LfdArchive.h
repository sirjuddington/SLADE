#pragma once

#include "Archive/Archive.h"

class LfdArchive : public TreelessArchive
{
public:
	LfdArchive() : TreelessArchive("lfd") {}
	~LfdArchive() = default;

	// LFD specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isLfdArchive(MemChunk& mc);
	static bool isLfdArchive(const std::string& filename);
};
