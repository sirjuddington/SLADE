
#ifndef __LIBARCHIVE_H__
#define	__LIBARCHIVE_H__

#include "Archive/Archive.h"

class LibArchive : public TreelessArchive
{
public:
	LibArchive();
	~LibArchive();

	// Lib specific
	uint32_t	getEntryOffset(ArchiveEntry* entry);
	void		setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Archive type info
	string			getFileExtensionString();
	string			getFormat();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool		loadEntryData(ArchiveEntry* entry);
	unsigned 	numEntries() { return getRoot()->numEntries(); }

	// Entry addition/modification
	ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false);
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);
	bool			renameEntry(ArchiveEntry* entry, string name);

	// Detection
	vector<mapdesc_t>	detectMaps() { vector<mapdesc_t> ret; return ret; }

	static bool isLibArchive(MemChunk& mc);
	static bool isLibArchive(string filename);
};

#endif	/* __LIBARCHIVE_H__ */
