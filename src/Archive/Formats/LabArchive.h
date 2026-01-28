#pragma once

#include "Archive/Archive.h"

namespace slade
{
class LabArchive : public TreelessArchive
{
public:
	LabArchive() : TreelessArchive("lab") {}
	~LabArchive() = default;

	// LAB specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);
	string   detectResType(ArchiveEntry* entry);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isLabArchive(MemChunk& mc);
	static bool isLabArchive(const string& filename);
};
} // namespace slade
