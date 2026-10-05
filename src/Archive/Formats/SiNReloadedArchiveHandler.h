#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class SiNReloadedArchiveHandler : public ArchiveFormatHandler
{
public:
	SiNReloadedArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::SiNReloaded) {}
	~SiNReloadedArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override;    // Open from MemChunk
	bool write(Archive& archive, string_view filename) override; // Write to file

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
