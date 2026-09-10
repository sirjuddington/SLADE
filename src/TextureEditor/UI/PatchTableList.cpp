
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTableList.cpp
// Description: PatchTableList class, a wxDataViewCtrl that displays the
//              contents of a patch table
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
#include "PatchTableList.h"
#include "Graphics/CTexture/PatchTable.h"
#include "PatchTableModel.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// PatchTableList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchTableList class constructor
// -----------------------------------------------------------------------------
PatchTableList::PatchTableList(wxWindow* parent, PatchTable* patch_table) :
	SDataViewCtrl(parent, wxDV_MULTIPLE),
	patch_table_{ patch_table }
{
	// Create & associate model
	wxObjectDataPtr model(new PatchTableModel());
	model->open(*patch_table_);
	AssociateModel(model.get());

	setupColumns();

	enableHeaderContextMenu();
}

// -----------------------------------------------------------------------------
// Returns the patch table index of the last selected item, or -1 if nothing
// is selected
// -----------------------------------------------------------------------------
int PatchTableList::selectedPatchIndex() const
{
	wxDataViewItemArray selection;
	GetSelections(selection);
	if (selection.IsEmpty())
		return -1;

	auto model = dynamic_cast<const PatchTableModel*>(GetModel());
	return model ? static_cast<int>(model->GetRow(selection.Last())) : -1;
}

// -----------------------------------------------------------------------------
// Returns the patch table indices of all currently selected items
// -----------------------------------------------------------------------------
vector<int> PatchTableList::selectedPatchIndices() const
{
	vector<int> indices;

	auto model = dynamic_cast<const PatchTableModel*>(GetModel());
	if (!model)
		return indices;

	wxDataViewItemArray selection;
	GetSelections(selection);
	for (const auto& item : selection)
		indices.push_back(static_cast<int>(model->GetRow(item)));

	return indices;
}

// -----------------------------------------------------------------------------
// Selects (or deselects) the item for patch table index [index]
// -----------------------------------------------------------------------------
void PatchTableList::selectPatch(int index, bool select, bool scroll_to)
{
	if (index < 0)
		return;

	auto model = dynamic_cast<const PatchTableModel*>(GetModel());
	if (!model)
		return;

	if (auto item = model->GetItem(static_cast<unsigned int>(index)); item.IsOk())
	{
		if (select)
			Select(item);
		else
			Unselect(item);
		if (scroll_to)
			EnsureVisible(item);
	}
}

// -----------------------------------------------------------------------------
// Returns the archive associated with the patch table, used for persisted
// column state
// -----------------------------------------------------------------------------
const Archive* PatchTableList::stateArchive() const
{
	return patch_table_ ? patch_table_->parent() : nullptr;
}

// -----------------------------------------------------------------------------
// Sets up the list columns
// -----------------------------------------------------------------------------
void PatchTableList::setupColumns()
{
	// Add columns
	col_index_   = addColumn(ColumnType::Text, 0, "#", 50, "PatchTableIndex");
	col_name_    = addColumn(ColumnType::Text, 1, "Name", 100, "PatchTableName", ColumnVisibility::AlwaysVisible);
	col_used_in_ = addColumn(ColumnType::Text, 2, "# Uses", 80, "PatchTableUsedIn");
	col_archive_ = addColumn(ColumnType::Text, 3, "In Archive", 130, "PatchTableArchive");

	// Load width/visibility state
	loadColumnState();

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);
}
