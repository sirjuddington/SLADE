#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class DiskArchiveHandler : public ArchiveFormatHandler
{
public:
	struct DiskEntry
	{
		char   name[64];
		size_t offset;
		size_t length;
	};

	DiskArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Disk) {}
	~DiskArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;
};
} // namespace slade
