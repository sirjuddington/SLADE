#pragma once

#include "Archive/Archive.h"

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

	std::string entry_path;
	std::string file_path;
	Action      action;
	// Note that this is nonsense for deleted files
	time_t mtime;

	DirEntryChange(
		Action           action = Action::Updated,
		std::string_view file   = "",
		std::string_view entry  = "",
		time_t           mtime  = 0) :
		entry_path{ entry },
		file_path{ file },
		action{ action },
		mtime{ mtime }
	{
	}
};

typedef std::map<std::string, DirEntryChange> IgnoredFileChanges;

class DirArchive : public Archive
{
public:
	DirArchive();
	~DirArchive() = default;

	// Accessors
	const vector<std::string>& removedFiles() const { return removed_files_; }
	time_t                     fileModificationTime(ArchiveEntry* entry) { return file_modification_times_[entry]; }

	// Opening
	bool open(std::string_view filename) override; // Open from File
	bool open(ArchiveEntry* entry) override;       // Open from ArchiveEntry
	bool open(MemChunk& mc) override;              // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;              // Write to MemChunk
	bool write(std::string_view filename, bool update = true) override; // Write to File
	bool save(std::string_view filename = "") override;                 // Save archive

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Dir stuff
	bool removeDir(std::string_view path, ArchiveTreeNode* base = nullptr) override;
	bool renameDir(ArchiveTreeNode* dir, std::string_view new_name) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(ArchiveEntry* entry, std::string_view add_namespace, bool copy = false) override;
	bool          removeEntry(ArchiveEntry* entry) override;
	bool          renameEntry(ArchiveEntry* entry, std::string_view name) override;

	// Detection
	MapDesc         mapDesc(ArchiveEntry* entry) override;
	vector<MapDesc> detectMaps() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// DirArchive-specific
	void ignoreChangedEntries(vector<DirEntryChange>& changes);
	void updateChangedEntries(vector<DirEntryChange>& changes);
	bool shouldIgnoreEntryChange(DirEntryChange& change);

private:
	char                            separator_;
	vector<StringPair>              renamed_dirs_;
	std::map<ArchiveEntry*, time_t> file_modification_times_;
	vector<std::string>             removed_files_;
	IgnoredFileChanges              ignored_file_changes_;
};

class DirArchiveTraverser : public wxDirTraverser
{
public:
	DirArchiveTraverser(vector<std::string>& pathlist, vector<std::string>& dirlist) :
		paths_{ pathlist },
		dirs_{ dirlist }
	{
	}
	~DirArchiveTraverser() = default;

	wxDirTraverseResult OnFile(const wxString& filename) override
	{
		paths_.push_back(filename.ToStdString());
		return wxDIR_CONTINUE;
	}

	wxDirTraverseResult OnDir(const wxString& dirname) override
	{
		dirs_.push_back(dirname.ToStdString());
		return wxDIR_CONTINUE;
	}

private:
	vector<std::string>& paths_;
	vector<std::string>& dirs_;
};
