#pragma once

#include "Archive/Archive.h"
#include "WadArchive.h"

class WadJArchive : public WadArchive
{
public:
	WadJArchive();
	~WadJArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	string detectNamespace(ArchiveEntry* entry) override;
	string detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override;

	static bool isWadJArchive(MemChunk& mc);
	static bool isWadJArchive(const string& filename);

	static bool jaguarDecode(MemChunk& mc);

private:
	vector<ArchiveEntry*> entries_;
	char                  wad_type_[4] = { 'P', 'W', 'A', 'D' };
};
