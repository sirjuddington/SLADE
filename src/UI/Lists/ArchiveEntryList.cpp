
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveEntryList.cpp
// Description: A list widget that shows all entries in an archive.
//              Keeps in sync with its associated archive automatically.
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
#include "ArchiveEntryList.h"
#include "General/ColourConfiguration.h"
#include "General/UndoRedo.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, elist_colname_width, 80, CVar::Flag::Save)
CVAR(Int, elist_colsize_width, 64, CVar::Flag::Save)
CVAR(Int, elist_coltype_width, 160, CVar::Flag::Save)
CVAR(Int, elist_colindex_width, 64, CVar::Flag::Save)
CVAR(Bool, elist_colsize_show, true, CVar::Flag::Save)
CVAR(Bool, elist_coltype_show, true, CVar::Flag::Save)
CVAR(Bool, elist_colindex_show, false, CVar::Flag::Save)
CVAR(Bool, elist_hrules, false, CVar::Flag::Save)
CVAR(Bool, elist_vrules, false, CVar::Flag::Save)
CVAR(Bool, elist_filter_dirs, false, CVar::Flag::Save)
CVAR(Bool, elist_type_bgcol, false, CVar::Flag::Save)
CVAR(Float, elist_type_bgcol_intensity, 0.18, CVar::Flag::Save)
CVAR(Bool, elist_name_monospace, false, CVar::Flag::Save)
CVAR(Bool, elist_alt_row_colour, false, CVar::Flag::Save)
CVAR(Int, elist_icon_size, 16, CVar::Flag::Save)
CVAR(Int, elist_icon_padding, 1, CVar::Flag::Save)
wxDEFINE_EVENT(EVT_AEL_DIR_CHANGED, wxCommandEvent);


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, list_font_monospace)


// -----------------------------------------------------------------------------
//
// ArchiveEntryList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveEntryList class constructor
// -----------------------------------------------------------------------------
ArchiveEntryList::ArchiveEntryList(wxWindow* parent) : VirtualListView(parent)
{
	// Create dummy 'up folder' entry
	entry_dir_back_ = std::make_unique<ArchiveEntry>();
	entry_dir_back_->setType(EntryType::folderType());
	entry_dir_back_->setState(ArchiveEntry::State::Unmodified);
	entry_dir_back_->setName("..");

	// Setup columns
	setupColumns();

	// Setup entry icons
	auto  icon_size    = elist_icon_size + elist_icon_padding * 2;
	auto* image_list   = new wxImageList(icon_size, icon_size, false, 0);
	auto  et_icon_list = EntryType::iconList();
	for (const auto& name : et_icon_list)
	{
		if (image_list->Add(icons::getPaddedIcon(icons::Entry, name, elist_icon_size, elist_icon_padding)) < 0)
			image_list->Add(icons::getPaddedIcon(icons::Entry, "default", elist_icon_size, elist_icon_padding));
	}

	wxListCtrl::SetImageList(image_list, wxIMAGE_LIST_SMALL);

	// Bind events
	Bind(wxEVT_LIST_COL_RIGHT_CLICK, &ArchiveEntryList::onColumnHeaderRightClick, this);
	Bind(wxEVT_LIST_COL_END_DRAG, &ArchiveEntryList::onColumnResize, this);
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchiveEntryList::onListItemActivated, this);

	// Setup flags
	SetSingleStyle(wxLC_HRULES, elist_hrules);
	SetSingleStyle(wxLC_VRULES, elist_vrules);
}

// -----------------------------------------------------------------------------
// Called when the widget requests the text for [item] at [column]
// -----------------------------------------------------------------------------
wxString ArchiveEntryList::itemText(long item, long column, long index) const
{
	// Get entry
	auto entry = entryAt(index, false);

	// Check entry
	if (!entry)
		return "INVALID INDEX";

	// Determine what column we want
	int col = columnType(column);

	if (col == 0)
		return entry->name(); // Name column
	else if (col == 1)
	{
		// Size column
		if (entry->type() == EntryType::folderType())
		{
			// Entry is a folder, return the number of entries+subdirectories in it
			ArchiveDir* dir = nullptr;

			// Get selected directory
			if (entry == entry_dir_back_.get())
				dir = current_dir_.lock()->parent().get(); // If it's the 'back directory', get the current dir's parent
			else if (auto archive = archive_.lock())
				dir = archive->dirAtPath(entry->name(), current_dir_.lock().get());

			// If it's null, return error
			if (!dir)
				return "INVALID DIRECTORY";

			// Return the number of items in the directory
			return wxString::Format("%d entries", dir->numEntries() + dir->numSubdirs());
		}
		else
			return entry->sizeString(); // Not a folder, just return the normal size string
	}
	else if (col == 2)
		return entry->typeString(); // Type column
	else if (col == 3)
	{
		// Index column
		if (entry->type() == EntryType::folderType())
			return "";
		else
			return wxString::Format("%d", entry->index());
	}
	else
		return "INVALID COLUMN"; // Invalid column
}

