#pragma once

#include "Archive/ArchiveEntry.h"

class wxFrame;

namespace slade
{
class ModifyOffsetsDialog;

namespace entryoperations
{
	bool rename(const vector<ArchiveEntry*>& entries, Archive* archive, bool each);
	bool renameDir(const vector<ArchiveDir*>& dirs, Archive* archive);
	bool exportEntry(ArchiveEntry* entry);
	bool exportEntries(const vector<ArchiveEntry*>& entries, const vector<ArchiveDir*>& dirs);

	bool openMapDB2(ArchiveEntry* entry);
	bool addToPatchTable(const vector<ArchiveEntry*>& entries);
	bool createTexture(const vector<ArchiveEntry*>& entries);
	bool convertTextures(const vector<ArchiveEntry*>& entries);
	bool findTextureErrors(const vector<ArchiveEntry*>& entries);
	bool cleanTextureIwadDupes(const vector<ArchiveEntry*>& entries);
	bool cleanZdTextureSinglePatch(const vector<ArchiveEntry*>& entries);
	bool compileACS(ArchiveEntry* entry, bool hexen = false, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);
	bool compileDECOHack(ArchiveEntry* entry, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);
	bool exportAsPNG(ArchiveEntry* entry, const wxString& filename);
	bool optimizePNG(ArchiveEntry* entry);

	// ANIMATED/SWITCHES
	bool convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwitches(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata, bool switches);
} // namespace entryoperations
} // namespace slade
