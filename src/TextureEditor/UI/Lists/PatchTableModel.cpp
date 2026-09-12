
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTableModel.cpp
// Description: PatchTableModel class, a wxDataViewIndexListModel for
//              displaying the contents of a patch table in a PatchTableList
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
#include "PatchTableModel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// PatchTableModel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Associates the model with [patch_table] and loads its patches
// -----------------------------------------------------------------------------
void PatchTableModel::open(PatchTable& patch_table)
{
	patch_table_ = &patch_table;

	refresh();

	// Patch table modified (patch added/removed/renamed etc.)
	connections_ += patch_table_->signals().modified.connect([this] { refresh(); });

	// Archives added/closed/modified can change patch usage & 'in archive' info
	auto& am_signals = app::archiveManager().signals();
	connections_ += am_signals.archive_added.connect([this](unsigned) { refresh(); });
	connections_ += am_signals.archive_closed.connect([this](unsigned) { refresh(); });
	connections_ += am_signals.archive_modified.connect([this](unsigned, bool) { refresh(); });
}

// -----------------------------------------------------------------------------
// Refreshes the model from the current state of the patch table
// -----------------------------------------------------------------------------
void PatchTableModel::refresh()
{
	Reset(patch_table_ ? static_cast<unsigned int>(patch_table_->nPatches()) : 0);
}

// -----------------------------------------------------------------------------
// Sets [variant] to the value at [row],[col]
// -----------------------------------------------------------------------------
void PatchTableModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (!patch_table_ || row >= patch_table_->nPatches())
		return;

	auto& patch = patch_table_->patch(row);

	switch (static_cast<Column>(col))
	{
	case Column::Index:  variant = WX_FMT("{:04d}", row); break;
	case Column::Name:   variant = wxString::FromUTF8(patch.name); break;
	case Column::UsedIn: variant = WX_FMT("{}", patch.used_in.size()); break;
	case Column::Archive:
		if (auto entry = patch_table_->patchEntry(row))
			variant = wxString::FromUTF8(entry->parent()->filename(false));
		else
			variant = wxS("(!) NOT FOUND");
		break;
	default: break;
	}
}

// -----------------------------------------------------------------------------
// Sets the value at [row],[col] to the value in [variant]
// (currently always returns false as the model is read-only for now)
// -----------------------------------------------------------------------------
bool PatchTableModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	return false;
}

// -----------------------------------------------------------------------------
// Compares [item1] and [item2] for sorting by [column]
// -----------------------------------------------------------------------------
int PatchTableModel::Compare(
	const wxDataViewItem& item1,
	const wxDataViewItem& item2,
	unsigned int          column,
	bool                  ascending) const
{
	if (!patch_table_)
		return 0;

	auto row1 = GetRow(item1);
	auto row2 = GetRow(item2);

	int result = 0;
	switch (static_cast<Column>(column))
	{
	case Column::Name:
		result = strutil::lower(patch_table_->patch(row1).name).compare(strutil::lower(patch_table_->patch(row2).name));
		break;

	case Column::UsedIn:
	{
		auto n1 = patch_table_->patch(row1).used_in.size();
		auto n2 = patch_table_->patch(row2).used_in.size();
		result  = n1 == n2 ? 0 : (n1 < n2 ? -1 : 1);
		break;
	}

	case Column::Archive:
	{
		auto entry1 = patch_table_->patchEntry(row1);
		auto entry2 = patch_table_->patchEntry(row2);
		auto name1  = entry1 ? entry1->parent()->filename(false) : string{};
		auto name2  = entry2 ? entry2->parent()->filename(false) : string{};
		result      = strutil::lower(name1).compare(strutil::lower(name2));
		break;
	}

	default: break;
	}

	// Tie-break by patch table index
	if (result == 0)
		result = static_cast<int>(row1) - static_cast<int>(row2);

	return ascending ? result : -result;
}
