#pragma once

#include "ArchiveFormat.h"
#include "MapDesc.h"

namespace slade
{
struct ArchiveSearchOptions;

class ArchiveFormatHandler
{
public:
	ArchiveFormatHandler(ArchiveFormat format, bool treeless = false);
	virtual ~ArchiveFormatHandler() = default;

	virtual void init(Archive& archive) {}

	// Archive type info
	virtual bool  isWritable() { return true; }
	bool          isTreeless() const { return treeless_; }
	virtual bool  hasFlatHack() { return false; }
	ArchiveFormat format() const { return format_; }

	// Opening
	virtual bool open(Archive& archive, string_view filename); // Open from File
	virtual bool open(Archive& archive, ArchiveEntry* entry);  // Open from ArchiveEntry
	virtual bool open(Archive& archive, const MemChunk& mc);   // Open from MemChunk

	// Writing/Saving
	virtual bool write(Archive& archive, MemChunk& mc);             // Write to MemChunk
	virtual bool write(Archive& archive, string_view filename);     // Write to File
	virtual bool save(Archive& archive, string_view filename = ""); // Save archive

	// Data
	virtual bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out);

	// Directory stuff
	virtual shared_ptr<ArchiveDir> createDir(Archive& archive, string_view path, shared_ptr<ArchiveDir> base = nullptr);
	virtual shared_ptr<ArchiveDir> removeDir(Archive& archive, string_view path, ArchiveDir* base = nullptr);
	virtual bool                   renameDir(Archive& archive, ArchiveDir* dir, string_view new_name);

	// Entry addition/removal
	virtual shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr);
	virtual shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		string_view              add_namespace);
	virtual shared_ptr<ArchiveEntry> addNewEntry(
		Archive&    archive,
		string_view name     = "",
		unsigned    position = 0xFFFFFFFF,
		ArchiveDir* dir      = nullptr);
	virtual bool removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted = true);

	// Entry moving
	virtual bool swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool moveEntry(
		Archive&      archive,
		ArchiveEntry* entry,
		unsigned      position = 0xFFFFFFFF,
		ArchiveDir*   dir      = nullptr);

	// Entry modification
	virtual bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false);

	// Detection
	virtual MapDesc         mapDesc(Archive& archive, ArchiveEntry* maphead);
	virtual vector<MapDesc> detectMaps(Archive& archive);
	virtual string          detectNamespace(Archive& archive, ArchiveEntry* entry);
	virtual string          detectNamespace(Archive& archive, unsigned index, ArchiveDir* dir = nullptr);

	// Search
	virtual ArchiveEntry*         findFirst(Archive& archive, ArchiveSearchOptions& options);
	virtual ArchiveEntry*         findLast(Archive& archive, ArchiveSearchOptions& options);
	virtual vector<ArchiveEntry*> findAll(Archive& archive, ArchiveSearchOptions& options);

	// Format detection
	virtual bool isThisFormat(const MemChunk& mc);
	virtual bool isThisFormat(const string& filename);

protected:
	ArchiveFormat format_;
	bool          treeless_ = false;

	// Temp shortcuts
	void detectAllEntryTypes(const Archive& archive);
};

namespace archive
{
	unique_ptr<ArchiveFormatHandler> formatHandler(ArchiveFormat format);
	unique_ptr<ArchiveFormatHandler> formatHandler(string_view format);
	ArchiveFormat                    detectArchiveFormat(const MemChunk& mc);
	ArchiveFormat                    detectArchiveFormat(const string& filename);
	bool                             isFormat(const MemChunk& mc, ArchiveFormat format);
	bool                             isFormat(const string& filename, ArchiveFormat format);
} // namespace archive
} // namespace slade
