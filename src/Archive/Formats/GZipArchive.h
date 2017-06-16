
#ifndef __GZIPARCHIVE_H__
#define __GZIPARCHIVE_H__

#include "Archive/Archive.h"

class GZipArchive : public TreelessArchive
{
public:
	GZipArchive();
	~GZipArchive();

	// Opening
	bool	open(MemChunk& mc) override;

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true) override;

	// Misc
	bool	loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry*	addEntry(
						ArchiveEntry* entry,
						unsigned position = 0xFFFFFFFF,
						ArchiveTreeNode* dir = nullptr,
						bool copy = false
					) override { return nullptr; }
	ArchiveEntry*	addEntry(
						ArchiveEntry* entry,
						string add_namespace,
						bool copy = false
					) override { return nullptr; }
	bool			removeEntry(ArchiveEntry* entry) override { return false ; }

	// Entry modification
	bool	renameEntry(ArchiveEntry* entry, string name) override;

	// Entry moving
	bool	swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2) override { return false ; }
	bool	moveEntry(
				ArchiveEntry* entry,
				unsigned position = 0xFFFFFFFF,
				ArchiveTreeNode* dir = nullptr
			) override { return false ; }

	// Search
	ArchiveEntry*			findFirst(SearchOptions& options) override;
	ArchiveEntry*			findLast(SearchOptions& options) override;
	vector<ArchiveEntry*>	findAll(SearchOptions& options) override;

	// Static functions
	static bool isGZipArchive(MemChunk& mc);
	static bool isGZipArchive(string filename);

private:
	string		comment_;
	MemChunk	xtra_;
	uint8_t		flags_;
	uint32_t	mtime_;
	uint8_t		xfl_;
	uint8_t		os_;
};

#endif //__GZIPARCHIVE_H__
