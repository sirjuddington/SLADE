#pragma once

#include "Archive/Archive.h"

namespace slade
{
class VWadArchive : public Archive
{
public:
	VWadArchive();
	~VWadArchive() override;

	// Opening
	bool open(string_view filename) override; // Open from File
	bool open(MemChunk& mc) override;         // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;         // Write to MemChunk
	bool write(string_view filename, bool update = true) override; // Write to File

	// Misc
	bool   loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// Metadata
	string getAuthor();
	string getTitle();
	string getComment();
	bool   isSigned();
	string getPublicKey();

	// Static functions
	static bool isVWadArchive(MemChunk& mc);
	static bool isVWadArchive(const string& filename);

private:
	string temp_file_;
	string author_;
	string title_;
	string comment_;
	bool   signed_;
	string pubkey_;

	void generateTempFileName(string_view filename);
};

namespace vwad
{
	string generatePrivateKey();
	string derivePublicKey(string_view privkey);
}
} // namespace slade