// -----------------------------------------------------------------------------
// Called when the widget requests the icon for [item]
// -----------------------------------------------------------------------------
int ArchiveEntryList::itemIcon(long item, long column, long index) const
{
	if (column > 0)
		return -1;

	// Get associated entry
	auto entry = entryAt(item);

	// If entry doesn't exist, return invalid image
	if (!entry)
		return -1;

	return entry->type()->index();
}

// -----------------------------------------------------------------------------
// Called when widget requests the attributes
// (text colour / background colour / font) for [item]
// -----------------------------------------------------------------------------
void ArchiveEntryList::updateItemAttr(long item, long column, long index) const
{
	// Get associated entry
	auto entry = entryAt(item);

	// Init attributes
	auto col_bg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
	item_attr_->SetTextColour(colourconfig::colour("error").toWx());
	item_attr_->SetBackgroundColour(col_bg);

	// If entry doesn't exist, return error colour
	if (!entry)
		return;

	// Set font
	if (elist_name_monospace && !list_font_monospace)
		item_attr_->SetFont((column == 0) ? font_monospace_ : font_normal_);
	else
		item_attr_->SetFont(list_font_monospace ? font_monospace_ : font_normal_);

	// Set background colour defined in entry type (if any)
	auto col = entry->type()->colour();
	if ((col.r != 255 || col.g != 255 || col.b != 255) && elist_type_bgcol)
	{
		ColRGBA bcol;

		bcol.r = (col.r * elist_type_bgcol_intensity) + (col_bg.Red() * (1.0 - elist_type_bgcol_intensity));
		bcol.g = (col.g * elist_type_bgcol_intensity) + (col_bg.Green() * (1.0 - elist_type_bgcol_intensity));
		bcol.b = (col.b * elist_type_bgcol_intensity) + (col_bg.Blue() * (1.0 - elist_type_bgcol_intensity));

		item_attr_->SetBackgroundColour(bcol.toWx());
	}

	// Alternating row colour
	if (elist_alt_row_colour && item % 2 > 0)
	{
		auto dark = item_attr_->GetBackgroundColour().ChangeLightness(95);
		item_attr_->SetBackgroundColour(dark);
	}

	// Set colour depending on entry state
	switch (entry->state())
	{
	case ArchiveEntry::State::Modified: item_attr_->SetTextColour(colourconfig::colour("modified").toWx()); break;
	case ArchiveEntry::State::New: item_attr_->SetTextColour(colourconfig::colour("new").toWx()); break;
	default: item_attr_->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT)); break;
	};

	// Locked state overrides others
	if (entry->isLocked())
		item_attr_->SetTextColour(colourconfig::colour("locked").toWx());
}

// -----------------------------------------------------------------------------
// Sets the archive for this widget to handle (can be NULL for no archive)
// -----------------------------------------------------------------------------
void ArchiveEntryList::setArchive(const shared_ptr<Archive>& archive)
{
	// Set archive (allow null)
	archive_ = archive;

	// Init new archive if given
	if (archive)
	{
		// Update list when archive is modified
		sc_archive_modified_ = archive->signals().modified.connect([this](Archive&) { updateEntries(); });

		// Open root directory
		current_dir_ = archive->rootDir();
		applyFilter();
		updateList();
	}
}

