#pragma once

#include "Archive/Archive.h"
#include "common.h"

struct DirEntryChange
{
	enum
	{
		UPDATED      = 0,
		DELETED_FILE = 1,
		DELETED_DIR  = 2,
		ADDED_FILE   = 3,
		ADDED_DIR    = 4
	};

	string entry_path;
	string file_path;
	int    action;
	// Note that this is nonsense for deleted files
	time_t mtime;

	DirEntryChange(int action = UPDATED, string file = "", string entry = "", time_t mtime = 0)
	{
		this->action     = action;
		this->entry_path = entry;
		this->file_path  = file;
		this->mtime      = mtime;
	}
};

typedef std::map<string, DirEntryChange> IgnoredFileChanges;

class DirArchive : public Archive
{
public:
	DirArchive();
	~DirArchive();

	// Accessors
	const vector<string>& removedFiles() const { return removed_files_; }
	time_t                fileModificationTime(ArchiveEntry* entry) { return file_modification_times_[entry]; }

	// Opening
	bool open(string filename) override;     // Open from File
	bool open(ArchiveEntry* entry) override; // Open from ArchiveEntry
	bool open(MemChunk& mc) override;        // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;    // Write to MemChunk
	bool write(string filename, bool update = true) override; // Write to File
	bool save(string filename = "") override;                 // Save archive

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Dir stuff
	bool removeDir(string path, ArchiveTreeNode* base = nullptr) override;
	bool renameDir(ArchiveTreeNode* dir, string new_name) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false) override;
	bool          removeEntry(ArchiveEntry* entry) override;
	bool          renameEntry(ArchiveEntry* entry, string name) override;

	// Detection
	MapDesc         getMapInfo(ArchiveEntry* maphead) override;
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
	string                          separator_;
	vector<key_value_t>             renamed_dirs_;
	std::map<ArchiveEntry*, time_t> file_modification_times_;
	vector<string>                  removed_files_;
	IgnoredFileChanges              ignored_file_changes_;
};

class DirArchiveTraverser : public wxDirTraverser
{
public:
	DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist) : paths_{ pathlist }, dirs_{ dirlist } {}
	~DirArchiveTraverser() {}

	wxDirTraverseResult OnFile(const wxString& filename) override
	{
		paths_.push_back(filename);
		return wxDIR_CONTINUE;
	}

	wxDirTraverseResult OnDir(const wxString& dirname) override
	{
		dirs_.push_back(dirname);
		return wxDIR_CONTINUE;
	}

private:
	vector<string>& paths_;
	vector<string>& dirs_;
};
