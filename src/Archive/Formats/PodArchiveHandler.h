#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class PodArchiveHandler : public ArchiveFormatHandler
{
public:
	PodArchiveHandler();
	~PodArchiveHandler() override = default;

	string_view getId() const { return id_; }
	void        setId(string_view id);

	// Opening
	bool open(Archive& archive, const MemChunk& mc) override;

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override;

	// Format detection
	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

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
