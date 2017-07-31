
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ScriptManager.cpp
// Description: ScriptManager namespace, provides the backend for the script
//              manager window. 
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ScriptManager.h"
#include "UI/ScriptManagerWindow.h"
#include "Archive/ArchiveManager.h"
#include "Lua.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace ScriptManager
{
	ScriptManagerWindow*	window = nullptr;
	vector<Script>			scripts_editor;
	vector<Script>			scripts_archive;
	vector<Script>			scripts_acs;
	vector<Script>			scripts_decorate;
	vector<Script>			scripts_zscript;
}


// ----------------------------------------------------------------------------
//
// ScriptManager Namespace Functions
//
// ----------------------------------------------------------------------------

namespace ScriptManager
{

// ----------------------------------------------------------------------------
// addScriptFromEntry
//
// Adds a new script to [list], created from [entry]. [cut_path] will be
// removed from the start of the script's path property
// ----------------------------------------------------------------------------
void addScriptFromEntry(ArchiveEntry::SPtr& entry, vector<ScriptManager::Script>& list, const string& cut_path)
{
	ScriptManager::Script s;
	s.name = entry->getName(true);
	s.path = entry->getPath();
	s.path.Replace(cut_path, "");
	s.source = entry;

	list.push_back(s);
	list.back().text = wxString::FromAscii((const char*)entry->getData(), entry->getSize());
}

// ----------------------------------------------------------------------------
// addScriptFromFile
//
// Adds a new script to [list], created from the file at [filename]
// ----------------------------------------------------------------------------
void addScriptFromFile(const string& filename, vector<ScriptManager::Script>& list)
{
	wxFileName fn(filename);

	ScriptManager::Script s;
	s.name = fn.GetName();
	s.path = fn.GetPath(true, wxPATH_UNIX);

	list.push_back(s);

	wxFile file(filename);
	file.ReadAll(&list.back().text);
	file.Close();
}

// ----------------------------------------------------------------------------
// loadGeneralScripts
//
// Loads all 'general' scripts from slade.pk3
// ----------------------------------------------------------------------------
void loadGeneralScripts()
{
	// Get 'scripts/general' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts/general");
	if (scripts_dir)
		for (auto& entry : scripts_dir->allEntries())
			addScriptFromEntry(entry, scripts_editor, "/scripts/general/");
}

// ----------------------------------------------------------------------------
// loadArchiveScripts
//
// Loads all Archive scripts from slade.pk3 and the user dir
// ----------------------------------------------------------------------------
void loadArchiveScripts()
{
	// Get 'scripts/archive' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts/archive");
	if (scripts_dir)
		for (auto& entry : scripts_dir->allEntries())
			if (!entry->getName().StartsWith("_"))
				addScriptFromEntry(entry, scripts_archive, "/scripts/archive/");

	// Load user archive scripts

	// If the directory doesn't exist create it
	auto user_scripts_dir = App::path("scripts/archive", App::Dir::User);
	if (!wxDirExists(user_scripts_dir))
		wxMkdir(user_scripts_dir);

	// Open the custom custom_scripts directory
	wxDir res_dir;
	res_dir.Open(user_scripts_dir);

	// Go through each file in the directory
	string filename = wxEmptyString;
	bool files = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		addScriptFromFile(user_scripts_dir + "/" + filename, scripts_archive);

		// Next file
		files = res_dir.GetNext(&filename);
	}
}

} // namespace ScriptManager

// ----------------------------------------------------------------------------
// ScriptManager::init
//
// Initialises the script manager
// ----------------------------------------------------------------------------
void ScriptManager::init()
{
	loadGeneralScripts();
	loadArchiveScripts();
}

// ----------------------------------------------------------------------------
// ScriptManager::open
//
// Opens the script manager window
// ----------------------------------------------------------------------------
void ScriptManager::open()
{
	if (!window)
		window = new ScriptManagerWindow();

	window->Show();
}

// ----------------------------------------------------------------------------
// ScriptManager::editorScripts
//
// Returns a list of all general editor scripts
// ----------------------------------------------------------------------------
vector<ScriptManager::Script>& ScriptManager::editorScripts()
{
	return scripts_editor;
}

// ----------------------------------------------------------------------------
// ScriptManager::archiveScripts
//
// Returns a list of all archive scripts
// ----------------------------------------------------------------------------
vector<ScriptManager::Script>& ScriptManager::archiveScripts()
{
	return scripts_archive;
}

// ----------------------------------------------------------------------------
// ScriptManager::populateArchiveScriptMenu
//
// Populates [menu] with all loaded archive scripts
// ----------------------------------------------------------------------------
void ScriptManager::populateArchiveScriptMenu(wxMenu* menu)
{
	int index = 0;
	for (auto& script : scripts_archive)
		menu->Append(SAction::fromId("arch_script")->getWxId() + index++, script.name);
}

// ----------------------------------------------------------------------------
// ScriptManager::runArchiveScript
//
// Runs the archive script at [index] on [archive]
// ----------------------------------------------------------------------------
void ScriptManager::runArchiveScript(Archive* archive, int index, wxWindow* parent)
{
	if (parent)
		Lua::setCurrentWindow(parent);

	if (!Lua::runArchiveScript(scripts_archive[index].text, archive))
		Lua::showErrorDialog(parent);
}
