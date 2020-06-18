
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveEntryTree.cpp
// Description: A wxDataViewCtrl-based widget that shows all entries in an
//              archive via the ArchiveViewModel dataview model. The model will
//              automatically keep in-sync with the associated Archive.
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
#include "ArchiveEntryTree.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "General/ColourConfiguration.h"
#include "General/SAction.h"
#include "General/UndoRedo.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include <wx/headerctrl.h>

using namespace slade;
using namespace ui;

// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
wxColour col_text_modified(0, 0, 0, 0);
wxColour col_text_new(0, 0, 0, 0);
wxColour col_text_locked(0, 0, 0, 0);
} // namespace slade::ui

CVAR(Int, elist_colsize_name_tree, 150, CVar::Save)
CVAR(Int, elist_colsize_name_list, 150, CVar::Save)
CVAR(Int, elist_colsize_size, 80, CVar::Save)
CVAR(Int, elist_colsize_type, 150, CVar::Save)
CVAR(Int, elist_colsize_index, 50, CVar::Save)
#ifdef __WXGTK__
// Disable by default in GTK because double-click seems to trigger it, which interferes
// with double-click to expand folders or open entries
CVAR(Bool, elist_rename_inplace, false, CVar::Save)
#else
CVAR(Bool, elist_rename_inplace, true, CVar::Save)
#endif


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, elist_icon_size)
EXTERN_CVAR(Int, elist_icon_padding)
EXTERN_CVAR(Bool, elist_filter_dirs)
EXTERN_CVAR(Bool, elist_colsize_show)
EXTERN_CVAR(Bool, elist_coltype_show)
EXTERN_CVAR(Bool, elist_colindex_show)
EXTERN_CVAR(Bool, list_font_monospace)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
bool archiveSupportsDirs(Archive* archive)
{
	if (archive)
		return archive->formatDesc().supports_dirs;

	return false;
}
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// ArchiveViewModel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Associates [archive] with this model, connecting to its signals and
// populating the root node with the archive's root directory
// -----------------------------------------------------------------------------
void ArchiveViewModel::openArchive(shared_ptr<Archive> archive, UndoManager* undo_manager)
{
	archive_      = archive;
	undo_manager_ = undo_manager;

	// Add root items
	wxDataViewItemArray items;
	getDirChildItems(items, *archive->rootDir());
	ItemsAdded({}, items);


	// --- Connect to Archive/ArchiveManager signals ---

	// Entry added
	connections_ += archive->signals().entry_added.connect([this](Archive& archive, ArchiveEntry& entry) {
		ItemAdded(createItemForDirectory(*entry.parentDir()), wxDataViewItem(&entry));
	});

	// Entry removed
	connections_ += archive->signals().entry_removed.connect(
		[this](Archive& archive, ArchiveDir& dir, ArchiveEntry& entry) {
			ItemDeleted(createItemForDirectory(dir), wxDataViewItem(&entry));
		});

	// Entry modified
	connections_ += archive->signals().entry_state_changed.connect(
		[this](Archive& archive, ArchiveEntry& entry) { ItemChanged(wxDataViewItem(&entry)); });

	// Dir added
	connections_ += archive->signals().dir_added.connect([this](Archive& archive, ArchiveDir& dir) {
		ItemAdded(createItemForDirectory(*dir.parent().get()), wxDataViewItem(dir.dirEntry()));
	});

	// Dir removed
	connections_ += archive->signals().dir_removed.connect(
		[this](Archive& archive, ArchiveDir& parent, ArchiveDir& dir) {
			ItemDeleted(createItemForDirectory(parent), wxDataViewItem(dir.dirEntry()));
		});

	// Entries reordered within dir
	connections_ += archive->signals().entries_swapped.connect(
		[this](Archive& archive, ArchiveDir& dir, unsigned index1, unsigned index2) {
			ItemChanged(wxDataViewItem(dir.entryAt(index1)));
			ItemChanged(wxDataViewItem(dir.entryAt(index2)));
		});

	// Bookmark added
	connections_ += app::archiveManager().signals().bookmark_added.connect(
		[this](ArchiveEntry* entry) { ItemChanged(wxDataViewItem(entry)); });

	// Bookmark(s) removed
	connections_ += app::archiveManager().signals().bookmarks_removed.connect(
		[this](const vector<ArchiveEntry*>& removed) {
			wxDataViewItemArray items;
			for (auto* entry : removed)
				if (entry)
					items.push_back(wxDataViewItem{ entry });
			ItemsChanged(items);
		});
}

