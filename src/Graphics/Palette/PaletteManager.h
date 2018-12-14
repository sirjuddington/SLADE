#pragma once

#include "Palette.h"

class PaletteManager
{
public:
	PaletteManager()  = default;
	~PaletteManager() = default;

	bool     init();
	bool     addPalette(Palette::UPtr pal, const string& name);
	int      numPalettes() const { return (int)palettes_.size(); }
	Palette* defaultPalette() { return &pal_default_; }
	Palette* globalPalette();
	Palette* palette(int index);
	Palette* palette(const string& name);
	string   palName(int index);
	string   palName(Palette* pal);

	bool loadResourcePalettes();
	bool loadCustomPalettes();

private:
	vector<Palette::UPtr> palettes_;
	vector<string>        pal_names_;
	Palette               pal_default_; // A greyscale palette
	Palette               pal_global_;  // The global palette (read from the base resource archive)
};
