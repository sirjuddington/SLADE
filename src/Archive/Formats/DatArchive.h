#pragma once

#include "Archive/Archive.h"

class DatArchive : public TreelessArchive
{
public:
	DatArchive() : TreelessArchive("dat") {}
	~DatArchive() = default;

	// Dat specific
	uint32_t getEntryOffset(ArchiveEntry* entry) const { return uint32_t((int)entry->exProp("Offset")); }
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset) const { entry->exProp("Offset") = (int)offset; }
	void     updateNamespaces();

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool     loadEntryData(ArchiveEntry* entry) override;
	unsigned numEntries() override { return rootDir()->numEntries(); }

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override;
	ArchiveEntry* addEntry(ArchiveEntry* entry, const wxString& add_namespace, bool copy = false) override;
	bool          removeEntry(ArchiveEntry* entry) override;

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, const wxString& name) override;

	// Detection
	wxString detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override;
	wxString detectNamespace(ArchiveEntry* entry) override;

	static bool isDatArchive(MemChunk& mc);
	static bool isDatArchive(const wxString& filename);

private:
	int sprites_[2];
	int flats_[2];
	int walls_[2];
};
