
#ifndef __GRPARCHIVE_H__
#define __GRPARCHIVE_H__

#include "Archive/Archive.h"

class GrpArchive : public TreelessArchive
{
public:
	GrpArchive();
	~GrpArchive();

	// GRP specific
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
	vector<MapDesc>	detectMaps() { vector<MapDesc> ret; return ret; }

	// Static functions
	static bool isGrpArchive(MemChunk& mc);
	static bool isGrpArchive(string filename);
};

#endif//__GRPARCHIVE_H__
