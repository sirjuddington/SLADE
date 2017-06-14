
#ifndef __ZIPARCHIVE_H__
#define __ZIPARCHIVE_H__

#include "Archive/Archive.h"

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
	MapDesc			getMapInfo(ArchiveEntry* maphead);
	vector<MapDesc>	detectMaps();

	// Search
	ArchiveEntry*			findFirst(SearchOptions& options);
	ArchiveEntry*			findLast(SearchOptions& options);
	vector<ArchiveEntry*>	findAll(SearchOptions& options);

	// Static functions
	static bool	isZipArchive(MemChunk& mc);
	static bool isZipArchive(string filename);

private:
	string	temp_file;

	void	generateTempFileName(string filename);
};

#endif//__ZIPARCHIVE_H__
