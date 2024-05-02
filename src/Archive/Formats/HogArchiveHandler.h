#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class HogArchiveHandler : public ArchiveFormatHandler
{
public:
	HogArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Hog, true) {}
	~HogArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;                         // Write to MemChunk

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
