#pragma once

#include "Archive/Archive.h"

namespace slade
{
class WadArchive : public TreelessArchive
{
public:
	WadArchive() : TreelessArchive("wad") {}
	~WadArchive() override = default;

	// Wad specific
	bool isIWAD() const { return iwad_; }
	bool isWritable() override;
	void updateNamespaces();

	// Opening
	bool open(const MemChunk& mc, bool detect_types) override;

	// Writing/Saving
	bool write(MemChunk& mc) override;         // Write to MemChunk
	bool write(string_view filename) override; // Write to File

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;
	bool                     removeEntry(ArchiveEntry* entry, bool set_deleted = true) override;

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name, bool force = false) override;

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* maphead) const override;
	vector<MapDesc> detectMaps() const override;
	string          detectNamespace(ArchiveEntry* entry) const override;
	string          detectNamespace(unsigned index, ArchiveDir* dir = nullptr) const override;
	void            detectIncludes();
	bool            hasFlatHack() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) const override;
	ArchiveEntry*         findLast(SearchOptions& options) const override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) const override;

	// Static functions
	static bool isWadArchive(const MemChunk& mc);
	static bool isWadArchive(const string& filename);

	static bool exportEntriesAsWad(string_view filename, vector<ArchiveEntry*> entries)
	{
		WadArchive wad;

		// Add entries to wad archive
		for (size_t a = 0; a < entries.size(); a++)
		{
			// Add each entry to the wad archive
			wad.addEntry(std::make_shared<ArchiveEntry>(*entries[a]), entries.size(), nullptr);
		}

		return wad.save(filename);
	}

	friend class WadJArchive;

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
