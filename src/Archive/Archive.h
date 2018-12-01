#pragma once

#include "ArchiveEntry.h"
#include "ArchiveTreeNode.h"
#include "General/ListenerAnnouncer.h"

struct ArchiveFormat
{
	string              id;
	string              name;
	bool                supports_dirs;
	bool                names_extensions;
	int                 max_name_length;
	string              entry_format;
	vector<key_value_t> extensions;

	ArchiveFormat(string id) :
		id{ id },
		name{ id },
		supports_dirs{ false },
		names_extensions{ true },
		max_name_length{ -1 }
	{
	}
};

class Archive : public Announcer
{
public:
	struct MapDesc
	{
		string        name;
		ArchiveEntry* head;
		ArchiveEntry* end;
		uint8_t       format;  // See MapTypes enum
		bool          archive; // True if head is an archive (for maps in zips)

		vector<ArchiveEntry*> unk; // Unknown map lumps (must be preserved for UDMF compliance)

		MapDesc()
		{
			head = end = nullptr;
			archive    = false;
			format     = MAP_UNKNOWN;
		}
	};

	static bool save_backup;

	Archive(string format = "");
	virtual ~Archive();

	string           formatId() const { return format_; }
	string           filename(bool full = true);
	ArchiveEntry*    parentEntry() const { return parent_; }
	Archive*         parentArchive() const { return (parent_ ? parent_->parent() : nullptr); }
	ArchiveTreeNode* rootDir() { return &dir_root_; }
	bool             isModified() const { return modified_; }
	bool             isOnDisk() const { return on_disk_; }
	bool             isReadOnly() const { return read_only_; }
	virtual bool     isWritable() { return true; }

	void setModified(bool modified);
	void setFilename(string filename) { this->filename_ = filename; }

	// Entry retrieval/info
	bool                       checkEntry(ArchiveEntry* entry);
	virtual ArchiveEntry*      entry(string name, bool cut_ext = false, ArchiveTreeNode* dir = nullptr);
	virtual ArchiveEntry*      entryAt(unsigned index, ArchiveTreeNode* dir = nullptr);
	virtual int                entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir = nullptr);
	virtual ArchiveEntry*      entryAtPath(string path);
	virtual ArchiveEntry::SPtr entryAtPathShared(string path);

	// Archive type info
	ArchiveFormat formatDesc() const;
	string        fileExtensionString() const;
	virtual bool  isTreeless() { return false; }

	// Opening
	virtual bool open(string filename);     // Open from File
	virtual bool open(ArchiveEntry* entry); // Open from ArchiveEntry
	virtual bool open(MemChunk& mc) = 0;    // Open from MemChunk

	// Writing/Saving
	virtual bool write(MemChunk& mc, bool update = true) = 0; // Write to MemChunk
	virtual bool write(string filename, bool update = true);  // Write to File
	virtual bool save(string filename = "");                  // Save archive

	// Misc
	virtual bool     loadEntryData(ArchiveEntry* entry) = 0;
	virtual unsigned numEntries();
	virtual void     close();
	void             entryStateChanged(ArchiveEntry* entry);
	void             putEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start = nullptr);
	void             putEntryTreeAsList(vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* start = nullptr);
	bool             canSave() const { return parent_ || on_disk_; }
	virtual bool     paste(ArchiveTreeNode* tree, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* base = nullptr);
	virtual bool     importDir(string directory);
	virtual bool     hasFlatHack() { return false; }

	// Directory stuff
	virtual ArchiveTreeNode* dir(string path, ArchiveTreeNode* base = nullptr);
	virtual ArchiveTreeNode* createDir(string path, ArchiveTreeNode* base = nullptr);
	virtual bool             removeDir(string path, ArchiveTreeNode* base = nullptr);
	virtual bool             renameDir(ArchiveTreeNode* dir, string new_name);

	// Entry addition/removal
	virtual ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false);
	virtual ArchiveEntry* addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false)
	{
		return addEntry(entry, 0xFFFFFFFF, nullptr, false);
	} // By default, add to the 'global' namespace (ie root dir)
	virtual ArchiveEntry* addNewEntry(string name = "", unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr);
	virtual ArchiveEntry* addNewEntry(string name, string add_namespace);
	virtual bool          removeEntry(ArchiveEntry* entry);

	// Entry moving
	virtual bool swapEntries(unsigned index1, unsigned index2, ArchiveTreeNode* dir = nullptr);
	virtual bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr);

	// Entry modification
	virtual bool renameEntry(ArchiveEntry* entry, string name);
	virtual bool revertEntry(ArchiveEntry* entry);

	// Detection
	virtual MapDesc         mapDesc(ArchiveEntry* maphead) { return MapDesc(); }
	virtual vector<MapDesc> detectMaps() { return {}; }
	virtual string          detectNamespace(ArchiveEntry* entry);
	virtual string          detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr);

	// Search
	struct SearchOptions
	{
		string           match_name;      // Ignore if empty
		EntryType*       match_type;      // Ignore if NULL
		string           match_namespace; // Ignore if empty
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
	string        format_;
	string        filename_;
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
	TreelessArchive(string format = "") : Archive(format) {}
	virtual ~TreelessArchive() {}

	// Entry retrieval/info
	ArchiveEntry* entry(string name, bool cut_ext = false, ArchiveTreeNode* dir = nullptr) override
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
	ArchiveTreeNode* dir(string path, ArchiveTreeNode* base = nullptr) override { return rootDir(); }
	ArchiveTreeNode* createDir(string path, ArchiveTreeNode* base = nullptr) override { return rootDir(); }
	bool             removeDir(string path, ArchiveTreeNode* base = nullptr) override { return false; }
	bool             renameDir(ArchiveTreeNode* dir, string new_name) override { return false; }

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override
	{
		return Archive::addEntry(entry, position, nullptr, copy);
	}
	ArchiveEntry* addNewEntry(string name = "", unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::addNewEntry(name, position, nullptr);
	}

	// Entry moving
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = nullptr) override
	{
		return Archive::moveEntry(entry, position, nullptr);
	}

	// Detection
	string detectNamespace(ArchiveEntry* entry) override { return "global"; }
	string detectNamespace(size_t index, ArchiveTreeNode* dir = nullptr) override { return "global"; }
};
