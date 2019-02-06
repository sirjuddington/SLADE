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

bool             loadIcons();
wxBitmap         getIcon(Type type, const wxString& name, bool large, bool log_missing = true);
wxBitmap         getIcon(Type type, const wxString& name);
bool             exportIconPNG(Type type, const wxString& name, const wxString& path);
vector<wxString> iconSets(Type type);
} // namespace Icons
