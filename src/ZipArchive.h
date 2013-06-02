
#ifndef __ZIPARCHIVE_H__
#define __ZIPARCHIVE_H__

#include "Archive.h"

class ZipArchive : public Archive
{
public:
	ZipArchive();
	~ZipArchive();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening
	bool	open(string filename);		// Open from File
	bool	open(MemChunk& mc);			// Open from MemChunk

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk
	bool	write(string filename, bool update = true);	// Write to File

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);

	// Detection
	mapdesc_t			getMapInfo(ArchiveEntry* maphead);
	vector<mapdesc_t>	detectMaps();
	string				detectNamespace(ArchiveEntry* entry);

	// Search
	ArchiveEntry*			findFirst(search_options_t& options);
	ArchiveEntry*			findLast(search_options_t& options);
	vector<ArchiveEntry*>	findAll(search_options_t& options);

	// Static functions
	static bool	isZipArchive(MemChunk& mc);
	static bool isZipArchive(string filename);
};

#endif//__ZIPARCHIVE_H__
