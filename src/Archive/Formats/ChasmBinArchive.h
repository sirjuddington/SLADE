#pragma once

#include "Archive/Archive.h"

class ChasmBinArchive : public Archive
{
public:
	ChasmBinArchive() : Archive("chasm_bin") {}

	// Opening/writing
	bool open(MemChunk& mc) override;
	bool write(MemChunk& mc, bool update = true) override;

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isChasmBinArchive(MemChunk& mc);
	static bool isChasmBinArchive(const wxString& filename);

private:
	static const uint32_t HEADER_SIZE     = 4 + 2;             // magic + number of entries
	static const uint32_t NAME_SIZE       = 1 + 12;            // length + characters
	static const uint32_t ENTRY_SIZE      = NAME_SIZE + 4 + 4; // name + size + offset
	static const uint16_t MAX_ENTRY_COUNT = 2048;              // the same for Demo and Full versions
};
