
#ifndef __GOBARCHIVE_H__
#define __GOBARCHIVE_H__

#include "Archive.h"

class GobArchive : public TreelessArchive
{
public:
	GobArchive();
	~GobArchive();

	// GOB specific
	uint32_t	getEntryOffset(ArchiveEntry* entry);
	void		setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool		loadEntryData(ArchiveEntry* entry);

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false);
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name);

	// Detection
	vector<mapdesc_t>	detectMaps() { vector<mapdesc_t> ret; return ret; }

	// Static functions
	static bool isGobArchive(MemChunk& mc);
	static bool isGobArchive(string filename);
};

#endif//__GOBARCHIVE_H__
