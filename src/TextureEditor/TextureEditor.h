#pragma once

#include "Graphics/CTexture/CTexture.h"

namespace slade
{
class CTexture;
class PatchTable;
class TextureXList;
}

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
	void      openTexture(const CTexture& texture);
	void      closeTexture();
	bool      texModified() const { return tex_modified_; }
	void      setTexModified(bool modified = true) { tex_modified_ = modified; }

	const vector<unsigned>& selectedPatches() const { return selected_patches_; }
	void                    selectPatch(unsigned index, bool selected = true);

	void setTextureSize(int width = -1, int height = -1);
	void setTextureScaleX(double scale);
	void setTextureScaleY(double scale);
	void setTextureFlag(string_view flag, bool on = true);
	void setTextureType(CTexture::Type type);

	void setPatchOffsetX(int offset);
	void setPatchOffsetY(int offset);
	void setPatchBlendType(CTPatchEx::BlendType type);
	void setPatchFlag(string_view flag, bool on = true);
	void setPatchRotation(int rotation);
	void setPatchAlpha(double alpha);
	void setPatchAlphaStyle(string_view style);
	void setPatchColour(const ColRGBA& colour);
	void setPatchTintAmount(double amount);
	void setPatchTranslation(string_view translation);

private:
	unique_ptr<PatchTable> patch_table_;
	shared_ptr<Archive>    archive_;

	struct TextureXEntry
	{
		weak_ptr<ArchiveEntry>   entry;
		unique_ptr<TextureXList> texturex;
	};
	vector<TextureXEntry> texturex_entries_;

	unique_ptr<CTexture> tex_current_; // The texture currently open in the editor
	bool                 tex_modified_ = false;
	vector<unsigned>     selected_patches_;
};
} // namespace slade::texeditor
