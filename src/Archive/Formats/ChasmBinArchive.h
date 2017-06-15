
#ifndef __CHASM_BIN_ARCHIVE_H__
#define __CHASM_BIN_ARCHIVE_H__

#include "Archive/Archive.h"

class ChasmBinArchive : public Archive
{
public:
	ChasmBinArchive();

	// Opening/writing
	bool open(MemChunk& mc) override;
	bool write(MemChunk& mc, bool update = true) override;

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isChasmBinArchive(MemChunk& mc);
	static bool isChasmBinArchive(string filename);
};

#endif // __CHASM_BIN_ARCHIVE_H__