// -----------------------------------------------------------------------------
// Creates/sets the list columns depending on user options
// -----------------------------------------------------------------------------
void ArchiveEntryList::setupColumns()
{
	// Remove existing columns
	while (GetColumnCount() > 0)
		DeleteColumn(0);

	// Create columns
	int col_num = 0;
	col_index_  = -1;
	col_name_   = 0;
	col_size_   = -1;
	col_type_   = -1;

	// Index
	if (elist_colindex_show)
	{
		AppendColumn("#");
		SetColumnWidth(col_num, elist_colindex_width);
		col_index_ = col_num++;
	}

	// Name (always)
	AppendColumn("Name");
	SetColumnWidth(col_num, elist_colname_width);
	col_name_ = col_num++;

	// Size
	if (elist_colsize_show)
	{
		AppendColumn("Size");
		SetColumnWidth(col_num, elist_colsize_width);
		col_size_ = col_num++;
	}

	// Type
	if (elist_coltype_show)
	{
		AppendColumn("Type");
		SetColumnWidth(col_num, elist_coltype_width);
		col_type_ = col_num++;
	}

	// Set editable
	setColumnEditable(col_name_); // Name column

	// Reset sorting
	sort_column_  = -1;
	sort_descend_ = false;
}

// -----------------------------------------------------------------------------
// Returns the 'type' of column at [column] (name, size or type)
// -----------------------------------------------------------------------------
int ArchiveEntryList::columnType(int column) const
{
	if (column == col_name_)
		return 0;
	else if (column == col_size_)
		return 1;
	else if (column == col_type_)
		return 2;
	else if (column == col_index_)
		return 3;

	return -1;
}

// -----------------------------------------------------------------------------
// Updates + refreshes the list
// -----------------------------------------------------------------------------
void ArchiveEntryList::updateList(bool clear)
{
	// If no current directory, set size to 0
	if (!current_dir_.lock())
	{
		SetItemCount(0);
		Refresh();
		return;
	}

	// Update list
	SetItemCount(items_.size());
	sortItems();

	Refresh();
}

// -----------------------------------------------------------------------------
// Filters the list to only entries and directories with names matching
// [filter], and with type categories matching [category].
// -----------------------------------------------------------------------------
void ArchiveEntryList::filterList(const wxString& filter, const wxString& category)
{
	// Update variables
	filter_text_     = filter;
	filter_category_ = category;

	// Save current selection
	auto selection = selectedEntries();
	auto focus     = focusedEntry();

	// Apply the filter
	clearSelection();
	applyFilter();

	for (int a = 0; a < GetItemCount(); a++)
	{
		auto entry = entryAt(a, false);
		for (auto& selected_entry : selection)
		{
			if (entry == selected_entry)
			{
				selectItem(a);
				break;
			}
		}

		if (entry == focus)
		{
			focusItem(a);
			EnsureVisible(a);
		}
	}
}

// -----------------------------------------------------------------------------
// Applies the current filter(s) to the list
// -----------------------------------------------------------------------------
void ArchiveEntryList::applyFilter()
{
	auto dir = current_dir_.lock();
	if (!dir)
		return;

	// Clear current filter list
	items_.clear();

	// Check if any filters were given
	if (filter_text_.IsEmpty() && filter_category_.IsEmpty())
	{
		// No filter, just refresh the list
		unsigned count = dir->numEntries() + dir->numSubdirs();
		for (unsigned a = 0; a < count; a++)
			items_.push_back(a);
		updateList();

		return;
	}

	// Filter by category
	unsigned index = 0;
	auto     entry = entryAt(index, false);
	while (entry)
	{
		if (filter_category_.IsEmpty() || entry->type() == EntryType::folderType())
			items_.push_back(index); // If no category specified, just add all entries to the filter
		else
		{
			// Check for category match
			if (strutil::equalCI(entry->type()->category(), filter_category_.ToStdString()))
				items_.push_back(index);
		}

		entry = entryAt(++index, false);
	}

	// Now filter by name if needed
	if (!filter_text_.IsEmpty())
	{
		// Split filter by ,
		auto terms = strutil::split(filter_text_.ToStdString(), ',');

		// Process filter strings
		for (auto& term : terms)
		{
			// Remove spaces
			strutil::replaceIP(term, " ", "");

			// Set to uppercase and add * to the end
			if (!term.empty())
			{
				strutil::upperIP(term);
				term += "*";
			}
		}

		// Go through filtered list
		for (unsigned a = 0; a < items_.size(); a++)
		{
			entry = entryAt(items_[a], false);

			// Don't filter folders if !elist_filter_dirs
			if (!elist_filter_dirs && entry->type() == EntryType::folderType())
				continue;

			// Check for name match with filter
			bool match = false;
			for (const auto& term : terms)
			{
				if (entry == entry_dir_back_.get() || strutil::matches(entry->upperName(), term))
				{
					match = true;
					continue;
				}
			}
			if (match)
				continue;

			// No match, remove from filtered list
			items_.erase(items_.begin() + a);
			a--;
		}
	}

	// Update the list
	updateList();
}

