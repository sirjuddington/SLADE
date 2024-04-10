#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class BZip2ArchiveHandler : public ArchiveFormatHandler
{
public:
	BZip2ArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Bz2, true) {}
	~BZip2ArchiveHandler() override = default;

	// Opening
	bool open(Archive& archive, const MemChunk& mc) override;

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override
	{
		return nullptr;
	}
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override
	{
		return nullptr;
	}
	bool removeEntry(Archive& archive, ArchiveEntry* entry, bool) override { return false; }

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool) override { return false; }

	// Entry moving
	bool swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2) override { return false; }
	bool moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr)
		override
	{
		return false;
	}

	// Search
	ArchiveEntry*         findFirst(Archive& archive, ArchiveSearchOptions& options) override;
	ArchiveEntry*         findLast(Archive& archive, ArchiveSearchOptions& options) override;
	vector<ArchiveEntry*> findAll(Archive& archive, ArchiveSearchOptions& options) override;

	// Static functions
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
