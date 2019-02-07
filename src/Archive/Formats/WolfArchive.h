#pragma once

#include "Archive/Archive.h"

class WolfArchive : public TreelessArchive
{
public:
	WolfArchive() : TreelessArchive("wolf") {}
	~WolfArchive() = default;

	// Wolf3D specific
	uint32_t getEntryOffset(ArchiveEntry* entry) const;
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset) const;

	// Opening
	bool open(const wxString& filename) override; // Open from File
	bool open(MemChunk& mc) override;             // Open from MemChunk

	bool openAudio(MemChunk& head, MemChunk& data);
	bool openGraph(MemChunk& head, MemChunk& data, MemChunk& dict);
	bool openMaps(MemChunk& head, MemChunk& data);

	// Writing/Saving
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

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, const wxString& name) override;

	static bool isWolfArchive(MemChunk& mc);
	static bool isWolfArchive(const wxString& filename);

private:
	struct WolfHandle
	{
		uint32_t offset;
		uint16_t size;
	};

	uint16_t spritestart_;
	uint16_t soundstart_;
};
