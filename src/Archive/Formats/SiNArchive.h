#ifndef __SIN_ARCHIVE_H__
#define __SIN_ARCHIVE_H__

#include "Archive/Archive.h"

class SiNArchive : public Archive
{
private:

public:
	SiNArchive();
	~SiNArchive();

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
	static bool isSiNArchive(MemChunk& mc);
	static bool isSiNArchive(string filename);
};

#endif//__SIN_ARCHIVE_H__