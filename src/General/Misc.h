#pragma once

class SImage;
class Archive;
class ArchiveEntry;
class Palette;
class Tokenizer;

namespace Misc
{
bool loadImageFromEntry(SImage* image, ArchiveEntry* entry, int index = 0);

// Palette detection
namespace PaletteHack
{
	static const int NONE      = 0;
	static const int ALPHA     = 1;
	static const int HERETIC   = 2;
	static const int SHADOW    = 3;
	static const int ROTT_N    = 4;
	static const int ROTT_D    = 5;
	static const int ROTT_F    = 6;
	static const int ROTT_A    = 7;
	static const int SOD_ID    = 8;
	static const int SOD_TITLE = 9;
	static const int SOD_END   = 10;
}; // namespace PaletteHack
int  detectPaletteHack(ArchiveEntry* entry);
bool loadPaletteFromArchive(Palette* pal, Archive* archive, int lump = PaletteHack::NONE);

std::string sizeAsString(uint32_t size);
std::string lumpNameToFileName(std::string_view lump);
std::string fileNameToLumpName(std::string_view file);
uint32_t    crc(const uint8_t* buf, uint32_t len);
Vec2i       findJaguarTextureDimensions(ArchiveEntry* entry, std::string_view name);

// Mass Rename
std::string massRenameFilter(vector<std::string>& names);
void        doMassRename(vector<std::string>& names, std::string_view name_filter);

// Dialog/Window sizes
struct WindowInfo
{
	std::string id;
	int         width, height, left, top;
	WindowInfo(std::string_view id, int w, int h, int l, int t) : id{ id }, width{ w }, height{ h }, left{ l }, top{ t }
	{
	}
};
WindowInfo getWindowInfo(std::string_view id);
void       setWindowInfo(std::string_view id, int width, int height, int left, int top);
void       readWindowInfo(Tokenizer& tz);
void       writeWindowInfo(wxFile& file);
} // namespace Misc
