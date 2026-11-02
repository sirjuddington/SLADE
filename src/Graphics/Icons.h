#pragma once

namespace slade
{
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

	bool loadIcons();

	wxBitmapBundle getIcon(Type type, string_view name, int size = -1, const Point2i& padding = { 0, 0 });
	wxBitmapBundle getInterfaceIcon(string_view name, int size = -1, InterfaceTheme theme = System);

	vector<string> iconSets(Type type);
	bool           iconExists(Type type, string_view name);
} // namespace icons
} // namespace slade
