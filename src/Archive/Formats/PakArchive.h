
#ifndef __PAK_ARCHIVE_H__
#define __PAK_ARCHIVE_H__

#include "Archive/Archive.h"

class PakArchive : public Archive
{
private:

public:
	PakArchive();
	~PakArchive();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Detection
	virtual vector<MapDesc>	detectMaps() { return vector<MapDesc>(); }

	// Static functions
	static bool isPakArchive(MemChunk& mc);
	static bool isPakArchive(string filename);
};

#endif//__PAK_ARCHIVE_H__
