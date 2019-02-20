
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
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include <filesystem>
#include <fstream>


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

std::map<ScriptType, std::string> script_templates;
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
Script* addEditorScriptFromEntry(ArchiveEntry::SPtr& entry, ScriptType type, std::string_view cut_path)
{
	auto s    = std::make_unique<Script>();
	s->type   = type;
	s->name   = entry->nameNoExt();
	s->path   = entry->path();
	s->source = entry;
	StrUtil::replaceIP(s->path, cut_path, {});

	auto& list = scripts_editor[type];
	list.push_back(std::move(s));
	list.back()->text = std::string((const char*)entry->rawData(), entry->size());

	return list.back().get();
}

// -----------------------------------------------------------------------------
// Adds a new editor script of [type], created from the file at [filename]
// -----------------------------------------------------------------------------
Script* addEditorScriptFromFile(std::string_view filename, ScriptType type)
{
	StrUtil::Path fn(filename);

	auto s  = std::make_unique<Script>();
	s->type = type;
	s->name = fn.fileName(false);
	s->path = fn.path(true);

	auto& list = scripts_editor[type];
	list.push_back(std::move(s));

	FileUtil::readFileToString(fn.fullPath(), list.back()->text);

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
	if (!FileUtil::dirExists(user_scripts_dir))
		FileUtil::createDir(user_scripts_dir);

	// Go through each file in the custom_scripts directory
	auto files = FileUtil::allFilesInDir(user_scripts_dir);
	for (const auto& filename : files)
		addEditorScriptFromFile(filename, ScriptType::Custom);
}

// -----------------------------------------------------------------------------
// Loads all editor scripts of [type] from slade.pk3 and the user dir
// (in scripts/[dir])
// -----------------------------------------------------------------------------
void loadEditorScripts(ScriptType type, std::string_view dir)
{
	// Get 'scripts/(dir)' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->dir(fmt::format("scripts/{}", dir));
	if (scripts_dir)
	{
		for (auto& entry : scripts_dir->allEntries())
		{
			if (!StrUtil::startsWith(entry->name(), '_'))
			{
				auto script       = addEditorScriptFromEntry(entry, type, fmt::format("/scripts/{}/", dir));
				script->read_only = true;
			}
		}
	}

	// Load user archive scripts

	// If the directory doesn't exist create it
	auto user_scripts_dir = App::path(fmt::format("scripts/{}", dir), App::Dir::User);
	if (!FileUtil::dirExists(user_scripts_dir))
		FileUtil::createDir(user_scripts_dir);

	// Go through each file in the custom_scripts directory
	auto files = FileUtil::allFilesInDir(user_scripts_dir);
	for (const auto& filename : files)
		addEditorScriptFromFile(filename, type);
}

// -----------------------------------------------------------------------------
// Exports all scripts in [list] to .lua files at [path]
// -----------------------------------------------------------------------------
void exportUserScripts(std::string_view path, ScriptList& list)
{
	// Check dir exists
	auto scripts_dir = App::path(path, App::Dir::User);
	if (FileUtil::dirExists(scripts_dir))
	{
		// Exists, clear lua files in directory
		for (const auto& item : std::filesystem::directory_iterator{ scripts_dir })
			if (item.is_regular_file() && item.path().extension() == "lua")
				std::filesystem::remove(item);
	}
	else
	{
		// Doesn't exist, create directory
		FileUtil::createDir(scripts_dir);
	}

	// Write scripts to directory
	for (auto& script : list)
	{
		if (script->read_only)
			continue;

		FileUtil::writeStringToFile(
			script->text, App::path(fmt::format("{}/{}.lua", path, script->name), App::Dir::User));
	}
}

// -----------------------------------------------------------------------------
// Loads text from the entry at [res_path] in slade.pk3 into [target]
// -----------------------------------------------------------------------------
void readResourceEntryText(std::string& target, std::string_view res_path)
{
	auto entry = App::archiveManager().programResourceArchive()->entryAtPath(res_path);
	if (entry)
		target.assign((const char*)entry->rawData(), entry->size());
}

// -----------------------------------------------------------------------------
// Returns the editor script of [type] matching [name], or nullptr if none by
// that name exist.
// If [user_only] is true, read only (internal) scripts will be ignored
// -----------------------------------------------------------------------------
Script* getEditorScript(std::string_view name, ScriptType type, bool user_only = true)
{
	for (auto& script : scripts_editor[type])
	{
		if (user_only && script->read_only)
			continue;

		if (StrUtil::equalCI(script->name, name))
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
bool ScriptManager::renameScript(Script* script, std::string_view new_name)
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
ScriptManager::Script* ScriptManager::createEditorScript(std::string_view name, ScriptType type)
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
void ScriptManager::populateEditorScriptMenu(wxMenu* menu, ScriptType type, std::string_view action_id)
{
	int index = 0;
	for (auto& script : scripts_editor[type])
		menu->Append(SAction::fromId(std::string{ action_id })->wxId() + index++, script->name);
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
