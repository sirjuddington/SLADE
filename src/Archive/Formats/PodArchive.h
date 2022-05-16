#pragma once

#include "Archive/Archive.h"

namespace slade
{
class PodArchive : public Archive
{
public:
	PodArchive();
	~PodArchive() override = default;

	string_view getId() const { return id_; }
	void        setId(string_view id);

	// Opening
	bool open(MemChunk& mc) override;

	// Writing/Saving
	bool write(MemChunk& mc) override;

	// Misc
	bool loadEntryData(const ArchiveEntry* entry, MemChunk& out) override;

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
} // namespace slade
