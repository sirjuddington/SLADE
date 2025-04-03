#pragma once

class wxFrame;

namespace slade
{
class UndoManager;
class Translation;
class ModifyOffsetsDialog;

namespace entryoperations
{
	// General
	bool rename(const vector<ArchiveEntry*>& entries, Archive* archive, bool each);
	bool renameDir(const vector<ArchiveDir*>& dirs, Archive* archive);
	bool exportEntry(ArchiveEntry* entry);
	bool exportEntries(const vector<ArchiveEntry*>& entries, const vector<ArchiveDir*>& dirs);
	bool sortEntries(
		Archive&                     archive,
		const vector<ArchiveEntry*>& entries,
		ArchiveDir&                  dir,
		UndoManager*                 undo_manager);
	bool openMapDB2(ArchiveEntry* entry);
	bool compileACS(ArchiveEntry* entry, bool hexen = false, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);
	bool compileDECOHack(ArchiveEntry* entry, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);

	// TEXTUREx
	bool addToPatchTable(const vector<ArchiveEntry*>& entries);
	bool createTexture(const vector<ArchiveEntry*>& entries);
	bool convertTextures(const vector<ArchiveEntry*>& entries);
	bool findTextureErrors(const vector<ArchiveEntry*>& entries);
	bool cleanTextureIwadDupes(const vector<ArchiveEntry*>& entries);
	bool cleanZdTextureSinglePatch(const vector<ArchiveEntry*>& entries);

	// Graphics
	bool exportAsPNG(ArchiveEntry* entry, const wxString& filename);
	bool optimizePNG(ArchiveEntry* entry);
	bool exportEntriesAsPNG(const vector<ArchiveEntry*>& entries);
	bool optimizePNGEntries(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool convertGfxEntries(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool remapGfxEntries(
		const vector<ArchiveEntry*>& entries,
		Translation&                 translation,
		UndoManager*                 undo_manager = nullptr);
	bool colourizeGfxEntries(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool tintGfxEntries(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool modifyOffsets(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool convertVoxelEntries(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool copyGfxOffsets(ArchiveEntry& entry);
	bool pasteGfxOffsets(const vector<ArchiveEntry*>& entries);

	// ANIMATED/SWITCHES
	bool convertAnimated(const ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwitches(const ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	bool convertSwanTbls(const ArchiveEntry* entry, MemChunk* animdata, bool switches);

	// Audio
	bool convertWavToDSound(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
	bool convertSoundToWav(const vector<ArchiveEntry*>& entries, UndoManager* undo_manager = nullptr);
} // namespace entryoperations
} // namespace slade
