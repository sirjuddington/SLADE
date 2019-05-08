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
wxBitmap       getIcon(Type type, string_view name, bool large, bool log_missing = true);
wxBitmap       getIcon(Type type, string_view name);
bool           exportIconPNG(Type type, string_view name, string_view path);
vector<string> iconSets(Type type);
} // namespace Icons
