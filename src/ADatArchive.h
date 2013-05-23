
#ifndef __ADAT_ARCHIVE_H__
#define __ADAT_ARCHIVE_H__

#include "Archive.h"

class ADatArchive : public Archive
{
private:

public:
	ADatArchive();
	~ADatArchive();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening
	bool	open(MemChunk& mc);			// Open from MemChunk

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk
	bool	write(string filename, bool update = true);	// Write to File

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Detection
	virtual vector<mapdesc_t>	detectMaps() { return vector<mapdesc_t>(); }
	string						detectNamespace(ArchiveEntry* entry);

	// Static functions
	static bool isADatArchive(MemChunk& mc);
	static bool isADatArchive(string filename);
};

#endif//__ADAT_ARCHIVE_H__
