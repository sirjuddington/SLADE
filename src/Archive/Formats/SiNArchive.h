#ifndef __SIN_ARCHIVE_H__
#define __SIN_ARCHIVE_H__

#include "Archive/Archive.h"

class SiNArchive : public Archive
{
public:
	SiNArchive();
	~SiNArchive();

	// Opening/writing
	bool	open(MemChunk& mc) override;						// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true) override;	// Write to MemChunk

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isSiNArchive(MemChunk& mc);
	static bool isSiNArchive(string filename);
};

#endif//__SIN_ARCHIVE_H__
