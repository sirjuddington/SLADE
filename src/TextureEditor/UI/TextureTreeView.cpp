
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureTreeView.cpp
// Description: TextureTreeView class, a wxDataViewCtrl that displays the
//              texture lists and their textures in a tree
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "TextureTreeView.h"
#include "TextureEditor/TextureEditor.h"
#include "TextureTreeModel.h"
#include "UI/State.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// TextureTreeView Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureTreeView class constructor
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Returns the texture for [item], or null if [item] is a texture list item
// -----------------------------------------------------------------------------
CTexture* TextureTreeView::textureForItem(const wxDataViewItem& item) const
{
	if (auto model = dynamic_cast<const TextureTreeModel*>(GetModel()))
		return model->textureForItem(item);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the texture list for [item], or null if [item] is a texture item
// -----------------------------------------------------------------------------
TextureXList* TextureTreeView::textureListForItem(const wxDataViewItem& item) const
{
	if (auto ctex = static_cast<CTexture*>(item.GetID()))
		return ctex->list();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the last selected item, or an invalid item if nothing is selected
// -----------------------------------------------------------------------------
wxDataViewItem TextureTreeView::lastSelectedItem() const
{
	wxDataViewItemArray selection;
	GetSelections(selection);
	if (!selection.IsEmpty())
		return selection.Last();
	return {};
}

// -----------------------------------------------------------------------------
// Returns all currently selected textures (texture list items are ignored)
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Expands all texture list (root) items in the tree
// -----------------------------------------------------------------------------
void TextureTreeView::expandAll()
{
	if (auto model = dynamic_cast<TextureTreeModel*>(GetModel()))
		for (const auto& item : model->texListItems())
			Expand(item);
}

// -----------------------------------------------------------------------------
// Sets up the tree columns
// -----------------------------------------------------------------------------
void TextureTreeView::setupColumns()
{
	auto archive = editor_->archive();

	// Add columns
	col_index_ = addColumn(ColumnType::Text, 0, "#", 50, "TextureListIndex", ColumnVisibility::Hidden);
	col_name_  = addColumn(ColumnType::IconAndText, 1, "Name", 130, "TextureListName", ColumnVisibility::AlwaysVisible);
	col_size_  = addColumn(ColumnType::Text, 2, "Size", 70, "TextureListSize", ColumnVisibility::Visible);
	col_patches_ = addColumn(ColumnType::Text, 4, "Patches", 70, "TextureListPatches", ColumnVisibility::Hidden);
	col_type_    = addColumn(ColumnType::Text, 3, "Type", 180, "TextureListType", ColumnVisibility::Hidden);
	SetExpanderColumn(col_name_);

	// Load width/visibility state
	loadColumnState(archive);

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);

	// Load sorting config
	loadSortState(archive);
}

// -----------------------------------------------------------------------------
// Returns the archive associated with the texture editor, used as a key for
// persisted column state
// -----------------------------------------------------------------------------
const Archive* TextureTreeView::stateArchive() const
{
	return editor_ ? editor_->archive() : nullptr;
}
