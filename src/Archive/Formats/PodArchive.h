#pragma once

#include "Archive/Archive.h"

class PodArchive : public Archive
{
public:
	PodArchive();
	~PodArchive() = default;

	std::string_view getId() const { return id_; }
	void             setId(std::string_view id);

	// Opening
	bool open(MemChunk& mc) override;

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isPodArchive(MemChunk& mc);
	static bool isPodArchive(const std::string& filename);

private:
	char id_[80];

	struct FileEntry
	{
		char     name[32];
		uint32_t size;
		uint32_t offset;
	};
};
