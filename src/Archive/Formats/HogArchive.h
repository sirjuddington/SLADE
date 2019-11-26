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
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name) override;

	// Static functions
	static bool isHogArchive(MemChunk& mc);
	static bool isHogArchive(const string& filename);
};
