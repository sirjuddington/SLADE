
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
	vector<Script::UPtr>	scripts_editor;
	vector<Script::UPtr>	scripts_custom;
	vector<Script::UPtr>	scripts_archive;
	vector<Script::UPtr>	scripts_acs;
	vector<Script::UPtr>	scripts_decorate;
	vector<Script::UPtr>	scripts_zscript;

	string	template_custom_script;
	string	template_archive_script;
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
Script* addScriptFromEntry(ArchiveEntry::SPtr& entry, vector<Script::UPtr>& list, const string& cut_path)
{
	auto s = std::make_unique<Script>();
	s->name = entry->getName(true);
	s->path = entry->getPath();
	s->path.Replace(cut_path, "");
	s->source = entry;

	list.push_back(std::move(s));
	list.back()->text = wxString::FromAscii((const char*)entry->getData(), entry->getSize());

	return list.back().get();
}

// ----------------------------------------------------------------------------
// addScriptFromFile
//
// Adds a new script to [list], created from the file at [filename]
// ----------------------------------------------------------------------------
Script* addScriptFromFile(const string& filename, vector<Script::UPtr>& list)
{
	wxFileName fn(filename);

	auto s = std::make_unique<Script>();
	s->name = fn.GetName();
	s->path = fn.GetPath(true, wxPATH_UNIX);

	list.push_back(std::move(s));

	wxFile file(filename);
	file.ReadAll(&list.back()->text);
	file.Close();

	return list.back().get();
}

// ----------------------------------------------------------------------------
// loadGeneralScripts
//
// Loads all 'general' scripts from slade.pk3 and custom user scripts
// ----------------------------------------------------------------------------
void loadGeneralScripts()
{
	// Get 'scripts/general' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts/general");
	if (scripts_dir)
		for (auto& entry : scripts_dir->allEntries())
		{
			auto script = addScriptFromEntry(entry, scripts_editor, "/scripts/general/");
			script->read_only = true;
		}
}

void loadCustomScripts()
{
	// If the directory doesn't exist create it
	auto user_scripts_dir = App::path("scripts/custom", App::Dir::User);
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
		addScriptFromFile(user_scripts_dir + "/" + filename, scripts_custom);

		// Next file
		files = res_dir.GetNext(&filename);
	}
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
	{
		for (auto& entry : scripts_dir->allEntries())
		{
			if (!entry->getName().StartsWith("_"))
			{
				auto script = addScriptFromEntry(entry, scripts_archive, "/scripts/archive/");
				script->read_only = true;
			}
			else if (entry->getName() == "_template.lua")
				template_archive_script = wxString::FromAscii(entry->getData(), entry->getSize());
		}
	}

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

void exportUserScripts(const string& path, vector<Script::UPtr>& list)
{
	// Check dir exists
	auto scripts_dir = App::path(path, App::Dir::User);
	if (wxDirExists(scripts_dir))
	{
		// Exists, clear directory
		wxDir res_dir;
		res_dir.Open(scripts_dir);
		string filename = wxEmptyString;
		bool files = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (files)
		{
			wxRemoveFile(filename);
			files = res_dir.GetNext(&filename);
		}
	}
	else
	{
		// Doesn't exist, create directory
		wxMkdir(scripts_dir);
	}

	// Write scripts to directory
	for (auto& script : list)
	{
		if (script->read_only)
			continue;

		wxFile f(App::path(S_FMT("%s/%s.lua", CHR(path), CHR(script->name)), App::Dir::User), wxFile::write);
		f.Write(script->text);
		f.Close();
	}
}

void readTemplate(string& target, const string& res_path)
{
	auto entry = App::archiveManager().programResourceArchive()->entryAtPath(res_path);
	target = wxString::FromAscii(entry->getData(), entry->getSize());
}

} // namespace ScriptManager

// ----------------------------------------------------------------------------
// ScriptManager::init
//
// Initialises the script manager
// ----------------------------------------------------------------------------
void ScriptManager::init()
{
	// Create user scripts directory if it doesn't exist
	auto user_scripts_dir = App::path("scripts", App::Dir::User);
	if (!wxDirExists(user_scripts_dir))
		wxMkdir(user_scripts_dir);

	// Init script templates
	readTemplate(template_archive_script, "scripts/archive/_template.lua");
	readTemplate(template_custom_script, "scripts/_template_custom.lua");

	// Load scripts
	loadGeneralScripts();
	loadCustomScripts();
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

void ScriptManager::saveUserScripts()
{
	// Custom scripts
	exportUserScripts("scripts/custom", scripts_custom);

	// Archive scripts
	exportUserScripts("scripts/archive", scripts_archive);
}

ScriptManager::Script* ScriptManager::createCustomScript(const string& name)
{
	// Check name
	for (auto& script : scripts_custom)
		if (!script->read_only && S_CMPNOCASE(script->name, name))
			return script.get();

	auto ns = std::make_unique<Script>();
	ns->text = template_custom_script;
	ns->name = name;
	scripts_custom.push_back(std::move(ns));

	return scripts_custom.back().get();
}

// ----------------------------------------------------------------------------
// ScriptManager::editorScripts
//
// Returns a list of all general editor scripts
// ----------------------------------------------------------------------------
vector<ScriptManager::Script::UPtr>& ScriptManager::editorScripts()
{
	return scripts_editor;
}

vector<ScriptManager::Script::UPtr>& ScriptManager::customScripts()
{
	return scripts_custom;
}

// ----------------------------------------------------------------------------
// ScriptManager::archiveScripts
//
// Returns a list of all archive scripts
// ----------------------------------------------------------------------------
vector<ScriptManager::Script::UPtr>& ScriptManager::archiveScripts()
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
		menu->Append(SAction::fromId("arch_script")->getWxId() + index++, script->name);
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

	if (!Lua::runArchiveScript(scripts_archive[index]->text, archive))
		Lua::showErrorDialog(parent);
}

ScriptManager::Script* ScriptManager::createArchiveScript(const string& name)
{
	// Check name
	for (auto& script : scripts_archive)
		if (S_CMPNOCASE(script->name, name))
			return script.get();

	auto ns = std::make_unique<Script>();
	ns->text = template_archive_script;
	ns->name = name;
	scripts_archive.push_back(std::move(ns));

	return scripts_archive.back().get();
}
