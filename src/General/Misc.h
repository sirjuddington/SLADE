#pragma once

namespace slade
{
class SImage;
class Tokenizer;

namespace misc
{
	bool loadImageFromEntry(SImage* image, ArchiveEntry* entry, int index = 0);

	// Palette detection
	namespace palhack
	{
		static constexpr int NONE      = 0;
		static constexpr int ALPHA     = 1;
		static constexpr int HERETIC   = 2;
		static constexpr int SHADOW    = 3;
		static constexpr int ROTT_N    = 4;
		static constexpr int ROTT_D    = 5;
		static constexpr int ROTT_F    = 6;
		static constexpr int ROTT_A    = 7;
		static constexpr int SOD_ID    = 8;
		static constexpr int SOD_TITLE = 9;
		static constexpr int SOD_END   = 10;
	} // namespace palhack
	int  detectPaletteHack(const ArchiveEntry* entry);
	bool loadPaletteFromArchive(Palette* pal, Archive* archive, int lump = palhack::NONE);

	string   sizeAsString(uint32_t size);
	string   lumpNameToFileName(string_view lump);
	string   fileNameToLumpName(string_view file, bool percent_encoding_only = false);
	uint32_t crc(const uint8_t* buf, uint32_t len);
	Vec2i    findJaguarTextureDimensions(const ArchiveEntry* entry, string_view name);

	// Mass Rename
	string massRenameFilter(const vector<string>& names);
	void   doMassRename(vector<string>& names, string_view name_filter);
} // namespace misc
} // namespace slade
