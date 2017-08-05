#pragma once

#include "Archive/ArchiveEntry.h"

namespace ScriptManager
{
	struct Script
	{
		string				text;
		string				name;
		string				path;
		ArchiveEntry::WPtr	source;
	};

	vector<Script>&	editorScripts();
	vector<Script>&	archiveScripts();

	void			init();
	void			open();
	
	void	populateArchiveScriptMenu(wxMenu* menu);
	void	runArchiveScript(Archive* archive, int index, wxWindow* parent = nullptr);
}
