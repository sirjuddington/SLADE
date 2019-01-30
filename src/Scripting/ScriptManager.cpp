
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ScriptManager.h"
#include "Archive/ArchiveManager.h"
#include "Lua.h"
#include "UI/ScriptManagerWindow.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace ScriptManager
{
ScriptManagerWindow*             window = nullptr;
std::map<ScriptType, ScriptList> scripts_editor;

ScriptList scripts_acs;
ScriptList scripts_decorate;
ScriptList scripts_zscript;

std::map<ScriptType, string> script_templates;
} // namespace ScriptManager


// -----------------------------------------------------------------------------
//
// ScriptManager Namespace Functions
//
// -----------------------------------------------------------------------------
namespace ScriptManager
{
// -----------------------------------------------------------------------------
// Adds a new editor script of [type], created from [entry]. [cut_path] will be
// removed from the start of the script's path property
// -----------------------------------------------------------------------------
Script* addEditorScriptFromEntry(ArchiveEntry::SPtr& entry, ScriptType type, const string& cut_path)
{
	auto s  = std::make_unique<Script>();
	s->type = type;
	s->name = entry->name(true);
	s->path = entry->path();
	s->path.Replace(cut_path, "");
	s->source = entry;

	auto& list = scripts_editor[type];
	list.push_back(std::move(s));
	list.back()->text = wxString::FromAscii((const char*)entry->rawData(), entry->size());

	return list.back().get();
}

// -----------------------------------------------------------------------------
// Adds a new editor script of [type], created from the file at [filename]
// -----------------------------------------------------------------------------
Script* addEditorScriptFromFile(const string& filename, ScriptType type)
{
	wxFileName fn(filename);

	auto s  = std::make_unique<Script>();
	s->type = type;
	s->name = fn.GetName();
	s->path = fn.GetPath(true, wxPATH_UNIX);

	auto& list = scripts_editor[type];
	list.push_back(std::move(s));

	wxFile file(filename);
	file.ReadAll(&list.back()->text);
	file.Close();

	return list.back().get();
}

// -----------------------------------------------------------------------------
// Loads all 'general' scripts from slade.pk3 (examples etc.)
// -----------------------------------------------------------------------------
void loadGeneralScripts()
{
	// Get 'scripts/general' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->dir("scripts/general");
	if (scripts_dir)
		for (auto& entry : scripts_dir->allEntries())
		{
			auto script       = addEditorScriptFromEntry(entry, ScriptType::Editor, "/scripts/general/");
			script->read_only = true;
		}
}

// -----------------------------------------------------------------------------
// Loads all custom scripts from the user dir
// -----------------------------------------------------------------------------
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
	bool   files    = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		addEditorScriptFromFile(user_scripts_dir + "/" + filename, ScriptType::Custom);

		// Next file
		files = res_dir.GetNext(&filename);
	}
}

// -----------------------------------------------------------------------------
// Loads all editor scripts of [type] from slade.pk3 and the user dir
// (in scripts/[dir])
// -----------------------------------------------------------------------------
void loadEditorScripts(ScriptType type, const string& dir)
{
	// Get 'scripts/(dir)' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->dir(S_FMT("scripts/%s", CHR(dir)));
	if (scripts_dir)
	{
		for (auto& entry : scripts_dir->allEntries())
		{
			if (!entry->name().StartsWith("_"))
			{
				auto script       = addEditorScriptFromEntry(entry, type, S_FMT("/scripts/%s/", CHR(dir)));
				script->read_only = true;
			}
		}
	}

	// Load user archive scripts

	// If the directory doesn't exist create it
	auto user_scripts_dir = App::path(S_FMT("scripts/%s", CHR(dir)), App::Dir::User);
	if (!wxDirExists(user_scripts_dir))
		wxMkdir(user_scripts_dir);

	// Open the custom custom_scripts directory
	wxDir res_dir;
	res_dir.Open(user_scripts_dir);

	// Go through each file in the directory
	string filename = wxEmptyString;
	bool   files    = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		auto script = addEditorScriptFromFile(user_scripts_dir + "/" + filename, type);

		// Next file
		files = res_dir.GetNext(&filename);
	}
}

