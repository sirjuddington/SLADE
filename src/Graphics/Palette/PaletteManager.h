#pragma once

namespace slade
{
class PaletteManager
{
public:
	PaletteManager();
	~PaletteManager();

	bool     init();
	bool     addPalette(unique_ptr<Palette> pal, string_view name);
	int      numPalettes() const { return static_cast<int>(palettes_.size()); }
	Palette* defaultPalette() const { return pal_default_.get(); }
	Palette* globalPalette() const;
	Palette* palette(int index) const;
	Palette* palette(string_view name) const;
	string   palName(int index);
	string   palName(const Palette* pal);

	bool loadResourcePalettes();
	bool loadCustomPalettes();

private:
	vector<unique_ptr<Palette>> palettes_;
	vector<string>              pal_names_;
	unique_ptr<Palette>         pal_default_; // A greyscale palette
	unique_ptr<Palette>         pal_global_;  // The global palette (read from the base resource archive)
};
} // namespace slade
