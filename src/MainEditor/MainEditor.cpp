
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor.h"
#include "MainWindow.h"
#include "MapEditor/MapEditorWindow.h"

/* MainWindow::getCurrentArchive
 * Returns the currently open archive (ie the current tab's archive,
 * if any)
 *******************************************************************/
Archive* MainEditor::getCurrentArchive()
{
	return theMainWindow->getArchiveManagerPanel()->currentArchive();
}

/* MainWindow::getCurrentEntry
 * Returns the currently open entry (current tab -> current entry
 * panel)
 *******************************************************************/
ArchiveEntry* MainEditor::getCurrentEntry()
{
	return theMainWindow->getArchiveManagerPanel()->currentEntry();
}

/* MainWindow::getCurrentEntrySelection
 * Returns a list of all currently selected entries, in the current
 * archive panel
 *******************************************************************/
vector<ArchiveEntry*> MainEditor::getCurrentEntrySelection()
{
	return theMainWindow->getArchiveManagerPanel()->currentEntrySelection();
}

/* MainWindow::openTextureEditor
 * Opens the texture editor for the current archive tab
 *******************************************************************/
void MainEditor::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	theMainWindow->getArchiveManagerPanel()->openTextureTab(theArchiveManager->archiveIndex(archive), entry);
}

/* MainWindow::openMapEditor
 * Opens the map editor for the current archive tab
 *******************************************************************/
void MainEditor::openMapEditor(Archive* archive)
{
	theMapEditor->chooseMap(archive);
}

/* MainWindow::openEntry
 * Opens [entry] in its own tab
 *******************************************************************/
void MainEditor::openEntry(ArchiveEntry* entry)
{
	theMainWindow->getArchiveManagerPanel()->openEntryTab(entry);
}

Palette8bit* MainEditor::getSelectedPalette()
{
	return theMainWindow->getPaletteChooser()->getSelectedPalette();
}
