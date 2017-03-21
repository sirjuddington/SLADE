#pragma once

class Archive;
class ArchiveEntry;
class Palette8bit;
namespace MainEditor
{
	Archive*				getCurrentArchive();
	ArchiveEntry*			getCurrentEntry();
	vector<ArchiveEntry*>	getCurrentEntrySelection();
	Palette8bit*			getSelectedPalette();

	void	openTextureEditor(Archive* archive, ArchiveEntry* entry = NULL);
	void	openMapEditor(Archive* archive);
	void	openEntry(ArchiveEntry* entry);
};
