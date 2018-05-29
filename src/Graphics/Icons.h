
#ifndef __ICONS_H__
#define	__ICONS_H__

#include "common.h"

class ArchiveTreeNode;

namespace Icons
{
	enum
	{
		GENERAL,
		ENTRY,
		TEXT_EDITOR,
	};

	bool			loadIcons();
	wxBitmap		getIcon(int type, string name, bool large, bool log_missing = true);
	wxBitmap		getIcon(int type, string name);
	bool			exportIconPNG(int type, string name, string path);
	vector<string>	getIconSets(int type);
}

#endif//__ICONS_H__
