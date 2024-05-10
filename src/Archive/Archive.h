#pragma once

namespace slade
{
struct ArchiveFormatInfo;
class ArchiveFormatHandler;
class EntryType;
enum class ArchiveFormat;
struct MapDesc;

struct ArchiveSearchOptions
{
	string      match_name;      // Ignore if empty
	EntryType*  match_type;      // Ignore if NULL
	string      match_namespace; // Ignore if empty
	ArchiveDir* dir;             // Root if NULL
	bool        ignore_ext;      // Defaults true
	bool        search_subdirs;  // Defaults false

	ArchiveSearchOptions()
	{
		match_name      = "";
		match_type      = nullptr;
		match_namespace = "";
		dir             = nullptr;
		ignore_ext      = true;
		search_subdirs  = false;
	}
};

class Archive
{
	friend class ArchiveFormatHandler;

public:
	static bool save_backup;

	Archive(string_view format = "");
	Archive(ArchiveFormat format);
	virtual ~Archive();

	string                 filename(bool full = true) const;
	ArchiveEntry*          parentEntry() const { return parent_.lock().get(); }
	Archive*               parentArchive() const;
	shared_ptr<ArchiveDir> rootDir() const { return dir_root_; }
	bool                   isModified() const { return modified_; }
	bool                   isOnDisk() const { return on_disk_; }
	bool                   isReadOnly() const { return read_only_; }
	bool                   isWritable() const;
	time_t                 fileModifiedTime() const { return file_modified_; }
	int64_t                libraryId() const { return library_id_; }

	void setModified(bool modified);
	void setFilename(string_view filename) { filename_ = filename; }
	void setLibraryId(int64_t library_id) const { library_id_ = library_id; }

	// Entry retrieval/info
	bool                     checkEntry(const ArchiveEntry* entry) const;
	ArchiveEntry*            entry(string_view name, bool cut_ext = false, const ArchiveDir* dir = nullptr) const;
	ArchiveEntry*            entryAt(unsigned index, const ArchiveDir* dir = nullptr) const;
	int                      entryIndex(ArchiveEntry* entry, const ArchiveDir* dir = nullptr) const;
	ArchiveEntry*            entryAtPath(string_view path) const;
	shared_ptr<ArchiveEntry> entryAtPathShared(string_view path) const;

	// Archive type info
	const ArchiveFormatInfo& formatInfo() const;
	string                   fileExtensionString() const;
	bool                     isTreeless() { return false; }

	// Format Handler
	ArchiveFormatHandler& formatHandler() const { return *format_handler_; }
	ArchiveFormat         format() const;
	string                formatId() const;

	// Opening
	bool open(string_view filename, bool detect_types); // Open from File
	bool open(ArchiveEntry* entry, bool detect_types);  // Open from ArchiveEntry
	bool open(const MemChunk& mc, bool detect_types);   // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc);             // Write to MemChunk
	bool write(string_view filename);     // Write to File
	bool save(string_view filename = ""); // Save archive

	// Misc
	bool     loadEntryData(const ArchiveEntry* entry, MemChunk& out);
	unsigned numEntries() const;
	void     close();
	void     entryStateChanged(ArchiveEntry* entry);
	void     putEntryTreeAsList(vector<ArchiveEntry*>& list, const ArchiveDir* start = nullptr) const;
	void     putEntryTreeAsList(vector<shared_ptr<ArchiveEntry>>& list, const ArchiveDir* start = nullptr) const;
	bool     canSave() const { return parent_.lock() || on_disk_; }
	bool     paste(ArchiveDir* tree, unsigned position = 0xFFFFFFFF, shared_ptr<ArchiveDir> base = nullptr);
	bool     importDir(string_view directory, bool ignore_hidden = false, shared_ptr<ArchiveDir> base = nullptr);
	bool     hasFlatHack() { return false; }

	// Directory stuff
	ArchiveDir*            dirAtPath(string_view path, ArchiveDir* base = nullptr) const;
	shared_ptr<ArchiveDir> createDir(string_view path, shared_ptr<ArchiveDir> base = nullptr);
	shared_ptr<ArchiveDir> removeDir(string_view path, ArchiveDir* base = nullptr);
	bool                   renameDir(ArchiveDir* dir, string_view new_name);

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr);
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace);
	shared_ptr<ArchiveEntry> addNewEntry(
		string_view name     = "",
		unsigned    position = 0xFFFFFFFF,
		ArchiveDir* dir      = nullptr);
	shared_ptr<ArchiveEntry> addNewEntry(string_view name, string_view add_namespace);
	bool                     removeEntry(ArchiveEntry* entry, bool set_deleted = true);

	// Entry moving
	bool swapEntries(unsigned index1, unsigned index2, const ArchiveDir* dir = nullptr);
	bool swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	bool moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveDir* dir = nullptr);

	// Entry modification
	bool renameEntry(ArchiveEntry* entry, string_view name, bool force = false);
	bool revertEntry(ArchiveEntry* entry);

	// Detection
	MapDesc         mapDesc(ArchiveEntry* maphead);
	vector<MapDesc> detectMaps() const;
	string          detectNamespace(ArchiveEntry* entry) const;
	string          detectNamespace(unsigned index, ArchiveDir* dir = nullptr) const;
	void            detectAllEntryTypes(bool show_in_splash_window = true) const;

	// Search
	ArchiveEntry*         findFirst(ArchiveSearchOptions& options);
	ArchiveEntry*         findLast(ArchiveSearchOptions& options);
	vector<ArchiveEntry*> findAll(ArchiveSearchOptions& options);
	vector<ArchiveEntry*> findModifiedEntries(ArchiveDir* dir = nullptr);

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

protected:
	string                 filename_;
	weak_ptr<ArchiveEntry> parent_;
	bool   on_disk_       = false; // Specifies whether the archive exists on disk (as opposed to being newly created)
	bool   read_only_     = false; // If true, the archive cannot be modified
	time_t file_modified_ = 0;

private:
	bool                             modified_ = true;
	shared_ptr<ArchiveDir>           dir_root_;
	Signals                          signals_;
	unique_ptr<ArchiveFormatHandler> format_handler_;
	mutable int64_t                  library_id_ = -1;
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
