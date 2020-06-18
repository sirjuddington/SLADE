#pragma once

#include "Archive/ArchiveEntry.h"
#include "Graphics/SImage/SIFormat.h"

class wxFrame;

namespace slade
{
class ModifyOffsetsDialog;

namespace entryoperations
{
	bool openMapDB2(ArchiveEntry* entry);
	//bool modifyGfxOffsets(ArchiveEntry* entry, ModifyOffsetsDialog* dialog);
	bool addToPatchTable(const vector<ArchiveEntry*>& entries);
	bool createTexture(const vector<ArchiveEntry*>& entries);
	bool convertTextures(const vector<ArchiveEntry*>& entries);
	bool findTextureErrors(const vector<ArchiveEntry*>& entries);
	bool compileACS(ArchiveEntry* entry, bool hexen = false, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);
	bool exportAsPNG(ArchiveEntry* entry, const wxString& filename);
	bool optimizePNG(ArchiveEntry* entry);

	// ANIMATED/SWITCHES
	bool convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwitches(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata, bool switches);
} // namespace entryoperations
} // namespace slade
