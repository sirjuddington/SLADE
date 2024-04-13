#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class TarArchiveHandler : public ArchiveFormatHandler
{
public:
	TarArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Tar) {}
	~TarArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
