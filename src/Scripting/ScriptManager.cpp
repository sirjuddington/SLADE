
#include "Main.h"
#include "ScriptManager.h"
#include "UI/ScriptManagerWindow.h"
#include "Archive/ArchiveManager.h"
#include "Lua.h"

namespace ScriptManager
{
	ScriptManagerWindow*	window = nullptr;
	vector<Script>			scripts_editor;
	vector<Script>			scripts_archive;
	vector<Script>			scripts_acs;
	vector<Script>			scripts_decorate;
	vector<Script>			scripts_zscript;
}

namespace ScriptManager
{

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

void loadGeneralScripts()
{
	// Get 'scripts/general' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts/general");
	if (scripts_dir)
		for (auto& entry : scripts_dir->allEntries())
			addScriptFromEntry(entry, scripts_editor, "/scripts/general/");
}

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

void ScriptManager::init()
{
	loadGeneralScripts();
	loadArchiveScripts();
}

void ScriptManager::open()
{
	if (!window)
		window = new ScriptManagerWindow();

	window->Show();
}

vector<ScriptManager::Script>& ScriptManager::editorScripts()
{
	return scripts_editor;
}

vector<ScriptManager::Script>& ScriptManager::archiveScripts()
{
	return scripts_archive;
}

void ScriptManager::populateArchiveScriptMenu(wxMenu* menu)
{
	int index = 0;
	for (auto& script : scripts_archive)
		menu->Append(SAction::fromId("arch_script")->getWxId() + index++, script.name);
}

void ScriptManager::runArchiveScript(Archive* archive, int index, wxWindow* parent)
{
	if (parent)
		Lua::setCurrentWindow(parent);

	if (!Lua::runArchiveScript(scripts_archive[index].text, archive))
		Lua::showErrorDialog(parent);
}
