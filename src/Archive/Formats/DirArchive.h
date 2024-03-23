#pragma once

#include "Archive/Archive.h"

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

class DirArchive : public Archive
{
public:
	DirArchive();
	~DirArchive() override = default;

	// Accessors
	const vector<string>& removedFiles() const { return removed_files_; }
	time_t                fileModificationTime(ArchiveEntry* entry) { return file_modification_times_[entry]; }
	bool                  hiddenFilesIgnored() const { return ignore_hidden_; }
	bool                  saveErrorsOccurred() const { return save_errors_; }

	// Opening
	bool open(string_view filename) override; // Open from File
	bool open(ArchiveEntry* entry) override;  // Open from ArchiveEntry
	bool open(const MemChunk& mc) override;   // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc) override;             // Write to MemChunk
	bool write(string_view filename) override;     // Write to File
	bool save(string_view filename = "") override; // Save archive

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

	// Dir stuff
	shared_ptr<ArchiveDir> removeDir(string_view path, ArchiveDir* base = nullptr) override;
	bool                   renameDir(ArchiveDir* dir, string_view new_name) override;

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace) override;
	bool                     removeEntry(ArchiveEntry* entry, bool set_deleted = true) override;
	bool                     renameEntry(ArchiveEntry* entry, string_view name, bool force = false) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* entry) override;
	vector<MapDesc> detectMaps() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// DirArchive-specific
	void ignoreChangedEntries(const vector<DirEntryChange>& changes);
	void updateChangedEntries(vector<DirEntryChange>& changes);
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
	DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist, bool ignore_hidden) :
		paths_{ pathlist },
		dirs_{ dirlist },
		ignore_hidden_{ ignore_hidden }
	{
	}
	~DirArchiveTraverser() override = default;

	wxDirTraverseResult OnFile(const wxString& filename) override
	{
		auto path_str = filename.ToStdString();

		if (ignore_hidden_ && strutil::startsWith(strutil::Path::fileNameOf(path_str), '.'))
			return wxDIR_CONTINUE;

		paths_.push_back(path_str);
		return wxDIR_CONTINUE;
	}

	wxDirTraverseResult OnDir(const wxString& dirname) override
	{
		if (ignore_hidden_)
		{
			auto path_str = dirname.ToStdString();
			std::replace(path_str.begin(), path_str.end(), '\\', '/');
			auto dir = strutil::afterLastV(path_str, '/');
			if (strutil::startsWith(dir, '.'))
				return wxDIR_IGNORE;
		}

		dirs_.push_back(dirname.ToStdString());
		return wxDIR_CONTINUE;
	}

private:
	vector<string>& paths_;
	vector<string>& dirs_;
	bool            ignore_hidden_ = true;
};
} // namespace slade
