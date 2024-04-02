#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class ADatArchiveHandler : public ArchiveFormatHandler
{
public:
	ADatArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::ADat) {}
	~ADatArchiveHandler() override = default;

	// Opening
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	static constexpr int DIRENTRY = 144;
};
} // namespace slade
