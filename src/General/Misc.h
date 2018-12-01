#pragma once

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

class SImage;
class Archive;
class ArchiveEntry;
class Palette;
class Tokenizer;

namespace Misc
{
bool     loadImageFromEntry(SImage* image, ArchiveEntry* entry, int index = 0);
int      detectPaletteHack(ArchiveEntry* entry);
bool     loadPaletteFromArchive(Palette* pal, Archive* archive, int lump = PAL_NOHACK);
string   sizeAsString(uint32_t size);
string   lumpNameToFileName(string lump);
string   fileNameToLumpName(string file);
uint32_t crc(const uint8_t* buf, uint32_t len);
ColHSL    rgbToHsl(double r, double g, double b);
ColRGBA   hslToRgb(double h, double s, double t);
ColLAB    rgbToLab(double r, double g, double b);
ColHSL    rgbToHsl(ColRGBA rgba);
ColRGBA   hslToRgb(ColHSL hsl);
ColLAB    rgbToLab(ColRGBA);
Vec2i findJaguarTextureDimensions(ArchiveEntry* entry, string name);

// Mass Rename
string massRenameFilter(wxArrayString& names);
void   doMassRename(wxArrayString& names, string name_filter);

// Dialog/Window sizes
struct WindowInfo
{
	string id;
	int    width, height, left, top;
	WindowInfo(string id, int w, int h, int l, int t)
	{
		this->id = id;
		width    = w;
		height   = h;
		left     = l;
		top      = t;
	}
};
WindowInfo getWindowInfo(string id);
void       setWindowInfo(string id, int width, int height, int left, int top);
void       readWindowInfo(Tokenizer& tz);
void       writeWindowInfo(wxFile& file);
} // namespace Misc
