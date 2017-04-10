
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor.h"
#include "UI/MainWindow.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "UI/ArchiveManagerPanel.h"
#include "UI/PaletteChooser.h"
#include "MapEditor/MapEditor.h"

namespace MainEditor
{
	MainWindow*	main_window = nullptr;
}

bool MainEditor::init()
{
	main_window = new MainWindow();
	return true;
}

MainWindow* MainEditor::window()
{
	return main_window;
}

wxWindow* MainEditor::windowWx()
{
	return main_window;
}

/* MainWindow::getCurrentArchive
 * Returns the currently open archive (ie the current tab's archive,
 * if any)
 *******************************************************************/
Archive* MainEditor::currentArchive()
{
	return main_window->getArchiveManagerPanel()->currentArchive();
}

/* MainWindow::getCurrentEntry
 * Returns the currently open entry (current tab -> current entry
 * panel)
 *******************************************************************/
ArchiveEntry* MainEditor::currentEntry()
{
	return main_window->getArchiveManagerPanel()->currentEntry();
}

/* MainWindow::getCurrentEntrySelection
 * Returns a list of all currently selected entries, in the current
 * archive panel
 *******************************************************************/
vector<ArchiveEntry*> MainEditor::currentEntrySelection()
{
	return main_window->getArchiveManagerPanel()->currentEntrySelection();
}

/* MainWindow::openTextureEditor
 * Opens the texture editor for the current archive tab
 *******************************************************************/
void MainEditor::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	main_window->getArchiveManagerPanel()->openTextureTab(theArchiveManager->archiveIndex(archive), entry);
}

/* MainWindow::openMapEditor
 * Opens the map editor for the current archive tab
 *******************************************************************/
void MainEditor::openMapEditor(Archive* archive)
{
	MapEditor::chooseMap(archive);
}

/* MainWindow::openEntry
 * Opens [entry] in its own tab
 *******************************************************************/
void MainEditor::openEntry(ArchiveEntry* entry)
{
	main_window->getArchiveManagerPanel()->openEntryTab(entry);
}

void MainEditor::setGlobalPaletteFromArchive(Archive * archive)
{
	main_window->getPaletteChooser()->setGlobalFromArchive(archive);
}

Palette8bit* MainEditor::currentPalette(ArchiveEntry* entry)
{
	return main_window->getPaletteChooser()->getSelectedPalette(entry);
}

EntryPanel* MainEditor::currentEntryPanel()
{
	return main_window->getArchiveManagerPanel()->currentArea();
}

void MainEditor::openDocs(string page_name)
{
	main_window->openDocs(page_name);
}
