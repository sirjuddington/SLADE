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
		TextEditor
	};

	enum InterfaceTheme
	{
		System = -1, // Determine whether dark or light from system
		Light  = 0,  // Light theme (black icons)
		Dark   = 1   // Dark theme (white icons)
	};

	bool           loadIcons();
	wxBitmap       getIcon(Type type, string_view name, int size = -1, Point2i padding = { 0, 0 });
	wxBitmap       getInterfaceIcon(string_view name, int size = -1, InterfaceTheme theme = System);
	vector<string> iconSets(Type type);
} // namespace icons
} // namespace slade
