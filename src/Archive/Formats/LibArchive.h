#pragma once

#include "Archive/Archive.h"

class LibArchive : public TreelessArchive
{
public:
	LibArchive() : TreelessArchive("lib") {}
	~LibArchive() = default;

	// Lib specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool     loadEntryData(ArchiveEntry* entry) override;
	unsigned numEntries() override { return rootDir()->numEntries(); }

	// Entry addition/modification
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override;
	ArchiveEntry* addEntry(ArchiveEntry* entry, const string& add_namespace, bool copy = false) override;
	bool          renameEntry(ArchiveEntry* entry, const string& name) override;

	static bool isLibArchive(MemChunk& mc);
	static bool isLibArchive(const string& filename);
};
