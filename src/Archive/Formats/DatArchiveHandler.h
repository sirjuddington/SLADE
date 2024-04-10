#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class DatArchiveHandler : public ArchiveFormatHandler
{
public:
	DatArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Dat, true) {}
	~DatArchiveHandler() override = default;

	// Dat specific
	void updateNamespaces(Archive& archive);

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;
	bool removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted = true) override;

	// Entry moving
	bool swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr)
		override;

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override;

	// Detection
	string detectNamespace(Archive& archive, unsigned index, ArchiveDir* dir = nullptr) override;
	string detectNamespace(Archive& archive, ArchiveEntry* entry) override;

	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	int sprites_[2] = {};
	int flats_[2]   = {};
	int walls_[2]   = {};
};
} // namespace slade
