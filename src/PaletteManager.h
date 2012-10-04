
#ifndef __PALETTEMANAGER_H__
#define	__PALETTEMANAGER_H__

#include "Palette.h"

class PaletteManager {
private:
	vector<Palette8bit*>	palettes;
	vector<string>			pal_names;
	Palette8bit*			pal_default;	// A greyscale palette
	Palette8bit*			pal_global;		// The global palette (read from the base resource archive)

	static PaletteManager*	instance;

public:
	PaletteManager();
	~PaletteManager();

	static PaletteManager*	getInstance() {
		if (!instance)
			instance = new PaletteManager();

		return instance;
	}

	bool			addPalette(Palette8bit* pal, string name);
	int				numPalettes() { return (int)palettes.size(); }
	Palette8bit*	defaultPalette() { return pal_default; }
	Palette8bit*	globalPalette();
	Palette8bit*	getPalette(int index);
	Palette8bit*	getPalette(string name);
	string			getPalName(int index);
	string			getPalName(Palette8bit* pal);

	bool	loadResourcePalettes();
	bool	loadCustomPalettes();
};

// Define for less cumbersome PaletteManager::getInstance()
#define thePaletteManager PaletteManager::getInstance()

#endif //__PALETTEMANAGER_H__