// -----------------------------------------------------------------------------
// Sets the current filter options for the model
// -----------------------------------------------------------------------------
void ArchiveViewModel::setFilter(string_view name, string_view category)
{
	// Check any change is required
	if (name.empty() && filter_name_.empty() && filter_category_ == category)
		return;

	filter_category_ = category;

	// Process filter string
	filter_name_.clear();
	if (!name.empty())
	{
		auto filter_parts = strutil::splitV(name, ',');
		for (auto p : filter_parts)
		{
			auto filter_part = strutil::trim(p);
			if (filter_part.empty())
				continue;

			strutil::upperIP(filter_part);
			filter_part += '*';
			filter_name_.push_back(filter_part);
		}
	}

	if (auto* archive = archive_.lock().get())
	{
		wxDataViewItemArray items;
		const auto&         dir = *archive->rootDir();

		// Remove root items (unfiltered)
		getDirChildItems(items, dir, false);
		ItemsDeleted({}, items);

		// Re-Add root items (filtered)
		items.clear();
		getDirChildItems(items, dir);
		ItemsAdded({}, items);
	}
}

// -----------------------------------------------------------------------------
// Returns the wxVariant type for the column [col]
// -----------------------------------------------------------------------------
wxString ArchiveViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0: return "wxDataViewIconText";
	case 1: return "string";
	case 2: return "string";
	case 3: return "string"; // Index is a number technically, but will need to be blank for folders
	default: return "string";
	}
}

// -----------------------------------------------------------------------------
// Sets [variant] the the value of [item] in the column [col]
// -----------------------------------------------------------------------------
void ArchiveViewModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
	// Check the item contains an entry
	auto* entry = static_cast<ArchiveEntry*>(item.GetID());
	if (!entry)
		return;

	// Name column
	if (col == 0)
	{
		auto pad      = Point2i{ 1, elist_icon_padding };
		auto icon_bmp = elist_icon_padding > 0 ?
							icons::getPaddedIcon(icons::Type::Entry, entry->type()->icon(), elist_icon_size, pad) :
							icons::getIcon(icons::Type::Entry, entry->type()->icon(), elist_icon_size);

		wxIcon icon;
		icon.CopyFromBitmap(icon_bmp);
		wxString name = entry->name();
		if (entry->state() != ArchiveEntry::State::Unmodified)
			variant << wxDataViewIconText(entry->name() + " *", icon);
		else
			variant << wxDataViewIconText(entry->name(), icon);
	}

	// Size column
	else if (col == 1)
		variant = entry->sizeString();

	// Type column
	else if (col == 2)
		variant = entry->typeString();

	// Index column
	else if (col == 3)
		variant = (entry->type() == EntryType::folderType()) ? "" : fmt::format("{}", entry->index());

	// Invalid
	else
		variant = "Invalid Column";
}

