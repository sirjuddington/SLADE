#pragma once

#include "Archive/Archive.h"

namespace slade
{
class WolfArchive : public TreelessArchive
{
public:
	WolfArchive() : TreelessArchive("wolf") {}
	~WolfArchive() override = default;

	// Opening
	bool open(string_view filename, bool detect_types) override; // Open from File
	bool open(const MemChunk& mc, bool detect_types) override;   // Open from MemChunk

	bool openAudio(MemChunk& head, const MemChunk& data);
	bool openGraph(const MemChunk& head, const MemChunk& data, MemChunk& dict, bool detect_types);
	bool openMaps(MemChunk& head, const MemChunk& data, bool detect_types);

	// Writing/Saving
	bool write(MemChunk& mc) override; // Write to MemChunk

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name, bool force = false) override { return false; }

	static bool isWolfArchive(const MemChunk& mc);
	static bool isWolfArchive(const string& filename);

private:
	struct WolfHandle
	{
		uint32_t offset;
		uint16_t size;
	};

	uint16_t spritestart_ = 0;
	uint16_t soundstart_  = 0;
};
} // namespace slade
