#pragma once

#include "Archive/ArchiveEntry.h"
#include "CTexture.h"
#include "PatchTable.h"

class TextureXList
{
public:
	// TEXTUREx texture patch
	struct Patch
	{
		int16_t  left;
		int16_t  top;
		uint16_t patch;
	};

	// Enum for different texturex formats
	enum class Format
	{
		Normal,
		Strife11,
		Nameless,
		Textures,
		Jaguar,
	};

	enum Flags
	{
		WorldPanning = 0x8000
	};

	TextureXList();
	~TextureXList();

	uint32_t nTextures() { return textures_.size(); }

	CTexture* texture(size_t index);
	CTexture* texture(string name);
	Format    format() { return txformat_; }
	string    textureXFormatString();
	int       textureIndex(string name);

	void setFormat(Format format) { txformat_ = format; }

	void      addTexture(CTexture* tex, int position = -1);
	CTexture* removeTexture(unsigned index, bool delete_texture = true);
	void      swapTextures(unsigned index1, unsigned index2);
	CTexture* replaceTexture(unsigned index, CTexture* replacement);

	void clear(bool clear_patches = false);
	void removePatch(string patch);

	bool readTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table, bool add = false);
	bool writeTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table);

	bool readTEXTURESData(ArchiveEntry* textures);
	bool writeTEXTURESData(ArchiveEntry* textures);

	bool convertToTEXTURES();
	bool findErrors();

private:
	vector<CTexture*> textures_;
	Format            txformat_;
	CTexture          tex_invalid_;
};
