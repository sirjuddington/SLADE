#pragma once

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

	CTexture* currentTexture() const { return tex_current_; }
	void      setCurrentTexture(CTexture* tex) { tex_current_ = tex; }

private:
	unique_ptr<PatchTable> patch_table_;
	shared_ptr<Archive>    archive_;

	struct TextureXEntry
	{
		weak_ptr<ArchiveEntry>   entry;
		unique_ptr<TextureXList> texturex;
	};
	vector<TextureXEntry> texturex_entries_;

	CTexture* tex_current_ = nullptr; // The texture currently open in the editor
};
} // namespace slade::texeditor
