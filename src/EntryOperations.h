
#ifndef __ENTRYOPERATIONS_H__
#define	__ENTRYOPERATIONS_H__

#include "ArchiveEntry.h"
#include "SIFormat.h"

class wxFrame;
class ModifyOffsetsDialog;
namespace EntryOperations
{
	bool	openExternal(ArchiveEntry* entry);
	bool	openMapDB2(ArchiveEntry* entry);
	bool	gfxConvert(ArchiveEntry* entry, string target_format, SIFormat::convert_options_t opt, int target_colformat = -1);
	bool	modifyGfxOffsets(ArchiveEntry* entry, ModifyOffsetsDialog* dialog);
	bool	setGfxOffsets(ArchiveEntry* entry, int x, int y);
	bool	modifyalPhChunk(ArchiveEntry* entry, bool value);
	bool	modifytRNSChunk(ArchiveEntry* entry, bool value);
	bool	getalPhChunk(ArchiveEntry* entry);
	bool	gettRNSChunk(ArchiveEntry* entry);
	bool	readgrAbChunk(ArchiveEntry* entry, point2_t& offsets);
	bool	addToPatchTable(vector<ArchiveEntry*> entries);
	bool	createTexture(vector<ArchiveEntry*> entries);
	bool	convertTextures(vector<ArchiveEntry*> entries);
	bool	findTextureErrors(vector<ArchiveEntry*> entries);
	bool	compileACS(ArchiveEntry* entry, bool hexen = false, ArchiveEntry* target = NULL, wxFrame* parent = NULL);
	bool	exportAsPNG(ArchiveEntry* entry, string filename);
	bool	optimizePNG(ArchiveEntry* entry);
};

#endif//__ENTRYOPERATIONS_H__
