
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormatHandler.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "General/UndoRedo.h"
#include "Graphics/Icons.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
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
wxColour                                   col_text_modified(0, 0, 0, 0);
wxColour                                   col_text_new(0, 0, 0, 0);
wxColour                                   col_text_locked(0, 0, 0, 0);
std::unordered_map<string, wxBitmapBundle> icon_cache;
} // namespace slade::ui

#ifdef __WXGTK__
// Disable by default in GTK because double-click seems to trigger it, which interferes
// with double-click to expand folders or open entries
CVAR(Bool, elist_rename_inplace, false, CVar::Save)
#else
CVAR(Bool, elist_rename_inplace, true, CVar::Save)
#endif
CVAR(Bool, elist_colsize_show, true, CVar::Flag::Save)
CVAR(Bool, elist_coltype_show, true, CVar::Flag::Save)
CVAR(Bool, elist_colindex_show, false, CVar::Flag::Save)
CVAR(Bool, elist_filter_dirs, false, CVar::Flag::Save)
CVAR(Bool, elist_type_bgcol, false, CVar::Flag::Save)
CVAR(Float, elist_type_bgcol_intensity, 0.18, CVar::Flag::Save)
CVAR(Int, elist_icon_size, 16, CVar::Flag::Save)
CVAR(Int, elist_icon_padding, 1, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, elist_icon_size)
EXTERN_CVAR(Int, elist_icon_padding)
EXTERN_CVAR(Bool, elist_filter_dirs)
EXTERN_CVAR(Bool, list_font_monospace)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Get the width of the name column from saved UI state.
// If it is saved for [archive] use that, otherwise use the relevant global
// width (based on if the archive supports directories)
// -----------------------------------------------------------------------------
int nameColumnWidth(const Archive* archive)
{
	if (hasSavedState(ENTRYLIST_NAME_WIDTH, archive))
		return getStateInt(ENTRYLIST_NAME_WIDTH, archive);
	else
	{
		if (archive->formatInfo().supports_dirs)
			return getStateInt(ENTRYLIST_NAME_WIDTH_TREE);
		else
			return getStateInt(ENTRYLIST_NAME_WIDTH_LIST);
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// ArchivePathPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchivePathPanel constructor
// -----------------------------------------------------------------------------
ArchivePathPanel::ArchivePathPanel(wxWindow* parent) : SAuiToolBar{ parent }
{
	text_path_ = new wxStaticText(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_START | wxST_NO_AUTORESIZE);

	addAction("arch_elist_homedir");
	AddControl(text_path_)->SetProportion(1);
	addAction("arch_elist_updir");

	Realize();
}

// -----------------------------------------------------------------------------
// ArchivePathPanel destructor
// -----------------------------------------------------------------------------
ArchivePathPanel::~ArchivePathPanel() {}

// -----------------------------------------------------------------------------
// Sets the current path to where [dir] is in its Archive
// -----------------------------------------------------------------------------
void ArchivePathPanel::setCurrentPath(const ArchiveDir* dir)
{
	if (dir == nullptr)
	{
		text_path_->SetLabel(wxEmptyString);
		text_path_->UnsetToolTip();
		return;
	}

	auto is_root = dir == dir->archive()->rootDir().get();

	// Build path string
	auto path = dir->path();
	if (!is_root)
		path.pop_back();                  // Remove ending / if not root dir
	strutil::replaceIP(path, "/", " > "); // Replace / with >
	strutil::trimIP(path);

	// Update UI
	text_path_->SetLabel(wxString::FromUTF8(path));
	if (is_root)
		text_path_->UnsetToolTip();
	else
		text_path_->SetToolTip(wxString::FromUTF8(path));
	enableItem("arch_elist_updir", !is_root);
	Refresh();
}


// -----------------------------------------------------------------------------
//
// ArchiveViewModel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Associates [archive] with this model, connecting to its signals and
// populating the root node with the archive's root directory
// -----------------------------------------------------------------------------
void ArchiveViewModel::openArchive(const shared_ptr<Archive>& archive, UndoManager* undo_manager, bool force_list)
{
	archive_      = archive;
	root_dir_     = archive->rootDir();
	undo_manager_ = undo_manager;
	view_type_    = archive->formatInfo().supports_dirs && !force_list ? ViewType::Tree : ViewType::List;

	// Refresh (will load all items)
	Cleared();


	// --- Connect to Archive/ArchiveManager signals ---

	// ReSharper disable CppMemberFunctionMayBeConst
	// ReSharper disable CppParameterMayBeConstPtrOrRef

	// Entry added
	connections_ += archive->signals().entry_added.connect(
		[this](Archive& archive, ArchiveEntry& entry)
		{
			if (entryIsInList(entry))
			{
				if (view_type_ == ViewType::Tree)
					ItemAdded(createItemForDirectory(*entry.parentDir()), wxDataViewItem(&entry));
				else
					ItemAdded({}, wxDataViewItem(&entry));
			}
		});

	// Entry removed
	connections_ += archive->signals().entry_removed.connect(
		[this](Archive& archive, ArchiveDir& dir, ArchiveEntry& entry)
		{
			if (entryIsInList(entry, true, &archive, &dir))
			{
				if (view_type_ == ViewType::Tree)
					ItemDeleted(createItemForDirectory(dir), wxDataViewItem(&entry));
				else if (root_dir_.lock().get() == &dir)
					ItemDeleted({}, wxDataViewItem(&entry));
			}
		});

	// Entry modified
	connections_ += archive->signals().entry_state_changed.connect(
		[this](Archive& archive, ArchiveEntry& entry)
		{
			if (entryIsInList(entry))
				ItemChanged(wxDataViewItem(&entry));
		});

	// Dir added
	connections_ += archive->signals().dir_added.connect(
		[this](Archive& archive, ArchiveDir& dir)
		{
			if (dirIsInList(dir))
			{
				if (view_type_ == ViewType::Tree)
					ItemAdded(createItemForDirectory(*dir.parent()), wxDataViewItem(dir.dirEntry()));
				else
					ItemAdded({}, wxDataViewItem(dir.dirEntry()));
			}
		});

	// Dir removed
	connections_ += archive->signals().dir_removed.connect(
		[this](Archive& archive, ArchiveDir& parent, ArchiveDir& dir)
		{
			if (dirIsInList(dir, true, &parent))
			{
				if (view_type_ == ViewType::Tree)
					ItemDeleted(createItemForDirectory(parent), wxDataViewItem(dir.dirEntry()));
				else if (root_dir_.lock().get() == &parent)
					ItemDeleted({}, wxDataViewItem(dir.dirEntry()));
			}
		});

	// Entries reordered within dir
	connections_ += archive->signals().entries_swapped.connect(
		[this](Archive& archive, ArchiveDir& dir, unsigned index1, unsigned index2)
		{
			if (view_type_ == ViewType::List && root_dir_.lock().get() != &dir)
				return;

			ItemChanged(wxDataViewItem(dir.entryAt(index1)));
			ItemChanged(wxDataViewItem(dir.entryAt(index2)));
		});

	// Bookmark added
	connections_ += app::archiveManager().signals().bookmark_added.connect(
		[this](ArchiveEntry* entry)
		{
			if (entryIsInList(*entry))
				ItemChanged(wxDataViewItem(entry));
		});

	// Bookmark(s) removed
	connections_ += app::archiveManager().signals().bookmarks_removed.connect(
		[this](const vector<ArchiveEntry*>& removed)
		{
			wxDataViewItemArray items;
			for (auto* entry : removed)
				if (entry && entryIsInList(*entry))
					items.push_back(wxDataViewItem{ entry });
			ItemsChanged(items);
		});

	// ReSharper enable CppMemberFunctionMayBeConst
	// ReSharper enable CppParameterMayBeConstPtrOrRef
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
		for (const auto p : filter_parts)
		{
			auto filter_part = strutil::trim(p);
			if (filter_part.empty())
				continue;

			strutil::upperIP(filter_part);
			filter_part += '*';
			filter_name_.push_back(filter_part);
		}
	}

	// Fully refresh the list
	Cleared();
}

// -----------------------------------------------------------------------------
// Sets the root directory
// -----------------------------------------------------------------------------
void ArchiveViewModel::setRootDir(const shared_ptr<ArchiveDir>& dir)
{
	// Check given dir is part of archive
	if (dir->archive() != archive_.lock().get())
		return;

	auto* cur_root = root_dir_.lock().get();
	if (!cur_root)
		return;

	// Change root dir and refresh
	root_dir_ = dir;
	Cleared();

	if (path_panel_)
		path_panel_->setCurrentPath(dir.get());
}

// -----------------------------------------------------------------------------
// Sets the root directory from an item
// -----------------------------------------------------------------------------
void ArchiveViewModel::setRootDir(const wxDataViewItem& item)
{
	// Check item is valid
	auto* entry = static_cast<ArchiveEntry*>(item.GetID());
	if (!entry || entry->type() != EntryType::folderType())
		return;

	return setRootDir(ArchiveDir::getShared(dirForDirItem(item)));
}

// -----------------------------------------------------------------------------
// Sets the associated path wxTextCtrl
// -----------------------------------------------------------------------------
void ArchiveViewModel::setPathPanel(ArchivePathPanel* path_panel)
{
	path_panel_ = path_panel;

	auto dir = root_dir_.lock();
	if (path_panel_ && dir)
		path_panel_->setCurrentPath(dir.get());
}

// -----------------------------------------------------------------------------
// Returns the wxVariant type for the column [col]
// -----------------------------------------------------------------------------
wxString ArchiveViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:  return wxS("wxDataViewIconText");
	case 1:  return wxS("string");
	case 2:  return wxS("string");
	case 3:  return wxS("string"); // Index is a number technically, but will need to be blank for folders
	default: return wxS("string");
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
		// Find icon in cache
		if (icon_cache.find(entry->type()->icon()) == icon_cache.end())
		{
			// Not found, add to cache
			const auto pad    = Point2i{ 1, elist_icon_padding };
			const auto bundle = icons::getIcon(icons::Type::Entry, entry->type()->icon(), elist_icon_size, pad);
			icon_cache[entry->type()->icon()] = bundle;
		}

		auto name = wxString::FromUTF8(entry->name());
		if (modified_indicator_ && entry->state() != EntryState::Unmodified)
			variant << wxDataViewIconText(name + wxS(" *"), icon_cache[entry->type()->icon()]);
		else
			variant << wxDataViewIconText(name, icon_cache[entry->type()->icon()]);
	}

	// Size column
	else if (col == 1)
	{
		if (entry->type() == EntryType::folderType())
		{
			if (view_type_ == ViewType::List)
			{
				if (auto dir = ArchiveDir::findDirByDirEntry(root_dir_.lock(), *entry))
					variant = WX_FMT("{}", dir->numEntries(true));
				else
					variant = "";
			}
			else
				variant = "";
		}
		else
			variant = wxString::FromUTF8(entry->sizeString());
	}

	// Type column
	else if (col == 2)
		variant = wxString::FromUTF8(entry->type() == EntryType::folderType() ? "Folder" : entry->typeString());

	// Index column
	else if (col == 3)
		variant = wxString::FromUTF8(
			entry->type() == EntryType::folderType() ? " " : fmt::format("{}", entry->index()));

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
	if (entry->isLocked() || entry->state() != EntryState::Unmodified)
	{
		// Init precalculated status text colours if necessary
		if (col_text_modified.Alpha() == 0)
		{
			const auto     col_modified = ColRGBA(0, 85, 255);
			const auto     col_new      = ColRGBA(0, 255, 0);
			const auto     col_locked   = ColRGBA(255, 0, 0);
			const auto     col_text     = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
			constexpr auto intensity    = 0.65;

			col_text_modified.Set(
				static_cast<uint8_t>(col_modified.r * intensity + col_text.Red() * (1.0 - intensity)),
				static_cast<uint8_t>(col_modified.g * intensity + col_text.Green() * (1.0 - intensity)),
				static_cast<uint8_t>(col_modified.b * intensity + col_text.Blue() * (1.0 - intensity)),
				255);

			col_text_new.Set(
				static_cast<uint8_t>(col_new.r * intensity + col_text.Red() * (1.0 - intensity)),
				static_cast<uint8_t>(col_new.g * intensity + col_text.Green() * (1.0 - intensity)),
				static_cast<uint8_t>(col_new.b * intensity + col_text.Blue() * (1.0 - intensity)),
				255);

			col_text_locked.Set(
				static_cast<uint8_t>(col_locked.r * intensity + col_text.Red() * (1.0 - intensity)),
				static_cast<uint8_t>(col_locked.g * intensity + col_text.Green() * (1.0 - intensity)),
				static_cast<uint8_t>(col_locked.b * intensity + col_text.Blue() * (1.0 - intensity)),
				255);
		}

		if (entry->isLocked())
			attr.SetColour(col_text_locked);
		else
			attr.SetColour(entry->state() == EntryState::New ? col_text_new : col_text_modified);

		has_attr = true;
	}

	// Set background colour defined in entry type (if any)
	if (col == 0 || view_type_ == ViewType::List)
	{
		auto etype_colour = entry->type()->colour();
		if ((etype_colour.r != 255 || etype_colour.g != 255 || etype_colour.b != 255) && elist_type_bgcol)
		{
			ColRGBA bcol;
			auto    col_bg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);

			bcol.r = (etype_colour.r * elist_type_bgcol_intensity)
					 + (col_bg.Red() * (1.0 - elist_type_bgcol_intensity));
			bcol.g = (etype_colour.g * elist_type_bgcol_intensity)
					 + (col_bg.Green() * (1.0 - elist_type_bgcol_intensity));
			bcol.b = (etype_colour.b * elist_type_bgcol_intensity)
					 + (col_bg.Blue() * (1.0 - elist_type_bgcol_intensity));

			attr.SetBackgroundColour(bcol);
			has_attr = true;
		}
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
		bool               ok;
		wxDataViewIconText value;
		value << variant;
		auto new_name = value.GetText();
		if (new_name.EndsWith(wxS(" *")))
			new_name.RemoveLast(2);

		// Ignore if no change
		if (new_name.utf8_string() == entry->name())
			return true;

		// Directory
		if (entry->type() == EntryType::folderType())
		{
			if (undo_manager_)
				undo_manager_->beginRecord("Rename Directory");

			// Rename the entry
			const auto dir = ArchiveDir::findDirByDirEntry(archive->rootDir(), *entry);
			if (dir)
				ok = archive->renameDir(dir.get(), new_name.utf8_string());
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
				ok = entry->parent()->renameEntry(entry, new_name.utf8_string());
			else
				ok = entry->rename(new_name.utf8_string());
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
	if (view_type_ == ViewType::List)
		return {};

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
		// List view items are never containers
		if (view_type_ == ViewType::List)
			return false;

		// Not a folder
		if (entry->type() != EntryType::folderType())
			return false;

#ifdef __WXMSW__
		// Empty folder
		else if (auto* archive = archive_.lock().get())
		{
			if (auto* dir = archive->dirAtPath(entry->path(true)))
				if (dir->entries().empty() && dir->subdirs().empty())
					return false;
		}
#endif
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
		dir = root_dir_.lock().get(); // 'Invalid' item is the current root dir

	if (!dir)
		return 0;

	// Get items for directory subdirs + entries
	getDirChildItems(children, *dir);

	return static_cast<unsigned int>(children.size());
}

