
#ifndef __ICONS_H__
#define	__ICONS_H__

#include <wx/bitmap.h>

class ArchiveTreeNode;

namespace Icons
{
	enum
	{
		GENERAL,
		ENTRY,
		TEXT_EDITOR,
	};

	bool		loadIcons();
	wxBitmap	getIcon(int type, string name, bool large = false);
	bool		exportIconPNG(int type, string name, string path);
}

#endif//__ICONS_H__
