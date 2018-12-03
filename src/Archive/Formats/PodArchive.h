#pragma once

#include "Archive/Archive.h"

class PodArchive : public Archive
{
public:
	PodArchive();
	~PodArchive() = default;

	string getId() const { return wxString(id_); }
	void   setId(const string& id);

	// Opening
	bool open(MemChunk& mc) override;

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isPodArchive(MemChunk& mc);
	static bool isPodArchive(const string& filename);

private:
	char id_[80];

	struct FileEntry
	{
		char     name[32];
		uint32_t size;
		uint32_t offset;
	};
};
