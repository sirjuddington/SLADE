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
	bool readDirectory(const MemChunk& mc, size_t dir_offset, size_t num_lumps, shared_ptr<ArchiveDir> parent);
	bool open(const MemChunk& mc, bool detect_types) override; // Open from MemChunk
	bool write(MemChunk& mc) override;                         // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Static functions
	static bool isResArchive(const MemChunk& mc);
	static bool isResArchive(const MemChunk& mc, size_t& d_o, size_t& n_l);
	static bool isResArchive(const string& filename);

private:
	static constexpr int RESDIRENTRYSIZE = 39; // The size of a res entry in the res directory
};
} // namespace slade
