
#ifndef __TAR_ARCHIVE_H__
#define __TAR_ARCHIVE_H__

#include "Archive.h"

class TarArchive : public Archive
{
private:

public:
	TarArchive();
	~TarArchive();

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

	// Static functions
	static bool isTarArchive(MemChunk& mc);
	static bool isTarArchive(string filename);
};

#endif//__TAR_ARCHIVE_H__
