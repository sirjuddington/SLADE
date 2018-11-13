#pragma once

#include "Archive/Archive.h"
#include "WadArchive.h"

class WadJArchive : public WadArchive
{
public:
	WadJArchive();
	~WadJArchive();

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	string detectNamespace(ArchiveEntry* entry) override;
	string detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override;

	static bool isWadJArchive(MemChunk& mc);
	static bool isWadJArchive(string filename);

private:
	vector<ArchiveEntry*> entries_;
	char                  wad_type_[4];
	ArchiveEntry*         patches_[2];
	ArchiveEntry*         sprites_[2];
	ArchiveEntry*         flats_[2];
	ArchiveEntry*         tx_[2];
};
