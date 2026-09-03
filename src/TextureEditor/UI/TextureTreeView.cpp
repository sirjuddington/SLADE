
#include "Main.h"
#include "TextureTreeView.h"
#include "TextureEditor/TextureEditor.h"
#include "TextureTreeModel.h"
#include "UI/State.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace texeditor;

TextureTreeView::TextureTreeView(wxWindow* parent, const TextureEditor& editor) :
	SDataViewCtrl(parent, wxDV_MULTIPLE),
	editor_(&editor)
{
	prop_sort_column_     = ui::TEXTURELIST_SORT_COLUMN;
	prop_sort_descending_ = ui::TEXTURELIST_SORT_DESCENDING;

	if (CVar::getBool("list_font_monospace"))
		SetFont(wxutil::monospaceFont(GetFont()));

	// Create & associate model
	wxObjectDataPtr model(new TextureTreeModel());
	model->open(editor);
	AssociateModel(model.get());

	setupColumns();

	// Save sort state when the sorting changes
	Bind(
		wxEVT_DATAVIEW_COLUMN_SORTED,
		[this](wxDataViewEvent& e)
		{
			saveSortState();
			e.Skip();
		});

	enableHeaderContextMenu();
}

CTexture* TextureTreeView::textureForItem(const wxDataViewItem& item) const
{
	if (auto model = dynamic_cast<const TextureTreeModel*>(GetModel()))
		return model->textureForItem(item);

	return nullptr;
}

TextureXList* TextureTreeView::textureListForItem(const wxDataViewItem& item) const
{
	if (auto ctex = static_cast<CTexture*>(item.GetID()))
		return ctex->list();

	return nullptr;
}

wxDataViewItem TextureTreeView::lastSelectedItem() const
{
	wxDataViewItemArray selection;
	GetSelections(selection);
	if (!selection.IsEmpty())
		return selection.Last();
	return {};
}

vector<CTexture*> TextureTreeView::selectedTextures() const
{
	vector<CTexture*>   textures;
	wxDataViewItemArray selection;
	GetSelections(selection);
	for (const auto& item : selection)
		if (auto tex = textureForItem(item))
			textures.push_back(tex);
	return textures;
}

void TextureTreeView::expandAll()
{
	if (auto model = dynamic_cast<TextureTreeModel*>(GetModel()))
		for (const auto& item : model->texListItems())
			Expand(item);
}

void TextureTreeView::setupColumns()
{
	auto archive = editor_->archive();

	col_index_ = addColumn(ColumnType::Text, 0, "#", 50, "TextureListIndex", ColumnVisibility::Hidden);
	col_name_  = addColumn(ColumnType::IconAndText, 1, "Name", 130, "TextureListName", ColumnVisibility::AlwaysVisible);
	col_size_  = addColumn(ColumnType::Text, 2, "Size", 70, "TextureListSize", ColumnVisibility::Visible);
	col_patches_ = addColumn(ColumnType::Text, 4, "Patches", 70, "TextureListPatches", ColumnVisibility::Hidden);
	col_type_    = addColumn(ColumnType::Text, 3, "Type", 180, "TextureListType", ColumnVisibility::Hidden);

	// Load width/visibility state
	loadColumnState(archive);

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);

	// Load sorting config
	loadSortState(archive);
}

const Archive* TextureTreeView::stateArchive() const
{
	return editor_ ? editor_->archive() : nullptr;
}
