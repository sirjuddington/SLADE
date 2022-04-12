#pragma once

#include "Archive/ArchiveEntry.h"
#include "CTexture.h"
#include "PatchTable.h"

namespace slade
{
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

	TextureXList() = default;
	TextureXList(Format format) : txformat_{ format } {}
	~TextureXList() = default;

	const vector<unique_ptr<CTexture>>& textures() const { return textures_; }
	uint32_t                            size() const { return textures_.size(); }

	CTexture* texture(size_t index);
	CTexture* texture(string_view name);
	Format    format() const { return txformat_; }
	string    textureXFormatString() const;
	int       textureIndex(string_view name) const;

	void setFormat(Format format) { txformat_ = format; }

	void                 addTexture(unique_ptr<CTexture> tex, int position = -1);
	unique_ptr<CTexture> removeTexture(unsigned index);
	void                 swapTextures(unsigned index1, unsigned index2);
	unique_ptr<CTexture> replaceTexture(unsigned index, unique_ptr<CTexture> replacement);

	void clear(bool clear_patches = false);
	void removePatch(string_view patch) const;

	bool readTEXTUREXData(ArchiveEntry* texturex, const PatchTable& patch_table, bool add = false);
	bool writeTEXTUREXData(ArchiveEntry* texturex, const PatchTable& patch_table) const;

	bool readTEXTURESData(ArchiveEntry* textures);
	bool writeTEXTURESData(ArchiveEntry* textures) const;

	bool convertToTEXTURES();
	bool findErrors() const;
	bool removeDupesFoundIn(TextureXList& texture_list);
	bool cleanTEXTURESsinglePatch(Archive* current_archive);

private:
	vector<unique_ptr<CTexture>> textures_;
	Format                       txformat_ = Format::Normal;
	CTexture tex_invalid_{ static_cast<string_view>("INVALID_TEXTURE") }; // Deliberately set the invalid name to >8 characters
};
} // namespace slade
