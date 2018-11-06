
#ifndef __PALETTEMANAGER_H__
#define	__PALETTEMANAGER_H__

#include "Palette.h"

class PaletteManager
{
public:
	PaletteManager();
	~PaletteManager();

	bool			init();
	bool			addPalette(Palette::UPtr pal, string name);
	int				numPalettes() const { return (int)palettes.size(); }
	Palette*	defaultPalette() { return &pal_default; }
	Palette*	globalPalette();
	Palette*	getPalette(int index);
	Palette*	getPalette(string name);
	string			getPalName(int index);
	string			getPalName(Palette* pal);

	bool	loadResourcePalettes();
	bool	loadCustomPalettes();

private:
	vector<Palette::UPtr>	palettes;
	vector<string>				pal_names;
	Palette					pal_default;	// A greyscale palette
	Palette					pal_global;		// The global palette (read from the base resource archive)
};

#endif //__PALETTEMANAGER_H__
