
#ifndef __WADJARCHIVE_H__
#define	__WADJARCHIVE_H__

#include "Archive.h"
#include "WadArchive.h"

class WadJArchive : public WadArchive
{
private:
	vector<ArchiveEntry*>	entries;
	char					wad_type[4];
	ArchiveEntry*			patches[2];
	ArchiveEntry*			sprites[2];
	ArchiveEntry*			flats[2];
	ArchiveEntry*			tx[2];

public:
	WadJArchive();
	~WadJArchive();

	// Opening/writing
	bool	open(MemChunk& mc);							// Open from MemChunk
	bool	write(MemChunk& mc, bool update = true);	// Write to MemChunk

	string	detectNamespace(ArchiveEntry* entry);

	static bool isWadJArchive(MemChunk& mc);
	static bool isWadJArchive(string filename);
};

#endif	/* __WADJARCHIVE_H__ */
