#pragma once

#include "Archive/Archive.h"

namespace slade
{
class GZipArchive : public TreelessArchive
{
public:
	GZipArchive() : TreelessArchive("gzip") {}
	~GZipArchive() override = default;

	// Opening
	bool open(const MemChunk& mc, bool detect_types) override;

	// Writing/Saving
	bool write(MemChunk& mc) override;

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override
	{
		return nullptr;
	}
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override
	{
		return nullptr;
	}
	bool removeEntry(ArchiveEntry* entry, bool) override { return false; }

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name) override;

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override { return false; }
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr) override
	{
		return false;
	}

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) const override;
	ArchiveEntry*         findLast(SearchOptions& options) const override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) const override;

	// Static functions
	static bool isGZipArchive(const MemChunk& mc);
	static bool isGZipArchive(const string& filename);

private:
	string   comment_;
	MemChunk xtra_;
	uint8_t  flags_ = 0;
	uint32_t mtime_ = 0;
	uint8_t  xfl_   = 0;
	uint8_t  os_    = 0;

	static constexpr int ID1       = 0x1F;
	static constexpr int ID2       = 0x8B;
	static constexpr int DEFLATE   = 0x08;
	static constexpr int FLG_FTEXT = 0x01;
	static constexpr int FLG_FHCRC = 0x02;
	static constexpr int FLG_FXTRA = 0x04;
	static constexpr int FLG_FNAME = 0x08;
	static constexpr int FLG_FCMNT = 0x10;
	static constexpr int FLG_FUNKN = 0xE0;
};
} // namespace slade
