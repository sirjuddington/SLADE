
#ifndef __DIR_ARCHIVE_H__
#define __DIR_ARCHIVE_H__

#include "Archive.h"

class DirArchive : public Archive
{
private:
	string				separator;
	vector<key_value_t>	renamed_dirs;

public:
	DirArchive();
	~DirArchive();

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
	bool	renameDir(ArchiveTreeNode* dir, string new_name);

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);
	//bool			removeEntry(ArchiveEntry* entry, bool delete_entry = true);

	// Detection
	mapdesc_t			getMapInfo(ArchiveEntry* maphead);
	vector<mapdesc_t>	detectMaps();
	string				detectNamespace(ArchiveEntry* entry);

	// Search
	ArchiveEntry*			findFirst(search_options_t& options);
	ArchiveEntry*			findLast(search_options_t& options);
	vector<ArchiveEntry*>	findAll(search_options_t& options);
};

#endif//__DIR_ARCHIVE_H__
