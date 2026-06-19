#pragma once

#include "Graphics/CTexture/CTexture.h"

namespace slade
{
class CTexture;
class PatchTable;
class TextureXList;
} // namespace slade

namespace slade::texeditor
{
class TextureEditor
{
public:
	TextureEditor(shared_ptr<Archive> archive);
	~TextureEditor();

	Archive* archive() const { return archive_.get(); }

	unsigned      nTextureLists() const { return texturex_entries_.size(); }
	TextureXList* textureList(unsigned index) const;
	ArchiveEntry* textureListEntry(unsigned index) const;
	string        textureListName(const TextureXList& list) const;

	CTexture* currentTexture() const { return tex_current_.get(); }
	void      openTexture(CTexture& texture);
	void      closeTexture();
	void      saveTexture() const;
	void      revertTexture() const;

	const vector<unsigned>& selectedPatches() const { return selected_patches_; }
	void                    selectPatch(unsigned index, bool selected = true);

	void setTextureModified(bool update_texture, bool update_patches) const;

	void setTextureSize(int width = -1, int height = -1) const;
	void setTextureScaleX(double scale) const;
	void setTextureScaleY(double scale) const;
	void setTextureFlag(string_view flag, bool on = true) const;
	void setTextureType(CTexture::Type type) const;
	void setTextureOffsetX(int offset) const;
	void setTextureOffsetY(int offset) const;

	void setPatchOffsetX(int offset) const;
	void setPatchOffsetY(int offset) const;
	void setPatchBlendType(CTPatchEx::BlendType type) const;
	void setPatchFlag(string_view flag, bool on = true) const;
	void setPatchRotation(int rotation) const;
	void setPatchAlpha(double alpha) const;
	void setPatchAlphaStyle(string_view style) const;
	void setPatchColour(const ColRGBA& colour) const;
	void setPatchTintAmount(double amount) const;
	void setPatchTranslation(string_view translation) const;

private:
	unique_ptr<PatchTable> patch_table_;
	shared_ptr<Archive>    archive_;

	struct TextureXEntry
	{
		weak_ptr<ArchiveEntry>   entry;
		unique_ptr<TextureXList> texturex;
	};
	vector<TextureXEntry> texturex_entries_;

	unique_ptr<CTexture> tex_current_;                  // The texture currently open in the editor
	CTexture*            tex_current_source_ = nullptr; // The source texture that tex_current_ was opened from
	vector<unsigned>     selected_patches_;
};
} // namespace slade::texeditor
