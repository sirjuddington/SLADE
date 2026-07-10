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

	Archive*    archive() const { return archive_.get(); }
	PatchTable* patchTable() const { return patch_table_.get(); }

	unsigned      nTextureLists() const { return texturex_entries_.size(); }
	TextureXList* textureList(unsigned index) const;
	ArchiveEntry* textureListEntry(unsigned index) const;
	string        textureListName(const TextureXList& list) const;

	CTexture* currentTexture() const { return tex_current_; }
	void      openTexture(CTexture& texture);
	void      closeTexture();
	void      revertTexture();

	bool currentModified() const { return tex_modified_; }
	void setCurrentModified(bool modified = true) const { tex_modified_ = modified; }

	const vector<unsigned>& selectedPatches() const { return selected_patches_; }
	void                    selectPatch(unsigned index, bool selected = true);

	string importPatchFile(string_view filename, bool add_to_patch_table = true) const;

	// Texture List Editing
	void newTexture(
		TextureXList* list,
		string_view   name,
		int           index  = -1,
		int           width  = 0,
		int           height = 0,
		string_view   patch  = {}) const;
	void deleteTexture(const CTexture& texture) const;
	void moveTexture(const CTexture& texture, Direction direction) const;
	void sortTextures(const vector<CTexture*>& textures) const;
	void renameTextures(const vector<CTexture*>& textures, bool each) const;
	bool exportAsPNG(const CTexture& texture, string_view filename, const Palette* palette, bool force_rgba);

	// Texture Editing
	void setTextureSize(int width = -1, int height = -1) const;
	void setTextureScaleX(double scale) const;
	void setTextureScaleY(double scale) const;
	void setTextureFlag(string_view flag, bool on = true) const;
	void setTextureType(CTexture::Type type) const;
	void setTextureOffsetX(int offset) const;
	void setTextureOffsetY(int offset) const;

	// Patch Editing
	void setPatchOffsetX(int offset) const;
	void setPatchOffsetY(int offset) const;
	void movePatch(const Vec2i& offset) const;
	void setPatchBlendType(CTPatchEx::BlendType type) const;
	void setPatchFlag(string_view flag, bool on = true) const;
	void setPatchRotation(int rotation) const;
	void setPatchAlpha(double alpha) const;
	void setPatchAlphaStyle(string_view style) const;
	void setPatchColour(const ColRGBA& colour) const;
	void setPatchTintAmount(double amount) const;
	void setPatchTranslation(string_view translation) const;

	// Patch List Editng
	void addPatch(string_view patch) const;
	void removePatch();
	void replacePatch(string_view patch) const;
	void duplicatePatch(int xoff, int yoff);
	void patchForward();
	void patchBack();

	struct Signals
	{
		// Texture modified (not the current texture)
		sigslot::signal<CTexture*> texture_modified;

		// Multiple textures modified
		sigslot::signal<const vector<CTexture*>&> textures_modified;

		// Texture added to a TextureXList (list, texture)
		sigslot::signal<TextureXList*, CTexture*> texture_added;

		// Texture removed from a TextureXList (list, texture)
		sigslot::signal<TextureXList*, CTexture*> texture_deleted;

		// Textures swapped in a TextureXList (list, index1, index2)
		sigslot::signal<TextureXList*, unsigned, unsigned> textures_swapped;

		// Current texture modified (texture, patch_list)
		// texture: the texture needs to be rebuilt (ie. visual changes)
		// patch_list: the patch list was modified (patches added/removed/reordered)
		sigslot::signal<bool, bool> current_texture_modified;

		// Current texture patch(es) modified (patch_indices)
		sigslot::signal<const vector<unsigned>&> patches_modified;
	};
	Signals& signals() const { return signals_; }

private:
	unique_ptr<PatchTable> patch_table_;
	shared_ptr<Archive>    archive_;
	mutable Signals        signals_;

	struct TextureXEntry
	{
		weak_ptr<ArchiveEntry>   entry;
		unique_ptr<TextureXList> texturex;
	};
	vector<TextureXEntry> texturex_entries_;

	CTexture*            tex_current_  = nullptr;
	mutable bool         tex_modified_ = false; // If any modifications have been made since the texture was opened
	unique_ptr<CTexture> tex_backup_;           // For revert
	vector<unsigned>     selected_patches_;

	void signalCurrentTextureModified(bool texture, bool patch_list, bool patches) const;
};
} // namespace slade::texeditor