// -----------------------------------------------------------------------------
// Opens the given directory (if it exists)
// -----------------------------------------------------------------------------
bool ArchiveEntryList::setDir(const shared_ptr<ArchiveDir>& dir)
{
	// If it doesn't exist, do nothing
	if (!dir)
		return false;

	// Set current dir
	current_dir_ = dir;

	// Clear current selection
	clearSelection();

	// Update filter
	applyFilter();

	// Update list
	updateList();

	// Fire event
	wxCommandEvent evt(EVT_AEL_DIR_CHANGED, GetId());
	ProcessWindowEvent(evt);

	return true;
}

void ArchiveEntryList::updateEntries()
{
	if (entries_update_)
	{
		applyFilter();
	}
}

// -----------------------------------------------------------------------------
// Opens the parent directory of the current directory (if it exists)
// -----------------------------------------------------------------------------
bool ArchiveEntryList::goUpDir()
{
	// Get parent directory
	return setDir(current_dir_.lock()->parent());
}

// -----------------------------------------------------------------------------
// Returns either the size of the entry at [index], or if it is a folder, the
// number of entries+subfolders within it
// -----------------------------------------------------------------------------
int ArchiveEntryList::entrySize(long index) const
{
	auto entry = entryAt(index, false);
	if (entry->type() == EntryType::folderType())
	{
		auto dir = archive_.lock()->dirAtPath(entry->name(), current_dir_.lock().get());
		return dir ? dir->numEntries() + dir->numSubdirs() : 0;
	}
	else
		return entry->size();
}

// -----------------------------------------------------------------------------
// Sorts the list items depending on the current sorting column
// -----------------------------------------------------------------------------
void ArchiveEntryList::sortItems()
{
	lv_current_ = this;
	std::sort(items_.begin(), items_.end(), [&](long left, long right) {
		auto le = entryAt(left, false);
		auto re = entryAt(right, false);

		// Sort folder->entry first
		if (le->type() == EntryType::folderType() && re->type() != EntryType::folderType())
			return true;
		if (re->type() == EntryType::folderType() && le->type() != EntryType::folderType())
			return false;

		// Name sort
		if (col_name_ >= 0 && col_name_ == sortColumn())
			return sort_descend_ ? le->upperName() > re->upperName() : le->upperName() < re->upperName();

		// Size sort
		if (col_size_ >= 0 && col_size_ == sortColumn())
			return sort_descend_ ? entrySize(left) > entrySize(right) : entrySize(left) < entrySize(right);

		// Index sort
		if (col_index_ >= 0 && col_index_ == sortColumn())
			return sort_descend_ ? left > right : left < right;

		// Other (default) sort
		return VirtualListView::defaultSort(left, right);
	});
}

// -----------------------------------------------------------------------------
// Returns the index of the first list item that is an entry (rather than a
// directory), or -1 if no directory/archive is open)
// -----------------------------------------------------------------------------
int ArchiveEntryList::entriesBegin() const
{
	// Check directory is open
	auto dir = current_dir_.lock();
	if (!dir)
		return -1;

	// Determine first entry index
	int index = 0;
	if (show_dir_back_ && dir->parent()) // Offset if '..' item exists
		index++;
	index += dir->numSubdirs(); // Offset by number of subdirs

	return index;
}

