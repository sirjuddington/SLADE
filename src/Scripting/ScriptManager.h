#pragma once

#include "Archive/ArchiveEntry.h"

namespace ScriptManager
{
	struct Script
	{
		string				text;
		string				name;
		string				path;
		bool				read_only = false;
		ArchiveEntry::WPtr	source;

		typedef std::unique_ptr<Script>	UPtr;
	};

	struct CreateResult
	{
		bool	created;
		Script*	script;
	};

	vector<Script::UPtr>&	editorScripts();
	vector<Script::UPtr>&	customScripts();
	vector<Script::UPtr>&	archiveScripts();

	void	init();
	void	open();
	void	saveUserScripts();

	Script*	createCustomScript(const string& name);
	
	void	populateArchiveScriptMenu(wxMenu* menu);
	void	runArchiveScript(Archive* archive, int index, wxWindow* parent = nullptr);
	Script*	createArchiveScript(const string& name);
}
