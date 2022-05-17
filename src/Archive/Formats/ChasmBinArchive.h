#pragma once

#include "Archive/Archive.h"

namespace slade
{
class ChasmBinArchive : public Archive
{
public:
	ChasmBinArchive() : Archive("chasm_bin") {}

	// Opening/writing
	bool open(const MemChunk& mc) override;
	bool write(MemChunk& mc) override;

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isChasmBinArchive(const MemChunk& mc);
	static bool isChasmBinArchive(const string& filename);

private:
	static constexpr uint32_t HEADER_SIZE     = 4 + 2;             // magic + number of entries
	static constexpr uint32_t NAME_SIZE       = 1 + 12;            // length + characters
	static constexpr uint32_t ENTRY_SIZE      = NAME_SIZE + 4 + 4; // name + size + offset
	static constexpr uint16_t MAX_ENTRY_COUNT = 2048;              // the same for Demo and Full versions
};
} // namespace slade
