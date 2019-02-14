#pragma once

#include "Archive/Archive.h"

class ADatArchive : public Archive
{
public:
	ADatArchive() : Archive("adat") {}
	~ADatArchive() = default;

	// Opening
	bool open(MemChunk& mc) override; // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;             // Write to MemChunk
	bool write(std::string_view filename, bool update = true) override; // Write to File

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isADatArchive(MemChunk& mc);
	static bool isADatArchive(const std::string& filename);

private:
	static const int DIRENTRY = 144;
};
