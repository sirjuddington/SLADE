#pragma once

#include "ArchiveDir.h"
#include "ArchiveEntry.h"
#include "General/Defs.h"
#include "General/ListenerAnnouncer.h"

struct ArchiveFormat
{
	string             id;
	string             name;
	bool               supports_dirs    = false;
	bool               names_extensions = true;
	int                max_name_length  = -1;
	string             entry_format;
	vector<StringPair> extensions;
	bool               prefer_uppercase = false;

	ArchiveFormat(string_view id) : id{ id }, name{ id } {}
};

class Archive : public Announcer
{
public:
	struct MapDesc
	{
		string        name;
		ArchiveEntry* head;
		ArchiveEntry* end;
		MapFormat     format;  // See MapTypes enum
		bool          archive; // True if head is an archive (for maps in zips)

		vector<ArchiveEntry*> unk; // Unknown map lumps (must be preserved for UDMF compliance)

		MapDesc()
		{
			head = end = nullptr;
			archive    = false;
			format     = MapFormat::Unknown;
		}
	};

	static bool save_backup;

	Archive(string_view format = "");
	virtual ~Archive();

	string                 formatId() const { return format_; }
	string                 filename(bool full = true) const;
	ArchiveEntry*          parentEntry() const { return parent_.lock().get(); }
	Archive*               parentArchive() const { return parent_.lock() ? parent_.lock()->parent() : nullptr; }
	shared_ptr<ArchiveDir> rootDir() const { return dir_root_; }
	bool                   isModified() const { return modified_; }
	bool                   isOnDisk() const { return on_disk_; }
	bool                   isReadOnly() const { return read_only_; }
	virtual bool           isWritable() { return true; }

	void setModified(bool modified);
	void setFilename(string_view filename) { this->filename_ = filename; }

	// Entry retrieval/info
	bool                             checkEntry(ArchiveEntry* entry);
	virtual ArchiveEntry*            entry(string_view name, bool cut_ext = false, ArchiveDir* dir = nullptr);
	virtual ArchiveEntry*            entryAt(unsigned index, ArchiveDir* dir = nullptr);
	virtual int                      entryIndex(ArchiveEntry* entry, ArchiveDir* dir = nullptr);
	virtual ArchiveEntry*            entryAtPath(string_view path);
	virtual shared_ptr<ArchiveEntry> entryAtPathShared(string_view path);

	// Archive type info
	ArchiveFormat formatDesc() const;
	string        fileExtensionString() const;
	virtual bool  isTreeless() { return false; }

	// Opening
	virtual bool open(string_view filename); // Open from File
	virtual bool open(ArchiveEntry* entry);  // Open from ArchiveEntry
	virtual bool open(MemChunk& mc) = 0;     // Open from MemChunk

	// Writing/Saving
	virtual bool write(MemChunk& mc, bool update = true) = 0;     // Write to MemChunk
	virtual bool write(string_view filename, bool update = true); // Write to File
	virtual bool save(string_view filename = "");                 // Save archive

