#pragma once

#include "Archive/Archive.h"

class ZipArchive : public Archive
{
public:
	ZipArchive() : Archive("zip") {}
	~ZipArchive();

	// Opening
	bool open(std::string_view filename) override; // Open from File
	bool open(MemChunk& mc) override;             // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;             // Write to MemChunk
	bool write(std::string_view filename, bool update = true) override; // Write to File

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(ArchiveEntry* entry, std::string_view add_namespace, bool copy = false) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// Static functions
	static bool isZipArchive(MemChunk& mc);
	static bool isZipArchive(const std::string& filename);

private:
	std::string temp_file_;

	void generateTempFileName(std::string_view filename);
};