// -----------------------------------------------------------------------------
// Exports all scripts in [list] to .lua files at [path]
// -----------------------------------------------------------------------------
void exportUserScripts(const string& path, ScriptList& list)
{
	// Check dir exists
	auto scripts_dir = App::path(path, App::Dir::User);
	if (wxDirExists(scripts_dir))
	{
		// Exists, clear directory
		wxDir res_dir;
		res_dir.Open(scripts_dir);
		string filename = wxEmptyString;
		bool   files    = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (files)
		{
			wxRemoveFile(S_FMT("%s/%s", CHR(scripts_dir), CHR(filename)));
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

// -----------------------------------------------------------------------------
// Loads text from the entry at [res_path] in slade.pk3 into [target]
// -----------------------------------------------------------------------------
void readResourceEntryText(string& target, const string& res_path)
{
	auto entry = App::archiveManager().programResourceArchive()->entryAtPath(res_path);
	if (entry)
		target = wxString::FromAscii(entry->rawData(), entry->size());
}

// -----------------------------------------------------------------------------
// Returns the editor script of [type] matching [name], or nullptr if none by
// that name exist.
// If [user_only] is true, read only (internal) scripts will be ignored
// -----------------------------------------------------------------------------
Script* getEditorScript(const string& name, ScriptType type, bool user_only = true)
{
	for (auto& script : scripts_editor[type])
	{
		if (user_only && script->read_only)
			continue;

		if (S_CMPNOCASE(script->name, name))
			return script.get();
	}

	return nullptr;
}

} // namespace ScriptManager

// -----------------------------------------------------------------------------
// Initialises the script manager
// -----------------------------------------------------------------------------
void ScriptManager::init()
{
	// Create user scripts directory if it doesn't exist
	auto user_scripts_dir = App::path("scripts", App::Dir::User);
	if (!wxDirExists(user_scripts_dir))
		wxMkdir(user_scripts_dir);

	// Init script templates
	readResourceEntryText(script_templates[ScriptType::Archive], "scripts/archive/_template.lua");
	readResourceEntryText(script_templates[ScriptType::Entry], "scripts/entry/_template.lua");
	readResourceEntryText(script_templates[ScriptType::Custom], "scripts/_template_custom.lua");
	readResourceEntryText(script_templates[ScriptType::Map], "scripts/map/_template.lua");

	// Load scripts
	loadGeneralScripts();
	loadCustomScripts();
	loadEditorScripts(ScriptType::Archive, "archive");
	loadEditorScripts(ScriptType::Entry, "entry");
	loadEditorScripts(ScriptType::Map, "map");
}

// -----------------------------------------------------------------------------
// Opens the script manager window
// -----------------------------------------------------------------------------
void ScriptManager::open()
{
	if (!window)
		window = new ScriptManagerWindow();

	window->Show();
}

// -----------------------------------------------------------------------------
// Saves all user scripts to disk
// -----------------------------------------------------------------------------
void ScriptManager::saveUserScripts()
{
	exportUserScripts("scripts/custom", scripts_editor[ScriptType::Custom]);
	exportUserScripts("scripts/archive", scripts_editor[ScriptType::Archive]);
	exportUserScripts("scripts/entry", scripts_editor[ScriptType::Entry]);
	exportUserScripts("scripts/map", scripts_editor[ScriptType::Map]);
}

// -----------------------------------------------------------------------------
// Renames [script] to [new_name].
// Returns false if the script couldn't be renamed
// -----------------------------------------------------------------------------
bool ScriptManager::renameScript(Script* script, const string& new_name)
{
	if (script->read_only || script->type == ScriptType::NonEditor)
		return false;

	// Check name
	if (getEditorScript(new_name, script->type))
		return false;

	script->name = new_name;
	return true;
}

// -----------------------------------------------------------------------------
// Deletes [script]. Returns false if the script couldn't be deleted
// -----------------------------------------------------------------------------
bool ScriptManager::deleteScript(Script* script)
{
	if (script->read_only || script->type == ScriptType::NonEditor)
		return false;

	auto& list = scripts_editor[script->type];
	for (unsigned a = 0; a < list.size(); a++)
	{
		if (list[a].get() == script)
		{
			list.erase(list.begin() + a);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Creates a new script of [type] named [name]. If a script by that name
// already exists, the existing script will be returned instead
// -----------------------------------------------------------------------------
ScriptManager::Script* ScriptManager::createEditorScript(const string& name, ScriptType type)
{
	// Check name
	auto script = getEditorScript(name, type);
	if (!script)
	{
		auto ns  = std::make_unique<Script>();
		ns->name = name;
		ns->text = script_templates[type];
		script   = ns.get();
		scripts_editor[type].push_back(std::move(ns));
	}

	return script;
}

// -----------------------------------------------------------------------------
// Returns a list of all editor scripts of [type]
// -----------------------------------------------------------------------------
vector<ScriptManager::Script::UPtr>& ScriptManager::editorScripts(ScriptType type)
{
	return scripts_editor[type];
}

// -----------------------------------------------------------------------------
// Populates [menu] with all loaded editor scripts of [type]
// -----------------------------------------------------------------------------
void ScriptManager::populateEditorScriptMenu(wxMenu* menu, ScriptType type, const string& action_id)
{
	int index = 0;
	for (auto& script : scripts_editor[type])
		menu->Append(SAction::fromId(action_id)->wxId() + index++, script->name);
}

// -----------------------------------------------------------------------------
// Runs the archive script at [index] on [archive]
// -----------------------------------------------------------------------------
void ScriptManager::runArchiveScript(Archive* archive, int index, wxWindow* parent)
{
	if (parent)
		Lua::setCurrentWindow(parent);

	if (!Lua::runArchiveScript(scripts_editor[ScriptType::Archive][index]->text, archive))
		Lua::showErrorDialog(parent);
}

// -----------------------------------------------------------------------------
// Runs the entry script at [index] on [archive]
// -----------------------------------------------------------------------------
void ScriptManager::runEntryScript(vector<ArchiveEntry*> entries, int index, wxWindow* parent)
{
	if (parent)
		Lua::setCurrentWindow(parent);

	if (!Lua::runEntryScript(scripts_editor[ScriptType::Entry][index]->text, entries))
		Lua::showErrorDialog(parent);
}

// -----------------------------------------------------------------------------
// Runs the map editor script at [index] on [map]
// -----------------------------------------------------------------------------
void ScriptManager::runMapScript(SLADEMap* map, int index, wxWindow* parent)
{
	if (parent)
		Lua::setCurrentWindow(parent);

	if (!Lua::runMapScript(scripts_editor[ScriptType::Map][index]->text, map))
		Lua::showErrorDialog(parent);
}
