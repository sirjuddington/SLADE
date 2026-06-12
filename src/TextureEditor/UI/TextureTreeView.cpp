
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
	if (CVar::getBool("list_font_monospace"))
		SetFont(wxutil::monospaceFont(GetFont()));

	// Create & associate model
	wxObjectDataPtr model(new TextureTreeModel());
	model->open(editor);
	AssociateModel(model.get());

	setupColumns();
}

CTexture* TextureTreeView::textureForItem(const wxDataViewItem& item)
{
	if (auto model = dynamic_cast<TextureTreeModel*>(GetModel()))
		return model->textureForItem(item);

	return nullptr;
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
	col_index_ = AppendTextColumn(
		wxS("#"),
		0,
		wxDATAVIEW_CELL_INERT,
		FromDIP(ui::getStateInt(ui::TEXTURELIST_INDEX_WIDTH, archive)),
		wxALIGN_NOT,
		ui::getStateBool(ui::TEXTURELIST_INDEX_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	col_name_ = AppendIconTextColumn(
		wxS("Name"),
		1,
		/*elist_rename_inplace ? wxDATAVIEW_CELL_EDITABLE : */ wxDATAVIEW_CELL_INERT,
		FromDIP(ui::getStateInt(ui::TEXTURELIST_NAME_WIDTH, archive)),
		wxALIGN_NOT,
		colstyle_visible);
	col_size_ = AppendTextColumn(
		wxS("Size"),
		2,
		wxDATAVIEW_CELL_INERT,
		FromDIP(ui::getStateInt(ui::TEXTURELIST_SIZE_WIDTH, archive)),
		wxALIGN_NOT,
		ui::getStateBool(ui::TEXTURELIST_SIZE_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	col_patches_ = AppendTextColumn(
		wxS("Patches"),
		4,
		wxDATAVIEW_CELL_INERT,
		FromDIP(ui::getStateInt(ui::TEXTURELIST_PATCHES_WIDTH, archive)),
		wxALIGN_NOT,
		ui::getStateBool(ui::TEXTURELIST_PATCHES_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	col_type_ = AppendTextColumn(
		wxS("Type"),
		3,
		wxDATAVIEW_CELL_INERT,
		FromDIP(ui::getStateInt(ui::TEXTURELIST_TYPE_WIDTH, archive)),
		wxALIGN_NOT,
		ui::getStateBool(ui::TEXTURELIST_TYPE_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	SetExpanderColumn(col_name_);

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);
}
