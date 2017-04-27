
#ifndef __DATARCHIVE_H__
#define	__DATARCHIVE_H__

#include "Archive/Archive.h"

class DatArchive : public TreelessArchive
{
private:
	int						sprites[2];
	int						flats[2];
	int						walls[2];

public:
	DatArchive();
	~DatArchive();

	// Dat specific
	uint32_t	getEntryOffset(ArchiveEntry* entry);
	void		setEntryOffset(ArchiveEntry* entry, uint32_t offset);
	void		updateNamespaces();

	// Archive type info
	string			getFileExtensionString();
	string			getFormat();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool		loadEntryData(ArchiveEntry* entry);
	unsigned 	numEntries() { return getRoot()->numEntries(); }

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false);
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);
	bool			removeEntry(ArchiveEntry* entry);

	// Entry moving
	bool	swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	bool	moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL);

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name);

	// Detection
	vector<mapdesc_t>	detectMaps() { vector<mapdesc_t> ret; return ret; }
	string				detectNamespace(size_t index, ArchiveTreeNode * dir = NULL);
	string				detectNamespace(ArchiveEntry* entry);

	static bool isDatArchive(MemChunk& mc);
	static bool isDatArchive(string filename);
};

#endif	/* __DATARCHIVE_H__ */
