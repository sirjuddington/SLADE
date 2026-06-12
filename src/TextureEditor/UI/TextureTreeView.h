#pragma once

#include "UI/Lists/SDataViewCtrl.h"

namespace slade
{
class CTexture;
}
namespace slade::texeditor
{
class TextureEditor;

class TextureTreeView : public ui::SDataViewCtrl
{
public:
	TextureTreeView(wxWindow* parent, const TextureEditor& editor);

	CTexture* textureForItem(const wxDataViewItem& item);

	void expandAll();

private:
	const TextureEditor* editor_ = nullptr;

	wxDataViewColumn* col_index_   = nullptr;
	wxDataViewColumn* col_name_    = nullptr;
	wxDataViewColumn* col_size_    = nullptr;
	wxDataViewColumn* col_type_    = nullptr;
	wxDataViewColumn* col_patches_ = nullptr;

	void setupColumns();
};
} // namespace slade::texeditor
