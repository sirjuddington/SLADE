#pragma once

#include "Archive/Archive.h"

namespace slade
{
class BZip2Archive : public TreelessArchive
{
public:
	BZip2Archive() : TreelessArchive("bz2") {}
	~BZip2Archive() override = default;

	// Opening
	bool open(const MemChunk& mc) override;

	// Writing/Saving
	bool write(MemChunk& mc) override;

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override
	{
		return nullptr;
	}
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override
	{
		return nullptr;
	}
	bool removeEntry(ArchiveEntry* entry) override { return false; }

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name) override { return false; }

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override { return false; }
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr) override
	{
		return false;
	}

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// Static functions
	static bool isBZip2Archive(const MemChunk& mc);
	static bool isBZip2Archive(const string& filename);
};
} // namespace slade
