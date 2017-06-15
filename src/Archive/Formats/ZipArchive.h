
#ifndef __ZIPARCHIVE_H__
#define __ZIPARCHIVE_H__

#include "Archive/Archive.h"

class ZipArchive : public Archive
{
public:
	ZipArchive();
	~ZipArchive();

	// Opening
	bool	open(string filename) override;	// Open from File
	bool	open(MemChunk& mc) override;	// Open from MemChunk

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true) override;		// Write to MemChunk
	bool	write(string filename, bool update = true) override;	// Write to File

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false) override;

	// Detection
	MapDesc			getMapInfo(ArchiveEntry* maphead) override;
	vector<MapDesc>	detectMaps() override;

	// Search
	ArchiveEntry*			findFirst(SearchOptions& options) override;
	ArchiveEntry*			findLast(SearchOptions& options) override;
	vector<ArchiveEntry*>	findAll(SearchOptions& options) override;

	// Static functions
	static bool	isZipArchive(MemChunk& mc);
	static bool isZipArchive(string filename);

private:
	string	temp_file_;

	void	generateTempFileName(string filename);
};

#endif//__ZIPARCHIVE_H__
