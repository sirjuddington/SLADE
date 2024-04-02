#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class SiNArchiveHandler : public ArchiveFormatHandler
{
public:
	SiNArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::SiN) {}
	~SiNArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Static functions
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
