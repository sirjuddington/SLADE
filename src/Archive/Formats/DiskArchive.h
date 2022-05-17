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
	~DiskArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc) override; // Open from MemChunk
	bool write(MemChunk& mc) override;      // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isDiskArchive(const MemChunk& mc);
	static bool isDiskArchive(const string& filename);
};
} // namespace slade
