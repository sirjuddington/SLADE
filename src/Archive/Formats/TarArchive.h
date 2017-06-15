
#ifndef __TAR_ARCHIVE_H__
#define __TAR_ARCHIVE_H__

#include "Archive/Archive.h"

class TarArchive : public Archive
{
public:
	TarArchive();
	~TarArchive();

	// Opening/writing
	bool	open(MemChunk& mc) override;						// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true) override;	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isTarArchive(MemChunk& mc);
	static bool isTarArchive(string filename);
};

#endif//__TAR_ARCHIVE_H__
