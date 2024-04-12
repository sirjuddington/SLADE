#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class GZipArchiveHandler : public ArchiveFormatHandler
{
public:
	GZipArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::GZip, true) {}
	~GZipArchiveHandler() override = default;

	// Opening
	bool open(Archive& archive, const MemChunk& mc) override;

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override
	{
		return nullptr;
	}
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override
	{
		return nullptr;
	}
	bool removeEntry(Archive& archive, ArchiveEntry* entry, bool) override { return false; }

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override;

	// Entry moving
	bool swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2) override { return false; }
	bool moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr)
		override
	{
		return false;
	}

	// Search
	ArchiveEntry*         findFirst(Archive& archive, ArchiveSearchOptions& options) override;
	ArchiveEntry*         findLast(Archive& archive, ArchiveSearchOptions& options) override;
	vector<ArchiveEntry*> findAll(Archive& archive, ArchiveSearchOptions& options) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

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
