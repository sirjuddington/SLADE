
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "MapEditor/MapEditor.h"
#include "UI/ArchiveManagerPanel.h"
#include "UI/ArchivePanel.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/MainWindow.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::maineditor
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
bool maineditor::init()
{
	main_window = new MainWindow();
	return true;
}

// -----------------------------------------------------------------------------
// Returns the main editor window
// -----------------------------------------------------------------------------
MainWindow* maineditor::window()
{
	return main_window;
}

// -----------------------------------------------------------------------------
// Returns the main editor window as a wxWindow.
// For use in UI code that doesn't need to know about the MainWindow class
// -----------------------------------------------------------------------------
wxWindow* maineditor::windowWx()
{
	return main_window;
}

// -----------------------------------------------------------------------------
// Returns the currently open archive (ie the current tab's archive, if any)
// -----------------------------------------------------------------------------
Archive* maineditor::currentArchive()
{
	return main_window->archiveManagerPanel()->currentArchive();
}

// -----------------------------------------------------------------------------
// Returns the currently open entry (current tab -> current entry panel)
// -----------------------------------------------------------------------------
ArchiveEntry* maineditor::currentEntry()
{
	return main_window->archiveManagerPanel()->currentEntry();
}

// -----------------------------------------------------------------------------
// Returns a list of all currently selected entries in the current archive panel
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> maineditor::currentEntrySelection()
{
	return main_window->archiveManagerPanel()->currentEntrySelection();
}

// -----------------------------------------------------------------------------
// Opens the texture editor for the current archive tab
// -----------------------------------------------------------------------------
void maineditor::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openTextureTab(app::archiveManager().archiveIndex(archive), entry);
}

// -----------------------------------------------------------------------------
// Opens the map editor for the current archive tab
// -----------------------------------------------------------------------------
void maineditor::openMapEditor(Archive* archive)
{
	mapeditor::chooseMap(archive);
}

// -----------------------------------------------------------------------------
// Opens the archive file at [filename]
// -----------------------------------------------------------------------------
void maineditor::openArchiveFile(string_view filename)
{
	main_window->archiveManagerPanel()->openFile(wxutil::strFromView(filename));
}

// -----------------------------------------------------------------------------
// Shows the tab for [archive], opening a new tab for it if needed
// -----------------------------------------------------------------------------
void ::maineditor::openArchiveTab(Archive* archive)
{
	main_window->archiveManagerPanel()->openTab(archive);
}

// -----------------------------------------------------------------------------
// Opens [entry] in its own tab
// -----------------------------------------------------------------------------
void maineditor::openEntry(ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openEntryTab(entry);
}

// -----------------------------------------------------------------------------
// Saves [archive] to disk under a different filename, opens a file dialog to
// select the new name/path.
// Returns false on error or if the dialog was cancelled, true otherwise
// -----------------------------------------------------------------------------
bool maineditor::saveArchiveAs(Archive* archive)
{
	return main_window->archiveManagerPanel()->saveArchiveAs(archive);
}

// -----------------------------------------------------------------------------
// Sets the global palette to the main palette in [archive] (eg. PLAYPAL)
// -----------------------------------------------------------------------------
void maineditor::setGlobalPaletteFromArchive(Archive* archive)
{
	main_window->paletteChooser()->setGlobalFromArchive(archive);
}

// -----------------------------------------------------------------------------
// Returns the currently selected palette
// -----------------------------------------------------------------------------
Palette* maineditor::currentPalette(ArchiveEntry* entry)
{
	return main_window->paletteChooser()->selectedPalette(entry);
}

// -----------------------------------------------------------------------------
// Returns the currently visible archive panel, or nullptr if the current tab
// isn't an archive panel
// -----------------------------------------------------------------------------
ArchivePanel* maineditor::currentArchivePanel()
{
	auto* panel = main_window->archiveManagerPanel()->currentPanel();

	if (panel && panel->GetName().CmpNoCase("archive") == 0)
		return dynamic_cast<ArchivePanel*>(panel);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently visible entry panel
// -----------------------------------------------------------------------------
EntryPanel* maineditor::currentEntryPanel()
{
	return main_window->archiveManagerPanel()->currentArea();
}
