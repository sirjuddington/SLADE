#pragma once

class ArchiveTreeNode;

namespace Icons
{
enum
{
	General,
	Entry,
	TextEditor,
};

bool           loadIcons();
wxBitmap       getIcon(int type, string name, bool large, bool log_missing = true);
wxBitmap       getIcon(int type, string name);
bool           exportIconPNG(int type, string name, string path);
vector<string> getIconSets(int type);
} // namespace Icons
