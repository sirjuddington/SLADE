#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class ZipArchiveHandler : public ArchiveFormatHandler
{
public:
	ZipArchiveHandler();
	~ZipArchiveHandler() override;

	void init(Archive& archive) override;

	// Opening
	bool open(Archive& archive, string_view filename) override; // Open from File
	bool open(Archive& archive, const MemChunk& mc) override;   // Open from MemChunk

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;         // Write to MemChunk
	bool write(Archive& archive, string_view filename) override; // Write to File

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;

	// Detection
	MapDesc         mapDesc(const Archive& archive, ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps(const Archive& archive) override;

	// Search
	ArchiveEntry*         findFirst(const Archive& archive, ArchiveSearchOptions& options) override;
	ArchiveEntry*         findLast(const Archive& archive, ArchiveSearchOptions& options) override;
	vector<ArchiveEntry*> findAll(const Archive& archive, ArchiveSearchOptions& options) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	string temp_file_;

	void generateTempFileName(string_view filename);
};
} // namespace slade