// -----------------------------------------------------------------------------
// Sets the cell attributes [attr] for [item] in column [col]
// -----------------------------------------------------------------------------
bool ArchiveViewModel::GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const
{
	auto* entry = static_cast<ArchiveEntry*>(item.GetID());
	if (!entry)
		return false;

	bool has_attr = false;

	// Bookmarked (bold name)
	if (col == 0 && app::archiveManager().isBookmarked(entry))
	{
		attr.SetBold(true);
		has_attr = true;
	}

	// Status colour
	if (entry->isLocked() || entry->state() != ArchiveEntry::State::Unmodified)
	{
		// Init precalculated status text colours if necessary
		if (col_text_modified.Alpha() == 0)
		{
			auto col_modified = ColRGBA(0, 85, 255);
			auto col_new      = ColRGBA(0, 255, 0);
			auto col_locked   = ColRGBA(255, 0, 0);
			auto col_text     = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
			auto intensity    = 0.65;

			col_text_modified.Set(
				(col_modified.r * intensity) + (col_text.Red() * (1.0 - intensity)),
				(col_modified.g * intensity) + (col_text.Green() * (1.0 - intensity)),
				(col_modified.b * intensity) + (col_text.Blue() * (1.0 - intensity)),
				255);

			col_text_new.Set(
				(col_new.r * intensity) + (col_text.Red() * (1.0 - intensity)),
				(col_new.g * intensity) + (col_text.Green() * (1.0 - intensity)),
				(col_new.b * intensity) + (col_text.Blue() * (1.0 - intensity)),
				255);

			col_text_locked.Set(
				(col_locked.r * intensity) + (col_text.Red() * (1.0 - intensity)),
				(col_locked.g * intensity) + (col_text.Green() * (1.0 - intensity)),
				(col_locked.b * intensity) + (col_text.Blue() * (1.0 - intensity)),
				255);
		}

		if (entry->isLocked())
			attr.SetColour(col_text_locked);
		else
			attr.SetColour(entry->state() == ArchiveEntry::State::New ? col_text_new : col_text_modified);

		has_attr = true;
	}

	return has_attr;
}

