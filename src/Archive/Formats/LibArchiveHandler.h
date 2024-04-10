#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class LibArchiveHandler : public ArchiveFormatHandler
{
public:
	LibArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Lib, true) {}
	~LibArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
