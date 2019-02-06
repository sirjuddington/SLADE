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
	bool write(const wxString& filename, bool update = true) override; // Write to File

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isADatArchive(MemChunk& mc);
	static bool isADatArchive(const wxString& filename);

private:
	static const int DIRENTRY = 144;
};
