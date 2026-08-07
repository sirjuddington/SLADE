#pragma once

#include <map>

#include "General/UndoRedo.h"

// Forward declarations
namespace slade
{
class CTPatch;
class TextureXList;
class CTexture;
} // namespace slade
namespace slade::texeditor
{
class TextureEditor;
}

namespace slade::texeditor
{
class TexturePropertyChangeUS : public UndoStep
{
public:
	TexturePropertyChangeUS(
		const TextureEditor& editor,
		const CTexture&      texture,
		string_view          property,
		const Property&      prev_value);

	virtual bool swapValue();

	bool doUndo() override { return swapValue(); }
	bool doRedo() override { return swapValue(); }

protected:
	string               property_;
	Property             prev_value_;
	const TextureXList*  tx_list_ = nullptr;
	int                  index_   = -1;
	const TextureEditor* editor_  = nullptr;
};

class TexturePatchListChangeUS : public UndoStep
{
public:
	TexturePatchListChangeUS(const TextureEditor& editor, const CTexture& texture);

	bool swapLists();

	bool doUndo() override { return swapLists(); }
	bool doRedo() override { return swapLists(); }

private:
	const TextureXList*         tx_list_ = nullptr;
	int                         index_   = -1;
	const TextureEditor*        editor_  = nullptr;
	vector<unique_ptr<CTPatch>> patches_;
};

class PatchPropertyChangeUS : public TexturePropertyChangeUS
{
public:
	PatchPropertyChangeUS(
		const TextureEditor& editor,
		const CTexture&      texture,
		int                  patch_index,
		string_view          property,
		const Property&      prev_value);

	bool swapValue() override;

private:
	int patch_index_ = -1;
};

class PatchMoveUS : public UndoStep
{
public:
	PatchMoveUS(
		const TextureEditor&    editor,
		const CTexture&         texture,
		const vector<unsigned>& patch_indices,
		const Vec2i&            offset);

	bool doUndo() override;
	bool doRedo() override;

private:
	const TextureEditor* editor_  = nullptr;
	const TextureXList*  tx_list_ = nullptr;
	int                  index_   = -1;
	vector<unsigned>     patch_indices_;
	Vec2i                offset_;
};

class TextureCreateDeleteUS : public UndoStep
{
public:
	TextureCreateDeleteUS(const TextureEditor& editor, TextureXList* list, int created_index);
	TextureCreateDeleteUS(
		const TextureEditor& editor,
		TextureXList*        list,
		unique_ptr<CTexture> tex_removed,
		int                  removed_index);
	~TextureCreateDeleteUS() override;

	bool deleteTexture();
	bool createTexture();

	bool doUndo() override;
	bool doRedo() override;

private:
	const TextureEditor* editor_  = nullptr;
	TextureXList*        tx_list_ = nullptr;
	unique_ptr<CTexture> tex_removed_;
	int                  index_   = -1;
	bool                 created_ = true;
};

class TextureModificationUS : public UndoStep
{
public:
	TextureModificationUS(TextureEditor& editor, const CTexture& texture);
	~TextureModificationUS() override;

	bool swapData() const;

	bool doUndo() override { return swapData(); }
	bool doRedo() override { return swapData(); }

private:
	TextureXList*        tx_list_ = nullptr;
	unique_ptr<CTexture> tex_copy_;
	int                  index_  = -1;
	TextureEditor*       editor_ = nullptr;
};

class TextureSwapUS : public UndoStep
{
public:
	TextureSwapUS(const TextureEditor& editor, TextureXList& texturex, int index1, int index2);
	~TextureSwapUS() override;

	bool doSwap() const;

	bool doUndo() override { return doSwap(); }
	bool doRedo() override { return doSwap(); }

private:
	const TextureEditor* editor_   = nullptr;
	TextureXList*        texturex_ = nullptr;
	int                  index1_   = -1;
	int                  index2_   = -1;
};

class TextureListReorderUS : public UndoStep
{
public:
	TextureListReorderUS(
		const TextureEditor&      editor,
		TextureXList&             texturex,
		unsigned                  first,
		unsigned                  last,
		std::map<unsigned, unsigned> index_swaps);
	~TextureListReorderUS() override;

	bool swapOrder();

	bool doUndo() override { return swapOrder(); }
	bool doRedo() override { return swapOrder(); }

private:
	const TextureEditor*          editor_      = nullptr;
	TextureXList*                 texturex_    = nullptr;
	unsigned                      first_       = 0;
	unsigned                      last_        = 0;
	std::map<unsigned, unsigned> index_swaps_;
};
} // namespace slade::texeditor
