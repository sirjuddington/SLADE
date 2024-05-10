#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
// From http://www.gamers.org/dEngine/quake/spec/quake-spec31.html#CWADF
struct Wad2Entry
{
	int32_t offset;   // Position of the entry in WAD
	int32_t dsize;    // Size of the entry in WAD file
	int32_t size;     // Size of the entry in memory
	char    type;     // type of entry
	char    cmprs;    // Compression. 0 if none.
	int16_t dummy;    // Not used
	char    name[16]; // 1 to 16 characters, '\0'-padded
};

class Wad2ArchiveHandler : public ArchiveFormatHandler
{
public:
	Wad2ArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Wad2, true) {}
	~Wad2ArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	bool wad3_ = false;
};
} // namespace slade
