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
	wxBitmap       getIcon(Type type, string_view name, int size = -1, Point2i padding = { 0, 0 });
	vector<string> iconSets(Type type);
} // namespace icons
} // namespace slade
