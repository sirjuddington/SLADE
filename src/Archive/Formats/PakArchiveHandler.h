#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class PakArchiveHandler : public ArchiveFormatHandler
{
public:
	PakArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Pak) {}
	~PakArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Static functions
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
