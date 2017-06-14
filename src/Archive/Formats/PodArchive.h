
#ifndef __POD_ARCHIVE_H__
#define __POD_ARCHIVE_H__

#include "Archive/Archive.h"

class PodArchive : public Archive
{
private:
	char	id[80];

	struct file_entry_t
	{
		char		name[32];
		uint32_t	size;
		uint32_t	offset;
	};

public:
	PodArchive();
	~PodArchive();

	string getId() { return wxString(id); }
	void setId(string id);

	// Archive type info
	string	getFileExtensionString();
	string	getFormat();

	// Opening
	bool	open(MemChunk& mc);

	// Writing/Saving
	bool	write(MemChunk& mc, bool update = true);

	// Misc
	bool	loadEntryData(ArchiveEntry* entry);

	// Detection
	vector<MapDesc>	detectMaps();

	// Static functions
	static bool isPodArchive(MemChunk& mc);
	static bool isPodArchive(string filename);
};

#endif//__POD_ARCHIVE_H__
