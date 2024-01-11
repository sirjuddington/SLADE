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

	bool loadIcons();

#if wxCHECK_VERSION(3, 1, 6)
	wxBitmapBundle getIcon(Type type, string_view name, int size = -1, Point2i padding = { 0, 0 });
	wxBitmapBundle getInterfaceIcon(string_view name, int size = -1, InterfaceTheme theme = System);
#else
	wxBitmap getIcon(Type type, string_view name, int size = -1, Point2i padding = { 0, 0 });
	wxBitmap getInterfaceIcon(string_view name, int size = -1, InterfaceTheme theme = System);
#endif

	vector<string> iconSets(Type type);
	bool           iconExists(Type type, string_view name);

	struct IconCache
	{
#if wxCHECK_VERSION(3, 1, 6)
		std::unordered_map<string, wxBitmapBundle> icons;
#else
		std::unordered_map<string, wxIcon> icons;
#endif

		bool isCached(const string& name) { return icons.find(name) != icons.end(); }
		void cacheIcon(Type type, const string& name, int size, Point2i padding = {});
	};
} // namespace icons
} // namespace slade
