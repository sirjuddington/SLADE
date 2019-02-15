#pragma once

#include "ArchiveEntry.h"
#include "ArchiveTreeNode.h"
#include "General/Defs.h"
#include "General/ListenerAnnouncer.h"

struct ArchiveFormat
{
	std::string        id;
	std::string        name;
	bool               supports_dirs    = false;
	bool               names_extensions = true;
	int                max_name_length  = -1;
	std::string        entry_format;
	vector<StringPair> extensions;
	bool               prefer_uppercase = false;

	ArchiveFormat(std::string_view id) : id{ id }, name{ id } {}
};

class Archive : public Announcer
{
public:
	struct MapDesc
	{
		std::string   name;
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

	typedef std::unique_ptr<Archive> UPtr;

	Archive(std::string_view format = "");
	virtual ~Archive();

	std::string      formatId() const { return format_; }
	std::string      filename(bool full = true) const;
	ArchiveEntry*    parentEntry() const { return parent_; }
	Archive*         parentArchive() const { return (parent_ ? parent_->parent() : nullptr); }
	ArchiveTreeNode* rootDir() { return &dir_root_; }
	bool             isModified() const { return modified_; }
	bool             isOnDisk() const { return on_disk_; }
	bool             isReadOnly() const { return read_only_; }
	virtual bool     isWritable() { return true; }

	void setModified(bool modified);
	void setFilename(std::string_view filename) { this->filename_ = filename; }

	// Entry retrieval/info
	bool                       checkEntry(ArchiveEntry* entry);
	virtual ArchiveEntry*      entry(std::string_view name, bool cut_ext = false, ArchiveTreeNode* dir = nullptr);
	virtual ArchiveEntry*      entryAt(unsigned index, ArchiveTreeNode* dir = nullptr);
	virtual int                entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir = nullptr);
	virtual ArchiveEntry*      entryAtPath(std::string_view path);
	virtual ArchiveEntry::SPtr entryAtPathShared(std::string_view path);

	// Archive type info
	ArchiveFormat formatDesc() const;
	std::string   fileExtensionString() const;
	virtual bool  isTreeless() { return false; }

	// Opening
	virtual bool open(std::string_view filename); // Open from File
	virtual bool open(ArchiveEntry* entry);       // Open from ArchiveEntry
	virtual bool open(MemChunk& mc) = 0;          // Open from MemChunk

	// Writing/Saving
	virtual bool write(MemChunk& mc, bool update = true) = 0;          // Write to MemChunk
	virtual bool write(std::string_view filename, bool update = true); // Write to File
	virtual bool save(std::string_view filename = "");                 // Save archive

	// Misc
	virtual bool     loadEntryData(ArchiveEntry* entry) = 0;
	virtual unsigned numEntries();
	virtual void     close();
	void             entryStateChanged(ArchiveEntry* entry);
	void             putEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start = nullptr);
	void             putEntryTreeAsList(vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* start = nullptr);
	bool             canSave() const { return parent_ || on_disk_; }
	virtual bool     paste(ArchiveTreeNode* tree, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* base = nullptr);
	virtual bool     importDir(std::string_view directory);
	virtual bool     hasFlatHack() { return false; }

	// Directory stuff
	virtual ArchiveTreeNode* dir(std::string_view path, ArchiveTreeNode* base = nullptr);
	virtual ArchiveTreeNode* createDir(std::string_view path, ArchiveTreeNode* base = nullptr);
	virtual bool             removeDir(std::string_view path, ArchiveTreeNode* base = nullptr);
	virtual bool             renameDir(ArchiveTreeNode* dir, std::string_view new_name);

	// Entry addition/removal
	virtual ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false);
	virtual ArchiveEntry* addEntry(ArchiveEntry* entry, std::string_view add_namespace, bool copy = false)
	{
		return addEntry(entry, 0xFFFFFFFF, nullptr, false);
	} // By default, add to the 'global' namespace (ie root dir)
	virtual ArchiveEntry* addNewEntry(
		std::string_view name     = "",
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr);
	virtual ArchiveEntry* addNewEntry(std::string_view name, std::string_view add_namespace);
	virtual bool          removeEntry(ArchiveEntry* entry);

	// Entry moving
	virtual bool swapEntries(unsigned index1, unsigned index2, ArchiveTreeNode* dir = nullptr);
	virtual bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr);

	// Entry modification
	virtual bool renameEntry(ArchiveEntry* entry, std::string_view name);
	virtual bool revertEntry(ArchiveEntry* entry);

	// Detection
	virtual MapDesc         mapDesc(ArchiveEntry* maphead) { return MapDesc(); }
	virtual vector<MapDesc> detectMaps() { return {}; }
	virtual std::string     detectNamespace(ArchiveEntry* entry);
	virtual std::string     detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr);

	// Search
	struct SearchOptions
	{
		std::string      match_name;      // Ignore if empty
		EntryType*       match_type;      // Ignore if NULL
		std::string      match_namespace; // Ignore if empty
		ArchiveTreeNode* dir;             // Root if NULL
		bool             ignore_ext;      // Defaults true
		bool             search_subdirs;  // Defaults false

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
	virtual vector<ArchiveEntry*> findModifiedEntries(ArchiveTreeNode* dir = nullptr);

	// Static functions
	static bool                   loadFormats(MemChunk& mc);
	static vector<ArchiveFormat>& allFormats() { return formats; }

protected:
	std::string   format_;
	std::string   filename_;
	ArchiveEntry* parent_;
	bool          on_disk_;   // Specifies whether the archive exists on disk (as opposed to being newly created)
	bool          read_only_; // If true, the archive cannot be modified

private:
	bool            modified_;
	ArchiveTreeNode dir_root_;

	static vector<ArchiveFormat> formats;
};

// Base class for list-based archive formats
class TreelessArchive : public Archive
{
public:
	TreelessArchive(std::string_view format = "") : Archive(format) {}
	virtual ~TreelessArchive() = default;

	// Entry retrieval/info
	ArchiveEntry* entry(std::string_view name, bool cut_ext = false, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::entry(name);
	}
	ArchiveEntry* entryAt(unsigned index, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::entryAt(index, nullptr);
	}
	int entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::entryIndex(entry, nullptr);
	}

	// Misc
	unsigned numEntries() override { return rootDir()->numEntries(); }
	void     getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start = nullptr)
	{
		return Archive::putEntryTreeAsList(list, nullptr);
	}
	bool paste(ArchiveTreeNode* tree, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* base = nullptr) override;
	bool isTreeless() override { return true; }

	// Directory stuff
	ArchiveTreeNode* dir(std::string_view path, ArchiveTreeNode* base = nullptr) override { return rootDir(); }
	ArchiveTreeNode* createDir(std::string_view path, ArchiveTreeNode* base = nullptr) override { return rootDir(); }
	bool             removeDir(std::string_view path, ArchiveTreeNode* base = nullptr) override { return false; }
	bool             renameDir(ArchiveTreeNode* dir, std::string_view new_name) override { return false; }

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override
	{
		return Archive::addEntry(entry, position, nullptr, copy);
	}
	ArchiveEntry* addNewEntry(
		std::string_view name     = "",
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr) override
	{
		return Archive::addNewEntry(name, position, nullptr);
	}

	// Entry moving
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::moveEntry(entry, position, nullptr);
	}

	// Detection
	std::string detectNamespace(ArchiveEntry* entry) override { return "global"; }
	std::string detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override { return "global"; }
};
