#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class RffArchiveHandler : public ArchiveFormatHandler
{
public:
	RffArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Rff, true) {}
	~RffArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;                         // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
