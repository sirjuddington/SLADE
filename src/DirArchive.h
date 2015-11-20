
#ifndef __DIR_ARCHIVE_H__
#define __DIR_ARCHIVE_H__

#include "Archive.h"
#include <map>
#include <wx/dir.h>

typedef std::map<ArchiveEntry*, time_t> mod_times_t;

struct dir_entry_change_t
{
	enum
	{
		UPDATED = 0,
		DELETED_FILE = 1,
		DELETED_DIR = 2,
		ADDED_FILE = 3,
		ADDED_DIR = 4
	};

	string	entry_path;
	string	file_path;
	int		action;

	dir_entry_change_t(int action, string file = "", string entry = "")
	{
		this->action = action;
		this->entry_path = entry;
		this->file_path = file;
	}
};

class DirArchive : public Archive
{
private:
	string				separator;
	vector<key_value_t>	renamed_dirs;
	mod_times_t			file_modification_times;
	vector<string>		removed_files;

public:
	DirArchive();
	~DirArchive();

	// Accessors
	vector<string>	getRemovedFiles() { return removed_files; }
	time_t			fileModificationTime(ArchiveEntry* entry) { return file_modification_times[entry]; }

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening
	bool	open(string filename);		// Open from File
	bool	open(ArchiveEntry* entry);	// Open from ArchiveEntry
	bool	open(MemChunk& mc);			// Open from MemChunk

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk
	bool	write(string filename, bool update = true);	// Write to File
	bool	save(string filename = "");					// Save archive

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Dir stuff
	bool	removeDir(string path, ArchiveTreeNode* base = NULL);
	bool	renameDir(ArchiveTreeNode* dir, string new_name);

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);
	bool			removeEntry(ArchiveEntry* entry, bool delete_entry = true);

	// Detection
	mapdesc_t			getMapInfo(ArchiveEntry* maphead);
	vector<mapdesc_t>	detectMaps();

	// Search
	ArchiveEntry*			findFirst(search_options_t& options);
	ArchiveEntry*			findLast(search_options_t& options);
	vector<ArchiveEntry*>	findAll(search_options_t& options);

	// DirArchive-specific
	void	updateChangedEntries(vector<dir_entry_change_t>& changes);
};

class DirArchiveTraverser : public wxDirTraverser
{
private:
	vector<string>&	paths;
	vector<string>&	dirs;

public:
	DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist) : paths(pathlist), dirs(dirlist) {}
	~DirArchiveTraverser() {}

	virtual wxDirTraverseResult OnFile(const wxString& filename)
	{
		paths.push_back(filename);
		return wxDIR_CONTINUE;
	}

	virtual wxDirTraverseResult OnDir(const wxString& dirname)
	{
		dirs.push_back(dirname);
		return wxDIR_CONTINUE;
	}
};

#endif//__DIR_ARCHIVE_H__
