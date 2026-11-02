#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class ResArchiveHandler : public ArchiveFormatHandler
{
public:
	ResArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Res) {}
	~ResArchiveHandler() override = default;

	// Opening/writing
	bool readDirectory(
		Archive&               archive,
		const MemChunk&        mc,
		size_t                 dir_offset,
		size_t                 num_lumps,
		shared_ptr<ArchiveDir> parent);
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

	static bool isResArchive(const MemChunk& mc, size_t& dir_offset, size_t& num_lumps);

private:
	static constexpr int RESDIRENTRYSIZE = 39; // The size of a res entry in the res directory
};
} // namespace slade
