
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MainEditor.cpp
// Description: Functions for interfacing with the main editor window
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MainEditor.h"
#include "Archive/ArchiveManager.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "UI/ArchiveManagerPanel.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/MainWindow.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace MainEditor
{
MainWindow* main_window = nullptr;
}


// -----------------------------------------------------------------------------
//
// MainEditor Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates and initialises the main editor window
// -----------------------------------------------------------------------------
bool MainEditor::init()
{
	main_window = new MainWindow();
	return true;
}

// -----------------------------------------------------------------------------
// Returns the main editor window
// -----------------------------------------------------------------------------
MainWindow* MainEditor::window()
{
	return main_window;
}

// -----------------------------------------------------------------------------
// Returns the main editor window as a wxWindow.
// For use in UI code that doesn't need to know about the MainWindow class
// -----------------------------------------------------------------------------
wxWindow* MainEditor::windowWx()
{
	return main_window;
}

// -----------------------------------------------------------------------------
// Returns the currently open archive (ie the current tab's archive, if any)
// -----------------------------------------------------------------------------
Archive* MainEditor::currentArchive()
{
	return main_window->archiveManagerPanel()->currentArchive();
}

// -----------------------------------------------------------------------------
// Returns the currently open entry (current tab -> current entry panel)
// -----------------------------------------------------------------------------
ArchiveEntry* MainEditor::currentEntry()
{
	return main_window->archiveManagerPanel()->currentEntry();
}

// -----------------------------------------------------------------------------
// Returns a list of all currently selected entries in the current archive panel
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> MainEditor::currentEntrySelection()
{
	return main_window->archiveManagerPanel()->currentEntrySelection();
}

// -----------------------------------------------------------------------------
// Opens the texture editor for the current archive tab
// -----------------------------------------------------------------------------
void MainEditor::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openTextureTab(App::archiveManager().archiveIndex(archive), entry);
}

// -----------------------------------------------------------------------------
// Opens the map editor for the current archive tab
// -----------------------------------------------------------------------------
void MainEditor::openMapEditor(Archive* archive)
{
	MapEditor::chooseMap(archive);
}

// -----------------------------------------------------------------------------
// Shows the tab for [archive], opening a new tab for it if needed
// -----------------------------------------------------------------------------
void ::MainEditor::openArchiveTab(Archive* archive)
{
	main_window->archiveManagerPanel()->openTab(archive);
}

// -----------------------------------------------------------------------------
// Opens [entry] in its own tab
// -----------------------------------------------------------------------------
void MainEditor::openEntry(ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openEntryTab(entry);
}

// -----------------------------------------------------------------------------
// Sets the global palette to the main palette in [archive] (eg. PLAYPAL)
// -----------------------------------------------------------------------------
void MainEditor::setGlobalPaletteFromArchive(Archive* archive)
{
	main_window->paletteChooser()->setGlobalFromArchive(archive);
}

// -----------------------------------------------------------------------------
// Returns the currently selected palette
// -----------------------------------------------------------------------------
Palette* MainEditor::currentPalette(ArchiveEntry* entry)
{
	return main_window->paletteChooser()->selectedPalette(entry);
}

// -----------------------------------------------------------------------------
// Returns the currently visible entry panel
// -----------------------------------------------------------------------------
EntryPanel* MainEditor::currentEntryPanel()
{
	return main_window->archiveManagerPanel()->currentArea();
}

// -----------------------------------------------------------------------------
// Opens the documentation tab to [page_name] (if enabled)
// -----------------------------------------------------------------------------
#ifdef USE_WEBVIEW_STARTPAGE
void MainEditor::openDocs(string_view page_name)
{
	main_window->openDocs(WxUtils::strFromView(page_name));
}
#endif
