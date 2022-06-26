#pragma once

#include "Archive/Archive.h"

namespace slade
{
class DatArchive : public TreelessArchive
{
public:
	DatArchive() : TreelessArchive("dat") {}
	~DatArchive() override = default;

	// Dat specific
	void updateNamespaces();

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
	bool                     removeEntry(ArchiveEntry* entry, bool set_deleted = true) override;

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name) override;

	// Detection
	string detectNamespace(unsigned index, ArchiveDir* dir = nullptr) const override;
	string detectNamespace(ArchiveEntry* entry) const override;

	static bool isDatArchive(const MemChunk& mc);
	static bool isDatArchive(const string& filename);

private:
	int sprites_[2];
	int flats_[2];
	int walls_[2];
};
} // namespace slade
