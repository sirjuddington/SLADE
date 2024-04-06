#pragma once

#include "ZipArchiveHandler.h"

struct archive;

namespace slade
{
// Inherit from ZipArchiveHandler as it works exactly the same, just with a different file format
class Zip7ArchiveHandler : public ZipArchiveHandler
{
public:
	Zip7ArchiveHandler() : ZipArchiveHandler(ArchiveFormat::Zip7) {}

	// Opening
	bool open(Archive& archive, string_view filename) override; // Open from File
	bool open(Archive& archive, const MemChunk& mc) override;   // Open from MemChunk

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;         // Write to MemChunk
	bool write(Archive& archive, string_view filename) override; // Write to File

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	bool open7z(Archive& archive, struct archive* archive_7z);
};
} // namespace slade
