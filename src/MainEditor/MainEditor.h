#pragma once

// Forward declarations
class Archive;
class ArchiveEntry;
class EntryPanel;
class MainWindow;
class Palette8bit;
class wxWindow;

namespace MainEditor
{
	bool	init();

	MainWindow*				window();
	wxWindow*				windowWx();
	Archive*				currentArchive();
	ArchiveEntry*			currentEntry();
	vector<ArchiveEntry*>	currentEntrySelection();
	Palette8bit*			currentPalette(ArchiveEntry* entry = nullptr);
	EntryPanel*				currentEntryPanel();

	void	openTextureEditor(Archive* archive, ArchiveEntry* entry = nullptr);
	void	openMapEditor(Archive* archive);
	void	openEntry(ArchiveEntry* entry);

	void	setGlobalPaletteFromArchive(Archive* archive);

#ifdef USE_WEBVIEW_STARTPAGE
	void	openDocs(string page_name = "");
#endif
};

// Define for less cumbersome MainEditor::window()
#define theMainWindow MainEditor::window()
