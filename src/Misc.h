
#ifndef __MISC_H__
#define	__MISC_H__

#include "SImage.h"
#include "Archive.h"
#include "ArchiveEntry.h"

enum
{
	PAL_NOHACK = 0,
	PAL_ALPHAHACK,
	PAL_HERETICHACK,
	PAL_SHADOWHACK,
	PAL_ROTTNHACK,
	PAL_ROTTDHACK,
	PAL_ROTTFHACK,
	PAL_ROTTAHACK,
	PAL_SODIDHACK,
	PAL_SODTITLEHACK,
	PAL_SODENDHACK,
};

namespace Misc
{
	bool		loadImageFromEntry(SImage* image, ArchiveEntry* entry, int index = 0);
	int			detectPaletteHack(ArchiveEntry* entry);
	bool		loadPaletteFromArchive(Palette8bit* pal, Archive* archive, int lump = PAL_NOHACK);
	string		sizeAsString(uint32_t size);
	string		lumpNameToFileName(string lump);
	string		fileNameToLumpName(string file);
	uint32_t	crc(const uint8_t* buf, uint32_t len);
	hsl_t		rgbToHsl(double r, double g, double b);
	rgba_t		hslToRgb(double h, double s, double t);
	lab_t		rgbToLab(double r, double g, double b);
	hsl_t		rgbToHsl(rgba_t rgba);
	rgba_t		hslToRgb(hsl_t hsl);
	lab_t		rgbToLab(rgba_t);
	point2_t	findJaguarTextureDimensions(ArchiveEntry* entry, string name);

	// Mass Rename
	string	massRenameFilter(wxArrayString& names);
	void	doMassRename(wxArrayString& names, string name_filter);
}

#endif //__MISC_H__