// -----------------------------------------------------------------------------
// Returns the ArchiveEntry associated with the list item at [index].
// Returns NULL if the index is out of bounds or no archive is open
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntryList::entryAt(int index, bool filtered) const
{
	// Check index & archive
	auto dir = current_dir_.lock();
	if (index < 0 || !archive_.lock() || !dir)
		return nullptr;

	// Modify index for filtered list
	if (filtered)
	{
		if ((unsigned)index >= items_.size())
			return nullptr;
		else
			index = items_[index];
	}

	// Index modifier if 'up folder' entry exists
	if (show_dir_back_ && dir->parent())
	{
		if (index == 0)
			return entry_dir_back_.get();
		else
			index--;
	}

	// Subdirectories
	int subdirs = dir->numSubdirs();
	if (index < subdirs)
		return dir->subdirAt(index)->dirEntry();

	// Entries
	if ((unsigned)index < subdirs + dir->numEntries())
		return dir->entryAt(index - subdirs);

	// Out of bounds
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the ArchiveEntry index associated with the list item at [index].
// Returns -1 if the index is out of bounds or no archive is open
// -----------------------------------------------------------------------------
int ArchiveEntryList::entryIndexAt(int index, bool filtered)
{
	// Check index & archive
	auto dir = current_dir_.lock();
	if (index < 0 || !archive_.lock() || !dir)
		return -1;

	// Modify index for filtered list
	if (filtered)
	{
		if ((unsigned)index >= items_.size())
			return -1;
		else
			index = items_[index];
	}

	// Index modifier if 'up folder' entry exists
	if (show_dir_back_ && dir->parent())
	{
		if (index == 0)
			return -1;
		else
			index--;
	}

	// Entries
	int subdirs = dir->numSubdirs();
	if ((unsigned)index < subdirs + dir->numEntries())
		return index - subdirs;

	// Out of bounds or subdir
	return -1;
}

// -----------------------------------------------------------------------------
// Gets the archive entry associated with the currently focused list item.
// Returns NULL if nothing is focused or no archive is open
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntryList::focusedEntry() const
{
	// Get the focus index
	int focus = focusedIndex();

	// Check that the focus index is valid
	if (focus < 0 || focus > GetItemCount())
		return nullptr;

	// Return the focused archive entry
	if (archive_.lock())
		return entryAt(focus);
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a vector of all selected archive entries
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchiveEntryList::selectedEntries() const
{
	// Init vector
	vector<ArchiveEntry*> ret;

	// Return empty if no archive open
	if (!archive_.lock())
		return ret;

	// Go through selection and add associated entries to the return vector
	ArchiveEntry* entry = nullptr;
	for (long index : selection())
	{
		entry = entryAt(index);
		if (entry && entry->type() != EntryType::folderType())
			ret.push_back(entry);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Gets the archive entry associated with the last selected item in the list.
// Returns NULL if no item is selected
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntryList::lastSelectedEntry() const
{
	int index = lastSelected();

	if (index >= 0 && archive_.lock())
		return entryAt(index);
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a vector of all currently selected directories
// -----------------------------------------------------------------------------
vector<ArchiveDir*> ArchiveEntryList::selectedDirectories() const
{
	vector<ArchiveDir*> ret;

	if (!archive_.lock())
		return ret;

	// Go through the selection
	for (long index : selection())
	{
		auto entry = entryAt(index);

		// If the selected entry is the 'back folder', ignore it
		if (entry == entry_dir_back_.get())
			continue;

		if (entry->type() == EntryType::folderType())
		{
			// If the entry is a folder type, get its ArchiveTreeNode counterpart
			auto dir = archive_.lock()->dirAtPath(entry->name(), current_dir_.lock().get());

			// Add it to the return list
			if (dir)
				ret.push_back(dir);
		}
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Called when a label has been edited
// -----------------------------------------------------------------------------
void ArchiveEntryList::labelEdited(int col, int index, const wxString& new_label)
{
	if (undo_manager_)
		undo_manager_->beginRecord("Rename Entry");

	// Rename the entry
	auto entry = entryAt(index);
	if (entry->parent())
		entry->parent()->renameEntry(entry, new_label.ToStdString());
	else
		entry->rename(new_label.ToStdString());

	if (undo_manager_)
		undo_manager_->endRecord(true);
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntryList::handleAction(string_view id)
{
	// Don't handle action if hidden
	if (!IsShown())
		return false;

	// Only interested in actions beginning with aelt_
	if (!strutil::startsWith(id, "aelt_"))
		return false;

	if (id == "aelt_sizecol")
	{
		elist_colsize_show = !elist_colsize_show;
		setupColumns();
		updateWidth();
		updateList();
		if (GetParent())
			GetParent()->Layout();
	}
	else if (id == "aelt_typecol")
	{
		elist_coltype_show = !elist_coltype_show;
		setupColumns();
		updateWidth();
		updateList();
		if (GetParent())
			GetParent()->Layout();
	}
	else if (id == "aelt_indexcol")
	{
		elist_colindex_show = !elist_colindex_show;
		setupColumns();
		updateWidth();
		updateList();
		if (GetParent())
			GetParent()->Layout();
	}
	else if (id == "aelt_hrules")
	{
		elist_hrules = !elist_hrules;
		SetSingleStyle(wxLC_HRULES, elist_hrules);
		Refresh();
	}
	else if (id == "aelt_vrules")
	{
		elist_vrules = !elist_vrules;
		SetSingleStyle(wxLC_VRULES, elist_vrules);
		Refresh();
	}
	else if (id == "aelt_bgcolour")
	{
		elist_type_bgcol = !elist_type_bgcol;
		Refresh();
	}
	else if (id == "aelt_bgalt")
	{
		elist_alt_row_colour = !elist_alt_row_colour;
		Refresh();
	}

	// Unknown action
	else
		return false;

	// Action handled, return true
	return true;
}


// -----------------------------------------------------------------------------
//
// ARCHIVEENTRYLIST EVENTS
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a column header is right clicked
// -----------------------------------------------------------------------------
void ArchiveEntryList::onColumnHeaderRightClick(wxListEvent& e)
{
	// Create simple popup menu with options to toggle columns
	wxMenu popup;
	SAction::fromId("aelt_indexcol")->addToMenu(&popup, true);
	SAction::fromId("aelt_sizecol")->addToMenu(&popup, true);
	SAction::fromId("aelt_typecol")->addToMenu(&popup, true);
	SAction::fromId("aelt_hrules")->addToMenu(&popup, true);
	SAction::fromId("aelt_vrules")->addToMenu(&popup, true);
	SAction::fromId("aelt_bgcolour")->addToMenu(&popup, true);
	SAction::fromId("aelt_bgalt")->addToMenu(&popup, true);
	popup.Check(SAction::fromId("aelt_indexcol")->wxId(), elist_colindex_show);
	popup.Check(SAction::fromId("aelt_sizecol")->wxId(), elist_colsize_show);
	popup.Check(SAction::fromId("aelt_typecol")->wxId(), elist_coltype_show);
	popup.Check(SAction::fromId("aelt_hrules")->wxId(), elist_hrules);
	popup.Check(SAction::fromId("aelt_vrules")->wxId(), elist_vrules);
	popup.Check(SAction::fromId("aelt_bgcolour")->wxId(), elist_type_bgcol);
	popup.Check(SAction::fromId("aelt_bgalt")->wxId(), elist_alt_row_colour);

	// Pop it up
	PopupMenu(&popup);
}

// -----------------------------------------------------------------------------
// Called when a column is resized
// -----------------------------------------------------------------------------
void ArchiveEntryList::onColumnResize(wxListEvent& e)
{
	// Save column widths
	elist_colname_width = GetColumnWidth(col_name_);
	if (elist_colsize_show)
		elist_colsize_width = GetColumnWidth(col_size_);
	if (elist_coltype_show)
		elist_coltype_width = GetColumnWidth(col_type_);
	if (elist_colindex_show)
		elist_colindex_width = GetColumnWidth(col_index_);
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a list item is 'activated' (double-click or enter)
// -----------------------------------------------------------------------------
void ArchiveEntryList::onListItemActivated(wxListEvent& e)
{
	// Get item entry
	auto entry = entryAt(e.GetIndex());

	// Do nothing if NULL (shouldn't be)
	auto current_dir = current_dir_.lock();
	if (!entry || !archive_.lock() || !current_dir)
		return;

	// If it's a folder, open it
	if (entry->type() == EntryType::folderType())
	{
		// Get directory to open
		shared_ptr<ArchiveDir> dir;
		if (entry == entry_dir_back_.get())
			dir = current_dir->parent(); // 'Back directory' entry, open current dir's parent
		else
			dir = ArchiveDir::subdirAtPath(current_dir, entry->name());

		// Check it exists (really should)
		if (!dir)
		{
			log::error("Error: Trying to open nonexistant directory");
			return;
		}

		// Set current dir
		setDir(dir);
	}
	else
		e.Skip();
}
