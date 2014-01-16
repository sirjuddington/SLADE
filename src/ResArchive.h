
#ifndef __RESARCHIVE_H__
#define __RESARCHIVE_H__

#include "Archive.h"

#define RESDIRENTRYSIZE 39	// The size of a res entry in the res directory

class ResArchive : public Archive
{
private:

public:
	ResArchive();
	~ResArchive();

	// Res specific
	uint32_t	getEntryOffset(ArchiveEntry* entry);
	void		setEntryOffset(ArchiveEntry* entry, uint32_t offset);
	void		updateNamespaces();

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening/writing
	bool	readDirectory(MemChunk& mc, size_t dir_offset, size_t num_lumps, ArchiveTreeNode* parent);
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
	static bool isResArchive(MemChunk& mc);
	static bool isResArchive(MemChunk& mc, size_t& d_o, size_t& n_l);
	static bool isResArchive(string filename);

};

#endif//__RESARCHIVE_H__
