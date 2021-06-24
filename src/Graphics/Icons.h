#pragma once


namespace slade
{
class ArchiveEntry;

namespace icons
{
	enum Type
	{
		Any     = -1,
		General = 0,
		Entry,
		TextEditor,
	};

	bool           loadIcons();
	wxBitmap       getIcon(Type type, string_view name, int size, bool log_missing = true);
	wxBitmap       getPaddedIcon(Type type, string_view name, int size, Point2i padding = { 1, 1 });
	wxBitmap       getIcon(Type type, string_view name);
	wxBitmap       getSVGIcon(Type type, string_view name, int size, Point2i padding = { 0, 0 });
	bool           iconExists(Type type, string_view name);
	ArchiveEntry*  getIconEntry(Type type, string_view name, int size);
	bool           exportIconPNG(Type type, string_view name, string_view path);
	vector<string> iconSets(Type type);
} // namespace icons
} // namespace slade
