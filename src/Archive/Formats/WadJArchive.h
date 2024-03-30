#pragma once

#include "WadArchive.h"

namespace slade
{
class WadJArchive : public WadArchive
{
public:
	WadJArchive();
	~WadJArchive() override = default;

	// Opening/writing
	bool open(const MemChunk& mc) override; // Open from MemChunk
	bool write(MemChunk& mc) override;      // Write to MemChunk

	string detectNamespace(ArchiveEntry* entry) override;
	string detectNamespace(unsigned index, ArchiveDir* dir = nullptr) override;

	static bool isWadJArchive(const MemChunk& mc);
	static bool isWadJArchive(const string& filename);

	static bool jaguarDecode(MemChunk& mc);

private:
	vector<ArchiveEntry*> entries_;
	char                  wad_type_[4] = { 'P', 'W', 'A', 'D' };
};
} // namespace slade
