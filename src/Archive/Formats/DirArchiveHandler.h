#pragma once

#include "Archive/ArchiveFormatHandler.h"
#include "Utility/StringPair.h"

namespace slade
{
struct DirEntryChange
{
	enum class Action
	{
		Updated     = 0,
		DeletedFile = 1,
		DeletedDir  = 2,
		AddedFile   = 3,
		AddedDir    = 4
	};

	string entry_path;
	string file_path;
	Action action;
	// Note that this is nonsense for deleted files
	time_t mtime;

	DirEntryChange(Action action = Action::Updated, string_view file = "", string_view entry = "", time_t mtime = 0) :
		entry_path{ entry },
		file_path{ file },
		action{ action },
		mtime{ mtime }
	{
	}
};

typedef std::map<string, DirEntryChange> IgnoredFileChanges;

class DirArchiveHandler : public ArchiveFormatHandler
{
public:
	DirArchiveHandler();
	~DirArchiveHandler() override = default;

	// Accessors
	const vector<string>& removedFiles() const { return removed_files_; }
	time_t                fileModificationTime(ArchiveEntry* entry) { return file_modification_times_[entry]; }
	bool                  hiddenFilesIgnored() const { return ignore_hidden_; }
	bool                  saveErrorsOccurred() const { return save_errors_; }

	// Opening
	bool open(Archive& archive, string_view filename) override; // Open from File
	bool open(Archive& archive, ArchiveEntry* entry) override;  // Open from ArchiveEntry
	bool open(Archive& archive, const MemChunk& mc) override;   // Open from MemChunk

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;             // Write to MemChunk
	bool write(Archive& archive, string_view filename) override;     // Write to File
	bool save(Archive& archive, string_view filename = "") override; // Save archive

	// Misc
	bool loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out) override;

	// Dir stuff
	shared_ptr<ArchiveDir> removeDir(Archive& archive, string_view path, ArchiveDir* base = nullptr) override;
	bool                   renameDir(Archive& archive, ArchiveDir* dir, string_view new_name) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;
	bool removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted = true) override;
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override;

	// Detection
	MapDesc         mapDesc(const Archive& archive, ArchiveEntry* entry) override;
	vector<MapDesc> detectMaps(const Archive& archive) override;

	// Search
	ArchiveEntry*         findFirst(const Archive& archive, ArchiveSearchOptions& options) override;
	ArchiveEntry*         findLast(const Archive& archive, ArchiveSearchOptions& options) override;
	vector<ArchiveEntry*> findAll(const Archive& archive, ArchiveSearchOptions& options) override;

	// DirArchiveHandler-specific
	void ignoreChangedEntries(const vector<DirEntryChange>& changes);
	void updateChangedEntries(Archive& archive, vector<DirEntryChange>& changes);
	bool shouldIgnoreEntryChange(const DirEntryChange& change);

private:
	char                            separator_;
	vector<StringPair>              renamed_dirs_;
	std::map<ArchiveEntry*, time_t> file_modification_times_;
	vector<string>                  removed_files_;
	IgnoredFileChanges              ignored_file_changes_;
	bool                            ignore_hidden_;
	bool                            save_errors_ = false;
};

class DirArchiveTraverser : public wxDirTraverser
{
public:
	DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist, bool ignore_hidden);
	~DirArchiveTraverser() override = default;

	wxDirTraverseResult OnFile(const wxString& filename) override;
	wxDirTraverseResult OnDir(const wxString& dirname) override;

private:
	vector<string>& paths_;
	vector<string>& dirs_;
	bool            ignore_hidden_ = true;
};
} // namespace slade
