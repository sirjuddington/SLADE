
#ifndef __PAK_ARCHIVE_H__
#define __PAK_ARCHIVE_H__

#include "Archive/Archive.h"

class PakArchive : public Archive
{
public:
	PakArchive();
	~PakArchive();

	// Opening/writing
	bool	open(MemChunk& mc) override;						// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true) override;	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isPakArchive(MemChunk& mc);
	static bool isPakArchive(string filename);
};

#endif//__PAK_ARCHIVE_H__
