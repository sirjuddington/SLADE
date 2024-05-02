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
	bool open(Archive& archive, string_view filename, bool detect_types) override; // Open from File
	bool open(Archive& archive, const MemChunk& mc, bool detect_types) override;   // Open from MemChunk

	bool openAudio(Archive& archive, MemChunk& head, const MemChunk& data, bool detect_types);
	bool openGraph(Archive& archive, const MemChunk& head, const MemChunk& data, MemChunk& dict, bool detect_types);
	bool openMaps(Archive& archive, MemChunk& head, const MemChunk& data, bool detect_types);

	// Writing/Saving
	bool write(Archive& archive, MemChunk& mc) override; // Write to MemChunk

	// Entry modification
	bool renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force = false) override
	{
		return false;
	}

	// Format detection
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
