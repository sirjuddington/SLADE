#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class ChasmBinArchiveHandler : public ArchiveFormatHandler
{
public:
	ChasmBinArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::ChasmBin) {}

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc, bool detect_types) override;
	bool write(Archive& archive, MemChunk& mc) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	static constexpr uint32_t HEADER_SIZE     = 4 + 2;             // magic + number of entries
	static constexpr uint32_t NAME_SIZE       = 1 + 12;            // length + characters
	static constexpr uint32_t ENTRY_SIZE      = NAME_SIZE + 4 + 4; // name + size + offset
	static constexpr uint16_t MAX_ENTRY_COUNT = 2048;              // the same for Demo and Full versions
};
} // namespace slade
