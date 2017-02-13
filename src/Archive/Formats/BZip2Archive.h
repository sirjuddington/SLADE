
#ifndef __BZIP2ARCHIVE_H__
#define __BZIP2ARCHIVE_H__

#include "Archive/Archive.h"

class BZip2Archive : public TreelessArchive
{

public:
	BZip2Archive();
	~BZip2Archive();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening
	bool	open(MemChunk& mc);			// Open from MemChunk

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool		loadEntryData(ArchiveEntry* entry);

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false) { return NULL; }
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false) { return NULL; }
	bool			removeEntry(ArchiveEntry* entry) { return false ; }

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name) { return false ; }

	// Entry moving
	bool	swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) { return false ; }
	bool	moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL) { return false ; }

	// Detection
	vector<mapdesc_t>	detectMaps() { vector<mapdesc_t> ret; return ret; }

	// Search
	ArchiveEntry*			findFirst(search_options_t& options);
	ArchiveEntry*			findLast(search_options_t& options);
	vector<ArchiveEntry*>	findAll(search_options_t& options);

	// Static functions
	static bool isBZip2Archive(MemChunk& mc);
	static bool isBZip2Archive(string filename);

};


#endif //__BZIP2ARCHIVE_H__