// -----------------------------------------------------------------------------
// Returns true if this model is a list (expanders will be hidden for a list
// model)
// -----------------------------------------------------------------------------
bool ArchiveViewModel::IsListModel() const
{
	return view_type_ == ViewType::List;
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
	if (!sort_enabled_)
		return 0;

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
		int cmpval;

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
			if (const auto archive = archive_.lock(); archive->format() == ArchiveFormat::Dir)
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
	if (const auto archive = archive_.lock())
	{
		if (&dir == root_dir_.lock().get())
			return {};
		else
			return wxDataViewItem{ dir.dirEntry() };
	}

	return {};
}

// -----------------------------------------------------------------------------
// Reloads the model with the specified [view_type] (if supported by the
// associated archive)
// -----------------------------------------------------------------------------
void ArchiveViewModel::reload(ViewType view_type)
{
	auto archive = archive_.lock().get();
	if (!archive)
		return;

	if (!archive->formatInfo().supports_dirs)
		view_type = ViewType::List;

	view_type_ = view_type;
	root_dir_  = archive->rootDir();
	Cleared();
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
// If [filter] is true, only adds children matching the current filter
// -----------------------------------------------------------------------------
void ArchiveViewModel::getDirChildItems(wxDataViewItemArray& items, const ArchiveDir& dir, bool filter) const
{
	if (filter)
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
// Returns true if [entry] is contained within the current list.
// If [filter] is true, also checks if the entry matches the current filter.
// [parent_archive] and [parent_dir] are used in the case where [entry] has
// been deleted and removed from its parent archive/dir
// -----------------------------------------------------------------------------
bool ArchiveViewModel::entryIsInList(
	const ArchiveEntry& entry,
	bool                filter,
	const Archive*      parent_archive,
	const ArchiveDir*   parent_dir) const
{
	if (!parent_archive)
		parent_archive = entry.parent();
	if (!parent_dir)
		parent_dir = entry.parentDir();

	if (auto archive = archive_.lock())
	{
		// Check entry is in archive
		if (parent_archive != archive.get())
			return false;

		// Check filter
		if (filter && !matchesFilter(entry))
			return false;

		// For list view, check if entry is in current dir
		if (view_type_ == ViewType::List && parent_dir != root_dir_.lock().get())
			return false;

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns true if [dir] is contained within the current list.
// If [filter] is true, also checks if the dir matches the current filter.
// [parent_dir] is used in the case where [dir] has been deleted and removed
// from its parent dir
// -----------------------------------------------------------------------------
bool ArchiveViewModel::dirIsInList(const ArchiveDir& dir, bool filter, const ArchiveDir* parent_dir) const
{
	if (elist_filter_dirs && filter && !matchesFilter(*dir.dirEntry()))
		return false;

	if (!parent_dir)
		parent_dir = dir.parent().get();

	switch (view_type_)
	{
	case ViewType::List: return parent_dir == root_dir_.lock().get();
	default:             return true;
	}
}

// -----------------------------------------------------------------------------
// Returns the ArchiveDir that [item] represents, or nullptr if it isn't a valid
// directory item
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveViewModel::dirForDirItem(const wxDataViewItem& item) const
{
	if (auto archive = archive_.lock())
	{
		auto* entry = static_cast<ArchiveEntry*>(item.GetID());
		return ArchiveDir::findDirByDirEntry(archive_.lock()->rootDir(), *entry).get();
	}

	return nullptr;
}


// -----------------------------------------------------------------------------
//
// ArchiveEntryTree Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveEntryTree class constructor
// -----------------------------------------------------------------------------
ArchiveEntryTree::ArchiveEntryTree(
	wxWindow*                  parent,
	const shared_ptr<Archive>& archive,
	UndoManager*               undo_manager,
	bool                       force_list) :
	SDataViewCtrl{ parent, wxDV_MULTIPLE },
	archive_{ archive }
{
	// Init settings
	SetRowHeight(FromDIP(elist_icon_size + (elist_icon_padding * 2) + 2));
	if (list_font_monospace)
		SetFont(wxutil::monospaceFont(GetFont()));

	// Create & associate model
	model_ = new ArchiveViewModel();
	model_->openArchive(archive, undo_manager, force_list);
	AssociateModel(model_);
	model_->DecRef();

	// Add Columns
	setupColumns();

	// --- Bind Events ---

	// Expand/Contract folders if activated
	Bind(
		wxEVT_DATAVIEW_ITEM_ACTIVATED,
		[this](wxDataViewEvent& e)
		{
			if (auto* entry = static_cast<ArchiveEntry*>(e.GetItem().GetID());
				entry && entry->type() == EntryType::folderType())
			{
				if (model_->viewType() == ArchiveViewModel::ViewType::Tree)
				{
					if (IsExpanded(e.GetItem()))
						Collapse(e.GetItem());
					else
						Expand(e.GetItem());
				}
				else
				{
					Freeze();
					model_->setRootDir(e.GetItem());
					Thaw();

					// Trigger selection change event (to update UI as needed)
					wxDataViewEvent de;
					de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
					ProcessWindowEvent(de);
				}
			}
			else
				e.Skip();
		});

	// Disable modified indicator (" *" after name) when in-place editing entry names
	Bind(
		wxEVT_DATAVIEW_ITEM_EDITING_STARTED,
		[this](wxDataViewEvent& e)
		{
			if (e.GetColumn() == 0)
				model_->showModifiedIndicators(false);
		});
	Bind(
		wxEVT_DATAVIEW_ITEM_START_EDITING,
		[this](wxDataViewEvent& e)
		{
			if (e.GetColumn() == 0)
				model_->showModifiedIndicators(false);
		});
	Bind(
		wxEVT_DATAVIEW_ITEM_EDITING_DONE,
		[this](wxDataViewEvent& e)
		{
			if (e.GetColumn() == 0)
				model_->showModifiedIndicators(true);
		});

	// Header left click (ie. sorting change)
	Bind(
		wxEVT_DATAVIEW_COLUMN_HEADER_CLICK,
		[this](wxDataViewEvent& e)
		{
			CallAfter(&ArchiveEntryTree::saveColumnConfig);
			e.Skip();
		});

	// Header right click
	Bind(
		wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK,
		[this](wxDataViewEvent& e)
		{
			// Popup context menu
			wxMenu context;
			context.Append(0, wxS("Reset Sorting"));
			context.AppendSeparator();
			appendColumnToggleItem(context, 3); // Index
			appendColumnToggleItem(context, 1); // Size
			appendColumnToggleItem(context, 2); // Type
			PopupMenu(&context);
			e.Skip();
		});

	// Header context menu
	Bind(
		wxEVT_MENU,
		[this](wxCommandEvent& e)
		{
			if (e.GetId() == 0)
			{
				// Reset Sorting
				if (col_name_->IsSortKey())
					col_name_->UnsetAsSortKey();
				if (col_size_->IsSortKey())
					col_size_->UnsetAsSortKey();
				if (col_type_->IsSortKey())
					col_type_->UnsetAsSortKey();
#ifdef __WXGTK__
				col_index_->SetSortOrder(true);
#else
				if (col_index_->IsSortKey())
					col_index_->UnsetAsSortKey();
#endif
				model_->Resort();
				saveColumnConfig();

				wxDataViewEvent de;
				de.SetEventType(wxEVT_DATAVIEW_COLUMN_SORTED);
				ProcessWindowEvent(de);
			}
			else if (e.GetId() == 1)
			{
				// Toggle index column
				col_index_->SetHidden(!col_index_->IsHidden());
				saveStateBool(ENTRYLIST_INDEX_VISIBLE, !col_index_->IsHidden());
				updateColumnWidths();
				saveColumnConfig();
			}
			else if (e.GetId() == 2)
			{
				// Toggle size column
				col_size_->SetHidden(!col_size_->IsHidden());
				saveStateBool(ENTRYLIST_SIZE_VISIBLE, !col_size_->IsHidden());
				updateColumnWidths();
				saveColumnConfig();
			}
			else if (e.GetId() == 3)
			{
				// Toggle type column
				col_type_->SetHidden(!col_type_->IsHidden());
				saveStateBool(ENTRYLIST_TYPE_VISIBLE, !col_type_->IsHidden());
				updateColumnWidths();
				saveColumnConfig();
			}
			else
				e.Skip();
		});
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
	for (int i = static_cast<int>(selection.size()) - 1; i >= 0; --i)
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
	if (!archive || !archive->formatInfo().supports_dirs)
		return {};

	vector<ArchiveDir*> dirs;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Add dirs from selected items
	const auto dir_root = archive->rootDir();
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
	if (!archive || !archive->formatInfo().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	const auto dir_root = archive->rootDir();
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (entry->type() == EntryType::folderType())
				if (const auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
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
	if (!archive || !archive->formatInfo().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	const auto dir_root = archive->rootDir();
	for (int i = static_cast<int>(selection.size()) - 1; i >= 0; --i)
		if (auto* entry = static_cast<ArchiveEntry*>(selection[i].GetID()))
			if (entry->type() == EntryType::folderType())
				if (const auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
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
	// List view - just return the current root dir
	if (model_->viewType() == ArchiveViewModel::ViewType::List)
		return model_->rootDir();

	auto* archive = archive_.lock().get();
	if (!archive)
		return nullptr;

	const auto item = lastSelectedItem();
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
	{
		if (entry->type() == EntryType::folderType())
			return model_->dirForDirItem(item);
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
	// List view - just return the current root dir
	if (model_->viewType() == ArchiveViewModel::ViewType::List)
		return model_->rootDir();

	// Tree view
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
// Returns a list of all expanded directories
// -----------------------------------------------------------------------------
vector<ArchiveDir*> ArchiveEntryTree::expandedDirs() const
{
	vector<ArchiveDir*> expanded_dirs;

	auto dirs = archive_.lock()->rootDir()->allDirectories();
	for (const auto& dir : dirs)
		if (IsExpanded(wxDataViewItem(dir->dirEntry())))
			expanded_dirs.push_back(dir.get());

	return expanded_dirs;
}

// -----------------------------------------------------------------------------
// Returns the current root directory of the tree (or list in case of list view)
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveEntryTree::currentRootDir() const
{
	// List view - current dir
	if (model_->viewType() == ArchiveViewModel::ViewType::List)
		return model_->rootDir();

	// Tree view - archive root dir
	if (auto* archive = archive_.lock().get())
		return archive->rootDir().get();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Set the filter options on the model
// -----------------------------------------------------------------------------
void ArchiveEntryTree::setFilter(string_view name, string_view category)
{
	// Get expanded dirs (if in tree view)
	vector<ArchiveDir*> expanded;
	auto                tree_view = model_->viewType() == ArchiveViewModel::ViewType::Tree;
	if (tree_view)
		expanded = expandedDirs();

	// Get selected items
	wxDataViewItemArray selected;
	GetSelections(selected);

	// Set filter on model
	Freeze();
	model_->setFilter(name, category);

	// Restore previously expanded directories
	if (tree_view)
	{
		for (auto* dir : expanded)
		{
			Expand(wxDataViewItem(dir->dirEntry()));

			// Have to collapse parent directories that weren't previously expanded, for whatever reason
			// the 'Expand' function used above will also expand any parent nodes which is annoying
			auto* pdir = dir->parent().get();
			while (pdir)
			{
				if (std::find(expanded.begin(), expanded.end(), pdir) == expanded.end())
					Collapse(wxDataViewItem(pdir->dirEntry()));

				pdir = pdir->parent().get();
			}
		}
	}

	if (!selected.empty())
	{
		SetSelections(selected);
		EnsureVisible(selected[0]);
	}

	Thaw();
}

// -----------------------------------------------------------------------------
// Collapses all currently expanded directory items
// -----------------------------------------------------------------------------
void ArchiveEntryTree::collapseAll(const ArchiveDir& dir_start)
{
	for (const auto& subdir : dir_start.subdirs())
		collapseAll(*subdir);

	Collapse(wxDataViewItem(dir_start.dirEntry()));
}

// -----------------------------------------------------------------------------
// Go up a directory
// -----------------------------------------------------------------------------
void ArchiveEntryTree::upDir()
{
	if (model_->viewType() != ArchiveViewModel::ViewType::List)
		return;

	if (auto dir_current = model_->rootDir())
	{
		if (!dir_current->parent())
			return;

		goToDir(dir_current->parent());
		Select(wxDataViewItem(dir_current->dirEntry()));
	}
}

// -----------------------------------------------------------------------------
// Go to the root directory of the archive
// -----------------------------------------------------------------------------
void ArchiveEntryTree::homeDir()
{
	if (model_->viewType() != ArchiveViewModel::ViewType::List)
		return;

	if (auto archive = archive_.lock())
		goToDir(archive->rootDir());
}

// -----------------------------------------------------------------------------
// Override of EnsureVisible to also open the correct directory if needed
// -----------------------------------------------------------------------------
void ArchiveEntryTree::EnsureVisible(const wxDataViewItem& item, const wxDataViewColumn* column)
{
	if (model_->viewType() == ArchiveViewModel::ViewType::List)
	{
		auto* entry = entryForItem(item);
		if (!entry)
			return;

		const auto archive = archive_.lock();
		if (!archive)
			return;

		// Go to entry's parent dir if needed
		if (archive->formatInfo().supports_dirs && model_->rootDir() != entry->parentDir())
			model_->setRootDir(ArchiveDir::getShared(entry->parentDir()));
	}

	wxDataViewCtrl::EnsureVisible(item, column);
}

// -----------------------------------------------------------------------------
// Creates and sets up the tree columns
// -----------------------------------------------------------------------------
void ArchiveEntryTree::setupColumns()
{
	const auto archive = archive_.lock().get();
	if (!archive)
		return;

	auto colstyle_visible = wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE;
	auto colstyle_hidden  = colstyle_visible | wxDATAVIEW_COL_HIDDEN;

	// Add Columns
	col_index_ = AppendTextColumn(
		wxS("#"),
		3,
		wxDATAVIEW_CELL_INERT,
		FromDIP(getStateInt(ENTRYLIST_INDEX_WIDTH, archive)),
		wxALIGN_NOT,
		getStateBool(ENTRYLIST_INDEX_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	col_name_ = AppendIconTextColumn(
		wxS("Name"),
		0,
		elist_rename_inplace ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT,
		FromDIP(nameColumnWidth(archive)),
		wxALIGN_NOT,
		colstyle_visible);
	col_size_ = AppendTextColumn(
		wxS("Size"),
		1,
		wxDATAVIEW_CELL_INERT,
		FromDIP(getStateInt(ENTRYLIST_SIZE_WIDTH, archive)),
		wxALIGN_NOT,
		getStateBool(ENTRYLIST_SIZE_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	col_type_ = AppendTextColumn(
		wxS("Type"),
		2,
		wxDATAVIEW_CELL_INERT,
		FromDIP(getStateInt(ENTRYLIST_TYPE_WIDTH, archive)),
		wxALIGN_NOT,
		getStateBool(ENTRYLIST_TYPE_VISIBLE, archive) ? colstyle_visible : colstyle_hidden);
	SetExpanderColumn(col_name_);

	// Last column will expand anyway, this ensures we don't get unnecessary horizontal scrollbars
	GetColumn(GetColumnCount() - 1)->SetWidth(0);

	// Load sorting config
	if (hasSavedState(ENTRYLIST_SORT_COLUMN, archive))
	{
		auto sort_column     = getStateString(ENTRYLIST_SORT_COLUMN, archive);
		auto sort_descending = getStateBool(ENTRYLIST_SORT_DESCENDING, archive);
		if (sort_column == "index")
			col_index_->SetSortOrder(!sort_descending);
		else if (sort_column == "name")
			col_name_->SetSortOrder(!sort_descending);
		else if (sort_column == "size")
			col_size_->SetSortOrder(!sort_descending);
		else if (sort_column == "type")
			col_type_->SetSortOrder(!sort_descending);

		model_->Resort();
	}
}

// -----------------------------------------------------------------------------
// Updates the currently visible columns' widths from saved UI state
// -----------------------------------------------------------------------------
void ArchiveEntryTree::updateColumnWidths()
{
	const auto archive = archive_.lock().get();
	if (!archive)
		return;

	// Get the last visible column (we don't want to save the width of this column since it stretches)
	wxDataViewColumn* last_col = nullptr;
	for (int i = static_cast<int>(GetColumnCount()) - 1; i >= 0; --i)
		if (!GetColumn(i)->IsHidden())
		{
			last_col = GetColumn(i);
			break;
		}

	Freeze();
	col_index_->SetWidth(getStateInt(ENTRYLIST_INDEX_WIDTH, archive));
	col_name_->SetWidth(nameColumnWidth(archive));
	col_size_->SetWidth(col_size_ == last_col ? 0 : getStateInt(ENTRYLIST_SIZE_WIDTH, archive));
	col_type_->SetWidth(col_type_ == last_col ? 0 : getStateInt(ENTRYLIST_TYPE_WIDTH, archive));
	Thaw();
}

void ArchiveEntryTree::saveColumnConfig()
{
	const auto archive = archive_.lock().get();
	if (!archive)
		return;

	// Visible columns
	saveStateBool(ENTRYLIST_INDEX_VISIBLE, col_index_->IsShown(), archive);
	saveStateBool(ENTRYLIST_SIZE_VISIBLE, col_size_->IsShown(), archive);
	saveStateBool(ENTRYLIST_TYPE_VISIBLE, col_type_->IsShown(), archive);

	// Sorting
	auto   sort_descending = false;
	string sort_column;
	if (col_index_->IsSortKey())
	{
		sort_column     = "index";
		sort_descending = !col_index_->IsSortOrderAscending();
	}
	else if (col_name_->IsSortKey())
	{
		sort_column     = "name";
		sort_descending = !col_name_->IsSortOrderAscending();
	}
	else if (col_size_->IsSortKey())
	{
		sort_column     = "size";
		sort_descending = !col_size_->IsSortOrderAscending();
	}
	else if (col_type_->IsSortKey())
	{
		sort_column     = "type";
		sort_descending = !col_type_->IsSortOrderAscending();
	}
	saveStateString(ENTRYLIST_SORT_COLUMN, sort_column, archive);
	saveStateBool(ENTRYLIST_SORT_DESCENDING, sort_descending, archive);
}

void ArchiveEntryTree::onAnyColumnResized()
{
	const auto archive = archive_.lock().get();
	if (!archive)
		return;

	// Get the last visible column (we don't want to save the width of this column since it stretches)
	auto last_col = lastVisibleColumn();

	// Index
	if (col_index_->IsShown())
	{
		auto width = ToDIP(col_index_->GetWidth());
		saveStateInt(ENTRYLIST_INDEX_WIDTH, width, archive, true);
	}

	// Name
	if (col_name_ != last_col)
	{
		auto width = ToDIP(col_name_->GetWidth());
		saveStateInt(
			archive->formatInfo().supports_dirs ? ENTRYLIST_NAME_WIDTH_TREE : ENTRYLIST_NAME_WIDTH_LIST, width);
		saveStateInt(ENTRYLIST_NAME_WIDTH, width, archive);
	}

	// Size
	if (col_size_ != last_col && col_size_->IsShown())
	{
		auto width = ToDIP(col_size_->GetWidth());
		saveStateInt(ENTRYLIST_SIZE_WIDTH, width, archive, true);
	}

	// Type
	if (col_type_ != last_col && col_type_->IsShown())
	{
		auto width = ToDIP(col_type_->GetWidth());
		saveStateInt(ENTRYLIST_TYPE_WIDTH, width, archive, true);
	}
}

// -----------------------------------------------------------------------------
// Sets the root directory to [dir] and updates UI accordingly
// -----------------------------------------------------------------------------
void ArchiveEntryTree::goToDir(const shared_ptr<ArchiveDir>& dir, bool expand)
{
	if (const auto archive = archive_.lock())
	{
		// Check dir is part of archive
		if (dir->archive() != archive.get())
			return;

		// List View
		if (model_->viewType() == ArchiveViewModel::ViewType::List)
		{
			// Do nothing if already at dir
			if (model_->rootDir() == dir.get())
				return;

			// Open dir
			Freeze();
			model_->setRootDir(dir);
			Thaw();

			// Trigger selection change event (to update UI as needed)
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
			ProcessWindowEvent(de);
		}

		// Tree View
		else
		{
			auto dir_item = model_->createItemForDirectory(*dir);

			// Select directory (only)
			SetSelections({});
			Select(dir_item);
			EnsureVisible(dir_item);

			// Expand if requested
			if (expand)
				Expand(dir_item);
		}
	}
}

// -----------------------------------------------------------------------------
// Reloads the list model, in [tree] or list view
// -----------------------------------------------------------------------------
void ArchiveEntryTree::reloadModel(bool tree)
{
	model_->reload(tree ? ArchiveViewModel::ViewType::Tree : ArchiveViewModel::ViewType::List);
}
