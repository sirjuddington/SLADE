#pragma once

#include "Archive/Archive.h"

class SiNArchive : public Archive
{
public:
	SiNArchive() : Archive("sin") {}
	~SiNArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isSiNArchive(MemChunk& mc);
	static bool isSiNArchive(const wxString& filename);
};
