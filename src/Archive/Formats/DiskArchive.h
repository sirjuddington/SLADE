#pragma once

#include "Archive/Archive.h"

namespace slade
{
class DiskArchive : public Archive
{
public:
	struct DiskEntry
	{
		char   name[64];
		size_t offset;
		size_t length;
	};

	DiskArchive() : Archive("disk") {}
	~DiskArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isDiskArchive(MemChunk& mc);
	static bool isDiskArchive(const string& filename);
};
} // namespace slade
