
#ifndef __WOLFARCHIVE_H__
#define	__WOLFARCHIVE_H__

#include "Archive/Archive.h"

struct WolfHandle
{
	uint32_t offset;
	uint16_t size;
};

class WolfArchive : public TreelessArchive
{
private:
	uint16_t	spritestart;
	uint16_t	soundstart;

public:
	WolfArchive();
	~WolfArchive();

	// Wolf3D specific
	uint32_t	getEntryOffset(ArchiveEntry* entry);
	void		setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Archive type info
	string			getFileExtensionString();
	string			getFormat();

	// Opening
	bool	open(string filename);		// Open from File
	bool	open(MemChunk& mc);			// Open from MemChunk

	bool	openAudio(MemChunk& head, MemChunk& data);
	bool	openGraph(MemChunk& head, MemChunk& data, MemChunk& dict);
	bool	openMaps(MemChunk& head, MemChunk& data);

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	// Misc
	bool		loadEntryData(ArchiveEntry* entry);
	unsigned 	numEntries() { return getRoot()->numEntries(); }

	// Entry addition/removal
	ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false);
	ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false);

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name);

	// Detection
	vector<MapDesc>	detectMaps() { vector<MapDesc> ret; return ret; }

	static bool isWolfArchive(MemChunk& mc);
	static bool isWolfArchive(string filename);
};

#endif	/* __WOLFARCHIVE_H__ */