// -----------------------------------------------------------------------------
// Sets the value of [item] on column [col] to the value in [variant]
// -----------------------------------------------------------------------------
bool ArchiveViewModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col)
{
	// Get+check archive and entry
	auto* archive = archive_.lock().get();
	if (!archive)
		return false;
	auto* entry = static_cast<ArchiveEntry*>(item.GetID());
	if (!entry)
		return false;

	// Name column
	if (col == 0)
	{
		bool               ok = false;
		wxDataViewIconText value;
		value << variant;
		auto new_name = value.GetText();
		if (new_name.EndsWith(" *"))
			new_name.RemoveLast(2);

		// Directory
		if (entry->type() == EntryType::folderType())
		{
			if (undo_manager_)
				undo_manager_->beginRecord("Rename Directory");

			// Rename the entry
			auto dir = ArchiveDir::findDirByDirEntry(archive->rootDir(), *entry);
			if (dir)
				ok = archive->renameDir(dir.get(), wxutil::strToView(new_name));
			else
				ok = false;
		}

		// Entry
		else
		{
			if (undo_manager_)
				undo_manager_->beginRecord("Rename Entry");

			// Rename the entry
			if (entry->parent())
				ok = entry->parent()->renameEntry(entry, new_name.ToStdString());
			else
				ok = entry->rename(new_name.ToStdString());
		}

		if (undo_manager_ && undo_manager_->currentlyRecording())
			undo_manager_->endRecord(ok);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns the parent item of [item]
// -----------------------------------------------------------------------------
wxDataViewItem ArchiveViewModel::GetParent(const wxDataViewItem& item) const
{
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
		if (auto* pdir = entry->parentDir())
		{
			// The root dir of the archive is the 'invalid' item (ie. hidden root node)
			if (pdir == archive_.lock()->rootDir().get())
				return {};
			else
				return wxDataViewItem{ pdir->dirEntry() };
		}

	return {};
}

// -----------------------------------------------------------------------------
// Returns true if [item] is a container (ie. has child items)
// -----------------------------------------------------------------------------
bool ArchiveViewModel::IsContainer(const wxDataViewItem& item) const
{
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
	{
		// Not a folder
		if (entry->type() != EntryType::folderType())
			return false;

		// Empty folder
		else if (auto* archive = archive_.lock().get())
		{
			if (auto* dir = archive->dirAtPath(entry->path(true)))
				if (dir->entries().size() == 0 && dir->subdirs().size() == 0)
					return false;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Populates [children] with the child items of [item], returning the number
// of children added
// -----------------------------------------------------------------------------
unsigned int ArchiveViewModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	auto* archive = archive_.lock().get();
	if (!archive)
		return 0;

	// Check if the item is a directory
	ArchiveDir* dir;
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
	{
		if (entry->type() == EntryType::folderType())
			dir = archive->dirAtPath(entry->path(true));
		else
			return 0; // Non-directory entry, no children
	}
	else
		dir = archive->rootDir().get(); // 'Invalid' item is the archive root dir

	// Get items for directory subdirs + entries
	getDirChildItems(children, *dir);

	return children.size();
}

// -----------------------------------------------------------------------------
// Returns true if this model is a list (expanders will be hidden for a list
// model)
// -----------------------------------------------------------------------------
bool ArchiveViewModel::IsListModel() const
{
	// Show as a list (no spacing for expanders) if the archive doesn't support directories
	if (auto* archive = archive_.lock().get())
		return !archive->formatDesc().supports_dirs;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the comparison value between [item1] and [item2] when sorting by
// [column]
// -----------------------------------------------------------------------------
int ArchiveViewModel::Compare(
	const wxDataViewItem& item1,
	const wxDataViewItem& item2,
	unsigned int          column,
	bool                  ascending) const
{
	auto* e1       = static_cast<ArchiveEntry*>(item1.GetID());
	auto* e1_type  = e1->type();
	auto* e2       = static_cast<ArchiveEntry*>(item2.GetID());
	auto* e2_type  = e2->type();
	auto* t_folder = EntryType::folderType();

	// Folder <-> Entry (always show folders first)
	if (e1_type == t_folder && e2_type != t_folder)
		return -1;
	else if (e1_type != t_folder && e2_type == t_folder)
		return 1;

	// Folder <-> Folder (always sort alphabetically for now)
	else if (e1_type == t_folder && e2_type == t_folder)
	{
		if (column == 0 && !ascending)
			return e2->upperName().compare(e1->upperName());
		else
			return e1->upperName().compare(e2->upperName());
	}

	// Entry <-> Entry
	else
	{
		int cmpval = 0;

		// Name column (order by name only)
		if (column == 0)
			cmpval = e1->upperName().compare(e2->upperName());

		// Size column (order by size -> name)
		else if (column == 1)
		{
			if (e1->size() > e2->size())
				cmpval = 1;
			else if (e1->size() < e2->size())
				cmpval = -1;
			else
				cmpval = e1->upperName().compare(e2->upperName());
		}

		// Type column (order by type name -> name)
		else if (column == 2)
		{
			cmpval = e1_type->name().compare(e2_type->name());
			if (cmpval == 0)
				cmpval = e1->upperName().compare(e2->upperName());
		}

		// Default
		else
		{
			// Directory archives default to alphabetical order
			if (auto archive = archive_.lock(); archive->formatId() == "folder")
				cmpval = e1->upperName().compare(e2->upperName());

			// Everything else defaults to index order
			else
				cmpval = e1->index() > e2->index() ? 1 : -1;
		}

		return ascending ? cmpval : -cmpval;
	}
}

// -----------------------------------------------------------------------------
// Returns a wxDataViewItem representing [dir]
// -----------------------------------------------------------------------------
wxDataViewItem ArchiveViewModel::createItemForDirectory(const ArchiveDir& dir) const
{
	if (auto archive = archive_.lock())
	{
		if (&dir == archive->rootDir().get())
			return {};
		else
			return wxDataViewItem{ dir.dirEntry() };
	}

	return {};
}

// -----------------------------------------------------------------------------
// Returns true if [entry] matches the current filter
// -----------------------------------------------------------------------------
bool ArchiveViewModel::matchesFilter(const ArchiveEntry& entry) const
{
	// Check for name match if needed
	if (!filter_name_.empty())
	{
		for (const auto& f : filter_name_)
			if (strutil::matches(entry.upperName(), f))
				return true;

		return false;
	}

	// Check for category match if needed
	if (!filter_category_.empty() && entry.type() != EntryType::folderType())
		if (!strutil::equalCI(entry.type()->category(), filter_category_))
			return false;

	return true;
}

// -----------------------------------------------------------------------------
// Populates [items] with all child entries/subrirs of [dir].
// If [filtered] is true, only adds children matching the current filter
// -----------------------------------------------------------------------------
void ArchiveViewModel::getDirChildItems(wxDataViewItemArray& items, const ArchiveDir& dir, bool filtered) const
{
	if (filtered)
	{
		for (const auto& subdir : dir.subdirs())
			if (!elist_filter_dirs || matchesFilter(*subdir->dirEntry()))
				items.push_back(wxDataViewItem{ subdir->dirEntry() });
		for (const auto& entry : dir.entries())
			if (matchesFilter(*entry))
				items.push_back(wxDataViewItem{ entry.get() });
	}
	else
	{
		for (const auto& subdir : dir.subdirs())
			items.push_back(wxDataViewItem{ subdir->dirEntry() });
		for (const auto& entry : dir.entries())
			items.push_back(wxDataViewItem{ entry.get() });
	}
}


// -----------------------------------------------------------------------------
//
// ArchiveEntryTree Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveEntryTree class constructor
// -----------------------------------------------------------------------------
ArchiveEntryTree::ArchiveEntryTree(wxWindow* parent, shared_ptr<Archive> archive, UndoManager* undo_manager) :
	wxDataViewCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE), archive_{ archive }
{
	// Init settings
	SetRowHeight(ui::scalePx(elist_icon_size + (elist_icon_padding * 2) + 2));
	if (list_font_monospace)
		SetFont(wxutil::monospaceFont(GetFont()));

	// Create & associate model
	model_ = new ArchiveViewModel();
	model_->openArchive(archive, undo_manager);
	AssociateModel(model_);
	model_->DecRef();

	// Add Columns
	setupColumns();

	// --- Bind Events ---

	// Expand/Contract folders if activated
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& e) {
		if (auto* entry = static_cast<ArchiveEntry*>(e.GetItem().GetID());
			entry && entry->type() == EntryType::folderType())
		{
			if (IsExpanded(e.GetItem()))
				Collapse(e.GetItem());
			else
				Expand(e.GetItem());
		}
		else
			e.Skip();
	});

	// Update column width cvars when we can
	Bind(wxEVT_IDLE, [this](wxIdleEvent&) { saveColumnWidths(); });

	// Header right click
	Bind(wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, [this](wxDataViewEvent& e) {
		// Popup context menu
		wxMenu context;
		context.Append(0, "Reset Sorting");
		context.AppendSeparator();
		context.AppendCheckItem(1, "Index", "Show the Index column")->Check(elist_colindex_show);
		context.AppendCheckItem(2, "Size", "Show the Size column")->Check(elist_colsize_show);
		context.AppendCheckItem(3, "Type", "Show the Type column")->Check(elist_coltype_show);
		PopupMenu(&context);
		e.Skip();
	});

	// Header context menu
	Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
		if (e.GetId() == 0)
		{
			// Reset Sorting
			col_name_->UnsetAsSortKey();
			col_size_->UnsetAsSortKey();
			col_type_->UnsetAsSortKey();
#ifdef __WXGTK__
			col_index_->SetSortOrder(true);
#else
			col_index_->UnsetAsSortKey();
#endif
			model_->Resort();
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_COLUMN_SORTED);
			ProcessWindowEvent(de);
		}
		else if (e.GetId() == 1)
		{
			// Toggle index column
			elist_colindex_show = !elist_colindex_show;
			col_index_->SetHidden(!elist_colindex_show);
			updateColumnWidths();
		}
		else if (e.GetId() == 2)
		{
			// Toggle size column
			elist_colsize_show = !elist_colsize_show;
			col_size_->SetHidden(!elist_colsize_show);
			updateColumnWidths();
		}
		else if (e.GetId() == 3)
		{
			// Toggle type column
			elist_coltype_show = !elist_coltype_show;
			col_type_->SetHidden(!elist_coltype_show);
			updateColumnWidths();
		}
		else
			e.Skip();
	});
}

