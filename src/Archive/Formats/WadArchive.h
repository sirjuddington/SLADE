#pragma once

#include "Archive/Archive.h"

class WadArchive : public TreelessArchive
{
public:
	WadArchive();
	~WadArchive();

	// Wad specific
	bool     isIWAD() const { return iwad_; }
	bool     isWritable() override;
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);
	void     updateNamespaces();

	// Opening
	bool open(MemChunk& mc) override;

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;    // Write to MemChunk
	bool write(string filename, bool update = true) override; // Write to File

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override;
	ArchiveEntry* addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false) override;
	bool          removeEntry(ArchiveEntry* entry) override;

	// Entry modification
	string processEntryName(string name);
	bool   renameEntry(ArchiveEntry* entry, string name) override;

	// Entry moving
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override;
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps() override;
	string          detectNamespace(ArchiveEntry* entry) override;
	string          detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override;
	void            detectIncludes();
	bool            hasFlatHack() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// Static functions
	static bool isWadArchive(MemChunk& mc);
	static bool isWadArchive(string filename);

	static bool exportEntriesAsWad(string filename, vector<ArchiveEntry*> entries)
	{
		WadArchive wad;

		// Add entries to wad archive
		for (size_t a = 0; a < entries.size(); a++)
		{
			// Add each entry to the wad archive
			wad.addEntry(entries[a], entries.size(), nullptr, true);
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

		NSPair(ArchiveEntry* start, ArchiveEntry* end)
		{
			this->start = start;
			this->end   = end;
		}
	};

	bool           iwad_;
	vector<NSPair> namespaces_;
};
