
#ifndef __GZIPARCHIVE_H__
#define __GZIPARCHIVE_H__

#include "Archive/Archive.h"

class GZipArchive : public TreelessArchive
{
private:
	string comment;
	MemChunk xtra;
	uint8_t flags;
	uint32_t mtime;
	uint8_t xfl;
	uint8_t os;

public:
	GZipArchive();
	~GZipArchive();

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
	bool	renameEntry(ArchiveEntry* entry, string name);

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
	static bool isGZipArchive(MemChunk& mc);
	static bool isGZipArchive(string filename);

};


#endif //__GZIPARCHIVE_H__
