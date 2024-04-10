#pragma once

#include "WadArchiveHandler.h"

namespace slade
{
class WadJArchiveHandler : public WadArchiveHandler
{
public:
	WadJArchiveHandler();
	~WadJArchiveHandler() override = default;

	// Opening/writing
	bool open(Archive& archive, const MemChunk& mc) override; // Open from MemChunk
	bool write(Archive& archive, MemChunk& mc) override;      // Write to MemChunk

	string detectNamespace(Archive& archive, ArchiveEntry* entry) override;
	string detectNamespace(Archive& archive, unsigned index, ArchiveDir* dir = nullptr) override;

	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

	static bool jaguarDecode(MemChunk& mc);

private:
	vector<ArchiveEntry*> entries_;
	char                  wad_type_[4] = { 'P', 'W', 'A', 'D' };
};
} // namespace slade
