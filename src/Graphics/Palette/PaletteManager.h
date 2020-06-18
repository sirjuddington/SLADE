#pragma once

#include "Palette.h"

namespace slade
{
class PaletteManager
{
public:
	PaletteManager()  = default;
	~PaletteManager() = default;

	bool     init();
	bool     addPalette(unique_ptr<Palette> pal, string_view name);
	int      numPalettes() const { return (int)palettes_.size(); }
	Palette* defaultPalette() { return &pal_default_; }
	Palette* globalPalette();
	Palette* palette(int index);
	Palette* palette(string_view name);
	string   palName(int index);
	string   palName(Palette* pal);

	bool loadResourcePalettes();
	bool loadCustomPalettes();

private:
	vector<unique_ptr<Palette>> palettes_;
	vector<string>              pal_names_;
	Palette                     pal_default_; // A greyscale palette
	Palette                     pal_global_;  // The global palette (read from the base resource archive)
};
} // namespace slade
