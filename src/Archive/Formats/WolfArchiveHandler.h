#pragma once

#include "Archive/ArchiveFormatHandler.h"

namespace slade
{
class WolfArchiveHandler : public ArchiveFormatHandler
{
public:
	WolfArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Wolf, true) {}
	~WolfArchiveHandler() override = default;

	// Opening
	bool open(Archive& archive, string_view filename) override; // Open from File
	bool open(Archive& archive, const MemChunk& mc) override;   // Open from MemChunk

	bool openAudio(Archive& archive, MemChunk& head, const MemChunk& data);
	bool openGraph(Archive& archive, const MemChunk& head, const MemChunk& data, MemChunk& dict);
	bool openMaps(Archive& archive, MemChunk& head, const MemChunk& data);

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override; // Write to MemChunk

	// Entry addition/removal
	shared_ptr<ArchiveEntry> addEntry(
		Archive&                 archive,
		shared_ptr<ArchiveEntry> entry,
		unsigned                 position = 0xFFFFFFFF,
		ArchiveDir*              dir      = nullptr) override;
	shared_ptr<ArchiveEntry> addEntry(Archive& archive, shared_ptr<ArchiveEntry> entry, string_view add_namespace)
		override;

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override
	{
		return false;
	}

	bool isThisFormat(const MemChunk& mc) override;
	bool isThisFormat(const string& filename) override;

private:
	struct WolfHandle
	{
		uint32_t offset;
		uint16_t size;
	};

	uint16_t spritestart_ = 0;
	uint16_t soundstart_  = 0;
};
} // namespace slade