// -----------------------------------------------------------------------------
// Returns the ArchiveDir that [item] represents, or nullptr if it isn't a valid
// directory item
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::dirForDirItem(const wxDataViewItem& item) const
{
	if (auto archive = archive_.lock())
	{
		auto* entry = static_cast<ArchiveEntry*>(item.GetID());
		return ArchiveDir::findDirByDirEntry(archive_.lock()->rootDir(), *entry).get();
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns true if the list currently has 'default' sorting (by entry index,
// ascending)
// -----------------------------------------------------------------------------
bool ArchiveEntryTree::isDefaultSorted() const
{
	auto* sort_col = GetSortingColumn();
	if (sort_col && sort_col != col_index_)
		return false;
	else if (sort_col == col_index_)
		return col_index_->IsSortOrderAscending();
	else
		return true;
}

// -----------------------------------------------------------------------------
// Returns all currently selected entries.
// If [include_dirs] is true, it will also return the entries for any selected
// directories
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchiveEntryTree::selectedEntries(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	vector<ArchiveEntry*> entries;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Add (non-folder) entries from selected items
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				entries.push_back(entry);

	return entries;
}

// -----------------------------------------------------------------------------
// Returns the first selected entry, or nullptr if none selected
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntryTree::firstSelectedEntry(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return nullptr;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first (non-folder) entry in selected items
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				return entry;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the last selected entry, or nullptr if none selected
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntryTree::lastSelectedEntry(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return nullptr;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find last (non-folder) entry in selected items
	for (int i = selection.size() - 1; i >= 0; --i)
		if (auto* entry = static_cast<ArchiveEntry*>(selection[i].GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				return entry;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns all currently selected directories
// -----------------------------------------------------------------------------
vector<ArchiveDir*> ArchiveEntryTree::selectedDirectories() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	vector<ArchiveDir*> dirs;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Add dirs from selected items
	auto dir_root = archive->rootDir();
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					dirs.push_back(dir.get());

	return dirs;
}

// -----------------------------------------------------------------------------
// Returns the first selected directory, or nullptr if none selected
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::firstSelectedDirectory() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	auto dir_root = archive->rootDir();
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					return dir.get();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the last selected directory, or nullptr if none selected
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::lastSelectedDirectory() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	auto dir_root = archive->rootDir();
	for (int i = selection.size() - 1; i >= 0; --i)
		if (auto* entry = static_cast<ArchiveEntry*>(selection[i].GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					return dir.get();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the first selected item, or an invalid item if none selected
// -----------------------------------------------------------------------------
wxDataViewItem ArchiveEntryTree::firstSelectedItem() const
{
	wxDataViewItemArray selection;
	if (GetSelections(selection) > 0)
		return selection[0];

	return {};
}

// -----------------------------------------------------------------------------
// Returns the last selected item, or an invalid item if none selected
// -----------------------------------------------------------------------------
wxDataViewItem ArchiveEntryTree::lastSelectedItem() const
{
	wxDataViewItemArray selection;
	if (GetSelections(selection) > 0)
		return selection.back();

	return {};
}

// -----------------------------------------------------------------------------
// Returns the 'current' selected directory, based on the last selected item.
// If the item is a directory, returns that, otherwise returns the entry's
// parent directory. If nothing is selected returns the archive root dir
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::currentSelectedDir() const
{
	auto* archive = archive_.lock().get();
	if (!archive)
		return nullptr;

	auto item = lastSelectedItem();
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
	{
		if (entry->type() == EntryType::folderType())
			return dirForDirItem(item);
		else
			return entry->parentDir();
	}

	return archive->rootDir().get();
}

// -----------------------------------------------------------------------------
// Returns the directory containing all currently selected entries, or nullptr
// if the selection isn't all within one directory
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::selectedEntriesDir() const
{
	wxDataViewItemArray selection;
	GetSelections(selection);

	ArchiveDir* dir = nullptr;
	for (const auto& item : selection)
	{
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
		{
			// Folder selected, return nullptr
			if (entry->type() == EntryType::folderType())
				return nullptr;

			if (!dir)
				dir = entry->parentDir();

			// Entry is in a different dir than the previous, return nullptr
			else if (dir != entry->parentDir())
				return nullptr;
		}
	}

	return dir;
}

// -----------------------------------------------------------------------------
// Set the filter options on the model
// -----------------------------------------------------------------------------
void ArchiveEntryTree::setFilter(string_view name, string_view category)
{
	Freeze();
	model_->setFilter(name, category);
	Thaw();
}

// -----------------------------------------------------------------------------'
// Collapses all currently expanded directory items
// -----------------------------------------------------------------------------
void ArchiveEntryTree::collapseAll(const ArchiveDir& dir_start)
{
	for (const auto& subdir : dir_start.subdirs())
		collapseAll(*subdir);

	Collapse(wxDataViewItem(dir_start.dirEntry()));
}

// -----------------------------------------------------------------------------
// Creates and sets up the tree columns
// -----------------------------------------------------------------------------
void ArchiveEntryTree::setupColumns()
{
	auto archive = archive_.lock();
	if (!archive)
		return;

	auto colstyle_visible = wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE;
	auto colstyle_hidden  = wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_HIDDEN;

	// Add Columns
	col_index_ = AppendTextColumn(
		"#",
		3,
		wxDATAVIEW_CELL_INERT,
		elist_colsize_index,
		wxALIGN_NOT,
		elist_colindex_show ? colstyle_visible : colstyle_hidden);
	col_name_ = AppendIconTextColumn(
		"Name",
		0,
		elist_rename_inplace ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT,
		archiveSupportsDirs(archive.get()) ? elist_colsize_name_tree : elist_colsize_name_list,
		wxALIGN_NOT,
		colstyle_visible);
	col_size_ = AppendTextColumn(
		"Size",
		1,
		wxDATAVIEW_CELL_INERT,
		elist_colsize_size,
		wxALIGN_NOT,
		elist_colsize_show ? colstyle_visible : colstyle_hidden);
	col_type_ = AppendTextColumn(
		"Type",
		2,
		wxDATAVIEW_CELL_INERT,
		elist_colsize_type,
		wxALIGN_NOT,
		elist_coltype_show ? colstyle_visible : colstyle_hidden);
	SetExpanderColumn(col_name_);

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);
}

// -----------------------------------------------------------------------------
// Saves the current column widths to their respective cvars
// -----------------------------------------------------------------------------
void ArchiveEntryTree::saveColumnWidths()
{
	// Get the last visible column (we don't want to save the width of this column since it stretches)
	wxDataViewColumn* last_col = nullptr;
	for (auto i = GetColumnCount() - 1; i >= 0; --i)
		if (!GetColumn(i)->IsHidden())
		{
			last_col = GetColumn(i);
			break;
		}

	if (last_col != col_name_)
	{
		if (archiveSupportsDirs(archive_.lock().get()))
			elist_colsize_name_tree = col_name_->GetWidth();
		else
			elist_colsize_name_list = col_name_->GetWidth();
	}

	if (last_col != col_size_ && !col_size_->IsHidden())
		elist_colsize_size = col_size_->GetWidth();

	if (last_col != col_type_ && !col_type_->IsHidden())
		elist_colsize_type = col_type_->GetWidth();

	if (!col_index_->IsHidden())
		elist_colsize_index = col_index_->GetWidth();
}

// -----------------------------------------------------------------------------
// Updates the currently visible columns' widths from their respective cvars
// -----------------------------------------------------------------------------
void ArchiveEntryTree::updateColumnWidths()
{
	auto archive = archive_.lock();
	if (!archive)
		return;

	// Get the last visible column (we don't want to save the width of this column since it stretches)
	wxDataViewColumn* last_col = nullptr;
	for (auto i = GetColumnCount() - 1; i >= 0; --i)
		if (!GetColumn(i)->IsHidden())
		{
			last_col = GetColumn(i);
			break;
		}

	Freeze();
	col_index_->SetWidth(elist_colsize_index);
	col_name_->SetWidth(archiveSupportsDirs(archive.get()) ? elist_colsize_name_tree : elist_colsize_name_list);
	col_size_->SetWidth(col_size_ == last_col ? 0 : elist_colsize_size);
	col_type_->SetWidth(col_type_ == last_col ? 0 : elist_colsize_type);
	Thaw();
}
