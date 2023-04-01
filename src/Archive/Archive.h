#pragma once

#include "ArchiveDir.h"
#include "ArchiveEntry.h"
#include "General/Defs.h"

namespace slade
{
struct ArchiveFormat
{
	string             id;
	string             name;
	bool               supports_dirs    = false;
	bool               names_extensions = true;
	int                max_name_length  = -1;
	string             entry_format;
	vector<StringPair> extensions;
	bool               prefer_uppercase      = false;
	bool               create                = false;
	bool               allow_duplicate_names = true;

	ArchiveFormat(string_view id) : id{ id }, name{ id } {}
};

class Archive
{
public:
	struct MapDesc
	{
		string                 name;
		weak_ptr<ArchiveEntry> head;
		weak_ptr<ArchiveEntry> end;     // The last entry of the map data
		MapFormat              format;  // See MapTypes enum
		bool                   archive; // True if head is an archive (for maps in zips)

		vector<ArchiveEntry*> unk; // Unknown map lumps (must be preserved for UDMF compliance)

		MapDesc()
		{
			archive = false;
			format  = MapFormat::Unknown;
		}

		vector<ArchiveEntry*> entries(const Archive& parent, bool include_head = false) const;
		void                  updateMapFormatHints() const;
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
	time_t                 fileModifiedTime() const { return file_modified_; }

	void setModified(bool modified);
	void setFilename(string_view filename) { filename_ = filename; }

	// Entry retrieval/info
	bool                             checkEntry(const ArchiveEntry* entry) const;
	virtual ArchiveEntry*            entry(string_view name, bool cut_ext = false, ArchiveDir* dir = nullptr) const;
	virtual ArchiveEntry*            entryAt(unsigned index, ArchiveDir* dir = nullptr) const;
	virtual int                      entryIndex(ArchiveEntry* entry, ArchiveDir* dir = nullptr) const;
	virtual ArchiveEntry*            entryAtPath(string_view path) const;
	virtual shared_ptr<ArchiveEntry> entryAtPathShared(string_view path) const;

	// Archive type info
	ArchiveFormat formatDesc() const;
	string        fileExtensionString() const;
	virtual bool  isTreeless() { return false; }

	// Opening
	virtual bool open(string_view filename);   // Open from File
	virtual bool open(ArchiveEntry* entry);    // Open from ArchiveEntry
	virtual bool open(const MemChunk& mc) = 0; // Open from MemChunk

	// Writing/Saving
	virtual bool write(MemChunk& mc) = 0;         // Write to MemChunk
	virtual bool write(string_view filename);     // Write to File
	virtual bool save(string_view filename = ""); // Save archive

	// Misc
	virtual bool     loadEntryData(const ArchiveEntry* entry, MemChunk& out) = 0;
	virtual unsigned numEntries();
	virtual void     close();
	void             entryStateChanged(ArchiveEntry* entry);
	void             putEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveDir* start = nullptr) const;
	void             putEntryTreeAsList(vector<shared_ptr<ArchiveEntry>>& list, ArchiveDir* start = nullptr) const;
	bool             canSave() const { return parent_.lock() || on_disk_; }
	virtual bool     paste(ArchiveDir* tree, unsigned position = 0xFFFFFFFF, shared_ptr<ArchiveDir> base = nullptr);
	virtual bool importDir(string_view directory, bool ignore_hidden = false, shared_ptr<ArchiveDir> base = nullptr);
	virtual bool hasFlatHack() { return false; }

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
	virtual bool                     removeEntry(ArchiveEntry* entry, bool set_deleted = true);

	// Entry moving
	virtual bool swapEntries(unsigned index1, unsigned index2, ArchiveDir* dir = nullptr);
	virtual bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr);

	// Entry modification
	virtual bool renameEntry(ArchiveEntry* entry, string_view name, bool force = false);
	virtual bool revertEntry(ArchiveEntry* entry);

	// Detection
	virtual MapDesc         mapDesc(ArchiveEntry* maphead) { return {}; }
	virtual vector<MapDesc> detectMaps() { return {}; }
	virtual string          detectNamespace(ArchiveEntry* entry);
	virtual string          detectNamespace(unsigned index, ArchiveDir* dir = nullptr);

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

	// Signals
	struct Signals
	{
		sigslot::signal<Archive&, bool>                            modified;
		sigslot::signal<Archive&>                                  saved;
		sigslot::signal<Archive&>                                  closed;
		sigslot::signal<Archive&, ArchiveEntry&>                   entry_added;
		sigslot::signal<Archive&, ArchiveDir&, ArchiveEntry&>      entry_removed; // Archive, Parent Dir, Removed Entry
		sigslot::signal<Archive&, ArchiveEntry&>                   entry_state_changed;
		sigslot::signal<Archive&, ArchiveEntry&, string_view>      entry_renamed;
		sigslot::signal<Archive&, ArchiveDir&, unsigned, unsigned> entries_swapped; // Archive, Dir, Index 1, Index 2
		sigslot::signal<Archive&, ArchiveDir&>                     dir_added;
		sigslot::signal<Archive&, ArchiveDir&, ArchiveDir&>        dir_removed; // Archive, Parent dir, Removed Dir
	};
	Signals& signals() { return signals_; }
	void     blockModificationSignals(bool block = true);

	// Static functions
	static bool                   loadFormats(const MemChunk& mc);
	static vector<ArchiveFormat>& allFormats() { return formats_; }

protected:
	string                 format_;
	string                 filename_;
	weak_ptr<ArchiveEntry> parent_;
	bool   on_disk_       = false; // Specifies whether the archive exists on disk (as opposed to being newly created)
	bool   read_only_     = false; // If true, the archive cannot be modified
	time_t file_modified_ = 0;

	// Helpers
	bool genericLoadEntryData(const ArchiveEntry* entry, MemChunk& out) const;
	void detectAllEntryTypes() const;

private:
	bool                   modified_ = true;
	shared_ptr<ArchiveDir> dir_root_;
	Signals                signals_;

	static vector<ArchiveFormat> formats_;
};

// Base class for list-based archive formats
class TreelessArchive : public Archive
{
public:
	TreelessArchive(string_view format = "") : Archive(format) {}
	~TreelessArchive() override = default;

	// Entry retrieval/info
	ArchiveEntry* entry(string_view name, bool cut_ext = false, ArchiveDir* dir = nullptr) const override
	{
		return Archive::entry(name);
	}
	ArchiveEntry* entryAt(unsigned index, ArchiveDir* dir = nullptr) const override
	{
		return Archive::entryAt(index, nullptr);
	}
	int entryIndex(ArchiveEntry* entry, ArchiveDir* dir = nullptr) const override
	{
		return Archive::entryIndex(entry, nullptr);
	}

	// Misc
	unsigned numEntries() override { return rootDir()->numEntries(); }
	void     getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveDir* start = nullptr) const
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
	string detectNamespace(unsigned index, ArchiveDir* dir = nullptr) override { return "global"; }
};

// Simple class that will block and unblock modification signals for an archive via RAII
class ArchiveModSignalBlocker
{
public:
	ArchiveModSignalBlocker(Archive& archive) : archive_{ &archive } { archive_->blockModificationSignals(); }
	~ArchiveModSignalBlocker() { archive_->blockModificationSignals(false); }

	void unblock() const { archive_->blockModificationSignals(false); }

private:
	Archive* archive_;
};

} // namespace slade
