#pragma once

// Forward declarations
class wxWindow;

namespace slade
{
class ArchivePanel;
class EntryPanel;
class MainWindow;

namespace maineditor
{
	enum class NewEntryType
	{
		Empty,
		Text,
		Palette,
		Animated,
		Switches
	};

	bool init();

	MainWindow*           window();
	wxWindow*             windowWx();
	Archive*              currentArchive();
	ArchiveEntry*         currentEntry();
	vector<ArchiveEntry*> currentEntrySelection();
	Palette*              currentPalette(ArchiveEntry* entry = nullptr);
	ArchivePanel*         currentArchivePanel();
	EntryPanel*           currentEntryPanel();

	void openTextureEditor(const Archive* archive, ArchiveEntry* entry = nullptr);
	void openMapEditor(Archive* archive);
	void openArchiveTab(const Archive* archive);
	void openEntry(ArchiveEntry* entry);
	bool saveArchiveAs(Archive* archive);

	void setGlobalPaletteFromArchive(Archive* archive);
} // namespace maineditor
} // namespace slade

// Define for less cumbersome maineditor::window()
#define theMainWindow maineditor::window()
