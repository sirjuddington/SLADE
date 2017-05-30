
#ifndef __PALETTEMANAGER_H__
#define	__PALETTEMANAGER_H__

#include "Palette.h"

class PaletteManager
{
public:
	PaletteManager();
	~PaletteManager();

	bool			init();
	bool			addPalette(Palette8bit::UPtr pal, string name);
	int				numPalettes() const { return (int)palettes.size(); }
	Palette8bit*	defaultPalette() { return &pal_default; }
	Palette8bit*	globalPalette();
	Palette8bit*	getPalette(int index);
	Palette8bit*	getPalette(string name);
	string			getPalName(int index);
	string			getPalName(Palette8bit* pal);

	bool	loadResourcePalettes();
	bool	loadCustomPalettes();

private:
	vector<Palette8bit::UPtr>	palettes;
	vector<string>				pal_names;
	Palette8bit					pal_default;	// A greyscale palette
	Palette8bit					pal_global;		// The global palette (read from the base resource archive)
};

#endif //__PALETTEMANAGER_H__
