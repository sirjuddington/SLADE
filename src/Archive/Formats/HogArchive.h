#pragma once

#include "Archive/Archive.h"

namespace slade
{
class HogArchive : public TreelessArchive
{
public:
	HogArchive() : TreelessArchive("hog") {}
	~HogArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name, bool force = false) override;

	// Static functions
	static bool isHogArchive(const MemChunk& mc);
	static bool isHogArchive(const string& filename);
};
} // namespace slade
