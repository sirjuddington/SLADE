#pragma once

#include "UI/Lists/SDataViewCtrl.h"

namespace slade
{
class TextureXList;
class CTexture;
class Archive;
} // namespace slade
namespace slade::texeditor
{
class TextureEditor;

class TextureTreeView : public ui::SDataViewCtrl
{
public:
	TextureTreeView(wxWindow* parent, const TextureEditor& editor);

	CTexture*     textureForItem(const wxDataViewItem& item) const;
	TextureXList* textureListForItem(const wxDataViewItem& item) const;

	wxDataViewItem    lastSelectedItem() const;
	vector<CTexture*> selectedTextures() const;
	void              selectTextureList(const TextureXList* list);

	void expandAll();

	void setFilter(string_view name);

private:
	const TextureEditor* editor_ = nullptr;

	wxDataViewColumn* col_index_   = nullptr;
	wxDataViewColumn* col_name_    = nullptr;
	wxDataViewColumn* col_size_    = nullptr;
	wxDataViewColumn* col_type_    = nullptr;
	wxDataViewColumn* col_patches_ = nullptr;

	const Archive* stateArchive() const override;

	void setupColumns();
};
} // namespace slade::texeditor
