
#include "Main.h"
#include "ScriptManager.h"
#include "UI/ScriptManagerWindow.h"
#include "Archive/ArchiveManager.h"

namespace ScriptManager
{
	ScriptManagerWindow*	window = nullptr;
	vector<Script>			scripts_editor;
	vector<Script>			scripts_acs;
	vector<Script>			scripts_decorate;
	vector<Script>			scripts_zscript;
}

void ScriptManager::init()
{
	// Get 'scripts' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts");
	if (scripts_dir)
	{
		for (auto& entry : scripts_dir->allEntries())
		{
			Script s;
			s.name = entry->getName();
			s.path = entry->getPath();
			s.path.Replace("/scripts/", "");
			s.source = entry;
					
			scripts_editor.push_back(s);
			scripts_editor.back().text = wxString::FromAscii((const char*)entry->getData(), entry->getSize());
		}
	}
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
