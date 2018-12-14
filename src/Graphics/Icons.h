#pragma once

class ArchiveTreeNode;

namespace Icons
{
enum Type
{
	Any     = -1,
	General = 0,
	Entry,
	TextEditor,
};

bool           loadIcons();
wxBitmap       getIcon(Type type, const string& name, bool large, bool log_missing = true);
wxBitmap       getIcon(Type type, const string& name);
bool           exportIconPNG(Type type, const string& name, const string& path);
vector<string> iconSets(Type type);
} // namespace Icons
