
#ifndef __DISKARCHIVE_H__
#define __DISKARCHIVE_H__

#include "Archive.h"

class DiskArchive : public Archive
{
private:

public:
	DiskArchive();
	~DiskArchive();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Detection
	virtual vector<mapdesc_t>	detectMaps() { return vector<mapdesc_t>(); }
	string						detectNamespace(ArchiveEntry* entry);

	// Static functions
	static bool isDiskArchive(MemChunk& mc);
	static bool isDiskArchive(string filename);
};

#endif//__DISKARCHIVE_H__
