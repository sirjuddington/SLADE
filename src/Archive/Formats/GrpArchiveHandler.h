#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class GrpArchiveHandler : public ArchiveFormatHandler
{
public:
	GrpArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Grp, true) {}
	~GrpArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
