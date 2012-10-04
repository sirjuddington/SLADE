
#ifndef __TEXTUREXLIST_H__
#define __TEXTUREXLIST_H__

#include "ArchiveEntry.h"
#include "CTexture.h"
#include "PatchTable.h"

// Enum for different texturex formats
enum TextureXFormat {
    TXF_NORMAL,
    TXF_STRIFE11,
    TXF_NAMELESS,
	TXF_TEXTURES,
	TXF_JAGUAR,
};

// Texture flags
#define TX_WORLDPANNING	0x8000

// TEXTUREx texture patch
struct tx_patch_t {
	int16_t		left;
	int16_t		top;
	uint16_t	patch;
};

class TextureXList {
private:
	vector<CTexture*>	textures;
	uint8_t				txformat;
	CTexture			tex_invalid;

public:
	TextureXList();
	~TextureXList();

	uint32_t	nTextures() { return textures.size(); }

	CTexture*	getTexture(size_t index);
	CTexture*	getTexture(string name);
	uint8_t		getFormat() { return txformat; }
	int			textureIndex(string name);

	void	setFormat(uint8_t format) { txformat = format; }

	void	addTexture(CTexture* tex, int position = -1);
	void	removeTexture(unsigned index);
	void	swapTextures(unsigned index1, unsigned index2);

	void	clear(bool clear_patches = false);
	void	removePatch(string patch);

	bool	readTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table, bool add = false);
	bool	writeTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table);

	bool	readTEXTURESData(ArchiveEntry* textures);
	bool	writeTEXTURESData(ArchiveEntry* textures);

	bool	convertToTEXTURES();

	string	getTextureXFormatString();
};

#endif//__TEXTUREXLIST_H__
