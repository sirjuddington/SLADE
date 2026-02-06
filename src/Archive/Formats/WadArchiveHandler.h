#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class WadArchiveHandler : public ArchiveFormatHandler
{
public:
	WadArchiveHandler(ArchiveFormat format = ArchiveFormat::Wad) : ArchiveFormatHandler(format, true) {}
	~WadArchiveHandler() override = default;

	// Wad specific
	bool isIWAD() const { return iwad_; }
	bool isWritable() override;
	void updateNamespaces(Archive& archive);

	// Opening
	bool open(Archive& archive, const MemChunk& mc) override;

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;         // Write to MemChunk
	bool write(Archive& archive, string_view filename) override; // Write to File

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;
	bool removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted = true) override;

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override;

	// Entry moving
	bool swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr)
		override;

	// Detection
	MapDesc         mapDesc(const Archive& archive, ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps(const Archive& archive) override;
	string          detectNamespace(const Archive& archive, ArchiveEntry* entry) override;
	string          detectNamespace(const Archive& archive, unsigned index, ArchiveDir* dir = nullptr) override;
	void            detectIncludes(Archive& archive);
	bool            hasFlatHack() override;

	// Search
	ArchiveEntry*         findFirst(const Archive& archive, ArchiveSearchOptions& options) override;
	ArchiveEntry*         findLast(const Archive& archive, ArchiveSearchOptions& options) override;
	vector<ArchiveEntry*> findAll(const Archive& archive, ArchiveSearchOptions& options) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

	static bool exportEntriesAsWad(string_view filename, const vector<ArchiveEntry*>& entries);

	friend class WadJArchiveHandler;

private:
	// Struct to hold namespace info
	struct NSPair
	{
		ArchiveEntry* start; // eg. P_START
		size_t        start_index;
		ArchiveEntry* end; // eg. P_END
		size_t        end_index;
		string        name; // eg. "P" (since P or PP is a special case will be set to "patches")

		NSPair(ArchiveEntry* start, ArchiveEntry* end) : start{ start }, start_index{ 0 }, end{ end }, end_index{ 0 } {}
	};

	bool           iwad_ = false;
	vector<NSPair> namespaces_;
};
} // namespace slade
