#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class LabArchiveHandler : public ArchiveFormatHandler
{
public:
	LabArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Lab, true) {}
	~LabArchiveHandler() override = default;

	// LAB specific
	string detectResType(ArchiveEntry* entry);

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
