
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

	auto colstyle_visible = wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE;
	auto colstyle_hidden  = colstyle_visible | wxDATAVIEW_COL_HIDDEN;

	// Add Columns
	col_index_ = AppendTextColumn(wxS("#"), 0, wxDATAVIEW_CELL_INERT, FromDIP(50), wxALIGN_NOT, colstyle_hidden);
	col_name_  = AppendIconTextColumn(
        wxS("Name"),
        1,
        /*elist_rename_inplace ? wxDATAVIEW_CELL_EDITABLE : */ wxDATAVIEW_CELL_INERT,
        FromDIP(130),
        wxALIGN_NOT,
        colstyle_visible);
	col_size_    = AppendTextColumn(wxS("Size"), 2, wxDATAVIEW_CELL_INERT, FromDIP(70), wxALIGN_NOT, colstyle_visible);
	col_patches_ = AppendTextColumn(
		wxS("Patches"), 4, wxDATAVIEW_CELL_INERT, FromDIP(70), wxALIGN_NOT, colstyle_hidden);
	col_type_ = AppendTextColumn(wxS("Type"), 3, wxDATAVIEW_CELL_INERT, FromDIP(180), wxALIGN_NOT, colstyle_hidden);
	SetExpanderColumn(col_name_);

	// Register columns for generic UI state persistence
	registerColumn(col_index_, "TextureListIndex");
	registerColumn(col_name_, "TextureListName", true);
	registerColumn(col_size_, "TextureListSize");
	registerColumn(col_patches_, "TextureListPatches");
	registerColumn(col_type_, "TextureListType");

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
