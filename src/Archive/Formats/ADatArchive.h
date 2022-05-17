#pragma once

#include "Archive/Archive.h"

namespace slade
{
class ADatArchive : public Archive
{
public:
	ADatArchive() : Archive("adat") {}
	~ADatArchive() override = default;

	// Opening
	bool open(const MemChunk& mc) override; // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isADatArchive(const MemChunk& mc);
	static bool isADatArchive(const string& filename);

private:
	static constexpr int DIRENTRY = 144;
};
} // namespace slade
