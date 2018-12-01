#pragma once

#include "Archive/ArchiveEntry.h"
#include "Graphics/SImage/SIFormat.h"

class wxFrame;
class ModifyOffsetsDialog;
namespace EntryOperations
{
bool openMapDB2(ArchiveEntry* entry);
bool gfxConvert(ArchiveEntry* entry, string target_format, SIFormat::ConvertOptions opt, int target_colformat = -1);
bool modifyGfxOffsets(ArchiveEntry* entry, ModifyOffsetsDialog* dialog);
bool setGfxOffsets(ArchiveEntry* entry, int x, int y);
bool modifyalPhChunk(ArchiveEntry* entry, bool value);
bool modifytRNSChunk(ArchiveEntry* entry, bool value);
bool getalPhChunk(ArchiveEntry* entry);
bool gettRNSChunk(ArchiveEntry* entry);
bool readgrAbChunk(ArchiveEntry* entry, Vec2i& offsets);
bool addToPatchTable(vector<ArchiveEntry*> entries);
bool createTexture(vector<ArchiveEntry*> entries);
bool convertTextures(vector<ArchiveEntry*> entries);
bool findTextureErrors(vector<ArchiveEntry*> entries);
bool compileACS(ArchiveEntry* entry, bool hexen = false, ArchiveEntry* target = nullptr, wxFrame* parent = nullptr);
bool exportAsPNG(ArchiveEntry* entry, string filename);
bool optimizePNG(ArchiveEntry* entry);

// ANIMATED/SWITCHES
bool convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
bool convertSwitches(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
bool convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata, bool switches);
} // namespace EntryOperations
