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

	void			init();
	void			open();
	vector<Script>&	editorScripts();
}
