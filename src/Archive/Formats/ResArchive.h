#pragma once

#include "Archive/Archive.h"

namespace slade
{
class ResArchive : public Archive
{
public:
	ResArchive() : Archive("res") {}
	~ResArchive() override = default;

	// Opening/writing
	bool readDirectory(MemChunk& mc, size_t dir_offset, size_t num_lumps, shared_ptr<ArchiveDir> parent);
	bool open(MemChunk& mc) override;  // Open from MemChunk
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isResArchive(MemChunk& mc);
	static bool isResArchive(MemChunk& mc, size_t& d_o, size_t& n_l);
	static bool isResArchive(const string& filename);

private:
	static constexpr int RESDIRENTRYSIZE = 39; // The size of a res entry in the res directory
};
} // namespace slade