	// Misc
	virtual bool     loadEntryData(ArchiveEntry* entry) = 0;
	virtual unsigned numEntries();
	virtual void     close();
	void             entryStateChanged(ArchiveEntry* entry);
	void             putEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveDir* start = nullptr) const;
	void             putEntryTreeAsList(vector<shared_ptr<ArchiveEntry>>& list, ArchiveDir* start = nullptr) const;
	bool             canSave() const { return parent_.lock() || on_disk_; }
	virtual bool     paste(ArchiveDir* tree, unsigned position = 0xFFFFFFFF, shared_ptr<ArchiveDir> base = nullptr);
	virtual bool     importDir(string_view directory);
	virtual bool     hasFlatHack() { return false; }

	// Directory stuff
	ArchiveDir*                    dirAtPath(string_view path, ArchiveDir* base = nullptr) const;
	virtual shared_ptr<ArchiveDir> createDir(string_view path, shared_ptr<ArchiveDir> base = nullptr);
	virtual shared_ptr<ArchiveDir> removeDir(string_view path, ArchiveDir* base = nullptr);
	virtual bool                   renameDir(ArchiveDir* dir, string_view new_name);

	// Entry addition/removal
	virtual shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr);
	virtual shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace)
	{
		return addEntry(entry, 0xFFFFFFFF, nullptr);
	} // By default, add to the 'global' namespace (ie root dir)
	virtual shared_ptr<ArchiveEntry> addNewEntry(
		string_view name     = "",
		unsigned    position = 0xFFFFFFFF,
		ArchiveDir* dir      = nullptr);
	virtual shared_ptr<ArchiveEntry> addNewEntry(string_view name, string_view add_namespace);
	virtual bool                     removeEntry(ArchiveEntry* entry);

	// Entry moving
	virtual bool swapEntries(unsigned index1, unsigned index2, ArchiveDir* dir = nullptr);
	virtual bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr);

	// Entry modification
	virtual bool renameEntry(ArchiveEntry* entry, string_view name);
	virtual bool revertEntry(ArchiveEntry* entry);

	// Detection
	virtual MapDesc         mapDesc(ArchiveEntry* maphead) { return MapDesc(); }
	virtual vector<MapDesc> detectMaps() { return {}; }
	virtual string          detectNamespace(ArchiveEntry* entry);
	virtual string          detectNamespace(size_t index, ArchiveDir* dir = nullptr);

	// Search
	struct SearchOptions
	{
		string      match_name;      // Ignore if empty
		EntryType*  match_type;      // Ignore if NULL
		string      match_namespace; // Ignore if empty
		ArchiveDir* dir;             // Root if NULL
		bool        ignore_ext;      // Defaults true
		bool        search_subdirs;  // Defaults false

		SearchOptions()
		{
			match_name      = "";
			match_type      = nullptr;
			match_namespace = "";
			dir             = nullptr;
			ignore_ext      = true;
			search_subdirs  = false;
		}
	};
	virtual ArchiveEntry*         findFirst(SearchOptions& options);
	virtual ArchiveEntry*         findLast(SearchOptions& options);
	virtual vector<ArchiveEntry*> findAll(SearchOptions& options);
	virtual vector<ArchiveEntry*> findModifiedEntries(ArchiveDir* dir = nullptr);

	// Static functions
	static bool                   loadFormats(MemChunk& mc);
	static vector<ArchiveFormat>& allFormats() { return formats_; }

protected:
	string                 format_;
	string                 filename_;
	weak_ptr<ArchiveEntry> parent_;
	bool                   on_disk_; // Specifies whether the archive exists on disk (as opposed to being newly created)
	bool                   read_only_; // If true, the archive cannot be modified

private:
	bool                   modified_;
	shared_ptr<ArchiveDir> dir_root_;

	static vector<ArchiveFormat> formats_;
};

// Base class for list-based archive formats
class TreelessArchive : public Archive
{
public:
	TreelessArchive(string_view format = "") : Archive(format) {}
	virtual ~TreelessArchive() = default;

	// Entry retrieval/info
	ArchiveEntry* entry(string_view name, bool cut_ext = false, ArchiveDir* dir = nullptr) override
	{
		return Archive::entry(name);
	}
	ArchiveEntry* entryAt(unsigned index, ArchiveDir* dir = nullptr) override
	{
		return Archive::entryAt(index, nullptr);
	}
	int entryIndex(ArchiveEntry* entry, ArchiveDir* dir = nullptr) override
	{
		return Archive::entryIndex(entry, nullptr);
	}

	// Misc
	unsigned numEntries() override { return rootDir()->numEntries(); }
	void     getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveDir* start = nullptr)
	{
		return Archive::putEntryTreeAsList(list, nullptr);
	}
	bool paste(ArchiveDir* tree, unsigned position = 0xFFFFFFFF, shared_ptr<ArchiveDir> base = nullptr) override;
	bool isTreeless() override { return true; }

	// Directory stuff
	shared_ptr<ArchiveDir> createDir(string_view path, shared_ptr<ArchiveDir> base = nullptr) override
	{
		return rootDir();
	}
	shared_ptr<ArchiveDir> removeDir(string_view path, ArchiveDir* base = nullptr) override { return nullptr; }
	bool                   renameDir(ArchiveDir* dir, string_view new_name) override { return false; }

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override
	{
		return Archive::addEntry(entry, position, nullptr);
	}
	shared_ptr<ArchiveEntry> addNewEntry(
		string_view name     = "",
		unsigned    position = 0xFFFFFFFF,
		ArchiveDir* dir      = nullptr) override
	{
		return Archive::addNewEntry(name, position, nullptr);
	}

	// Entry moving
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr) override
	{
		return Archive::moveEntry(entry, position, nullptr);
	}

	// Detection
	string detectNamespace(ArchiveEntry* entry) override { return "global"; }
	string detectNamespace(size_t index, ArchiveDir* dir = nullptr) override { return "global"; }
};
