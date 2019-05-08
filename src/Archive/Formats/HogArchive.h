#pragma once

#include "Archive/Archive.h"

class HogArchive : public TreelessArchive
{
public:
	HogArchive() : TreelessArchive("hog") {}
	~HogArchive() = default;

	// HOG specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override;
	ArchiveEntry* addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy = false) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name) override;

	// Static functions
	static bool isHogArchive(MemChunk& mc);
	static bool isHogArchive(const string& filename);
};
